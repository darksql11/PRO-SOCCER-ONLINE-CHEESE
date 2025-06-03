#pragma once
#include <Windows.h>
#include <d3d9.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx9.h"
#include "SimpleMath.h"
#include "Globals.h"
#include "Memory.h"
#include <dwmapi.h>

// Overlay functions
bool OverlayInit(HWND targetWindow);
void OverlayRender();
void OverlayShutdown();
void ResetDevice();

// DirectX variables
extern LPDIRECT3D9 g_pD3D;
extern LPDIRECT3DDEVICE9 g_pd3dDevice;
extern D3DPRESENT_PARAMETERS g_d3dpp;
extern bool g_initialized;

// ImGui variables
extern ImFont* g_font;
extern ImFont* g_fontBig;
extern ImFont* g_fontSmall;