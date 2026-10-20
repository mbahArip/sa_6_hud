/*
    GTA SA Plugin SDK — Version Guard
    Ensures the plugin only runs on SA 1.0 US (Compact or Hoodlum).
    Any other exe gets an error dialog; DLL continues loading but plugin
    hooks are not installed.

    Detection source (injector/injector.hpp:756-759):
      case 0x824570: GTA SA 1.0 US Compact
      case 0x82457C: GTA SA 1.0 US Hoodlum (cracker == 'H')
      Both set: game='S', major=1, minor=0, region='U'
      Hoodlum distinguished by: ReadMemory<uint8_t>(0x406A20) == 0xE9

    So IsSA() && IsUS() && major==1 && minor==0 covers both with one check.
    IsHoodlum() is available if you need to branch behavior between them.
*/
#include <plugin.h>
#include "GameVersion.h"    // plugin-sdk wrapper; internally uses address_manager::Detect()

// Define before including to customise the error dialog title.
#ifndef MY_PLUGIN_NAME
#define MY_PLUGIN_NAME "My SA Plugin"
#endif

using namespace plugin;
using namespace injector;

// Call once at the top of your plugin constructor.
// Returns true  → supported, safe to install hooks.
// Returns false → unsupported, error dialog already shown, do not install hooks.
inline bool CheckSupportedVersion() {
    game_version_manager gvm;
    gvm.PluginName = MY_PLUGIN_NAME;

    if (!gvm.Detect()) {
        // Could not identify the exe at all
        gvm.RaiseCouldNotDetect();
        return false;
    }

    // SA 1.0 US covers both Compact (0x824570) and Hoodlum (0x82457C)
    if (gvm.IsSA() && gvm.IsUS() && gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 0)
        return true;

    // Everything else: SA EU, SA 1.1, VC, III, IV, Steam, unknown
    gvm.RaiseIncompatibleVersion();
    // Dialog text: "An incompatible exe version has been detected! (GTA SA 1.1.0.0 US)\nContact the mod creator!"
    return false;
}

// ---- Plugin with version guard ------------------------------------------

class MyPlugin {
public:
    MyPlugin() {
        if (!CheckSupportedVersion()) return;

        // All hooks installed only after version is confirmed
        Events::gameProcessEvent += [] {
            // ...
        };
    }
} myPlugin;

/*
  NOTES:

  If you need to branch Compact vs Hoodlum at runtime:
    game_version_manager gvm; gvm.Detect();
    if (gvm.IsHoodlum()) { ... } else { ... }

  If you use ADDRESS_BY_VERSION macros, the sdk already handles both:
    ADDRESS_BY_VERSION(compact_addr, hoodlum_addr, eu_addr, us11_addr, eu11_addr, steam_addr)
  — Hoodlum maps to the second argument.

  address_manager::set_name(MY_PLUGIN_NAME) sets the plugin name globally for
  all future RaiseCouldNotDetect / RaiseIncompatibleVersion calls.
*/
