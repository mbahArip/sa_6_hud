/*
    GTA SA Plugin SDK — Hot-Reloadable Plugin Template
    Demonstrates:
      - Registering Events with explicit hook IDs
      - Exporting GtaMcpUnload / PluginUnload to cleanly detach before FreeLibrary
      - Safe hot reloading via gta-sa-mcp (`plugin_reload`) without game restart
    Target: SA 1.0 US Compact + Hoodlum, 32-bit x86
*/
#include <plugin.h>
#include "CMessages.h"

using namespace plugin;
using namespace injector;

// Unique hook IDs for this plugin instance
enum ePluginHookIds {
    HOOK_GAME_PROCESS = 0x2001,
    HOOK_DRAW_HUD     = 0x2002,
    HOOK_RW_INIT      = 0x2003,
    HOOK_RW_SHUTDOWN  = 0x2004
};

static void OnGameProcess() {
    CPed* player = FindPlayerPed();
    if (!player) return;

    if (KeyPressed(VK_F5)) {
        CMessages::AddMessageJumpQ("Plugin hot-reloaded successfully!", 2000, 0);
    }
}

static void OnDrawHud() {
    // 2D HUD drawing code
}

static bool CheckVersion() {
    game_version_manager gvm;
    gvm.PluginName = "HotReloadablePlugin";
    if (!gvm.Detect()) { gvm.RaiseCouldNotDetect(); return false; }
    if (gvm.IsSA() && gvm.IsUS() && gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 0)
        return true;
    gvm.RaiseIncompatibleVersion();
    return false;
}

class HotReloadablePlugin {
public:
    HotReloadablePlugin() {
        if (!CheckVersion()) return;

        // Register events with explicit numeric IDs so they can be removed on unload
        Events::gameProcessEvent.AddAfter(OnGameProcess, HOOK_GAME_PROCESS);
        Events::drawHudEvent.AddAfter(OnDrawHud, HOOK_DRAW_HUD);
    }

    ~HotReloadablePlugin() {
        // Fallback cleanup in destructor
        Events::gameProcessEvent.RemoveAfterById(HOOK_GAME_PROCESS);
        Events::drawHudEvent.RemoveAfterById(HOOK_DRAW_HUD);
    }
} gPlugin;

// Exported cleanup function called by debug_bridge / FreeLibrary before DLL unload
extern "C" __declspec(dllexport) void GtaMcpUnload() {
    Events::gameProcessEvent.RemoveAfterById(HOOK_GAME_PROCESS);
    Events::drawHudEvent.RemoveAfterById(HOOK_DRAW_HUD);
}
