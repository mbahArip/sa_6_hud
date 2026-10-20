# 10 - Minimap Group Proportional 90% Scale

## Objective
Scale down the entire minimap group (radar, inner elements, and stacked information cards) to 90% (0.90x), maintaining exact aspect ratios, typography scales, gauge geometry, and spacing across the entire group.

## Scope of Scaling
1. **Radar Base Frame & Elements**:
   - `g_radarRect`: Base height `sH * 0.25f * radarSizeScale`, width `h * 1.52f`.
   - Player puck disc radius, north badge, clock font, and distance indicator scale with `radarSizeScale`.
2. **Minimap Stacked Information Cards**:
   - GPS Turn Navigation card height, badge, font sizes, and text margins scale with `radarSizeScale`.
   - Place/Zone information card height, font size, and text margins scale with `radarSizeScale`.
   - Vehicle information card height, split divider position, font sizes, circular gauge radius, track stroke, and nitro badge scale with `radarSizeScale`.
   - Telemetry gauge horizontal slot spacing in `RadarInfoCardsUpdate.inc` scales with `radarSizeScale`.
3. **Configuration & Default**:
   - Update default `radarSizeScale` in `Config.h` to `0.90f`.
   - Support `RADAR_SIZE_SCALE` in `[Minimap.Radar]` section of `6hud.ini` (default `0.90`).
   - Serialize `RADAR_SIZE_SCALE` under `[Minimap.Radar]` in `SaveConfig()`.

## Files Modified
- `src/Config.h`: Default `radarSizeScale = 0.90f`.
- `src/Config.cpp`: Read/write `RADAR_SIZE_SCALE` under `[Minimap.Radar]`.
- `6hud.ini`: Add `RADAR_SIZE_SCALE = 0.90` to `[Minimap.Radar]`.
- `src/RadarRenderCards.inc`: Apply `cardScale = g_config.radarSizeScale` to card heights, divider, text margins, font scales, and gauges.
- `src/RadarInfoCardsUpdate.inc`: Scale gauge horizontal spacing by `radarSizeScale`.
- `src/RadarRenderPlayer.inc`: Scale puck disc radius by `radarSizeScale`.
- `src/RadarRenderNorth.inc`: Scale north badge radius by `radarSizeScale`.
- `src/RadarRenderDistance.inc`: Scale iFruit icon, distance font, and clock font by `radarSizeScale`.
