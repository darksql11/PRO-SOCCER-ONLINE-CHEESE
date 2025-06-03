#pragma once

// Windows API için gerekli tanımlamalar
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Uygulama için gerekli tanımlamalar
#define IDM_ABOUT                        104
#define IDM_EXIT                         105
#define IDD_ABOUTBOX                     102

// Fonksiyon prototipleri
HWND FindGameWindow();
void UpdateGameState();
void HandleHotkeys();
void RenderOverlay();
void Cleanup(); 