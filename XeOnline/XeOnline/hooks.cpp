#include "stdafx.h"

Detour<HRESULT> *XuiPNGTextureLoaderDetour = new Detour<HRESULT>;
CXamShutdownNavButton btnXeOnlineMenu;

namespace xbox {
	namespace hooks {
		namespace hud {
			HRESULT xuiRegisterClass(const XUIClass *pClass, HXUICLASS *phClass)
			{
				if (wcscmp(pClass->szClassName, L"ShutdownNavButton") == 0)
					btnXeOnlineMenu.Register();

				return XuiRegisterClass(pClass, phClass);
			}

			HRESULT xuiUnregisterClass(LPCWSTR szClassName)
			{
				if (wcscmp(szClassName, L"ShutdownNavButton") == 0)
					btnXeOnlineMenu.Unregister();

				return XuiUnregisterClass(szClassName);
			}

			HRESULT xuiSceneCreate(PWCHAR szBasePath, PWCHAR szScenePath, void* pvInitData, HXUIOBJ* phScene)
			{
				xbox::utilities::log("XuiCreateScene: Loading %ls", szScenePath);

				HRESULT result = XuiSceneCreate(szBasePath, szScenePath, pvInitData, phScene);

				if (wcscmp(szScenePath, L"GuideMain.xur") == 0)
				{
					updateUserTime();

					// Set header text
					HXUIOBJ headerLabel;
					XuiElementGetChildById(*phScene, L"Header", &headerLabel);
					XuiControlSetText(headerLabel, L"XeOnline Guide");

					// get Tabscene
					HXUIOBJ hTabscene;
					XuiElementGetFirstChild(*phScene, &hTabscene);

#pragma region change_highlighted_text_color
					HXUIOBJ txtGamesSel, txtHomeSel, txtMediaSel, txtSystemSel;
					XUIElementPropVal propVal;
					DWORD propId = 0;

					propVal.SetColorVal(0xFF2980B9);

					XuiElementGetChildById(hTabscene, L"txt_gamesSel", &txtGamesSel);
					XuiObjectGetPropertyId(txtGamesSel, L"TextColor", &propId);
					XuiObjectSetProperty(txtGamesSel, propId, 0, &propVal);

					XuiElementGetChildById(hTabscene, L"txt_homeSel", &txtHomeSel);
					XuiObjectGetPropertyId(txtHomeSel, L"TextColor", &propId);
					XuiObjectSetProperty(txtHomeSel, propId, 0, &propVal);

					XuiElementGetChildById(hTabscene, L"txt_MediaSel", &txtMediaSel);
					XuiObjectGetPropertyId(txtMediaSel, L"TextColor", &propId);
					XuiObjectSetProperty(txtMediaSel, propId, 0, &propVal);

					XuiElementGetChildById(hTabscene, L"txt_SystemSel", &txtSystemSel);
					XuiObjectGetPropertyId(txtSystemSel, L"TextColor", &propId);
					XuiObjectSetProperty(txtSystemSel, propId, 0, &propVal);
#pragma endregion

#pragma region custom_text
					HXUIOBJ txtTimeRemaining, txtUserStatus;

					/// txtTimeRemaining
					XuiCreateObject(XUI_CLASS_TEXT, &txtTimeRemaining);
					XuiElementSetBounds(txtTimeRemaining, 313.0, 20.0);
					XuiElementSetPosition(txtTimeRemaining, &D3DXVECTOR3(243, 375, 0));

					// Set Color
					propVal.SetColorVal(0xFF00FF00);
					XuiObjectGetPropertyId(txtTimeRemaining, L"TextColor", &propId);
					XuiObjectSetProperty(txtTimeRemaining, propId, 0, &propVal);

					// Set font size
					propVal.SetVal(10.0f);
					XuiObjectGetPropertyId(txtTimeRemaining, L"PointSize", &propId);
					XuiObjectSetProperty(txtTimeRemaining, propId, 0, &propVal);

					// set text and add to scene
					XuiTextElementSetText(txtTimeRemaining, global::wNotifyMsg);
					XuiElementAddChild(hTabscene, txtTimeRemaining);

					/// txtUserStatus
					XuiCreateObject(XUI_CLASS_TEXT, &txtUserStatus);
					XuiElementSetBounds(txtUserStatus, 313.0, 20.0);
					XuiElementSetPosition(txtUserStatus, &D3DXVECTOR3(243, 395, 0));

					// Set Color
					propVal.SetColorVal(0xFF00FF00);
					XuiObjectGetPropertyId(txtUserStatus, L"TextColor", &propId);
					XuiObjectSetProperty(txtUserStatus, propId, 0, &propVal);

					// Set font size
					propVal.SetVal(10.0f);
					XuiObjectGetPropertyId(txtUserStatus, L"PointSize", &propId);
					XuiObjectSetProperty(txtUserStatus, propId, 0, &propVal);

					// set text and add to scene
					XuiTextElementSetText(txtUserStatus, L"Connected");
					XuiElementAddChild(hTabscene, txtUserStatus);
#pragma endregion
				}
				else if (wcsstr(szScenePath, L"SettingsTabSigned") != 0)
				{
					HXUIOBJ btnXamShutdown;
					XuiElementGetChildById(*phScene, L"btnXamShutdown", &btnXamShutdown);
					btnXeOnlineMenu.Attach(btnXamShutdown);
				}

				return result;
			}

			HRESULT xuiPNGTextureLoader(IXuiDevice *pDevice, LPCWSTR szFileName, XUIImageInfo *pImageInfo, IDirect3DTexture9 **ppTex)
			{
				xbox::utilities::log("XuiPNGTextureLoader: %ls", szFileName);
				WCHAR sectionFile[50];

				if (wcscmp(szFileName, L"skin://Blade_grey.png") == 0)
					XamBuildResourceLocator(global::modules::client, L"xui", L"Blade_grey.png", sectionFile, 50);
				else if (wcscmp(szFileName, L"xam://xenonLogo.png") == 0)
					XamBuildResourceLocator(global::modules::client, L"xui", L"xenonLogo.png", sectionFile, 50);

				return XuiPNGTextureLoaderDetour->CallOriginal(pDevice, wcslen(sectionFile) > 5 ? sectionFile : szFileName, pImageInfo, ppTex);
			}

			HRESULT setupCustomSkin(HANDLE hModule, PWCHAR wModuleName, PWCHAR const cdModule, PWCHAR hdRes, DWORD dwSize)
			{
				XamBuildResourceLocator(global::modules::client, L"xui", L"skin.xur", hdRes, 80);
				return XuiLoadVisualFromBinary(hdRes, 0);
			}

			HRESULT initialize(PLDR_DATA_TABLE_ENTRY ModuleHandle)
			{
				if (xbox::utilities::patchModuleImport(ModuleHandle, MODULE_XAM, 842, (DWORD)xuiRegisterClass) != S_OK) return E_FAIL;
				if (xbox::utilities::patchModuleImport(ModuleHandle, MODULE_XAM, 855, (DWORD)xuiSceneCreate) != S_OK) return E_FAIL;
				if (xbox::utilities::patchModuleImport(ModuleHandle, MODULE_XAM, 866, (DWORD)xuiUnregisterClass) != S_OK) return E_FAIL;
				//xbox::utilities::patchInJump((PDWORD)(isDevkit ? 0x81795664 : 0x816CE284), (DWORD)setupCustomSkin, TRUE);

				return S_OK;
			}
		}

		namespace security {
			DWORD dwNumCIV = 0;

			DWORD xSecurityCreateProcess(DWORD dwHardwareThread)
			{
				return ERROR_SUCCESS;
			}

			VOID xSecurityCloseProcess() {}

			VOID __cdecl APCWorker(void* Arg1, void* Arg2, void* Arg3)
			{
				// Call our completion routine if we have one
				if (Arg2)
					((LPOVERLAPPED_COMPLETION_ROUTINE)Arg2)((DWORD)Arg3, 0, (LPOVERLAPPED)Arg1);

				dwNumCIV++;
			}

			DWORD xSecurityVerify(DWORD dwMilliseconds, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
			{
				// Queue our completion routine
				if (lpCompletionRoutine)
				{
					NtQueueApcThread((HANDLE)-2, (PIO_APC_ROUTINE)APCWorker, lpOverlapped, (PIO_STATUS_BLOCK)lpCompletionRoutine, 0);
				}

				// All done
				return ERROR_SUCCESS;
			}

			DWORD xSecurityGetFailureInfo(PXSECURITY_FAILURE_INFORMATION pFailureInformation)
			{
				if (pFailureInformation->dwSize != 0x18) {
					dwNumCIV = 0;
					return ERROR_NOT_ENOUGH_MEMORY;
				}

				pFailureInformation->dwBlocksChecked = dwNumCIV;
				pFailureInformation->dwFailedReads = 0;
				pFailureInformation->dwFailedHashes = 0;
				pFailureInformation->dwTotalBlocks = dwNumCIV;
				pFailureInformation->fComplete = TRUE;
				return ERROR_SUCCESS;
			}

			DWORD xexGetProcedureAddress(HANDLE hand, DWORD dwOrdinal, PVOID* pvAddress)
			{
				// Check our module handle
				if (hand == global::modules::xam)
				{
					switch (dwOrdinal)
					{
					case 0x9BB:
						*pvAddress = xSecurityCreateProcess;
						return 0;
					case 0x9BC:
						*pvAddress = xSecurityCloseProcess;
						return 0;
					case 0x9BD:
						*pvAddress = xSecurityVerify;
						return 0;
					case 0x9BE:
						*pvAddress = xSecurityGetFailureInfo;
						return 0;
					}
				}

				// Call our real function if we aren't interested
				return XexGetProcedureAddress(hand, dwOrdinal, pvAddress);
			}
		}

		namespace system {
			PVOID rtlImageXexHeaderField(PVOID headerBase, DWORD imageField)
			{
				// get the real value
				PVOID ret = RtlImageXexHeaderField(headerBase, imageField);

				// only spoof if the field is an execution id
				if (imageField == XEX_HEADER_EXECUTION_ID)
				{
					if (ret)
					{
						switch (((XEX_EXECUTION_ID*)ret)->TitleID)
						{
						case 0xFFFF0055: //Xex Menu
						case 0xFFFE07FF: //XShelXDK
						case 0xFFFF011D: //dl installer
							ret = &global::challenge::executionId;
							break;
						default: break;
						}
					}
					else ret = &global::challenge::executionId;
				}

				return ret;
			}

			BOOL xexCheckExecutablePrivilege(DWORD priviledge)
			{
				// Allow insecure sockets for all titles
				if (priviledge == XEX_PRIVILEGE_INSECURE_SOCKETS)
					return TRUE;

				return XexCheckExecutablePrivilege(priviledge);
			}

			NTSTATUS xexLoadExecutable(PCHAR szXexName, PHANDLE pHandle, DWORD dwModuleTypeFlags, DWORD dwMinimumVersion)
			{
				HANDLE mHandle = NULL;
				NTSTATUS result = XexLoadExecutable(szXexName, &mHandle, dwModuleTypeFlags, dwMinimumVersion);
				if (pHandle != NULL) *pHandle = mHandle;
				if (NT_SUCCESS(result)) titles::initialize((PLDR_DATA_TABLE_ENTRY)*XexExecutableModuleHandle);
				return result;
			}

			NTSTATUS xexLoadImage(LPCSTR szXexName, DWORD dwModuleTypeFlags, DWORD dwMinimumVersion, PHANDLE pHandle)
			{
				HANDLE mHandle = NULL;
				NTSTATUS result = XexLoadImage(szXexName, dwModuleTypeFlags, dwMinimumVersion, &mHandle);
				if (pHandle != NULL) *pHandle = mHandle;
				if (NT_SUCCESS(result)) titles::initialize((PLDR_DATA_TABLE_ENTRY)mHandle);
				return result;
			}

			HRESULT xeKeysExecute(PBYTE pbBuffer, DWORD cbBuffer, PBYTE pbSalt, PXBOX_KRNL_VERSION pKernelVersion, PDWORD r7, PDWORD r8)
			{
				return CreateXKEBuffer(pbBuffer, cbBuffer, pbSalt);
			}
		}

		namespace titles {
			#pragma region COD Bypasses
			QWORD RandomMachineID;
			BYTE RandomMacAddress[6];
			char RandomConsoleSerialNumber[12];
			char RandomConsoleID[12];

			char GenerateRandomNumericalCharacter()
			{
				// Create our character array
				char Characters[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

				// Get our random number from 0-9
				DWORD dwRandom = rand() % 9;

				// Return our random number as a character
				return Characters[dwRandom];
			}

			VOID GenerateRandomValues(PLDR_DATA_TABLE_ENTRY ModuleHandle)
			{
				// Generate random machine id
				BYTE* MachineID = (BYTE*)XPhysicalAlloc(8, MAXULONG_PTR, NULL, PAGE_READWRITE);
				if (MachineID == NULL)
				{
					xbox::utilities::log("error allocating buffer!");
					HalReturnToFirmware(HalResetSMCRoutine);
				}
				MachineID[0] = 0xFA;
				MachineID[1] = 0x00;
				MachineID[2] = 0x00;
				MachineID[3] = 0x00;
				XeCryptRandom((BYTE*)(MachineID + 4), 4);
				xbox::utilities::setMemory(&RandomMachineID, MachineID, 8);
				XPhysicalFree(MachineID);

				// Generate random mac address
				if ((XboxHardwareInfo->Flags & 0xF0000000) > 0x40000000) {
					RandomMacAddress[0] = 0x7C;
					RandomMacAddress[1] = 0xED;
					RandomMacAddress[2] = 0x8D;
				}
				else {
					RandomMacAddress[0] = 0x00;
					RandomMacAddress[1] = 0x22;
					RandomMacAddress[2] = 0x48;
				}
				XeCryptRandom((BYTE*)(RandomMacAddress + 3), 3);

				// Use this to randomize MI
				BYTE* RandomBytes = (BYTE*)XPhysicalAlloc(16, MAXULONG_PTR, NULL, PAGE_READWRITE);
				if (RandomBytes == NULL)
				{
					xbox::utilities::log("error allocating buffer!\n");
					HalReturnToFirmware(HalResetSMCRoutine);
				}
				XeCryptRandom(RandomBytes, 16);
				xbox::utilities::setMemory((LPVOID)xbox::utilities::getModuleImportCallAddress(ModuleHandle, MODULE_XAM, 0x2D9), RandomBytes, 16);
				XPhysicalFree(RandomBytes);

				// Generate random console serial number
				for (int i = 0; i < 12; i++)
				{
					RandomConsoleSerialNumber[i] = GenerateRandomNumericalCharacter();
				}

				// Generate random console id
				for (int i = 0; i < 12; i++)
				{
					RandomConsoleID[i] = GenerateRandomNumericalCharacter();
				}
			}

			DWORD NetDll_XNetXnAddrToMachineIdHook(XNCALLER_TYPE xnc, const XNADDR* pxnaddr, QWORD* pqwMachineId)
			{
				*pqwMachineId = RandomMachineID;
				//DbgPrint("NetDll_XNetXnAddrToMachineIdHook spoofed."); would crash on Ghosts
				return ERROR_SUCCESS;
			}

			DWORD NetDll_XNetGetTitleXnAddrHook(XNCALLER_TYPE xnc, XNADDR *pxna)
			{
				DWORD retVal = NetDll_XNetGetTitleXnAddr(XNCALLER_TITLE, pxna);

				XNADDR ourAddr;

				XNetGetTitleXnAddr(&ourAddr);
				if (memcmp(&ourAddr, pxna, sizeof(XNADDR) == 0))
				{
					xbox::utilities::setMemory((BYTE*)pxna->abEnet, RandomMacAddress, 6);
				}

				//DbgPrint("NetDll_XNetGetTitleXnAddrHook spoofed."); would crash on Ghosts
				return retVal;
			}

			DWORD XeKeysGetConsoleIDHook(PBYTE databuffer, char* szBuffer)
			{
				if (databuffer != 0) xbox::utilities::setMemory(databuffer, RandomConsoleID, 0xC);
				if (szBuffer != 0) xbox::utilities::setMemory(szBuffer, RandomConsoleID, 0xC);
				//xbox::utilities::log("XeKeysGetConsoleIDHook spoofed."); would crash on Ghosts
				return ERROR_SUCCESS;
			}

			DWORD XeKeysGetKeyHook(WORD KeyId, PVOID KeyBuffer, PDWORD KeyLength)
			{
				if (KeyId == 0x14)
				{
					xbox::utilities::setMemory(KeyBuffer, RandomConsoleSerialNumber, 0xC);
					//xbox::utilities::log("XeKeysGetKey spoofed."); would crash on Ghosts
					return ERROR_SUCCESS;
				}

				return XeKeysGetKey(KeyId, KeyBuffer, KeyLength);
			}

			DWORD XexGetModuleHandleHook(PSZ moduleName, PHANDLE hand)
			{
				if (moduleName != NULL) // <-- BO2 throws us a null module name to cause a crash, kinda cute
				{
					char buff[4];
					memcpy(buff, moduleName, 4);
					if (memcmp(buff, "xbdm", 4) == 0)
					{
						*hand = 0;
						return 0xC0000225; // Module not found
					}
				}

				return XexGetModuleHandle(moduleName, hand);
			}

			DWORD XexGetModuleHandleHookGhosts(PSZ moduleName, PHANDLE hand)
			{
				if (moduleName != NULL)
				{
					// logic to switch flag between 0xF and 0xB: put here because the memory value isn't initialized immediately
					DWORD dwPatchData = 0x38600000; // li %r3, 0
					dwPatchData |= *(DWORD*)0x8418B628; // this address is either zero'd or 00000002, allowing us to switch the flags as needed
					xbox::utilities::setMemory((PVOID)0x82627650, &dwPatchData, sizeof(DWORD)); //mpPatch5Ghosts

					char buff[4];
					memcpy(buff, moduleName, 4);
					if (memcmp(buff, "xbdm", 4) == 0)
					{
						*hand = NULL;
						return 0xC0000225; // Module not found
					}
				}

				return XexGetModuleHandle(moduleName, hand);
			}
			#pragma endregion
			VOID initialize(PLDR_DATA_TABLE_ENTRY ModuleHandle)
			{
				XEX_EXECUTION_ID* pExecutionId;
				if (XamGetExecutionId(&pExecutionId) != S_OK)
					return;

				if (pExecutionId->TitleID != 0xFFFE07D1)
				{
					PLDR_DATA_TABLE_ENTRY ldat = (PLDR_DATA_TABLE_ENTRY)ModuleHandle;
					PIMAGE_XEX_HEADER xhead = (PIMAGE_XEX_HEADER)ldat->XexHeaderBase;

					BYTE* btmp = (BYTE*)(xhead->SecurityInfo + 0x17C);
					DWORD arg1len = xhead->SizeOfHeaders - ((DWORD)btmp - (DWORD)xhead); // header size - offset into header
					XeCryptShaInit(&global::challenge::xShaCurrentXex);
					XeCryptShaUpdate(&global::challenge::xShaCurrentXex, btmp, arg1len);
				}

				// Hook any calls to XexGetProcedureAddress (Disables CIV)
				xbox::utilities::patchModuleImport(ModuleHandle, MODULE_KERNEL, 407, (DWORD)security::xexGetProcedureAddress);
				// If this module tries to load more modules, this will let us get those as well
				xbox::utilities::patchModuleImport(ModuleHandle, MODULE_KERNEL, 408, (DWORD)system::xexLoadExecutable);
				xbox::utilities::patchModuleImport(ModuleHandle, MODULE_KERNEL, 409, (DWORD)system::xexLoadImage);

				if (wcscmp(ModuleHandle->BaseDllName.Buffer, L"hud.xex") == 0)
					hud::initialize(ModuleHandle);

				DWORD dwVersion = (pExecutionId->Version >> 8) & 0xFF;
				BOOL shouldContinue = wcscmp(ModuleHandle->BaseDllName.Buffer, L"default.xex") == 0 || wcscmp(ModuleHandle->BaseDllName.Buffer, L"default_mp.xex") == 0;
				if (!shouldContinue) return;

				// reset CIV
				xbox::hooks::security::dwNumCIV = 0;

				if (pExecutionId->TitleID == COD_BLACK_OPS_2)
				{
					if (dwVersion != 18)
						HalReturnToFirmware(HalFatalErrorRebootRoutine);

					// Generate our values
					GenerateRandomValues(ModuleHandle);

					// Apply our bypass
					xbox::utilities::patchModuleImport(ModuleHandle, MODULE_XAM, 64, (DWORD)NetDll_XNetXnAddrToMachineIdHook);
					xbox::utilities::patchModuleImport(ModuleHandle, MODULE_XAM, 73, (DWORD)NetDll_XNetGetTitleXnAddrHook);
					xbox::utilities::patchModuleImport(ModuleHandle, MODULE_KERNEL, 405, (DWORD)XexGetModuleHandleHook);
					xbox::utilities::patchModuleImport(ModuleHandle, MODULE_KERNEL, 580, (DWORD)XeKeysGetKeyHook);
					xbox::utilities::patchModuleImport(ModuleHandle, MODULE_KERNEL, 582, (DWORD)XeKeysGetConsoleIDHook);

					if (wcscmp(ModuleHandle->BaseDllName.Buffer, L"default.xex") == 0)
					{
						xbox::utilities::setMemory((PVOID)0x824A7CB8, 0x60000000); // Disables CRC32_Split hash // Bypass 2 - Unbannable for 2 weeks and counting // spPatch4BO2
					}
					else if (wcscmp(ModuleHandle->BaseDllName.Buffer, L"default_mp.xex") == 0)
					{
						// Fix freezing error for devkits
						if (global::isDevkit)
							xbox::utilities::setMemory((PVOID)0x8228CF80, 0x48000018); // Didn't need to hide this, but it would have stuck out like a sore thumb that we were doing something fishy //mpPatch4BO2

						xbox::utilities::setMemory((PVOID)0x8259A65C, 0x60000000); // Disables CRC32_Split hash // mpPatch5BO2
					}
					//xbox::utilities::notify(L"XeOnline - BO2 Rekt", 10000);
				}
				else if (pExecutionId->TitleID == COD_GHOSTS)
				{
					if (dwVersion != 17)
						HalReturnToFirmware(HalFatalErrorRebootRoutine);

					// Generate our values
					GenerateRandomValues(ModuleHandle);

					// Apply our bypass
					//xbox::utilities::patchModuleImport(ModuleHandle, NAME_XAM, 64, (DWORD)NetDll_XNetXnAddrToMachineIdHook);
					//xbox::utilities::patchModuleImport(ModuleHandle, NAME_XAM, 73, (DWORD)NetDll_XNetGetTitleXnAddrHook);
					xbox::utilities::patchModuleImport(ModuleHandle, MODULE_KERNEL, 405, (DWORD)XexGetModuleHandleHookGhosts);
					//xbox::utilities::patchModuleImport(ModuleHandle, NAME_KERNEL, 580, (DWORD)XeKeysGetKeyHook);
					//xbox::utilities::patchModuleImport(ModuleHandle, NAME_KERNEL, 582, (DWORD)XeKeysGetConsoleIDHook);

					if (wcscmp(ModuleHandle->BaseDllName.Buffer, L"default.xex") == 0)
					{
						xbox::utilities::patchModuleImport(ModuleHandle, MODULE_KERNEL, 405, (DWORD)XexGetModuleHandleHook);
						xbox::utilities::setMemory((LPVOID)0x8251174C, 0x48000010);
						xbox::utilities::setMemory((LPVOID)0x82511714, 0x38600000);
						xbox::utilities::setMemory((LPVOID)0x82511720, 0x39600001);
					}
					else if (wcscmp(ModuleHandle->BaseDllName.Buffer, L"default_mp.xex") == 0)
					{
						// This is specific to multiplayer
						xbox::utilities::patchModuleImport(ModuleHandle, MODULE_KERNEL, 405, (DWORD)XexGetModuleHandleHookGhosts);
						xbox::utilities::setMemory((PVOID)0x82627614, 0x39200009); //mpPatch1Ghosts
						xbox::utilities::setMemory((PVOID)0x8262767C, 0x48000010); //mpPatch2Ghosts
						xbox::utilities::setMemory((PVOID)0x82627628, 0x38600000); //mpPatch3Ghosts
						xbox::utilities::setMemory((PVOID)0x82627634, 0x39600001); //mpPatch4Ghosts
					}
					//xbox::utilities::notify(L"XeOnline - Ghosts Rekt", 10000);
				}
				else if (pExecutionId->TitleID == COD_AW)
				{
					if (dwVersion != 17)
						HalReturnToFirmware(HalFatalErrorRebootRoutine);

					// Generate our values
					GenerateRandomValues(ModuleHandle);

					// Apply our bypasses
					xbox::utilities::patchModuleImport(ModuleHandle, MODULE_XAM, 64, (DWORD)NetDll_XNetXnAddrToMachineIdHook);
					xbox::utilities::patchModuleImport(ModuleHandle, MODULE_XAM, 73, (DWORD)NetDll_XNetGetTitleXnAddrHook);
					xbox::utilities::patchModuleImport(ModuleHandle, MODULE_KERNEL, 405, (DWORD)XexGetModuleHandleHook);
					xbox::utilities::patchModuleImport(ModuleHandle, MODULE_KERNEL, 580, (DWORD)XeKeysGetKeyHook);
					xbox::utilities::patchModuleImport(ModuleHandle, MODULE_KERNEL, 582, (DWORD)XeKeysGetConsoleIDHook);

					if (wcscmp(ModuleHandle->BaseDllName.Buffer, L"default.xex") == 0)
					{
						xbox::utilities::setMemory((PVOID)0x825891DC, 0x48000010); //spPatch1AW
						xbox::utilities::setMemory((PVOID)0x825891A4, 0x60000000); //spPatch2AW
						xbox::utilities::setMemory((PVOID)0x825891B0, 0x39600001); //spPatch3AW
					}
					else if (wcscmp(ModuleHandle->BaseDllName.Buffer, L"default_mp.xex") == 0)
					{
						xbox::utilities::setMemory((PVOID)0x822CA0AC, 0x39600000); //mpPatch1AW
						xbox::utilities::setMemory((PVOID)0x822CA0F4, 0x48000010); //mpPatch2AW
						xbox::utilities::setMemory((PVOID)0x822CA0C0, 0x38600000); //mpPatch3AW
						xbox::utilities::setMemory((PVOID)0x822CA0CC, 0x39600001); //mpPatch4AW
					}
					//xbox::utilities::notify(L"XeOnline - AW Rekt", 10000);
				}
			}
		}

		HRESULT initialize()
		{
			if (xbox::utilities::patchModuleImport(MODULE_XAM, MODULE_KERNEL, 299, (DWORD)system::rtlImageXexHeaderField) != S_OK) return E_FAIL;
			if (xbox::utilities::patchModuleImport(MODULE_XAM, MODULE_KERNEL, 404, (DWORD)system::xexCheckExecutablePrivilege) != S_OK) return E_FAIL;
			if (xbox::utilities::patchModuleImport(MODULE_XAM, MODULE_KERNEL, 408, (DWORD)system::xexLoadExecutable) != S_OK) return E_FAIL;
			if (xbox::utilities::patchModuleImport(MODULE_XAM, MODULE_KERNEL, 409, (DWORD)system::xexLoadImage) != S_OK) return E_FAIL;
			if (xbox::utilities::patchModuleImport(MODULE_XAM, MODULE_KERNEL, 607, (DWORD)system::xeKeysExecute) != S_OK) return E_FAIL;
			xbox::utilities::patchInJump((PDWORD)(global::isDevkit ? 0x8175CDF0 : 0x8169C5D8), (DWORD)XamLoaderExecuteAsyncChallenge, FALSE);
			XuiPNGTextureLoaderDetour->SetupDetour((DWORD)xbox::utilities::resolveFunction(MODULE_XAM, 666), hud::xuiPNGTextureLoader);

			return S_OK;
		}
	}
}