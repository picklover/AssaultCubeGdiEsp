#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

typedef struct _IOPACKAGE
{
	PULONG controlcode;
	PULONG para1;
	PULONG para2;
	PULONG para3;
	PULONG pret;
}IOPACKAGE, * PIOPACKAGE;

class ProcMem
{
public:
	BOOL useKe;
	HANDLE hProcess;
	DWORD pid;
	ULONG_PTR base;
private:
	HKEY hKey;
	IOPACKAGE iop;
public:
	ProcMem():useKe(false), hProcess(NULL),pid(NULL),base(NULL),hKey(NULL),iop{0} {}
	~ProcMem() {
		if(hProcess)
			CloseHandle(hProcess);
		if(useKe)
			SHDeleteKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\DRVMEM");
	}

private:
	void keReadProcessMemory(void* lpBaseAddress, void* buffer, ULONG size) {
		iop.controlcode = (PULONG)0x2244CC;
		iop.para1 = (PULONG)lpBaseAddress;
		iop.para2 = (PULONG)buffer;
		iop.para3 = (PULONG)size;
		iop.pret = 0;
		if (RegCreateKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\DRVMEM", &hKey) == ERROR_SUCCESS) {
			RegSetValueEx(hKey, L"ReadMEM", 0, REG_BINARY, (CONST BYTE*) & iop, 40);
			RegCloseKey(hKey);
		}
	}

	void keWriteProcessMemory(void* lpBaseAddress, void* buffer, ULONG size) {
		iop.controlcode = (PULONG)0x2244F4;
		iop.para1 = (PULONG)lpBaseAddress;
		iop.para2 = (PULONG)buffer;
		iop.para3 = (PULONG)size;
		iop.pret = 0;
		if (RegCreateKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\DRVMEM", &hKey) == ERROR_SUCCESS) {
			RegSetValueEx(hKey, L"WriteMEM", 0, REG_BINARY, (CONST BYTE*) & iop, 40);
			RegCloseKey(hKey);
		}
	}

public:
	BOOL Init(LPCSTR ClassName, LPCSTR WindowName, DWORD pid) {
		if (!pid) {
			HWND hWindow = FindWindowA(ClassName, WindowName);
			if (!hWindow) {
				std::cout << "[ProcMem] Process Is Not Running!" << std::endl;
				return false;
			}
			GetWindowThreadProcessId(hWindow, &this->pid);
		}
		else
			this->pid = pid;
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, this->pid);
		if (!hProcess) {
			std::cout << "[ProcMem] Can Not Open Target Process!" << std::endl;
			return false;
		}
		HANDLE hModule = INVALID_HANDLE_VALUE;
		MODULEENTRY32 EP;
		hModule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, this->pid);
		EP.dwSize = sizeof(MODULEENTRY32);
		Module32First(hModule, &EP);
		base = (ULONG_PTR)EP.modBaseAddr;
		CloseHandle(hModule);
		return true;
	}
	BOOL InitKe(LPCSTR ClassName, LPCSTR WindowName, DWORD pid) {
		useKe = true;
		if (!pid) {
			HWND hWindow = FindWindowA(ClassName, WindowName);
			if (!hWindow) {
				std::cout << "[ProcMem] Process Is Not Running!" << std::endl;
				return false;
			}
			GetWindowThreadProcessId(hWindow, &this->pid);
		}
		else {
			this->pid = pid;
		}
		iop.controlcode = (PULONG)0x2244A4;
		iop.para1 = 0;
		iop.para2 = (PULONG)pid;
		iop.para3 = (PULONG)1;
		iop.pret = 0;
		if (RegCreateKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\DRVMEM", &hKey) != ERROR_SUCCESS)
		{
			std::cout << "[ProcMem] Can't create registry key for IO!" << std::endl;
			return false;
		}
		RegSetValueEx(hKey, L"InitEprocess", 0, REG_BINARY, (CONST BYTE*) & iop, 40);
		RegCloseKey(hKey);
		return true;
	}
	template <class T>
	T read(ULONG_PTR(Address)) {
		try {
			if (pid > 0) {
				T B;
				if (useKe == false) {
					ReadProcessMemory(hProcess, (LPVOID)(Address), &B, sizeof(T), NULL);
				}
				else {
					keReadProcessMemory((LPVOID)(Address), &B, sizeof(T));
				}
				return B;
			}
			else
				throw 1;
		}
		catch (int error) {
			std::cout << "[ProcMem] ReadMem Failed!" << std::endl;
		}
	}

	template <class cData>
	cData write(ULONG_PTR(Address), cData B) {
		try {
			if (pid > 0) {
				if (useKe == false) {
					WriteProcessMemory(hProcess, (LPVOID)(Address), &B, sizeof(B), NULL);
				}
				else {
					keWriteProcessMemory((LPVOID)(Address), &B, sizeof(B));
				}
				return B;
			}
			else
				throw 1;
		}
		catch (int error) {
			std::cout << "[Procmem] WriteMem Failed!" << std::endl;
		}
	}
};
