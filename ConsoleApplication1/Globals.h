#pragma once
#include <string>
#include <Windows.h>
#include "imgui/imgui.h"
#include <vector>

// ESP ayarları
struct ESPSettings {
    bool enabled = false;
    bool showBox = true;
    bool showSkeleton = true;
    bool showDistance = true;
    bool showTeammates = false;
    bool showBall = true;
    ImVec4 enemyColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    ImVec4 teammateColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    ImVec4 ballColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    float boxThickness = 2.0f;
    float skeletonThickness = 2.0f;
};

struct Settings {
    bool speedEnabled = false;
    float speedValue = 1.0f;
    bool staminaEnabled = false;
    float staminaValue = 999999.0f;
    bool powerShotEnabled = false;
    float powerShotValue = 9999999999999.0f;  // Orijinal değer
    bool fovEnabled = false;
    float fovValue = 90.0f;
    bool dribblingEnabled = false;
    float dribblingValue = 1.0f;

    // Hotkeys
    int staminaHotkey = VK_F2;
    int powerShotHotkey = VK_F3;

    // Hotkey değiştirme modları
    bool changingStaminaHotkey = false;
    bool changingPowerShotHotkey = false;
    bool keyPressed = false;
};

struct Globals
{
    // System(Base
    bool process_active = false;
    bool ShowMenu = true;

    // GameData
    RECT GameRect{};
    POINT GamePoint{};

    // Aim
    bool AimBot = true;
    bool AimBotPrediction = true;
    float AimFov = 100.f;

    // Visual
    bool ESP = true;
    bool ESP_Box = true;
    bool ESP_Line = false;
    bool ESP_Distance = true;
    bool ESP_HealthBar = true;
    bool ESP_Skeleton = true;
    float ESP_MaxDistance = 1000.f;

    // Misc
    bool GodMode = false;
    bool NoRecoil = false;
    bool NoSpread = false;

    // System(Cheat
    bool Crosshair = false;
    bool StreamProof = false;

    // Key
    int MenuKey = VK_INSERT;
    int AimKey0 = VK_RBUTTON;
    int AimKey1 = VK_LBUTTON;

    // Global değişkenler
    ESPSettings esp;
    Settings settings;
};

extern Globals g;
extern bool IsKeyDown(int VK);
extern const char* KeyNames[];