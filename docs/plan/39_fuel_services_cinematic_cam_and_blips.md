# Fuel Services Cinematic Camera & Model Configuration

## Overview
Adds smooth cinematic camera choreography for vehicle fuel and repair services, fixes fixed-cam aim tracking in GTA SA's `CCam::Process_Fixed`, removes cluttering native pump radar blips, and adds configurable gas pump model names.

## Implementation Details

### 1. Camera Orbit Choreography, Framing & Collision Awareness (`src/ui/cards/RadarInfoCardsUpdate.inc`)
- **Menu Camera Framing**:
  - Positions camera 2.5m behind vehicle exterior facing pump nozzle at eye level (`+0.60f`).
  - Probes line-of-sight ray from car body toward desired camera point (`ResolveCameraCollision`). If wall or obstacle sits behind car, pulls camera safely in front (0.35m bumper). Zero wall clip.
- **Staged Operation Flow**:
  1. `select` -> `ORBIT_GLIDE_OUT`: Camera smoothstep glides (0.8s) from menu pump view to vehicle front 3/4 quarter angle opposite pump side. Fill timer paused (`progress = 0%`).
  2. `start` -> `ORBIT_RUNNING`: Camera arrives at front 3/4 position. Operation timer starts (`s_serviceOpTimer`), fuel / repair gauge fills from start value to 100%, and camera slowly drifts horizontally (`0.08f` rad/s, reversed direction away from pump).
  3. `done` -> `ORBIT_GLIDE_IN`: Operation hits 100%. Camera smoothstep glides (0.8s) from live orbit spot back into menu pump view.
  4. `menu restored`: Sits back at pump view, `s_serviceOp` resets to `SERVICE_OP_NONE`, and menu returns to selectable rows.
- **Collision-Aware Orbit**:
  - Casts `CWorld::ProcessLineOfSight` ray from car `lookAt` to desired orbit point each frame (`checkBuildings = true`, `checkObjects = true`).
  - Pulls camera inward with 0.35m safety buffer when hitting building geometry, canopy columns, or walls (clamped min 2.0m).
  - Asymmetric spring-arm smoothing (fast push-in `dt * 8.0f`, smooth release `dt * 3.0f`) to prevent pop on thin pillars.
- **Opposite Rotation**:
  - Pump on driver/left side: starts front-right (`vehYaw + 45°`), rotates clockwise (`s_orbitDir = +1.0f`) along passenger side.
  - Pump on passenger/right side: starts front-left (`vehYaw - 45°`), rotates counter-clockwise (`s_orbitDir = -1.0f`) along driver side.
- **Fixed-Cam Aim Tracking**:
  - Writes directly to active `CCam` members (`m_vecCamFixedModeSource`, `m_vecSource`, `m_vecCamFixedModeVector`, `m_vecTargetCoorsForFudgeInter`) inside `DriveOrbit`.
  - Raised pump focus aim point `s_serviceCamLookAt.z` to eye level (`+0.60f`).

### 2. In-Place Menu Progress Indicator (`src/ui/cards/RadarRenderFuelServices.inc`)
- During active operations, menu options swap in-place for progress ring and state label (`Refuelling...` / `Repairing...`).
- Key badge text cleared during running operations for clean circular arc indicator.

### 3. Configurable Pump Models & Clean Live Detection (`src/ui/cards/RadarInfoCards.inc`, `Config.h`, `Config.cpp`, `6hud.ini`)
- Added `FUEL_PUMP_MODEL_NAMES` comma-separated setting (defaults to `washgaspump, petrolpumpnew, vegspetrolpump`).
- Dynamically parses pump model names and resolves model IDs via `CModelInfo::GetModelInfo`.
- Removed native pump radar blips, `InitPumpClusters`, global IPL loading, and slot 63 texture patching to prevent radar clutter.
- Service trigger now directly scans live `CPools::ms_pObjectPool` for closest pump matching configured models within `REFUEL_RADIUS`.

### 4. Sequential Part-by-Part Repair Flow (`src/ui/cards/RadarInfoCardsUpdate.inc`)
- Pre-allocated zero-heap repair action queue (`s_repairQueue[32]`) scanning `CAutomobile::m_damageManager` upon selecting repair.
- Separates damaged components into two sequential phases:
  1. **Damaged / Dented First** (`t = 0.08 .. 0.48`): pops unbent panels, restores dented doors, replaces flat tyres (`FixTyre`), and fixes light glass.
  2. **Detached Second** (`t = 0.54 .. 0.90`): reattaches missing/knocked-off doors and lost bumpers (`FixDoor`, `FixPanel`).
- Plays native `AE_FRONTEND_BUY_CAR_MOD` mechanical tool audio clink as each part snaps restored.
- Calls authoritative `veh->Fix()` at `t >= 1.0f` to normalize engine fluids, smoke/fire cleanup, and health (1000.0f).

### 5. GTA 6 1.5-Row Sliding Carousel & Vertical Connecting Strip (`src/ui/cards/RadarRenderFuelServices.inc`)
- Viewport size locked to 1.5 rows (`rowH * 1.5f`) with active item centered at anchor Y.
- Smooth carousel sliding via `s_serviceScrollRow` animated toward target selection.
- Vertical connecting strip (`DrawVerticalStrip`) drawn along badge/pip center X with 24 vertical slices via raw Direct3D 9 `DrawPrimitiveUP` (`D3DFVF_2DVERT`), bypassing RenderWare flat-shading state with true hardware `D3DSHADE_GOURAUD` per-pixel interpolation.
- Texture Stage 0 state restored to `D3DTOP_MODULATE` immediately after raw D3D draw to preserve font rendering colors (fixes black HUD money text).
- Connecting strip clamps to outer perimeters of top and bottom nodes (`rowY[topIdx] + topR` to `rowY[botIdx] - botR`), eliminating double-alpha opacity overlap inside pip circles and badge discs.
- Active item renders full circular button prompt disc; inactive item renders connected circular pip node (`•`).
- Vertical grip separator upgraded to a mini gauge chip with circular background disc and outer track ring (without radial fill).
- Expanded horizontal spacing between key badge, grip chip, and label text for breathing room.
- Distance-based gradient fade (`CalcRowFade`) stays 100% solid through `0.50 * rowH`, ramping down to 0% at `1.25 * rowH` (75% through adjacent row) with matching hardware scissor bounds.
- Camera framing height lowered to `0.50f` above pump and car body, with pullback distance set to `3.0f` meters.

## Verification
- Built Win32 Release target with MSBuild (`6hud.asi`).
- Verified clean build and deployment to game directory.
