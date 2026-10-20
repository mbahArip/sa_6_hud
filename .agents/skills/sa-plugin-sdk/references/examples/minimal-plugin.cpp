/*
    GTA SA Plugin SDK — Minimal Plugin Template
    Demonstrates: version guard, Events, KeyPressed, FindPlayerPed, CMessages
    Target: SA 1.0 US Compact + Hoodlum, 32-bit x86
*/
#include <plugin.h>
#include "CMessages.h"

using namespace plugin;
using namespace injector;

// Accepts SA 1.0 US Compact (0x824570) and Hoodlum (0x82457C).
// Shows error dialog and returns false for any other exe.
static bool CheckVersion() {
    game_version_manager gvm;
    gvm.PluginName = "My SA Plugin";
    if (!gvm.Detect()) { gvm.RaiseCouldNotDetect(); return false; }
    if (gvm.IsSA() && gvm.IsUS() && gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 0)
        return true;
    gvm.RaiseIncompatibleVersion();
    return false;
}

class MyPlugin {
public:
    MyPlugin() {
        if (!CheckVersion()) return;

        // Per-frame game logic
        Events::gameProcessEvent += [] {
            CPed* player = FindPlayerPed();
            if (!player) return;

            if (KeyPressed(VK_F5)) {
                CMessages::AddMessageJumpQ("Hello from plugin!", 3000, 0);
            }
        };

        // 2D/HUD draw pass
        Events::drawHudEvent += [] {
            // draw 2D elements here (CFont, CSprite2d, RwIm2D, etc.)
        };

        // D3D resource management
        Events::d3dLostEvent += [] {
            // release D3DPOOL_DEFAULT resources
        };
        Events::d3dResetEvent += [] {
            // re-create D3DPOOL_DEFAULT resources
        };

        // RenderWare init/shutdown (texture loading)
        Events::initRwEvent += [] {
            // load TXD, textures here
        };
        Events::shutdownRwEvent += [] {
            // destroy textures here
        };
    }
} myPlugin;  // static instance — ctor runs on DLL load
