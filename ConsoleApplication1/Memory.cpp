#include "Memory.h"
#include <cstring>

bool Memory::AttachProcess(const char* targetName, InitMode init_mode)
{
    // how to get PID
    if (init_mode == WINDOW_TITLE || init_mode == WINDOW_CLASS)
    {
        HWND hWnd = init_mode == WINDOW_TITLE ? FindWindowA(nullptr, targetName) : FindWindowA(targetName, nullptr);

        if (!hWnd) {
            MessageBoxA(nullptr, "please open fivem", "Initialize Failed", MB_TOPMOST | MB_ICONERROR | MB_OK);
            return false;
        }

        GetWindowThreadProcessId(hWnd, &m_dwPID);
    }
    else if (init_mode == PROCESS_NAME)
    {
        PROCESSENTRY32 process = GetProcess(targetName);

        if (process.th32ProcessID == 0) {
            MessageBoxA(nullptr, "please open fivem", "Initialize Failed", MB_TOPMOST | MB_ICONERROR | MB_OK);
            return false;
        }

        m_dwPID = process.th32ProcessID;
    }

    // プロセスハンドルを取得
    m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_dwPID);

    if (!m_hProcess) {
        MessageBoxA(nullptr, "Failed to get process handle", "Init Error", MB_TOPMOST | MB_ICONERROR | MB_OK);
        return false;
    }

    char ModuleName[128]{};
    GetModuleBaseNameA(m_hProcess, nullptr, ModuleName, sizeof(ModuleName));
    m_gClientBaseAddr = GetModuleBase(ModuleName);

    return true;
}

void Memory::DetachProcess()
{
    CloseHandle(m_hProcess);
}

char* Memory::GetModuleName()
{
    char pModule[128]{};
    GetModuleBaseNameA(m.m_hProcess, nullptr, pModule, sizeof(pModule));
    return pModule;
}

uintptr_t Memory::FindPattern(const std::vector<uint8_t>& read_data, const std::string pattern, int offset, int extra)
{
    std::vector<uint8_t> bytes;
    std::istringstream patternStream(pattern);
    std::string byteStr;

    while (patternStream >> byteStr) {
        if (byteStr == "?" || byteStr == "??")
            bytes.push_back(0);
        else
            bytes.push_back(static_cast<uint8_t>(strtol(byteStr.c_str(), nullptr, 16)));
    }

    for (size_t i = 1000000; i < read_data.size(); ++i)
    {
        bool patternMatch = true;
        for (size_t j = 0; j < bytes.size(); ++j)
        {
            if (bytes[j] != 0 && read_data[i + j] != bytes[j])
            {
                patternMatch = false;
                break;
            }
        }

        if (patternMatch) {
            uintptr_t patternAddress = m_gClientBaseAddr + i;
            int32_t of;
            ReadProcessMemory(m_hProcess, reinterpret_cast<LPCVOID>(patternAddress + offset), &of, sizeof(of), nullptr);
            uintptr_t result = patternAddress + of + extra;
            bytes.clear();
            return (result - m_gClientBaseAddr);
        }
    }
    return 0;
}

MODULEINFO Memory::GetModuleInfo(const std::string moduleName)
{
    DWORD cb;
    HMODULE hMods[256]{};
    MODULEINFO modInfo{};

    if (EnumProcessModules(m_hProcess, hMods, sizeof(hMods), &cb)) 
    {
        for (unsigned int i = 0; i < (cb / sizeof(HMODULE)); i++) 
        {
            char szModName[MAX_PATH];
            if (GetModuleBaseNameA(m_hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(char)))
            {
                if (strcmp(moduleName.c_str(), szModName) == 0) {
                    GetModuleInformation(m_hProcess, hMods[i], &modInfo, sizeof(modInfo));
                    break;
                }
            }
        }
    }

    return modInfo;
}

uintptr_t Memory::GetModuleBase(const std::string moduleName)
{
    MODULEENTRY32 entry{};
    entry.dwSize = sizeof(MODULEENTRY32);
    const auto snapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_dwPID);

    while (Module32Next(snapShot, &entry))
    {
        if (wcscmp(std::wstring(moduleName.begin(), moduleName.end()).c_str(), entry.szModule) == 0)
        {
            CloseHandle(snapShot);
            return reinterpret_cast<uintptr_t>(entry.modBaseAddr);
        }
    }

    if (snapShot)
        CloseHandle(snapShot);

    return reinterpret_cast<uintptr_t>(entry.modBaseAddr);
}

MODULEENTRY32 Memory::GetModule(const std::string moduleName)
{
    MODULEENTRY32 entry{};
    entry.dwSize = sizeof(MODULEENTRY32);
    const auto snapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_dwPID);

    while (Module32Next(snapShot, &entry))
    {
        if (wcscmp(std::wstring(moduleName.begin(), moduleName.end()).c_str(), entry.szModule) == 0)
        {
            CloseHandle(snapShot);
            return entry;
        }
    }

    if (snapShot)
        CloseHandle(snapShot);

    return entry;
}

PROCESSENTRY32 Memory::GetProcess(const std::string processName)
{
    PROCESSENTRY32 entry{};
    entry.dwSize = sizeof(PROCESSENTRY32);
    const auto snapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    while (Process32Next(snapShot, &entry))
    {
        if (wcscmp(std::wstring(processName.begin(), processName.end()).c_str(), entry.szExeFile) == 0)
        {
            CloseHandle(snapShot);
            return entry;
        }
    }

    CloseHandle(snapShot);
    return PROCESSENTRY32();
}

bool Memory::WriteMemory(HANDLE processHandle, uintptr_t address, float value) {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi)) == 0) {
        return false;
    }
    DWORD oldProtect;
    if (VirtualProtectEx(processHandle, (LPVOID)address, sizeof(float), PAGE_READWRITE, &oldProtect)) {
        bool success = WriteProcessMemory(processHandle, (LPVOID)address, &value, sizeof(float), nullptr);
        VirtualProtectEx(processHandle, (LPVOID)address, sizeof(float), oldProtect, &oldProtect);
        return success;
    }
    return false;
}

bool Memory::ReadMemory(HANDLE processHandle, uintptr_t address, float& value) {
    return ReadProcessMemory(processHandle, (LPCVOID)address, &value, sizeof(float), nullptr);
}

uintptr_t Memory::GetModuleBaseAddress(DWORD processId, const wchar_t* moduleName) {
    uintptr_t moduleBaseAddress = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W moduleEntry;
        moduleEntry.dwSize = sizeof(moduleEntry);
        if (Module32FirstW(snapshot, &moduleEntry)) {
            do {
                if (_wcsicmp(moduleEntry.szModule, moduleName) == 0) {
                    moduleBaseAddress = (uintptr_t)moduleEntry.modBaseAddr;
                    break;
                }
            } while (Module32NextW(snapshot, &moduleEntry));
        }
        CloseHandle(snapshot);
    }
    return moduleBaseAddress;
}

std::vector<uintptr_t> Memory::FindPattern(HANDLE processHandle, uintptr_t start, size_t size, const char* pattern, const char* mask) {
    std::vector<uintptr_t> results;
    std::vector<BYTE> buffer(size);
    SIZE_T bytesRead;

    if (ReadProcessMemory(processHandle, (LPCVOID)start, buffer.data(), size, &bytesRead)) {
        for (size_t i = 0; i < bytesRead - strlen(mask); i++) {
            bool found = true;
            for (size_t j = 0; j < strlen(mask); j++) {
                if (mask[j] != '?' && pattern[j] != buffer[i + j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                results.push_back(start + i);
            }
        }
    }
    return results;
}

bool Memory::IsProcessRunning(const wchar_t* processName) {
    return GetProcessIdByName(processName) != 0;
}

DWORD Memory::GetProcessIdByName(const wchar_t* processName) {
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);
        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (_wcsicmp(processEntry.szExeFile, processName) == 0) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    return processId;
}

HANDLE Memory::OpenProcessByName(const wchar_t* processName) {
    DWORD processId = GetProcessIdByName(processName);
    if (processId == 0) return NULL;
    return OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
}

Memory m;