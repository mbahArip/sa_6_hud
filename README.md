# 6HUD

> [!WARNING]
> **100% AI-Coded & Work-In-Progress (WIP)**  
> This entire codebase was generated and iterated via LLM agents. Features, memory patches, and layout subsystems are under active development and experimental testing. Expect rapid changes, occasional bugs, or edge cases.

Modern, minimalist HUD, tactical radar, and dynamic navigation system for **Grand Theft Auto: San Andreas (Classic 1.0 US)** built as a native C++ ASI plugin.

---

## Highlights

- **Modern Vector & Minimalist Radar**: High-resolution map tile support, auto-scaling, altitude dynamic zoom, enclosed space zoom, and 16:9 aspect ratio correction.
- **Dynamic GPS Ribbon**: Real-time road pathfinding line with contrast stroke, turn indicators, pedestrian routing on foot, and pause menu map overlay.
- **Acrylic Frosted Glass Shader**: Custom Direct3D 9 pixel shader delivering real-time blurred backdrops behind HUD cards with configurable intensity and multi-pass downsampling.
- **Modular HUD Positioning**: Flexible layout engine supporting grouped flow stacks (`minimap`, `status`, `criminal`, `notification`) or free screen anchors with Cartesian offsets.
- **In-Game Settings Menu**: Live interactive config menu to preview, tweak, and save visual parameters and layout anchors without restarting.
- **Adaptive Info Cards**: Contextual cards for neighborhood/district names, vehicle telemetry (speedometer, class, fuel levels), and idle-timeout auto-hiding.
- **Modern Weapon & Cash Display**: Compact top-right HUD cards with smooth slide animations, clip/reserve ammo modes, weapon icons, and cash flow delta counters.
- **Expanded Police & Wanted Mechanics (Optional Modules)**:
  - **Search Area & Evasion**: Dynamic radar search radius and evasion countdown when breaking line-of-sight.
  - **Crime Zone Heat Perimeter**: Persistent lingering crime zones after evasion.
  - **Emergency Siren Auras**: Screen-edge dynamic lightbar strobes reacting to approaching cop sirens.
  - **Threat Curve Arcs**: Tactical proximity arcs indicating nearby line-of-sight police threats.
  - **Criminal Profile System**: 3-tier heat memory (outfit heat, vehicle license plate/tuning heat, gun heat cooldown) with mask & disguise recognition suppression.
  - **Civilian 911 Snitch System**: Ambient witnesses fleeing crimes and dispatching police calls.

---

## Tested Environment

### Game Executable

| Property | Value |
|---|---|
| **Version** | GTA San Andreas **1.0 US "HOODLUM"** (verified via the `plugin-sdk` signature check at `0x401000` → `0x16197BE9`) |
| **File** | `gta_sa.exe` — 14,383,616 bytes |
| **MD5** | `07B6C7496D10ADA10A7AFB930BF1C709` |
| **SHA-1** | `A1B4203E0B4671D18B123D0E2B027446BD087EE7` |
| **SHA-256** | `21903BCD40957F130A0BD802A44FC4CDBEC2A8224F757B03EC3CB7467CC06EF3` |
| **Readme** | `ReadMe/Readme.txt` → `V1.00` |

> [!NOTE]
> `6hud` patches vanilla draw routines at fixed addresses (`0x586880`, `0x58EAF0`, `0x58AA50`, `0x58AEA0`) and resolves the radar TXD pool at `0xBA8478`. These are valid **only** for the 1.0 US HOODLUM executable. Other builds (1.0 EU, 1.1 US/EU, Steam, Compact) will not work.

### ASI Plugins Installed During Testing

`6hud` was verified running alongside the following ASI plugins.

| Plugin | Version | Role |
|---|---|---|
| `6hud.asi` | this project | 6HUD radar, HUD & navigation |
| `CLEO.asi` | 5.4.0 | CLEO script engine |
| `modloader.asi` | — | Silent's Modloader (file priority & TXD overrides) |
| `SilentPatchSA.asi` | 1.1.34.1 | Silent's Patch (EXE compatibility, input, windowing) |
| `III.VC.SA.LimitAdjuster.asi` | 1.7.0 | Draw distance / streaming / ped limits |
| `III.VC.SA.WindowedMode.asi` | 1.0.0.0 | Windowed mode support (ThirteenAG) |
| `FramerateVigilante.SA.asi` | — | Frame rate cap / monitor sync |
| `imfast.asi` | — | Faster loading times |
| `StreamMemFix1.0.asi` | 1.0 | Streaming memory fix |
| `StreamMemFix2.2_test2.asi` | 2.2 | Streaming memory fix |

---

## Development & Prerequisites

### Required SDK
Building this project requires the extended **plugin-sdk** fork by **Dryxio**:
- Repository: [Dryxio/plugin-sdk-sa](https://github.com/Dryxio/plugin-sdk-sa)

This fork contains reverse-engineered game structures, hooks, and symbol definitions from the complete GTA SA reverse-engineering project required to link `6hud`.

### Toolchain Requirements
- **OS**: Windows 10 / 11
- **Compiler**: Visual Studio 2022 / 2026 (MSVC v143 / v145 with C++20 / C++latest support)
- **Architecture**: 32-bit (x86 / Win32)
- **DirectX**: Microsoft DirectX 9 SDK (June 2010 or compatible header/lib bundle)
- **Target Game**: GTA San Andreas v1.0 US HOODLUM (`gta_sa.exe`, 14,383,616 bytes) — see [Tested Environment](#tested-environment)

### Build Instructions

1. Clone `6hud` with its submodules:
   ```bash
   git clone --recurse-submodules https://github.com/mbahArip/sa_6_hud.git
   cd sa_6_hud
   ```
   Already cloned? Run `git submodule update --init --recursive`.

   The `vendor/sa_ui` submodule provides the [sa_ui](https://github.com/mbahArip/sa_ui) header-only UI library used by the in-game settings menu.

2. Clone the extended SDK by Dryxio:
   ```bash
   git clone https://github.com/Dryxio/plugin-sdk-sa.git
   ```

3. Point the build at your SDK. `$(SdkRoot)` resolves from the `SA_SDK_ROOT` environment variable, then falls back to a local path:
   ```powershell
   $env:SA_SDK_ROOT = "C:\path\to\plugin-sdk-sa"
   ```
   Or override it for a single build:
   ```powershell
   MSBuild.exe 6hud.vcxproj /p:Configuration=Release /p:Platform=Win32 /p:SdkRoot="C:\path\to\plugin-sdk-sa"
   ```

4. Open `6hud.vcxproj` in Visual Studio or build from CLI:
   ```powershell
   MSBuild.exe 6hud.vcxproj /p:Configuration=Release /p:Platform=Win32
   ```

5. Build output (`Release/6hud.asi`) and assets from `assets/` will copy to your game directory.

---

## Installation

1. Install an ASI Loader (e.g. [Silent's ASI Loader](https://gtaforums.com/topic/523431-silents-asi-loader/)).
2. Place `6hud.asi`, `6hud.ini`, and the `6hud/` folder (containing textures and `colors.ini`) into your GTA San Andreas root directory.
3. Start the game.

---

## Configuration

All modules, keybinds, layout anchors, offsets, colors, and wanted extensions are configured via `6hud.ini`:
- `[Global]`: Master keys, units (Metric/Imperial), idle timeouts, blur shader settings.
- `[layout.*]`: Positioning mode (`group` vs `free`), group index, screen anchor, and pixel offsets for all elements.
- `[Minimap.*]`: Radar zoom, GPS ribbon parameters, info cards, fuel extensions.
- `[Wanted.*]`: Star badges, search areas, heat zones, siren auras, threat arcs, criminal profile, and snitch system.

---

## Project Dependencies

| Dependency | Role | Source |
|---|---|---|
| [Dryxio/plugin-sdk-sa](https://github.com/Dryxio/plugin-sdk-sa) | Game structures, hooks, memory patches | Required to build |
| [mbahArip/sa_ui](https://github.com/mbahArip/sa_ui) | Header-only native UI library (settings menu) | `vendor/sa_ui` submodule |

---

## Credits & Acknowledgments

- **100% Code Generation**: LLM agents (Claude / Gemini models via Antigravity harness).
- **SDK**: [Dryxio/plugin-sdk-sa](https://github.com/Dryxio/plugin-sdk-sa) by Dryxio and the Plugin-SDK community.
- **UI Library**: [mbahArip/sa_ui](https://github.com/mbahArip/sa_ui).
- **Reference & Inspiration**: Classic GTA modding community, v-hud by gennariarmando, and GTA V / GTA VI modern UI designs.
