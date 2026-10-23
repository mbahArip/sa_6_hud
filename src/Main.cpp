#include <plugin.h>
#include "Config.h"
#include "I18n.h"
#include "Radar.h"
#include "Menu.h"

extern "C" __declspec(dllexport) char GtaMcpDebugLog[4096] = {};
extern "C" __declspec(dllexport) int  GtaMcpDebugLen = 0;

using namespace plugin;

// Stable IDs for safe hot-reload event de-registration
enum eHud6HookId : int {
    HOOK_INIT_GAME     = 0x6001,
    HOOK_GAME_PROCESS  = 0x6002,
    HOOK_DRAW_HUD      = 0x6003,
    HOOK_DRAW_RADAR    = 0x6004,
    HOOK_RESTART_GAME  = 0x6005,
    HOOK_D3D_LOST      = 0x6006,
    HOOK_D3D_RESET     = 0x6007,
};

static void UpdateVanillaPatches() {
    static bool isInitialized = false;
    static bool lastRadarEnabled = false;
    static bool lastInfoCardsEnabled = false;

    bool radarEnabled = hud6::g_config.enabled;
    bool infoCardsEnabled = hud6::g_config.showInfoCards;
    if (isInitialized && radarEnabled == lastRadarEnabled && infoCardsEnabled == lastInfoCardsEnabled) {
        return;
    }

    // Block vanilla HUD minimap (0x586880 = CRadar::DrawRadarMap) if custom 6hud radar is enabled
    // Original byte in GTA SA 1.0 US is 0x83 (sub esp, 2Ch)
    plugin::patch::SetUChar(0x586880, radarEnabled ? 0xC3 : 0x83);

    // Block vanilla weapon icon, ammo, money, and health bars (0x58EAF0 = CHud::DrawPlayerInfo)
    // Original byte in GTA SA 1.0 US is 0x81 (sub esp, 1A0h)
    plugin::patch::SetUChar(0x58EAF0, radarEnabled ? 0xC3 : 0x81);

    // Block vanilla zone and vehicle names if info cards are enabled
    // Original bytes in GTA SA 1.0 US are 0x51 (push ecx)
    plugin::patch::SetUChar(0x58AA50, infoCardsEnabled ? 0xC3 : 0x51);
    plugin::patch::SetUChar(0x58AEA0, infoCardsEnabled ? 0xC3 : 0x51);

    lastRadarEnabled = radarEnabled;
    lastInfoCardsEnabled = infoCardsEnabled;
    isInitialized = true;
}

static void OnInitGame() {
    hud6::LoadConfig();
    hud6::ApplyPendingConfig();
    hud6::LoadI18n();
    UpdateVanillaPatches();
    hud6::Radar::Init();
    hud6::Menu::Init();
}

static void OnGameProcess() {
    hud6::Radar::Process();
    hud6::ApplyPendingConfig();
    // Cheap when nothing changed: each branch early-outs on the module's state.
    // Picks up menu toggles so a disabled module's hook is removed immediately.
    hud6::Radar::SyncModulePatches();
    hud6::Menu::Process();
    UpdateVanillaPatches();
}

static void OnDrawHud() {
    hud6::Radar::Render();
    hud6::Menu::Render();
}

static void OnDrawRadar() {
    hud6::Radar::DrawMenuMapOverlay();
}

static void OnRestartGame() {
    hud6::Radar::Restart();
}

extern "C" __declspec(dllexport) void GtaMcpUnload() {
    hud6::Menu::Shutdown();
    hud6::Radar::Shutdown();

    // Restore vanilla functions on unload
    plugin::patch::SetUChar(0x586880, 0x83);
    plugin::patch::SetUChar(0x58EAF0, 0x81);
    plugin::patch::SetUChar(0x58AA50, 0x51);
    plugin::patch::SetUChar(0x58AEA0, 0x51);

    Events::initGameEvent.RemoveById(HOOK_INIT_GAME);
    Events::gameProcessEvent.RemoveById(HOOK_GAME_PROCESS);
    Events::drawHudEvent.RemoveById(HOOK_DRAW_HUD);
    Events::drawRadarOverlayEvent.RemoveById(HOOK_DRAW_RADAR);
    Events::restartGameEvent.RemoveById(HOOK_RESTART_GAME);
    Events::d3dLostEvent.RemoveById(HOOK_D3D_LOST);
    Events::d3dResetEvent.RemoveById(HOOK_D3D_RESET);
}

class Hud6Plugin {
public:
    Hud6Plugin() {
        hud6::LogWrite("boot: 6hud plugin loading...");
        hud6::LoadConfig();
        hud6::LogWrite("boot: LoadConfig completed");
        UpdateVanillaPatches();
        hud6::LogWrite("boot: UpdateVanillaPatches completed");

        Events::initGameEvent.AddAfterAtId(HOOK_INIT_GAME, OnInitGame);
        Events::gameProcessEvent.AddAfterAtId(HOOK_GAME_PROCESS, OnGameProcess);
        Events::drawHudEvent.AddAfterAtId(HOOK_DRAW_HUD, OnDrawHud);
        Events::drawRadarOverlayEvent.AddAfterAtId(HOOK_DRAW_RADAR, OnDrawRadar);
        Events::restartGameEvent.AddAfterAtId(HOOK_RESTART_GAME, OnRestartGame);
        Events::d3dLostEvent.AddAfterAtId(HOOK_D3D_LOST, hud6::Radar::OnD3dLost);
        Events::d3dResetEvent.AddAfterAtId(HOOK_D3D_RESET, hud6::Radar::OnD3dReset);
        hud6::LogWrite("boot: events registered");

        if (FindPlayerPed(0)) {
            OnInitGame();
        }

        hud6::LogWrite("boot: 6hud initialized successfully");
    }
} g_hud6Plugin;
