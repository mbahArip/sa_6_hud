# 6hud Developer & Agent Guidelines

## Overview
`6hud` is a standalone GTA San Andreas plugin (.asi) that replaces the classic 2D radar with a sleek, GTA 6-style landscape HUD featuring:
- 2.5D perspective tilt on vehicles and top-down view on foot
- Direct3D 9 stencil-masked rounded rect bezel frame
- Multi-layered charcoal and slate glass borders
- 3x3 2D map texture streaming from San Andreas radar TXD pool
- Circular disc trace blips and normalized icon sprites
- Rounded perimeter boundary clamping (`ProjectToRim`)
- GTA 6 circular navigation arrow puck and North compass badge
- Police chase sinusoidal siren overlay
- GPS waypoint road ribbon routing

## Project Structure
```text
6hud/
├── 6hud.vcxproj            ; Visual Studio C++ project file (Win32 .asi)
├── 6hud.ini                ; Runtime configuration file
├── AGENT.md                ; Agent & developer guidelines (this file)
├── docs/
│   ├── overview.md         ; Architecture, lifecycle, feature map
│   ├── radar_pipeline.md   ; Stencil masking, projection math, clamping
│   └── configuration.md    ; INI configuration guide
└── src/
    ├── Main.cpp            ; Plugin entry point and event hooks
    ├── Config.h / .cpp     ; INI reader and runtime settings
    └── Radar.h / .cpp      ; Projection, geometry, streaming, blips & HUD rendering
```

## Build & Deployment
- **Compiler**: Visual Studio 2022 / 2026 MSBuild (v145 or v143 toolset).
- **Target**: Win32 Release (`6hud.asi`).
- **Build command**:
  ```powershell
  & "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" "6hud.vcxproj" /p:Configuration=Release /p:Platform=Win32 /v:m
  ```
- **Install destination**: Copy `6hud.asi` and `6hud.ini` directly to the GTA SA root directory (`D:\Games\gta sa\GTA San Andreas\`).

## Critical GTA SA Addresses & Hooks
- `0x586B00 = 0xC3`: RET patch at `CRadar::DrawMap` to prevent vanilla radar draw flickering.
- `CHud::bScriptDontDisplayRadar`: Set to `true` during active gameplay to hide vanilla HUD elements.
- `0xBA8478`: Pointer to `gRadarTxdIds` array (144 integers mapping radar tiles `0..143` to TXD pool slots).
- `0xBA677C`: `FrontEndMenuManager.m_nTargetBlipIndex` (active pause menu waypoint index).
- `ThePaths.DoPathSearch(...)`: Vehicle road routing function in `CPathFind`.

## Engineering & Coding Standards
1. **Safety First (SEH Guards)**: Wrap critical projection and rendering loops in structured exception handlers (`__try / __except`) with logging to `6hud.log`.
2. **Minimal Diffs**: Keep algorithms clean, direct, and inline where single-use.
3. **No Dynamic Allocations in Render Loops**: Pre-allocate vertex buffers and node arrays to avoid heap stalls.
4. **Clean Affine Projection**: Use linear $Y$-axis compression (`scale = hh / radarRange`, `sy = -ry * cosP * scale`) to keep road tiles undistorted.
5. **Always Request Confirmation**: Before launching or killing GTA SA, confirm with the user.
