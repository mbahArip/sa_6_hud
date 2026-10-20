# 6hud Architecture & System Overview

## Purpose
`6hud` modernizes the Grand Theft Auto: San Andreas radar interface by implementing the visual identity, fluid dynamics, and framing inspired by GTA 6.

## Architecture & Data Flow

```text
       ┌───────────────────────┐
       │      6hud.ini         │ (Configuration)
       └──────────┬────────────┘
                  │ LoadConfig()
                  ▼
       ┌───────────────────────┐
       │     Config.cpp        │ <─── g_config
       └──────────┬────────────┘
                  │
  Events::gameProcessEvent
                  │
                  ▼
       ┌───────────────────────┐
       │    Radar::Process()   │ <─── CPlayerPed / TheCamera / ThePaths
       │ - UpdatePlayerData()  │
       │ - UpdateCamera()      │ (Calculates basis, tilt, smoothed ranges)
       │ - UpdateGps()         │ (Calculates road routing nodes)
       └──────────┬────────────┘
                  │
   Events::drawHudEvent
                  │
                  ▼
       ┌───────────────────────┐
       │    Radar::Render()    │
       │  ┌──────────────────┐ │
       │  │ Stencil & Map 2D │ │ (3x3 tile streaming from CTxdStore)
       │  ├──────────────────┤ │
       │  │ Police Tint      │ │ (Sinusoidal siren flashing)
       │  ├──────────────────┤ │
       │  │ GPS Road Ribbon  │ │ (Subdivided Catmull-Rom spline)
       │  ├──────────────────┤ │
       │  │ Bezel & Border   │ │ (3.5px charcoal + slate highlight)
       │  ├──────────────────┤ │
       │  │ Blip Badges      │ │ (Circular colored discs + icons)
       │  ├──────────────────┤ │
       │  │ Pin & Distance   │ │ (Waypoint marker + metric/imperial label)
       │  ├──────────────────┤ │
       │  │ North Badge      │ │ (Bezel-clamped puck)
       │  ├──────────────────┤ │
       │  │ Player Puck      │ │ (White chevron in dark badge)
       │  └──────────────────┘ │
       └───────────────────────┘
```

## Next-Gen Roadmap & Unimplemented Features
For a detailed breakdown of pending systems, proxy loaders, typography overhaul, micro-bars, and GPS glow passes, see [unimplemented_features.md](unimplemented_features.md).

1. **Game Startup**: Plugin loads `6hud.ini`, patches `0x586B00` (RET at `CRadar::DrawMap`), and binds lifecycle events.
2. **Warmup Phase**: During loading screens and the first 10 frames of gameplay, the radar remains passive to allow entity pools to stabilize.
3. **Active Gameplay**:
   - `gameProcessEvent`: Reads player position, camera orientation, and computes smoothed pitch/range interpolation.
   - `drawHudEvent`: Hides vanilla radar via `CHud::bScriptDontDisplayRadar` and executes Direct3D 9 stencil drawing pass.
4. **Menus / Cutscenes**: Automatically pauses rendering to prevent interfering with scripted camera cuts and pause menus.
