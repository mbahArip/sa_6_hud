---
name: sa-plugin-sdk
description: >
  GTA San Andreas (classic, NOT Definitive Edition) plugin development using
  plugin-sdk. Use when the user asks to write a .asi/.dll plugin, hook game
  functions, spawn vehicles/peds, read/write game memory, attach custom data
  to entities, draw HUD elements, or otherwise mod GTA SA with C++.
metadata:
  version: 1.0.0
  repo: https://github.com/Dryxio/plugin-sdk-sa
  local_repo: C:\Users\me\dev\sa_project\plugin-sdk-sa
  game_path: D:\games\gta sa\GTA San Andreas
  target: GTA SA 1.0 US Compact (entry 0x824570, base 0x400000)
  sdk_version: 1005 (2026-08-14)
---

# GTA San Andreas Plugin SDK

Repo: [Dryxio/plugin-sdk-sa](https://github.com/Dryxio/plugin-sdk-sa) (local clone at `C:\Users\me\dev\sa_project\plugin-sdk-sa`, referenced by `$(SdkRoot)` in the `.vcxproj`). Classic SA only — not Definitive Edition.

## How to use this skill

Load references on demand — never all at once:

| Need | Load |
|---|---|
| Hook points / event signatures & addresses | [references/events-sa.md](references/events-sa.md) |
| `plugin::patch`, `Call<>`, address macros | [references/patch-and-call-api.md](references/patch-and-call-api.md) |
| `injector::` memory/hook primitives | [references/injector-api.md](references/injector-api.md) |
| Screen coordinates, 1080p canvas & UI scaling | [references/screen-coordinates.md](references/screen-coordinates.md) |
| Working code examples | [references/examples/](references/examples/) |

## Build requirements

- **Platform:** 32-bit x86 Win32 (MSVC or Clang `-m32`)
- **Output:** `.asi` loaded by ASI Loader, or `.dll` / `.cleo`
- **Preprocessor:** `GTASA`, `_CRT_SECURE_NO_WARNINGS`, `_CRT_NON_CONFORMING_SWPRINTFS`
- **Include paths:**
  ```
  shared/
  shared/game/
  plugin_sa/
  plugin_sa/game_sa/
  plugin_sa/game_sa/rw/
  injector/
  ```
- **Solution:** `plugin.sln` → `plugin_sa/Plugin_SA.vcxproj`

## Plugin entry pattern

Every plugin is a static class instance — its constructor runs on DLL load:

```cpp
#include <plugin.h>
using namespace plugin;

class MyPlugin {
public:
    MyPlugin() {
        Events::gameProcessEvent += [] { /* per-frame */ };
        Events::drawHudEvent += [] { /* 2D draw */ };
        Events::vehicleCtorEvent += [](CVehicle* v) { /* vehicle created */ };
        Events::pedCtorEvent += [](CPed* p) { /* ped created */ };
        Events::initRwEvent += [] { /* load textures */ };
        Events::shutdownRwEvent += [] { /* free textures */ };
        Events::d3dLostEvent += [] { /* release D3DPOOL_DEFAULT */ };
        Events::d3dResetEvent += [] { /* re-create D3DPOOL_DEFAULT */ };
    }
} myPlugin;
```

See [references/examples/minimal-plugin.cpp](references/examples/minimal-plugin.cpp) for full boilerplate.

## Entity class hierarchy & sizes

```
CPlaceable (0x18)
 └── CEntity (0x38)          RwObject*, modelIndex, flags
      ├── CBuilding / CAnimatedBuilding
      ├── CDummy
      ├── CObject (0x17C)
      └── CPhysical (0x140)  velocity, forces, mass
           ├── CPed (0x79C)  weapons[13], intelligence, health/armour
           │    ├── CPlayerPed (0x7A4)
           │    ├── CCivilianPed
           │    └── CCopPed
           └── CVehicle (0x5A0)  handling, autopilot, damage, passengers
                ├── CAutomobile (0x988) — doors[6], nodes, suspension
                │    ├── CHeli
                │    └── CMonsterTruck
                ├── CBike (0x814) / CBmx
                ├── CPlane / CBoat / CTrain / CQuadBike / CTrailer
```

## Key field offsets (SA 1.0 US)

| Class | Field | Offset |
|---|---|---|
| `CEntity` | `m_pRwObject` | `0x18` |
| `CEntity` | `m_nModelIndex` | `0x22` |
| `CPhysical` | `m_vecMoveSpeed` | `0x44` |
| `CPed` | `m_pIntelligence` | `0x47C` |
| `CPed` | `m_fHealth` | `0x540` |
| `CPed` | `m_fArmour` | `0x548` |
| `CPed` | `m_aWeapons[13]` | `0x5A0` |
| `CPed` | `m_nSelectedWepSlot` | `0x718` |
| `CPed` | `m_nFightingStyle` | `0x72D` |
| `CVehicle` | `m_pHandlingData` | `0x384` |
| `CVehicle` | `m_autoPilot` | `0x390` |
| `CVehicle` | `m_pDriver` | `0x460` |
| `CVehicle` | `m_apPassengers[8]` | `0x464` |
| `CVehicle` | `m_fHealth` | `0x4C0` |
| `CVehicle` | `m_eDoorLock` | `0x4F8` |
| `CVehicle` | `m_nVehicleClass` | `0x590` |

## Memory pools & pool iteration

```cpp
// Pool iteration (PoolIterator from shared/extensions/PoolIterator.h)
for (auto* ped : CPools::ms_pPedPool)     { /* ... */ }
for (auto* veh : CPools::ms_pVehiclePool) { /* ... */ }

// Key pools
CPool<CPed, CCopPed>            *& CPools::ms_pPedPool;
CPool<CVehicle, CHeli>          *& CPools::ms_pVehiclePool;
CPool<CBuilding>                *& CPools::ms_pBuildingPool;
CPool<CObject, CCutsceneObject> *& CPools::ms_pObjectPool;
```

## Entity extenders — attach data to game entities

```cpp
struct MyVehicleData { bool tagged; MyVehicleData(CVehicle*) : tagged(false) {} };
static VehicleExtendedData<MyVehicleData> vehicleData;  // one static per plugin

Events::vehicleRenderEvent += [](CVehicle* v) {
    MyVehicleData& d = vehicleData.Get(v);
    if (!d.tagged) { /* first render */ d.tagged = true; }
};
```

Works identically with `PedExtendedData<T>` and `ObjectExtendedData<T>`.
See [references/examples/entity-extender.cpp](references/examples/entity-extender.cpp).

## Quick-call API

```cpp
// __cdecl
Call<0xADDRESS>(args...);
CallAndReturn<Ret, 0xADDRESS>(args...) -> Ret;

// __thiscall (methods)
CallMethod<0xADDRESS, ClassName>(thisPtr, args...);
CallMethodAndReturn<Ret, 0xADDRESS, ClassName>(thisPtr, args...) -> Ret;

// Symbolic (from meta headers)
Call<addrof(CStreaming::RequestModel)>(modelId, flags);
CallMethod<addrof(CPed::GiveWeapon), CPed>(ped, WEAPONTYPE_M4, 300, true);

// Multi-version address
uintptr_t addr = ADDRESS_BY_VERSION(compact, hoodlum, eu10, us11, eu11, steam);
```

Full API: [references/patch-and-call-api.md](references/patch-and-call-api.md)

## Global singletons

```cpp
CPlayerPed* player   = FindPlayerPed(0);
CVehicle*   vehicle  = FindPlayerVehicle(0, false);
CVector     pos      = FindPlayerCoors(0);
CCamera&    cam      = TheCamera;
float       dt       = CTimer::ms_fTimeStep;
uint32_t    ms       = CTimer::m_snTimeInMilliseconds;
```

## Key enums location

All in `plugin_sa/game_sa/enums/`:
`eWeaponType`, `eVehicleType`, `eVehicleClass`, `ePedType`, `ePedState`,
`ePedBones`, `eModelID`, `eTaskType`, `eMoveState`, `eFightingStyle`,
`eStats`, `eScriptCommands`, `eGangID`, `eRadioID`, `eCamMode`, `ePadButtons`

## Reference implementations & UI libraries

- **sa-ui** (vendored as a git submodule at `vendor/sa_ui` — skill: `sa-ui`): Modern shadcn-inspired native in-game UI library (cards, buttons, tabs, inputs, dialogs, custom .ttf/.otf fonts, zinc/slate themes) built on `RwIm2D` + `Direct3D 9`. Header-only include: `<sa_ui/sa_ui.h>`. Upstream repo: [mbahArip/sa_ui](https://github.com/mbahArip/sa_ui).
- **v-hud** (optional local reference at `C:\Users\me\dev\sa_project\references\v-hud` — clone of https://github.com/gennariarmando/v-hud): full-source SA `whoami`-style HUD mod (GTA V-style HUD: radar, blips, wanted stars, health/armor, map). Use it as a working reference for HUD drawing techniques, radar map rendering, blip projection, and rounded/solid UI shapes in SA. Key dirs: `source/` (C++), `resources/` (textures), `shaders/`.

## File map

| Need | Path |
|---|---|
| Master include | `shared/plugin.h` |
| All events | `shared/Events.h` |
| Patch API | `shared/Patch.h` |
| Call templates | `shared/PluginBase.h` |
| Injector | `injector/injector.hpp` |
| AOB pattern scan | `hooking/Hooking.Patterns.h` |
| SafetyHook (trampoline) | `safetyhook/safetyhook.hpp` |
| Ped class | `plugin_sa/game_sa/CPed.h` |
| Vehicle class | `plugin_sa/game_sa/CVehicle.h` |
| World / player helpers | `plugin_sa/game_sa/CWorld.h` |
| Pools | `plugin_sa/game_sa/CPools.h` |
| Streaming | `plugin_sa/game_sa/CStreaming.h` |
| Enums | `plugin_sa/game_sa/enums/` |
| Function addresses | `plugin_sa/game_sa/meta/` |
| RenderWare (3.6) | `plugin_sa/game_sa/rw/` |
| Entity extenders | `shared/extender/` |
| Screen coord macros | `shared/extensions/Screen.h` |
| Script opcodes | `shared/extensions/ScriptCommands.h` |
| Pool iteration | `shared/extensions/PoolIterator.h` |
| Key input | `shared/extensions/KeyCheck.h` |
| Font helpers | `shared/extensions/FontPrint.h` |

## Examples index

Load the specific file when the task matches — don't load all:

| Task | File |
|---|---|
| Version check (SA 1.0 US only, error dialog) | [references/examples/version-guard.cpp](references/examples/version-guard.cpp) |
| Bare plugin skeleton | [references/examples/minimal-plugin.cpp](references/examples/minimal-plugin.cpp) |
| Spawn vehicle (all subclass ctors) | [references/examples/vehicle-spawner.cpp](references/examples/vehicle-spawner.cpp) |
| Spawn ped | [references/examples/ped-spawner.cpp](references/examples/ped-spawner.cpp) |
| Per-entity plugin data | [references/examples/entity-extender.cpp](references/examples/entity-extender.cpp) |
| Give/set weapons, weapon slots | [references/examples/weapon-management.cpp](references/examples/weapon-management.cpp) |
| CFont, SCREEN_COORD, RwIm2D | [references/examples/hud-drawing.cpp](references/examples/hud-drawing.cpp) |
| Task assignment, CDecisionMaker | [references/examples/task-system.cpp](references/examples/task-system.cpp) |
| Nop/RedirectCall/StaticHook | [references/examples/memory-patching.cpp](references/examples/memory-patching.cpp) |
| Custom save game & stats persistence | [references/examples/save-game-storage.cpp](references/examples/save-game-storage.cpp) |
| INI config reader/writer (nested sections) | [references/examples/ini-config-parser.cpp](references/examples/ini-config-parser.cpp) |
| Radar blip coloring (single & RGB channel-mask) | [references/examples/radar-blip-coloring.cpp](references/examples/radar-blip-coloring.cpp) |
| Hot-reloadable plugin skeleton | [references/examples/hot-reloadable-plugin.cpp](references/examples/hot-reloadable-plugin.cpp) |

## Hot-reload during development

Use `tools["gta-sa-mcp"].plugin_reload` to swap the `.asi` without restarting GTA SA.
The MCP bridge unloads the old module and loads the fresh build in-process.

### Safe Hot-Reload Contract
To avoid game crashes on `FreeLibrary`, always register events with an explicit ID and export `GtaMcpUnload`:
```cpp
enum ePluginHookIds { HOOK_TICK = 0x1337 };

// In ctor:
Events::gameProcessEvent.AddAfter(OnTick, HOOK_TICK);

// Exported cleanup:
extern "C" __declspec(dllexport) void GtaMcpUnload() {
    Events::gameProcessEvent.RemoveAfterById(HOOK_TICK);
}
```

```
// Standard dev loop — game stays running the whole time:
1. Edit src/
2. MSBuild → Release/6hud.asi
3. tools["gta-sa-mcp"].plugin_reload({ pluginName: "6hud.asi", path: "<abs-path-to-Release/6hud.asi>" })
4. tools["gta-sa-mcp"].screenshot()  // verify
```

**Shell build command:**
```
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" 6hud.vcxproj /p:Configuration=Release /p:Platform=Win32
```

**Hot-reload (execute block):**
```js
await tools["gta-sa-mcp"].plugin_reload({
  pluginName: "6hud.asi",
  path: "C:\\Users\\me\\dev\\sa_project\\6hud\\Release\\6hud.asi"
});
```

**Only kill + relaunch when:**
- The plugin crashes or hangs the game process
- Hot-reload times out with "queued command expired waiting for gameplay"
- A static initializer or hook that runs at DLL load must re-run

**Kill → copy → launch sequence (fallback):**
```js
await tools["gta-sa-mcp"].game_kill();
// shell: Copy-Item -Path "Release\6hud.asi" -Destination "D:\Games\gta sa\GTA San Andreas\6hud.asi" -Force
await tools["gta-sa-mcp"].game_launch();
```

**Debug log export (optional)** — add to plugin for runtime diagnostics:
```cpp
extern "C" __declspec(dllexport) char GtaMcpDebugLog[4096];
extern "C" __declspec(dllexport) int  GtaMcpDebugLen;
// then: snprintf(GtaMcpDebugLog, sizeof(GtaMcpDebugLog), "..."); GtaMcpDebugLen = strlen(GtaMcpDebugLog);
```
Read with `tools["gta-sa-mcp"].plugin_log_read({ pluginName: "6hud.asi", clear: true })`.
