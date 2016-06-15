#include "stdafx.h"

static BYTE call8EINit_Retail[28] = {
	0x38, 0x60, 0x00, 0x00, 0x48, 0x00, 0xB1, 0x2B, 0x3C, 0x60, 0xBE, 0xEF, 0x38, 0x21, 0x00, 0x10,
	0xE9, 0x81, 0xFF, 0xF8, 0x7D, 0x88, 0x03, 0xA6, 0x4E, 0x80, 0x00, 0x20
};

static BYTE call8EInit_Devkit[28] = {
	0x38, 0x60, 0x00, 0x00, 0x48, 0x00, 0xAD, 0xA3, 0x3C, 0x60, 0xBE, 0xEF, 0x38, 0x21, 0x00, 0x10,
	0xE9, 0x81, 0xFF, 0xF8, 0x7D, 0x88, 0x03, 0xA6, 0x4E, 0x80, 0x00, 0x20
};


QWORD __declspec(naked) HvxFreebootCall(DWORD Type, QWORD Source, QWORD Destination, QWORD Size) // 2 and 3 = cache stuff, 4 = execute code, 5 = peek / poke
{
	__asm
	{
		mr r7, r6
		mr r6, r5
			mr r5, r4
			mr r4, r3
			lis r3, 0x7262
			ori r3, r3, 0x7472
			li r0, 0x0
			sc
			blr
	}
}

QWORD __declspec(naked) HvxExpansionInstall(DWORD PhysicalAddress, DWORD CodeSize) {
	__asm {
		li	r0, 0x70
		sc
		blr
	}
}

QWORD __declspec(naked) HvxExpansionCall(QWORD Type = 0, QWORD Source = 0, QWORD Param3 = 0, QWORD Param4 = 0) {
	__asm {
		mr r7, r6
		mr r6, r5
		mr r5, r4
		mr r4, r3
		lis r3, 0x4856
		ori r3, r3, 0x5050
		li	r0, 0x71
		sc
		blr
	}
}

namespace xbox {
	namespace hypervisor {
		BYTE peekByte(QWORD Address) { return (BYTE)HvxExpansionCall(PEEK_BYTE, Address); }
		WORD peekWord(QWORD Address) { return (WORD)HvxExpansionCall(PEEK_WORD, Address); }
		DWORD peekDword(QWORD Address) { return (DWORD)HvxExpansionCall(PEEK_DWORD, Address); }
		QWORD peekQword(QWORD Address) { return HvxExpansionCall(PEEK_QWORD, Address); }
		HRESULT peekBytes(QWORD Address, PVOID Buffer, DWORD Size) {

			// Create a physical buffer to peek into
			VOID* data = XPhysicalAlloc(Size, MAXULONG_PTR, 0, PAGE_READWRITE);
			ZeroMemory(data, Size);

			HRESULT result = (HRESULT)HvxExpansionCall(PEEK_BYTES, Address, (QWORD)MmGetPhysicalAddress(data), Size);

			// If its successful copy it back
			if (result == S_OK) memcpy(Buffer, data, Size);

			// Free our physical data and return our result
			XPhysicalFree(data);
			return result;
		}

		HRESULT pokeByte(QWORD Address, BYTE Value) { return (HRESULT)HvxExpansionCall(POKE_BYTE, Address, Value); }
		HRESULT pokeWord(QWORD Address, WORD Value) { return (HRESULT)HvxExpansionCall(POKE_WORD, Address, Value); }
		HRESULT pokeDword(QWORD Address, DWORD Value) { return (HRESULT)HvxExpansionCall(POKE_DWORD, Address, Value); }
		HRESULT pokeQword(QWORD Address, QWORD Value) { return (HRESULT)HvxExpansionCall(POKE_QWORD, Address, Value); }
		HRESULT pokeBytes(QWORD Address, const void* Buffer, DWORD Size) {

			// Create a physical buffer to poke from
			VOID* data = XPhysicalAlloc(Size, MAXULONG_PTR, 0, PAGE_READWRITE);
			memcpy(data, Buffer, Size);

			HRESULT result = (HRESULT)HvxExpansionCall(POKE_BYTES, Address, (QWORD)MmGetPhysicalAddress(data), Size);

			// Free our physical data and return our result
			XPhysicalFree(data);
			return result;
		}

		PBYTE getCpuKey()
		{
			BYTE fuseCpu[0x10];
			*(QWORD*)(fuseCpu) = peekQword(0x8000020000020600);
			*(QWORD*)(fuseCpu + 8) = peekQword(0x8000020000020A00);
			return fuseCpu;
		}

		HRESULT reloadKv()
		{
			PBYTE physBuff = (PBYTE)XPhysicalAlloc(0x100, MAXULONG_PTR, 0, MEM_LARGE_PAGES | PAGE_READWRITE | PAGE_NOCACHE);

			if (physBuff == NULL)
				return E_FAIL;

			ZeroMemory(physBuff, 0x100);
			memcpy(physBuff, global::isDevkit ? call8EInit_Devkit : call8EINit_Retail, sizeof(call8EINit_Retail));

			if (HvxFreebootCall(4, 0xFE00, 0x8000000000000000 | (DWORD)MmGetPhysicalAddress(physBuff), 7) == 0)
			{
				XPhysicalFree(physBuff);
				return E_FAIL;
			}

			XPhysicalFree(physBuff);
			return ERROR_SUCCESS;
		}

		HRESULT setupCleanMemory(PBYTE eccData)
		{
			WCHAR sectionFile[50];
			XamBuildResourceLocator(global::modules::client, L"stuff", L"hv.bin", sectionFile, 50);

			HXUIRESOURCE hResource;
			BOOL isMemoryResource = FALSE;
			if (XuiResourceOpen(sectionFile, &hResource, &isMemoryResource) != S_OK)
				return E_FAIL;

			// make sure its a memory resource
			if (!isMemoryResource) return E_FAIL;

			// get the buffer
			const BYTE* resourceBuffer = 0;
			if (XuiResourceGetBuffer(hResource, &resourceBuffer) != S_OK)
				return E_FAIL;
			
			// make sure hv is correct size
			if (XuiResourceGetTotalSize(hResource) != 0x40000) return E_FAIL;
			XuiResourceClose(hResource);

			// copy the buffer
			PBYTE cleanHv = (PBYTE)XPhysicalAlloc(0x40000, MAXULONG_PTR, 0, PAGE_READWRITE);
			memcpy(cleanHv, resourceBuffer, 0x40000);

			// save console hv to poke back after saving cleaned memory
			PBYTE consoleHv = (PBYTE)XPhysicalAlloc(0x40000, MAXULONG_PTR, 0, PAGE_READWRITE);
			peekBytes(0x8000010000000000, consoleHv, 0x10000);
			peekBytes(0x8000010200010000, consoleHv + 0x10000, 0x10000);
			peekBytes(0x8000010400020000, consoleHv + 0x20000, 0x10000);
			peekBytes(0x8000010600030000, consoleHv + 0x30000, 0x10000);

			// copy kv specific data to clean hv
			*(WORD*)(cleanHv + 0x6) = xbox::keyvault::data::bldrFlags;
			*(DWORD*)(cleanHv + 0x14) = xbox::keyvault::data::updSeqFlags;
			*(DWORD*)(cleanHv + 0x30) = xbox::keyvault::data::hvStatusFlags;
			*(DWORD*)(cleanHv + 0x74) = xbox::keyvault::data::cTypeFlags;
			memcpy(cleanHv + 0x20, xbox::keyvault::data::cpuKey, 0x10);

			// copy per boot data to clean hv
			memcpy(cleanHv + 0x10000, consoleHv + 0x10000, 0xC0);
			memcpy(cleanHv + 0x10100, consoleHv + 0x10100, 0x30);
			memcpy(cleanHv + 0x164C0, consoleHv + 0x164C0, 0x14);
			memcpy(cleanHv + 0x16590, consoleHv + 0x16590, 0x10);
			memcpy(cleanHv + 0x16800, consoleHv + 0x16800, 0x102);
			memcpy(cleanHv + 0x16A10, consoleHv + 0x16A10, 0x10);
			memcpy(cleanHv + 0x16D18, consoleHv + 0x16D18, 0x4);
			
			// write clean hv so we can dump clean memory, then free it from memory
			xbox::hypervisor::pokeBytes(0x8000010000000000, cleanHv, 0x10000);
			xbox::hypervisor::pokeBytes(0x8000010200010000, cleanHv + 0x10000, 0x10000);
			xbox::hypervisor::pokeBytes(0x8000010400020000, cleanHv + 0x20000, 0x10000);
			xbox::hypervisor::pokeBytes(0x8000010600030000, cleanHv + 0x30000, 0x10000);
			XPhysicalFree(cleanHv);

			// dump clean memory
			xbox::hypervisor::peekBytes(0x0000000000000034, eccData, 0xC);
			xbox::hypervisor::peekBytes(0x8000000000000040, eccData + 0xC, 0x30);
			xbox::hypervisor::peekBytes(0x0000000000000070, eccData + 0x3C, 0x4);
			xbox::hypervisor::peekBytes(0x0000000000000078, eccData + 0x40, 0x8);
			xbox::hypervisor::peekBytes(0x8000020000010002, eccData + 0x48, 0x3FE);
			xbox::hypervisor::peekBytes(0x80000000000100C0, eccData + 0x446, 0x40);
			xbox::hypervisor::peekBytes(0x8000000000010350, eccData + 0x486, 0x30);
			xbox::hypervisor::peekBytes(0x800002000001040E, eccData + 0x4B6, 0x176);
			xbox::hypervisor::peekBytes(0x8000000000016100, eccData + 0x62C, 0x40);
			xbox::hypervisor::peekBytes(0x8000000000016D20, eccData + 0x66C, 0x60);
			xbox::hypervisor::peekBytes(0x80000200000105B6, eccData + 0x6CC, 0x24A);
			xbox::hypervisor::peekBytes(0x8000020000010800, eccData + 0x916, 0x400);
			xbox::hypervisor::peekBytes(0x8000020000010C00, eccData + 0xD16, 0x400);

			// write back the real hv and free it
			xbox::hypervisor::pokeBytes(0x8000010000000000, consoleHv, 0x10000);
			xbox::hypervisor::pokeBytes(0x8000010200010000, consoleHv + 0x10000, 0x10000);
			xbox::hypervisor::pokeBytes(0x8000010400020000, consoleHv + 0x20000, 0x10000);
			xbox::hypervisor::pokeBytes(0x8000010600030000, consoleHv + 0x30000, 0x10000);
			XPhysicalFree(consoleHv);

			return S_OK;
		}

		HRESULT initialize() {

			// Allocate physcial memory for this expansion
			VOID* pPhysExp = XPhysicalAlloc(0x1000, MAXULONG_PTR, 0, PAGE_READWRITE);
			DWORD physExpAdd = (DWORD)MmGetPhysicalAddress(pPhysExp);

			// Copy over our expansion data
			ZeroMemory(pPhysExp, 0x1000);
			memcpy(pPhysExp, HvPeekPokeExp, sizeof(HvPeekPokeExp));

			// Now we can install our expansion
			HRESULT result = (HRESULT)HvxExpansionInstall(physExpAdd, 0x1000);

			// Free our allocated data
			XPhysicalFree(pPhysExp);

			// Return our install result
			return result;
		}
	}
}