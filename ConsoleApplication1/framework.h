#pragma once

// Windows API için gerekli tanımlamalar
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

// Windows uygulaması için gerekli tanımlamalar
#define MAX_LOADSTRING 100

// Global değişkenler
extern HINSTANCE hInst;
extern TCHAR szTitle[MAX_LOADSTRING];
extern TCHAR szWindowClass[MAX_LOADSTRING];

// Fonksiyon prototipleri
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM); 