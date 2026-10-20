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
- **Target Game**: GTA San Andreas v1.0 US (`gta_sa.exe`, 14,383,616 bytes)

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
