# 6HUD Project Instructions

## Overview
`6hud` is a standalone GTA San Andreas plugin (`.asi`) that replaces the classic 2D radar with a sleek, GTA 6-style landscape HUD featuring:
- Modern vector radar shell with high-resolution map tile streaming
- Direct3D 9 stencil-masked rounded rect bezel frame with frosted glass blur shader
- GPS waypoint road ribbon routing with turn indicators
- Modular HUD positioning engine (grouped stacks + free screen anchors)
- Optional police/wanted extension modules (search area, heat zone, siren auras, threat arcs, criminal profile, civilian 911 snitch system)

## Project Paths & Environment
- **Game Directory**: `D:\Games\gta sa\GTA San Andreas`
- **Plugin SDK Repository**: `C:\Users\me\dev\sa_project\plugin-sdk-sa` (fork of [Dryxio/plugin-sdk-sa](https://github.com/Dryxio/plugin-sdk-sa))
- **Target**: GTA San Andreas 1.0 US, Win32 (`gta_sa.exe`)

## Project Structure
```text
6hud/
├── 6hud.vcxproj            ; Visual Studio C++ project file (Win32 .asi)
├── 6hud.ini                ; Runtime configuration file (fully commented)
├── .agents/skills/         ; Project-scoped agent skills (sa-plugin-sdk, sa-ui)
├── assets/                 ; Textures + colors.ini shipped to game folder
├── shaders/                ; Standalone .hlsl shader sources
├── vendor/sa_ui/           ; Git submodule: header-only native UI library
├── docs/
│   ├── overview.md         ; Architecture, lifecycle, feature map
│   ├── radar_pipeline.md   ; Stencil masking, projection math, clamping
│   ├── configuration.md    ; INI configuration guide
│   ├── unimplemented_features.md
│   └── plan/               ; One plan file per implemented feature
└── src/
    ├── Main.cpp            ; Plugin entry point and event hooks
    ├── Config.h / .cpp     ; INI reader, serializers, runtime settings
    ├── Layout.h            ; Modular positioning model (groups, anchors, offsets)
    ├── Menu.h / .cpp       ; In-game settings menu
    ├── Radar.h / .cpp      ; Radar subsystem entry + .inc implementation units
    └── Radar*.inc          ; Radar pipeline modules (render, heat, GPS, wanted, ...)
```

## Build & Deployment
- **Compiler**: Visual Studio 2022 / 2026 MSBuild (v143 / v145 toolset), Win32 Release.
- **Build command**:
  ```powershell
  & "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" "6hud.vcxproj" /p:Configuration=Release /p:Platform=Win32 /v:m
  ```
- **SDK path**: `$(SdkRoot)` resolves from the `SA_SDK_ROOT` environment variable, then falls back to `C:\Users\me\dev\sa_project\plugin-sdk-sa`. Override per build with `/p:SdkRoot="<path>"`.
- **Submodules**: `vendor/sa_ui` must be initialized (`git submodule update --init --recursive`) before building.
- **Post-build**: copies `6hud.asi` and `assets/*` into the game directory automatically.
- **Install destination**: `D:\Games\gta sa\GTA San Andreas\` (holds `6hud.asi`, `6hud.ini`, `6hud/` assets folder).

## Critical GTA SA Addresses & Hooks
- `0x586880` = `CRadar::DrawRadarMap` — patched to `0xC3` while the 6HUD radar is enabled, restored to `0x83` otherwise.
- `0x58EAF0` = `CHud::DrawPlayerInfo` — patched to `0xC3` to hide vanilla weapon icon, ammo, money, and health bars; restored to `0x81`.
- `0x58AA50` / `0x58AEA0` — vanilla zone and vehicle name draw routines, patched to `0xC3` when 6HUD info cards are active; restored to `0x51`.
- `0xBA8478` — pointer to `gRadarTxdIds` array (144 integers mapping radar tiles `0..143` to TXD pool slots).
- `FrontEndMenuManager.m_nTargetBlipIndex` — active pause menu waypoint index.
- `ThePaths.DoPathSearch(...)` — vehicle road routing function in `CPathFind`.
- `CHud::bScriptDontDisplayRadar` — toggled to hide vanilla HUD elements during active gameplay.

## Planning & Documentation Rules
- For every implementation or confirmed plan, write a markdown file under `docs/plan/` using the naming format `<number>_<planned_feature>.md`.
- If a plan spans multiple features, split into separate plan files so each feature has exactly 1 plan file.
- Every `6hud.ini` option must carry an inline comment block documenting purpose, type, available options/enums, min/max range, and default value.

## SDK & Safety Auditing Rules
- Always audit or recheck game structures, hook points, memory offsets, and API signatures against the `sa-plugin-sdk` skill or its source repository (`C:\Users\me\dev\sa_project\plugin-sdk-sa`) to verify safety and compatibility with GTA San Andreas 1.0 US.
- When performing an audit, invoke the `audit-loop` skill to conduct a rigorous, iterative review-and-fix pass before finalizing changes.

## Engineering & Coding Standards
1. **Safety First (SEH Guards)**: Wrap critical projection and rendering loops in structured exception handlers (`__try / __except`) with logging to `6hud.log`.
2. **Minimal Diffs**: Keep algorithms clean, direct, and inline where single-use.
3. **No Dynamic Allocations in Render Loops**: Pre-allocate vertex buffers and node arrays to avoid heap stalls.
4. **Clean Affine Projection**: Use linear Y-axis compression (`scale = hh / radarRange`, `sy = -ry * cosP * scale`) to keep road tiles undistorted.
5. **No Dangling Pointers**: Resolve pooled entities through `CPools` SCM handles instead of caching raw `CVehicle*`/`CPed*` pointers.
6. **Never Call Game Functions Before The Game Is Ready**: Game-code calls such as `CKeyGen::GetUppercaseKey` are unsafe during plugin load; use the local `hud6::HashUppercaseKey` implementation instead.
7. **Always Request Confirmation**: Before launching or killing GTA SA, confirm with the user.
