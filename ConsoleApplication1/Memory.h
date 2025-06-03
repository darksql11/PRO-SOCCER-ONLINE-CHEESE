#pragma once
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <Psapi.h>
#include <sstream>
#include <cstdint>

enum InitMode
{
	WINDOW_TITLE,
	WINDOW_CLASS,
	PROCESS_NAME
};

class Memory
{
private:
	DWORD m_dwPID;
	HANDLE m_hProcess;

	uintptr_t GetModuleBase(const std::string moduleName);
	MODULEENTRY32 GetModule(const std::string moduleName);
	PROCESSENTRY32 GetProcess(const std::string processName);
public:
	uintptr_t m_gClientBaseAddr;

	bool AttachProcess(const char* targetName, InitMode init_mode);
	void DetachProcess();

	char* GetModuleName();
	MODULEINFO GetModuleInfo(const std::string moduleName);
	uintptr_t FindPattern(const std::vector<uint8_t>& read_data, const std::string pattern, int offset, int extra);

	template <typename T>
	constexpr const T Read(const uintptr_t& address) const noexcept
	{
		T value{};
		ReadProcessMemory(m_hProcess, reinterpret_cast<const void*>(address), &value, sizeof(T), NULL);
		return value;
	}
	template <typename T>
	constexpr void Write(const uintptr_t& address, const T& value) const noexcept
	{
		WriteProcessMemory(m_hProcess, reinterpret_cast<void*>(address), &value, sizeof(T), NULL);
	}
	uintptr_t ReadChain(uintptr_t address, const std::vector<uintptr_t>& offsets)
	{
		uintptr_t result = Read<uintptr_t>(address + offsets.at(0));
		for (int i = 1; i < offsets.size(); i++)
			result = Read<uintptr_t>(result + offsets.at(i));

		return result;
	}
	bool ReadString(const uintptr_t address, LPVOID buffer, SIZE_T size) const noexcept
	{
		return !!::ReadProcessMemory(m_hProcess, (void*)(address), buffer, size, nullptr);
	}
	std::string ReadStringA(const uintptr_t address) const noexcept
	{
		char vOut[MAX_PATH]{};
		ReadProcessMemory(m_hProcess, (void*)(address), vOut, sizeof(vOut), nullptr);

		return std::string(vOut);
	}
	std::vector<uint8_t> ReadBytes(uintptr_t address, size_t size)
	{
		std::vector<uint8_t> buffer(size);
		ReadProcessMemory(m_hProcess, reinterpret_cast<LPCVOID>(address), buffer.data(), size, nullptr);
		return buffer;
	}

	static bool WriteMemory(HANDLE processHandle, uintptr_t address, float value);
	static bool ReadMemory(HANDLE processHandle, uintptr_t address, float& value);
	static uintptr_t GetModuleBaseAddress(DWORD processId, const wchar_t* moduleName);
	static std::vector<uintptr_t> FindPattern(HANDLE processHandle, uintptr_t start, size_t size, const char* pattern, const char* mask);
	static bool IsProcessRunning(const wchar_t* processName);
	static DWORD GetProcessIdByName(const wchar_t* processName);
	static HANDLE OpenProcessByName(const wchar_t* processName);
};

extern Memory m;