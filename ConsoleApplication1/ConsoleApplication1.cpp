#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx9.h"
#include <d3d9.h>
#include <tchar.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <deque>
#include "resource.h"
#include "framework.h"
#include "Overlay.h"
#include "Memory.h"
#include "Globals.h"
#include "ConsoleApplication1.h"

// Global variables
    DWORD processId = 0;
HANDLE processHandle = nullptr;
uintptr_t moduleBase = 0;
uintptr_t speedTargetAddress = 0;
uintptr_t staminaTargetAddress = 0;
uintptr_t powerShotTargetAddress = 0;
uintptr_t fovTargetAddress = 0;
uintptr_t dribblingTargetAddress = 0;

// Match state tracking
bool isInMatch = false;
int lastScore = 0;
int currentScore = 0;

// Global variables
Globals g;

// Oyun penceresini bul
HWND FindGameWindow() {
    const char* windowTitles[] = {
        "Pro Soccer Online",
        "Pro Soccer",
        "ProSoccerOnline-Win64-Shipping",
        "ProSoccerOnline"
    };

    for (const char* title : windowTitles) {
        HWND hwnd = FindWindowA(NULL, title);
        if (hwnd != NULL) {
            char debugText[256];
            sprintf_s(debugText, "Found game window with title: %s\n", title);
            OutputDebugStringA(debugText);
            return hwnd;
        }
    }

    HWND hwnd = GetTopWindow(NULL);
    while (hwnd) {
        char windowTitle[256];
        GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
        
        if (strstr(windowTitle, "Soccer") || strstr(windowTitle, "Pro")) {
            char debugText[256];
            sprintf_s(debugText, "Found potential game window: %s\n", windowTitle);
            OutputDebugStringA(debugText);
            return hwnd;
        }
        
        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }

    OutputDebugStringA("Game window not found!\n");
    return NULL;
}

// Windows entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Oyun penceresini bul
    HWND gameWindow = FindGameWindow();
    if (!gameWindow) {
        MessageBoxA(NULL, "Game window not found! Please start the game first.", "Error", MB_OK);
        return 1;
    }

    // Overlay'i başlat
    if (!OverlayInit(gameWindow)) {
        MessageBoxA(NULL, "Overlay init failed!", "Error", MB_OK);
        return 1;
    }

    // Ana döngü
    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        // Windows mesajlarını işle
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                return 0;
        }

        // Oyun penceresi hala var mı kontrol et
        if (!FindGameWindow()) {
            break;
        }

        // Insert tuşu ile menüyü aç/kapat
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            g.ShowMenu = !g.ShowMenu;
        }

        // Hotkey kontrolü
        if (GetAsyncKeyState(g.settings.staminaHotkey) & 1) {
            g.settings.staminaEnabled = !g.settings.staminaEnabled;
            if (!g.settings.staminaEnabled) {
                    float defaultStamina = 100.0f;
                Memory::WriteMemory(processHandle, staminaTargetAddress, defaultStamina);
            }
        }

        if (GetAsyncKeyState(g.settings.powerShotHotkey) & 1) {
            g.settings.powerShotEnabled = !g.settings.powerShotEnabled;
            if (!g.settings.powerShotEnabled) {
                    float defaultPower = 1000.0f;
                Memory::WriteMemory(processHandle, powerShotTargetAddress, defaultPower);
            }
        }

        // Overlay'i render et
        OverlayRender();
        Sleep(1);
    }

    // Temizlik
    OverlayShutdown();
    return 0;
}