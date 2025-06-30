#pragma once
#include <windows.h>
#include <psapi.h>
#include <shlwapi.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <string>
#include <conio.h>
#include <tlhelp32.h>
#include <locale>
#include <codecvt>
#include <VersionHelpers.h>  
#include "ramf.hpp"
#include <shlobj_core.h> 
#include <map>
#include <deque>
#include <iomanip>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace std;

HybridVector<wstring> processes;
int selectedIndex = 0;



struct WindowEnumContext {
    wstring targetExe;
    HybridVector<DWORD> matchedPIDs;
};

std::wstring GetExecutablePath() {
    std::vector<wchar_t> pathBuffer(MAX_PATH);
    DWORD result = GetModuleFileNameW(NULL, pathBuffer.data(), MAX_PATH);

    if (result >= MAX_PATH) {
        pathBuffer.resize(32767);
        result = GetModuleFileNameW(NULL, pathBuffer.data(), static_cast<DWORD>(pathBuffer.size()));
    }

    if (result == 0) {
        DWORD err = GetLastError();
        throw std::runtime_error("Error getting path. Code: " + std::to_string(err));
    }

    return std::wstring(pathBuffer.data());
}

void DisableConsoleMouseInput() {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD prevMode;
    GetConsoleMode(hInput, &prevMode);
    SetConsoleMode(hInput, prevMode & ~(ENABLE_QUICK_EDIT_MODE | ENABLE_MOUSE_INPUT));
}

bool IsUserAdmin() {
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup)) {
        if (!CheckTokenMembership(NULL, AdministratorsGroup, &isAdmin)) {
            isAdmin = FALSE;
        }
        FreeSid(AdministratorsGroup);
    }
    return isAdmin == TRUE;
}

void admin() {
    if (IsUserAdmin()) {
        return;
    }

    wstring exePath = GetExecutablePath();
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = exePath.c_str();
    sei.nShow = SW_SHOWNORMAL;

    if (ShellExecuteEx(&sei)) {
        ExitProcess(0);
    }
    else {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) {
            wcout << L"Operation cancelled by user" << endl;
        }
        else {
            wcout << L"Admin request failed: " << err << endl;
        }
    }
}

wstring MultiByteToWide(const string& str) {
    int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], count);
    return wstr;
}

string WideToMultiByte(const wstring& wstr) {
    int count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    string str(count, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], count, NULL, NULL);
    return str;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    if (!IsWindowVisible(hwnd)) return TRUE;

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return TRUE;

    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProc) {
        WCHAR exeName[MAX_PATH];
        if (GetModuleFileNameExW(hProc, NULL, exeName, MAX_PATH)) {
            const WCHAR* exeBaseName = PathFindFileNameW(exeName);
            wstring exeStr = exeBaseName;

            if (lParam == 0) {
                if (find(processes.begin(), processes.end(), exeStr) == processes.end()) {
                    processes.push_back(exeStr);
                }
            }
            else {
                WindowEnumContext* ctx = (WindowEnumContext*)lParam;
                if (_wcsicmp(exeBaseName, ctx->targetExe.c_str()) == 0) {
                    ctx->matchedPIDs.push_back(pid);
                }
            }
        }
        CloseHandle(hProc);
    }
    return TRUE;
}

HybridVector<DWORD> FindProcessesByName(const wstring& name) {
    HybridVector<DWORD> pids;
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return pids;

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, name.c_str()) == 0) {
                pids.push_back(pe32.th32ProcessID);
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return pids;
}

void GetTaskManagerLikeMemoryInfo(DWORD pid, SIZE_T& privateWS, SIZE_T& sharedWS) {
    privateWS = 0;
    sharedWS = 0;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) return;

    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        privateWS = pmc.PrivateUsage;
        sharedWS = pmc.WorkingSetSize - pmc.PrivateUsage;
    }
    CloseHandle(hProcess);
}

void coutRAM(const wstring& exeP, SIZE_T currentMem = 0, bool forceUpdate = false) {
    static map<wstring, deque<SIZE_T>> memoryHistory;
    auto& history = memoryHistory[exeP];

    if (!forceUpdate) {
        HybridVector<DWORD> pids = FindProcessesByName(exeP);
        currentMem = 0;
        if (pids.empty()) {
            wcout << L"Process '" << exeP << L"' not found.             " << flush;
            return;
        }
        for (DWORD pid : pids) {
            SIZE_T privateWS, sharedWS;
            GetTaskManagerLikeMemoryInfo(pid, privateWS, sharedWS);
            currentMem += privateWS + sharedWS;
        }
    }

    history.push_back(currentMem);
    if (history.size() > 5) history.pop_front();

    SIZE_T minNonZero = 0;
    for (SIZE_T val : history) {
        if (val > 0 && (minNonZero == 0 || val < minNonZero)) {
            minNonZero = val;
        }
    }

    wcout << L"\r~RAM: ";
    if (currentMem > 0) {
        wcout << setw(6) << (currentMem / (1024 * 1024)) << L" MB      ";
    }
    else if (minNonZero > 0) {
        wcout << L"~" << setw(5) << (minNonZero / (1024 * 1024)) << L" MB (min)";
    }
    else {
        wcout << setw(6) << L"0 MB      ";
    }
    wcout << flush;
}

void ram(const wstring& exeP) {
    HybridVector<DWORD> pids = FindProcessesByName(exeP);
    SIZE_T currentMem = 0;
    bool hasMemory = false;

    for (DWORD pid : pids) {
        SIZE_T privateWS, sharedWS;
        GetTaskManagerLikeMemoryInfo(pid, privateWS, sharedWS);
        currentMem += privateWS + sharedWS;
        if (privateWS + sharedWS > 0) hasMemory = true;
    }

    if (hasMemory) {
        for (DWORD pid : pids) {
            if (HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid)) {
                EmptyWorkingSet(hProcess);
                CloseHandle(hProcess);
            }
        }
        this_thread::sleep_for(300ms);
    }
    coutRAM(exeP, currentMem, true);
}



void DrawMenu() {
    system("cls");
    wcout << L"Use arrows to select, Enter to optimize, Esc to exit\n\n";
    for (int i = 0; i < (int)processes.size(); i++) {
        if (i == selectedIndex)
            wcout << L"> " << processes[i] << endl;
        else
            wcout << L"  " << processes[i] << endl;
    }
}