# 32 - Custom PNG Tiles on the ESC Pause Map

## Objective
When custom map tiles are enabled, draw the same high-resolution PNG tiles on
the ESC pause map instead of the vanilla TXD tiles. Transparent water pixels in
the PNGs show the day/night sea color underneath.

---

## 1. Why the pause map was already blank

`CRadar::DrawMap` (`0x586B00`) ends with `JMP 0x586880`, which is
`CRadar::DrawRadarMap` — the same function 6HUD patches to `0xC3` while its own
radar is active.

Confirmed by reading the exe bytes at `0x586B00`:

```
00586D4E  E9 2D FB FF FF   JMP 0x586880
```

So the pause map's tile draw was already suppressed. It rendered blips only.
No new hook was needed to remove vanilla tiles; only a way to add ours.

---

## 2. Hook Choice

No function hook. The plugin already receives `drawRadarOverlayEvent`, which
fires while the pause map is being composed, and
`Radar::DrawMenuMapOverlay()` already draws the GPS ribbon and wanted areas
there. Custom tiles are drawn in the same function, before those overlays, so
blips and the GPS ribbon land on top of the map.

`OnDrawRadar` previously gated that call behind `g_config.enableGps`. The gate
was removed: the overlay must run for tiles even when GPS is off. GPS drawing
inside `DrawMenuMapOverlay` still returns early on its own.

---

## 3. Projection

The pause map uses the game's own radar transform, not a hand-derived one:

```cpp
CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(wx, wy));
CRadar::TransformRadarPointToScreenSpace(scr, radarSpace);
sx = scr.x * sW / 640.0f;
sy = scr.y * sH / 448.0f;
```

This is the identical chain `DrawMenuMapOverlay` already used for the GPS
ribbon, so tiles, ribbon, and blips are guaranteed to agree, at any resolution
and any `FrontEndMenuManager.m_fMapZoom` / `m_fMapBaseX` / `m_fMapBaseY` state.
No panel-rect constants are hardcoded.

---

## 4. Draw Order

1. **Sea base** — one quad over the full `[-3000, +3000]` world square, colored
   by the `radarSeaColor` ↔ `radarSeaColorNight` blend. Drawn untextured with
   alpha blending disabled so it is a solid backdrop.
2. **144 tile quads** — `BlitMapTileBlended` per tile, with the half-texel UV
   inset from plan 31. Transparent water pixels let the sea base through.
3. **Wanted search/heat areas** — existing code.
4. **GPS ribbon** — existing code.

Tiles fully off-screen are skipped by an AABB test against the viewport.

---

## 5. Configuration

`6hud.ini`, section `[Minimap.Radar]`:

```ini
# Also draw the custom PNG tiles on the ESC pause map (requires USE_CUSTOM_MAP_TILE = 1)
# Transparent water areas are filled with minimap.radar.sea / sea_night from colors.ini
# Type: boolean
# Options: 0 = Leave pause map tiles to the game, 1 = Draw custom PNG tiles on pause map
# Default: 1 (follows USE_CUSTOM_MAP_TILE when omitted)
USE_CUSTOM_PAUSE_MAP = 1
```

`src/Config.h`:

```cpp
bool useCustomPauseMap = true;
```

`src/Config.cpp` reads it with a fallback to `useCustomMapTile` when the key is
absent, and writes it back in `SaveConfig`, so an older ini file keeps working
and a fresh one gains the key.

Effective gate:

```cpp
if (g_config.enabled && g_config.useCustomMapTile && g_config.useCustomPauseMap)
```

---

## 6. Verification

1. Press ESC, open the map. Tiles are the custom PNG set, not vanilla TXD.
2. Water reads as `minimap.radar.sea` by day and `sea_night` at night.
3. No seams between tiles (plan 31).
4. Zoom and pan the pause map: tiles track the map exactly, no drift against
   blips or the GPS ribbon.
5. Set `USE_CUSTOM_PAUSE_MAP = 0`, reload: pause map returns to blips-only,
   minimap still uses custom tiles.
6. Change `sea` / `sea_night` in `6hud/colors.ini`, reload: pause map water
   follows.
