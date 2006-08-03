#include "HookImportFunction.h"
#include <tlhelp32.h>


// These code come from: http://dev.csdn.net/article/2/2786.shtm
// I fixed a bug in it and improved it to hook all the modules of a program.

#define MakePtr(cast, ptr, AddValue) (cast)((DWORD)(ptr)+(DWORD)(AddValue))

static PIMAGE_IMPORT_DESCRIPTOR GetNamedImportDescriptor(HMODULE hModule, LPCSTR szImportModule)
{
	if ((szImportModule == NULL) || (hModule == NULL))
		return NULL;
	PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER) hModule;
	if (IsBadReadPtr(pDOSHeader, sizeof(IMAGE_DOS_HEADER)) || (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)) {
		return NULL;
	}
	PIMAGE_NT_HEADERS pNTHeader = MakePtr(PIMAGE_NT_HEADERS, pDOSHeader, pDOSHeader->e_lfanew);
	if (IsBadReadPtr(pNTHeader, sizeof(IMAGE_NT_HEADERS)) || (pNTHeader->Signature != IMAGE_NT_SIGNATURE))
		return NULL;
	if (pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress == 0)
		return NULL;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = MakePtr(PIMAGE_IMPORT_DESCRIPTOR, pDOSHeader, pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	while (pImportDesc->Name) {
		PSTR szCurrMod = MakePtr(PSTR, pDOSHeader, pImportDesc->Name);
		if (stricmp(szCurrMod, szImportModule) == 0)
			break;
		pImportDesc++;
	}
	if (pImportDesc->Name == (DWORD)0)
		return NULL;
	return pImportDesc;
}

static BOOL IsNT()
{
	OSVERSIONINFO stOSVI;
	memset(&stOSVI, 0, sizeof(OSVERSIONINFO));
	stOSVI.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	BOOL bRet = GetVersionEx(&stOSVI);
	if (FALSE == bRet) return FALSE;
	return (VER_PLATFORM_WIN32_NT == stOSVI.dwPlatformId);
}

static BOOL HookImportFunction(HMODULE hModule, LPCSTR szImportModule, LPCSTR szFunc, PROC paHookFuncs, PROC* paOrigFuncs)
{
	if (!IsNT() && ((DWORD)hModule >= 0x80000000))
		return FALSE;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = GetNamedImportDescriptor(hModule, szImportModule);
	if (pImportDesc == NULL)
		return FALSE;
	PIMAGE_THUNK_DATA pOrigThunk = MakePtr(PIMAGE_THUNK_DATA, hModule, pImportDesc->OriginalFirstThunk);
	PIMAGE_THUNK_DATA pRealThunk = MakePtr(PIMAGE_THUNK_DATA, hModule, pImportDesc->FirstThunk);
	while (pOrigThunk->u1.Function) {
		if (IMAGE_ORDINAL_FLAG != (pOrigThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)) {
			PIMAGE_IMPORT_BY_NAME pByName = MakePtr(PIMAGE_IMPORT_BY_NAME, hModule, pOrigThunk->u1.AddressOfData);
			// When hook EditPlus, read pByName->Name[0] will case this dll terminate, so call IsBadReadPtr() here.
			if (IsBadReadPtr(pByName, sizeof(IMAGE_IMPORT_BY_NAME))) {
				pOrigThunk++;
				pRealThunk++;
				continue;				
			}
			if ('\0' == pByName->Name[0]) {
				pOrigThunk++;
				pRealThunk++;
				continue;
			}
			BOOL bDoHook = FALSE;
			if ((szFunc[0] == pByName->Name[0]) && (strcmpi(szFunc, (char*)pByName->Name) == 0)) {
				if (paHookFuncs)
					bDoHook = TRUE;
			}
			if (bDoHook) {
				MEMORY_BASIC_INFORMATION mbi_thunk;
				VirtualQuery(pRealThunk, &mbi_thunk, sizeof(MEMORY_BASIC_INFORMATION));
				VirtualProtect(mbi_thunk.BaseAddress, mbi_thunk.RegionSize, PAGE_READWRITE, &mbi_thunk.Protect);
				if (paOrigFuncs)
					*paOrigFuncs = (PROC)pRealThunk->u1.Function;
				pRealThunk->u1.Function = (DWORD)paHookFuncs;
				DWORD dwOldProtect;
				VirtualProtect(mbi_thunk.BaseAddress, mbi_thunk.RegionSize, mbi_thunk.Protect, &dwOldProtect);
				return TRUE;
			}
		}
		pOrigThunk++;
		pRealThunk++;
	}
	return FALSE;
}

BOOL HookAPI(LPCSTR szImportModule, LPCSTR szFunc, PROC paHookFuncs, PROC* paOrigFuncs)
{
	if ((szImportModule == NULL) || (szFunc == NULL)) {
		return FALSE;
	}
	HANDLE hSnapshot;
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,0);
	MODULEENTRY32 me = {sizeof(MODULEENTRY32)};
	BOOL bOk = Module32First(hSnapshot,&me);
	while (bOk) {
		HookImportFunction(me.hModule, szImportModule, szFunc, paHookFuncs, paOrigFuncs);
		bOk = Module32Next(hSnapshot,&me);
	}
	return TRUE;
}
