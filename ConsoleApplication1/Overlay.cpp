#include "Overlay.h"
#include "Globals.h"
#include "Memory.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_dx9.h"
#include "imgui/backends/imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>

// Global variables
static LPDIRECT3D9 g_pD3D = nullptr;
static LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr;
static D3DPRESENT_PARAMETERS g_d3dpp = {};
static HWND g_hwnd = nullptr;

// Game variables (extern olarak tanımlanıyor)
extern DWORD processId;
extern HANDLE processHandle;
extern uintptr_t moduleBase;
extern uintptr_t speedTargetAddress;
extern uintptr_t staminaTargetAddress;
extern uintptr_t powerShotTargetAddress;
extern uintptr_t fovTargetAddress;
extern uintptr_t dribblingTargetAddress;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Helper functions for Direct3D9
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();

// Global debug buffer
static char debugText[256];

// Global değişkenler
static bool wasPowerShotEnabled = false;
static bool wasLeftMouseDown = false;
static float lastPowerShotValue = 1000.0f;
static DWORD lastPowerShotUpdate = 0;
static bool wasStaminaEnabled = false;
static float lastStaminaValue = 100.0f;
static DWORD lastStaminaUpdate = 0;

// Tuş isimlerini döndüren yardımcı fonksiyon
std::string GetKeyName(int key) {
    switch (key) {
    case VK_F1: return "F1";
    case VK_F2: return "F2";
    case VK_F3: return "F3";
    case VK_F4: return "F4";
    case VK_F5: return "F5";
    case VK_F6: return "F6";
    case VK_F7: return "F7";
    case VK_F8: return "F8";
    case VK_F9: return "F9";
    case VK_F10: return "F10";
    case VK_F11: return "F11";
    case VK_F12: return "F12";
    case VK_INSERT: return "Insert";
    case VK_DELETE: return "Delete";
    case VK_HOME: return "Home";
    case VK_END: return "End";
    case VK_PRIOR: return "Page Up";
    case VK_NEXT: return "Page Down";
    case VK_NUMPAD0: return "Num 0";
    case VK_NUMPAD1: return "Num 1";
    case VK_NUMPAD2: return "Num 2";
    case VK_NUMPAD3: return "Num 3";
    case VK_NUMPAD4: return "Num 4";
    case VK_NUMPAD5: return "Num 5";
    case VK_NUMPAD6: return "Num 6";
    case VK_NUMPAD7: return "Num 7";
    case VK_NUMPAD8: return "Num 8";
    case VK_NUMPAD9: return "Num 9";
    case VK_MULTIPLY: return "Num *";
    case VK_ADD: return "Num +";
    case VK_SEPARATOR: return "Num ,";
    case VK_SUBTRACT: return "Num -";
    case VK_DECIMAL: return "Num .";
    case VK_DIVIDE: return "Num /";
    case VK_NUMLOCK: return "Num Lock";
    case VK_SCROLL: return "Scroll Lock";
    case VK_CAPITAL: return "Caps Lock";
    case VK_SHIFT: return "Shift";
    case VK_CONTROL: return "Ctrl";
    case VK_MENU: return "Alt";
    case VK_LSHIFT: return "Left Shift";
    case VK_RSHIFT: return "Right Shift";
    case VK_LCONTROL: return "Left Ctrl";
    case VK_RCONTROL: return "Right Ctrl";
    case VK_LMENU: return "Left Alt";
    case VK_RMENU: return "Right Alt";
    case VK_LWIN: return "Left Windows";
    case VK_RWIN: return "Right Windows";
    case VK_APPS: return "Apps";
    case VK_SLEEP: return "Sleep";
    case VK_BROWSER_BACK: return "Browser Back";
    case VK_BROWSER_FORWARD: return "Browser Forward";
    case VK_BROWSER_REFRESH: return "Browser Refresh";
    case VK_BROWSER_STOP: return "Browser Stop";
    case VK_BROWSER_SEARCH: return "Browser Search";
    case VK_BROWSER_FAVORITES: return "Browser Favorites";
    case VK_BROWSER_HOME: return "Browser Home";
    case VK_VOLUME_MUTE: return "Volume Mute";
    case VK_VOLUME_DOWN: return "Volume Down";
    case VK_VOLUME_UP: return "Volume Up";
    case VK_MEDIA_NEXT_TRACK: return "Next Track";
    case VK_MEDIA_PREV_TRACK: return "Previous Track";
    case VK_MEDIA_STOP: return "Stop";
    case VK_MEDIA_PLAY_PAUSE: return "Play/Pause";
    case VK_LAUNCH_MAIL: return "Mail";
    case VK_LAUNCH_MEDIA_SELECT: return "Media Select";
    case VK_LAUNCH_APP1: return "App1";
    case VK_LAUNCH_APP2: return "App2";
        default:
        if (key >= 'A' && key <= 'Z') return std::string(1, (char)key);
        if (key >= '0' && key <= '9') return std::string(1, (char)key);
        if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) return "Num " + std::string(1, (char)(key - VK_NUMPAD0 + '0'));
        return "Unknown";
    }
}

// Memory functions
DWORD GetProcessId(const wchar_t* processName) {
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

uintptr_t GetModuleBaseAddress(DWORD processId, const wchar_t* moduleName) {
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

bool IsValidMemoryAddress(HANDLE processHandle, uintptr_t address) {
    MEMORY_BASIC_INFORMATION mbi;
    return VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi)) != 0;
}

bool WriteMemory(HANDLE processHandle, uintptr_t address, float value) {
    if (!processHandle) {
        OutputDebugStringA("WriteMemory: processHandle is null!\n");
        return false;
    }

    if (!IsValidMemoryAddress(processHandle, address)) {
        sprintf_s(debugText, "WriteMemory: Invalid memory address 0x%llX\n", address);
        OutputDebugStringA(debugText);
        return false;
    }

    DWORD oldProtect;
    if (VirtualProtectEx(processHandle, (LPVOID)address, sizeof(float), PAGE_READWRITE, &oldProtect)) {
        bool success = WriteProcessMemory(processHandle, (LPVOID)address, &value, sizeof(float), nullptr);
        VirtualProtectEx(processHandle, (LPVOID)address, sizeof(float), oldProtect, &oldProtect);
        
        if (success) {
            sprintf_s(debugText, "WriteMemory: Successfully wrote %.1f to 0x%llX\n", value, address);
            OutputDebugStringA(debugText);
        }
        else {
            sprintf_s(debugText, "WriteMemory: Failed to write %.1f to 0x%llX (Error: %lu)\n", value, address, GetLastError());
            OutputDebugStringA(debugText);
        }
        
        return success;
    }
    
    sprintf_s(debugText, "WriteMemory: Failed to change memory protection at 0x%llX (Error: %lu)\n", address, GetLastError());
    OutputDebugStringA(debugText);
    return false;
}

uintptr_t PointerZinciriOku(HANDLE hProc, DWORD64 ptr, std::vector<unsigned int> offsets) {
    uintptr_t addr = ptr;
    for (unsigned int i = 0; i < offsets.size(); ++i) {
        bool Okundumu = ReadProcessMemory(hProc, (BYTE*)addr, &addr, sizeof(DWORD64), 0);
        addr += offsets[i];
    }
    return addr;
}

// Function to update memory addresses
void UpdateMemoryAddresses() {
    if (!processHandle || !moduleBase) {
        OutputDebugStringA("UpdateMemoryAddresses: processHandle or moduleBase is null!\n");
        return;
    }

    // Update speed address
    uintptr_t speedBaseAddress = moduleBase + 0x04611F80;
    speedTargetAddress = PointerZinciriOku(processHandle, speedBaseAddress, { 0x30, 0x5C8, 0xF8, 0x20, 0x98 });
    sprintf_s(debugText, "Updated speed address: 0x%llX (base: 0x%llX)\n", speedTargetAddress, speedBaseAddress);
    OutputDebugStringA(debugText);

    // Update stamina address
    uintptr_t staminaBaseAddress = moduleBase + 0x0461CF28;
    staminaTargetAddress = PointerZinciriOku(processHandle, staminaBaseAddress, { 0x0, 0xA0, 0x578, 0xA0, 0x50, 0x6C8 });
    sprintf_s(debugText, "Updated stamina address: 0x%llX (base: 0x%llX)\n", staminaTargetAddress, staminaBaseAddress);
    OutputDebugStringA(debugText);

    // Update power shot address
    uintptr_t powerShotBaseAddress = moduleBase + 0x04611F80;
    powerShotTargetAddress = PointerZinciriOku(processHandle, powerShotBaseAddress, { 0x30, 0x50, 0x610 });
    
    char buffer[256];
    sprintf_s(buffer, "PowerShot Address: %llX", powerShotTargetAddress);
    OutputDebugStringA(buffer);

    // Update FOV address
    uintptr_t fovBaseAddress = moduleBase + 0x04611F80;
    fovTargetAddress = PointerZinciriOku(processHandle, fovBaseAddress, { 0x30, 0x260, 0x558, 0x1F8 });
    sprintf_s(debugText, "Updated FOV address: 0x%llX (base: 0x%llX)\n", fovTargetAddress, fovBaseAddress);
    OutputDebugStringA(debugText);

    // Update dribbling address
    uintptr_t dribblingBaseAddress = moduleBase + 0x04611F80;
    dribblingTargetAddress = PointerZinciriOku(processHandle, dribblingBaseAddress, { 0x30, 0x130, 0x20, 0x98 });
    sprintf_s(debugText, "Updated dribbling address: 0x%llX (base: 0x%llX)\n", dribblingTargetAddress, dribblingBaseAddress);
    OutputDebugStringA(debugText);
}

// Power Shot kontrol fonksiyonu
void UpdatePowerShot() {
    if (!processHandle || !powerShotTargetAddress) return;

    bool isLeftMouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool isPowerShotHotkeyPressed = (GetAsyncKeyState(g.settings.powerShotHotkey) & 1) != 0;
    DWORD currentTime = GetTickCount();

    // Hotkey ile Power Shot'u aç/kapat
    if (isPowerShotHotkeyPressed) {
        g.settings.powerShotEnabled = !g.settings.powerShotEnabled;
        if (!g.settings.powerShotEnabled) {
            WriteMemory(processHandle, powerShotTargetAddress, 1000.0f);
            lastPowerShotValue = 1000.0f;
        }
    }

    // Power Shot aktifse ve sol tık basılıysa
    if (g.settings.powerShotEnabled && isLeftMouseDown) {
        // Her 0.5 milisaniyede bir güncelle
        if (currentTime - lastPowerShotUpdate >= 0.5) {
            float maxPower = 9999999999999.0f;
            if (WriteMemory(processHandle, powerShotTargetAddress, maxPower)) {
                lastPowerShotValue = maxPower;
                lastPowerShotUpdate = currentTime;
                OutputDebugStringA("Power Shot: Shooting with max power (repeated)\n");
            }
        }
    }
    // Power Shot aktif ama sol tık basılı değilse
    else if (g.settings.powerShotEnabled && !isLeftMouseDown) {
        float normalPower = 1000.0f;
        if (lastPowerShotValue != normalPower) {
            if (WriteMemory(processHandle, powerShotTargetAddress, normalPower)) {
                lastPowerShotValue = normalPower;
                OutputDebugStringA("Power Shot: Waiting with normal power\n");
            }
        }
    }
    // Power Shot kapalıysa
    else if (!g.settings.powerShotEnabled && lastPowerShotValue != 1000.0f) {
        if (WriteMemory(processHandle, powerShotTargetAddress, 1000.0f)) {
            lastPowerShotValue = 1000.0f;
            OutputDebugStringA("Power Shot: Disabled\n");
        }
    }
}

// Stamina kontrol fonksiyonu
void UpdateStamina() {
    if (!processHandle || !staminaTargetAddress) return;

    bool isStaminaHotkeyPressed = (GetAsyncKeyState(g.settings.staminaHotkey) & 1) != 0;
    DWORD currentTime = GetTickCount();

    // Hotkey ile Stamina'yı aç/kapat
    if (isStaminaHotkeyPressed) {
        g.settings.staminaEnabled = !g.settings.staminaEnabled;
        if (!g.settings.staminaEnabled) {
            WriteMemory(processHandle, staminaTargetAddress, 100.0f);
            lastStaminaValue = 100.0f;
            wasStaminaEnabled = false;
        }
        else {
            wasStaminaEnabled = true;
        }
    }

    // Stamina aktifse
    if (g.settings.staminaEnabled) {
        // Her 0.5 milisaniyede bir güncelle
        if (currentTime - lastStaminaUpdate >= 0.5) {
            float maxStamina = 9999999999999.0f;  // Power Shot ile aynı değer
            if (WriteMemory(processHandle, staminaTargetAddress, maxStamina)) {
                lastStaminaValue = maxStamina;
                lastStaminaUpdate = currentTime;
                OutputDebugStringA("Stamina: Set to max (repeated)\n");
            }
        }
    }
    // Stamina kapalıysa ve önceden açıksa
    else if (wasStaminaEnabled) {
        float defaultStamina = 100.0f;
        if (WriteMemory(processHandle, staminaTargetAddress, defaultStamina)) {
            lastStaminaValue = defaultStamina;
            wasStaminaEnabled = false;
            OutputDebugStringA("Stamina: Disabled\n");
        }
    }
}

// Win32 message handler
LRESULT WINAPI OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool OverlayInit(HWND targetWindow)
{
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, OverlayWndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, _T("ImGui Example"), nullptr };
    RegisterClassEx(&wc);

    // Get target window position and size
    RECT targetRect;
    GetWindowRect(targetWindow, &targetRect);

    // Create overlay window with WS_EX_LAYERED
    g_hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,  // Removed WS_EX_TRANSPARENT to make menu clickable
        wc.lpszClassName,
        _T("Overlay"),
        WS_POPUP,
        targetRect.left,
        targetRect.top,
        targetRect.right - targetRect.left,
        targetRect.bottom - targetRect.top,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    // Make window transparent
    SetLayeredWindowAttributes(g_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    // Initialize Direct3D
    if (!CreateDeviceD3D(g_hwnd))
    {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return false;
    }

    // Show the window
    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Özel stil ayarları
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 4.0f;

    // Renkler
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.80f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.80f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.90f, 0.00f, 1.00f);

    return true;
}

void OverlayRender()
{
    // Power Shot ve Stamina'yı sürekli güncelle
    UpdatePowerShot();
    UpdateStamina();

    // Start the Dear ImGui frame
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Create menu window
    if (g.ShowMenu)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("Pro Soccer Online Menu", &g.ShowMenu, ImGuiWindowFlags_NoCollapse);

        // Main Menu Bar
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save Settings", "Ctrl+S")) {}
                if (ImGui::MenuItem("Load Settings", "Ctrl+L")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) { g.ShowMenu = false; }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Reset All", "Ctrl+R")) {}
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Check for game process every frame
        processId = GetProcessId(L"ProSoccerOnline-Win64-Shipping.exe");

        if (!processHandle) {
            if (processId) {
                if (ImGui::Button("Connect to Game")) {
                    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
                    if (processHandle) {
                        moduleBase = GetModuleBaseAddress(processId, L"ProSoccerOnline-Win64-Shipping.exe");
                        if (moduleBase) {
                            UpdateMemoryAddresses();
                        }
                    }
                }
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Game found! Click 'Connect to Game' to start.");
            }
            else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Waiting for game to start...");
            }
        }
        else {
            // Check if game is still running
            if (!processId) {
                CloseHandle(processHandle);
                processHandle = nullptr;
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Game closed. Waiting for restart...");
            }
            else {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Connected");
                ImGui::Separator();

                // Periodically update addresses (every 5 seconds)
                static DWORD lastUpdateTime = GetTickCount();
                if (GetTickCount() - lastUpdateTime > 5000) {
                    UpdateMemoryAddresses();
                    lastUpdateTime = GetTickCount();
                }

                // Speed settings
                ImGui::BeginChild("SpeedSettings", ImVec2(300, 100), true);
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "Speed Settings");
                if (ImGui::Checkbox("Enable Speed Boost", &g.settings.speedEnabled)) {
                    if (!g.settings.speedEnabled) {
                        float defaultSpeed = 1.0f;
                        WriteMemory(processHandle, speedTargetAddress, defaultSpeed);
                    }
                }
                if (g.settings.speedEnabled) {
                    if (ImGui::SliderFloat("Speed Multiplier", &g.settings.speedValue, 1.0f, 2.0f, "%.1fx")) {
                        WriteMemory(processHandle, speedTargetAddress, g.settings.speedValue);
                    }
                }
                ImGui::EndChild();

                ImGui::SameLine();

                // Stamina settings
                ImGui::BeginChild("StaminaSettings", ImVec2(300, 100), true);
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "Stamina Settings");
                ImGui::Checkbox("Infinite Stamina", &g.settings.staminaEnabled);
                
                // Debug bilgisi
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Process ID: %lu", processId);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Module Base: 0x%llX", moduleBase);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Stamina Address: 0x%llX", staminaTargetAddress);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Hotkey: %s", GetKeyName(g.settings.staminaHotkey).c_str());
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Status: %s", g.settings.staminaEnabled ? "Active" : "Disabled");
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Current Stamina: %.1f", lastStaminaValue);
                ImGui::EndChild();

                // Power shot settings
                ImGui::BeginChild("PowerShotSettings", ImVec2(300, 100), true);
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "Power Shot Settings");
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "WARNING: Very powerful!");
                ImGui::Checkbox("Enable Power Shot", &g.settings.powerShotEnabled);
                
                // Debug bilgisi
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Process ID: %lu", processId);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Module Base: 0x%llX", moduleBase);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Power Shot Address: 0x%llX", powerShotTargetAddress);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Hotkey: %s", GetKeyName(g.settings.powerShotHotkey).c_str());
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Status: %s", g.settings.powerShotEnabled ? "Active" : "Disabled");
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Current Power: %.1f", lastPowerShotValue);
                ImGui::EndChild();

                // FOV settings
                ImGui::BeginChild("FOVSettings", ImVec2(300, 100), true);
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "FOV Settings");
                if (ImGui::Checkbox("Enable FOV Changer", &g.settings.fovEnabled)) {
                    if (!g.settings.fovEnabled) {
                        float defaultFov = 90.0f;
                        WriteMemory(processHandle, fovTargetAddress, defaultFov);
                    }
                }
                if (g.settings.fovEnabled) {
                    if (ImGui::SliderFloat("FOV Value", &g.settings.fovValue, 60.0f, 170.0f, "%.1f")) {
                        WriteMemory(processHandle, fovTargetAddress, g.settings.fovValue);
                    }
                }
                ImGui::EndChild();

                // Dribbling settings
                ImGui::BeginChild("DribblingSettings", ImVec2(300, 100), true);
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "Dribbling Settings");
                if (ImGui::Checkbox("Enable Dribbling Boost", &g.settings.dribblingEnabled)) {
                    if (!g.settings.dribblingEnabled) {
                        float defaultDribbling = 1.0f;
                        WriteMemory(processHandle, dribblingTargetAddress, defaultDribbling);
                    }
                }
                if (g.settings.dribblingEnabled) {
                    if (ImGui::SliderFloat("Dribbling Multiplier", &g.settings.dribblingValue, 0.0f, 1.0f, "%.2f")) {
                        WriteMemory(processHandle, dribblingTargetAddress, g.settings.dribblingValue);
                    }
                }
                ImGui::EndChild();

                // Hotkey ayarları
                ImGui::BeginChild("HotkeySettings", ImVec2(300, 150), true);
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "Hotkey Settings");

                // Stamina Hotkey
                if (ImGui::Button(g.settings.changingStaminaHotkey ? "Press any key..." :
                    ("Stamina Hotkey: " + GetKeyName(g.settings.staminaHotkey)).c_str())) {
                    g.settings.changingStaminaHotkey = true;
                    g.settings.keyPressed = false;
                }

                // Power Shot Hotkey
                if (ImGui::Button(g.settings.changingPowerShotHotkey ? "Press any key..." :
                    ("Power Shot Hotkey: " + GetKeyName(g.settings.powerShotHotkey)).c_str())) {
                    g.settings.changingPowerShotHotkey = true;
                    g.settings.keyPressed = false;
                }

                ImGui::EndChild();

                ImGui::Separator();
                if (ImGui::Button("Exit", ImVec2(120, 30))) {
                    g.ShowMenu = false;
                }
            }
        }

        // Hotkey değiştirme kontrolü
        if (g.settings.changingStaminaHotkey) {
            if (!g.settings.keyPressed) {
                for (int i = 0; i < 256; i++) {
                    if (GetAsyncKeyState(i) & 0x8000) {
                        g.settings.staminaHotkey = i;
                        g.settings.keyPressed = true;
                        g.settings.changingStaminaHotkey = false;
                        break;
                    }
                }
            }
        }

        if (g.settings.changingPowerShotHotkey) {
            if (!g.settings.keyPressed) {
                for (int i = 0; i < 256; i++) {
                    if (GetAsyncKeyState(i) & 0x8000) {
                        g.settings.powerShotHotkey = i;
                        g.settings.keyPressed = true;
                        g.settings.changingPowerShotHotkey = false;
            break;
                    }
                }
            }
        }

        // Hotkey kontrolü
        if (!g.settings.changingStaminaHotkey && !g.settings.changingPowerShotHotkey) {
            if (GetAsyncKeyState(g.settings.staminaHotkey) & 1) {
                g.settings.staminaEnabled = !g.settings.staminaEnabled;
                if (!g.settings.staminaEnabled) {
                    float defaultStamina = 100.0f;
                    WriteMemory(processHandle, staminaTargetAddress, defaultStamina);
                }
            }

            if (GetAsyncKeyState(g.settings.powerShotHotkey) & 1) {
                g.settings.powerShotEnabled = !g.settings.powerShotEnabled;
                if (!g.settings.powerShotEnabled) {
                    float defaultPower = 1000.0f;
                    WriteMemory(processHandle, powerShotTargetAddress, defaultPower);
                }
            }
        }

        ImGui::End();
    }

    // Rendering
    ImGui::EndFrame();
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    D3DCOLOR clear_col_dx = D3DCOLOR_RGBA(0, 0, 0, 0);
    g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
    if (g_pd3dDevice->BeginScene() >= 0)
    {
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        g_pd3dDevice->EndScene();
    }
    HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

    // Handle loss of D3D9 device
    if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
        ResetDevice();
}

void OverlayShutdown()
{
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(g_hwnd);
    UnregisterClass(_T("ImGui Example"), GetModuleHandle(nullptr));
}

// Helper functions for Direct3D9
bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    g_d3dpp.BackBufferWidth = 0;
    g_d3dpp.BackBufferHeight = 0;

    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}