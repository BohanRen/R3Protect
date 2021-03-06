// user64.cpp: 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <windows.h>  
#include <tlhelp32.h>  
#include <Psapi.h>  
#include <fstream>
#include <sstream>
#pragma comment(lib,"psapi.lib") 
#include "user64.h"
#include "MinHook/include/MinHook.h"

#define HOOK_PROCESS_NAME L"taskmgr.exe"

HINSTANCE g_hInstance = NULL;
HWND g_hWnd = NULL;
HHOOK HT = NULL;
bool isAimProcess = false;
DWORD ProtectPID = 0;

typedef HANDLE (__stdcall WINAPI *OPENPROCESS)(DWORD, BOOL, DWORD);

// Pointer for calling original MessageBoxW.
OPENPROCESS fpOpenProcess = NULL;

LRESULT CALLBACK CProc(int nCode, WPARAM wParam, LPARAM lParam) {
	return CallNextHookEx(HT, nCode, wParam, lParam);
}

//安装钩子  
extern "C" __declspec(dllexport) BOOL SetHook() {
	HT = SetWindowsHookEx(WH_CALLWNDPROC, CProc, g_hInstance, 0);
	if (HT == NULL) {
		return false;
	}
	return true;
}

//卸载钩子  
extern "C" __declspec(dllexport) BOOL UnHook() {
	BOOL HM_BOOL = FALSE;
	if (HT != NULL) {
		HM_BOOL = UnhookWindowsHookEx(HT);
	}
	return HM_BOOL;
}


wchar_t* GetProcessName(DWORD processID) {
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	wchar_t *procName = new wchar_t[MAX_PATH];
	GetModuleFileNameEx(hProcess, NULL, procName, MAX_PATH);
	CloseHandle(hProcess);
	return procName;
}
wchar_t* GetProcessName(wchar_t *FileName) {
	size_t len = wcslen(FileName);
	size_t i = len - 1;
	for (; i >= 0; i--) {
		if (FileName[i] == L'\\') {
			break;
		}
	}
	wchar_t *temp = FileName + i + 1;
	return temp;
}

HANDLE WINAPI MyOpenProcess(DWORD dwDesiredAccess,
	BOOL bInheritHandle,
	DWORD dwProcessId)
{
	if (dwProcessId == ProtectPID) {
		dwDesiredAccess &= ~PROCESS_TERMINATE;
	}
	return fpOpenProcess(dwDesiredAccess,bInheritHandle,dwProcessId);
}

int hookOpenProcess() {
	if (MH_Initialize() != MH_OK)
	{
		MessageBoxA(0, "hook 失败", "", 0);
		return -1;
	}
	if (MH_CreateHook(&OpenProcess, &MyOpenProcess,
		reinterpret_cast<void**>(&fpOpenProcess)) != MH_OK)
	{
		MessageBoxA(0,"hook 失败","",0);
		return -1;
	}

	if (MH_EnableHook(&OpenProcess) != MH_OK)
	{
		MessageBoxA(0, "hook 失败", "", 0);
		return 1;
	}
}

void dllmain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved) {

	g_hInstance = hModule;
	std::ifstream fin("C://protect.syscfg");
	int tmp;
	fin >> tmp;
	ProtectPID = tmp - 20030922;
	wchar_t *procName = GetProcessName(GetCurrentProcessId());
	if (_wcsicmp(HOOK_PROCESS_NAME, GetProcessName(procName)) == 0) {
		hookOpenProcess();
		isAimProcess = true;
	}
}

void dllExit() {
	if (!isAimProcess) {
		return;
	}
	if (MH_DisableHook(&OpenProcess) != MH_OK)
	{
		MessageBoxA(0, "hook 失败", "", 0);
	}
}