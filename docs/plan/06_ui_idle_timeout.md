# Plan 06: Global UI Idle Timeout

## Objective
Unify auto-hide and peek timers under a global `UI_IDLE_TIMEOUT` config in `[global]`.

## Specifications
- **Config**: `[global]` -> `UI_IDLE_TIMEOUT = 4.0` (seconds).
- **Minimap / Radar**: Replace hardcoded 3.5s peek timer in `RadarProcess.inc` with `g_config.globalIdleTimeout`.
- **Weapon HUD**: Auto-hides after `UI_IDLE_TIMEOUT` seconds of inactivity.
- **Money HUD**: Auto-hides after `UI_IDLE_TIMEOUT` seconds outdoors.
- **'J' Key Peek**: Resets and holds alpha to 1.0 for minimap, weapon HUD, and money HUD while active.
