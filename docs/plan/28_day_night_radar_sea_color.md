# 28 - Day/Night Radar Sea Color

## Objective
The custom high-resolution radar tiles (`assets/radar/map/<day|night>/radar<NN>.png`)
are transparent over water so the flat sea quad underneath shows through. That sea
quad used a single hardcoded color, which clashed with both the day and the night
tile sets. Give the water a day and a night color and blend between them with the
same curve the tiles already use.

---

## 1. Problem

`RadarRenderMap.inc` drew the sea fill from `g_config.radarSeaColor`, a single
`CRGBA` defaulting to `#5daece`. The custom tiles crossfade from day to night via
`GetDayNightBlend()`, but the water underneath did not — so at dusk the water stayed
bright daylight cyan while the tiles darkened around it.

---

## 2. Solution

Two colors, blended with the existing day/night curve.

### `assets/colors.ini`

```ini
[minimap.radar]
sea = #86aebf
sea_night = #5c5795
```

### `src/Config.h`

```cpp
CRGBA radarSeaColor      = { 134, 174, 191, 255 }; // #86aebf (minimap.radar.sea) - daytime water
CRGBA radarSeaColorNight = {  92,  87, 149, 255 }; // #5c5795 (minimap.radar.sea_night) - night water
```

### `src/Config.cpp`

```cpp
g_config.radarSeaColor      = readColor("minimap.radar", "sea",       { 134, 174, 191, 255 });
g_config.radarSeaColorNight = readColor("minimap.radar", "sea_night", {  92,  87, 149, 255 });
```

`SaveConfig` writes both keys back so the in-game menu never drops `sea_night`.

### `src/RadarRenderMap.inc`

Blended once per frame, before the tile loop:

```cpp
float seaNightBlend = GetDayNightBlend();
CRGBA seaCol = CRGBA(
    (unsigned char)Lerpf((float)g_config.radarSeaColor.r, (float)g_config.radarSeaColorNight.r, seaNightBlend),
    (unsigned char)Lerpf((float)g_config.radarSeaColor.g, (float)g_config.radarSeaColorNight.g, seaNightBlend),
    (unsigned char)Lerpf((float)g_config.radarSeaColor.b, (float)g_config.radarSeaColorNight.b, seaNightBlend),
    (unsigned char)Lerpf((float)g_config.radarSeaColor.a, (float)g_config.radarSeaColorNight.a, seaNightBlend)
);
```

Both sea-quad sites (inside the 12x12 grid and outside it) use `seaCol`.

---

## 3. Day/Night Curve

`GetDayNightBlend()` returns `0.0` (day) to `1.0` (night):

| Game hour | Blend |
|---|---|
| 07:00 – 19:00 | `0.0` (full day) |
| 19:00 – 21:00 | smoothstep up to `1.0` |
| 21:00 – 05:00 | `1.0` (full night) |
| 05:00 – 07:00 | smoothstep back down to `0.0` |

The water and the tiles therefore transition in lockstep.

---

## 4. Layering

Custom tiles replace the vanilla TXD tile, they do not overlay it. When a custom tile
exists for a slot, the renderer draws it and `continue`s, skipping the TXD path. The
sea quad is drawn *before* the tile, so it shows through the tile's transparent water
pixels — which is exactly the intent.

If `USE_CUSTOM_MAP_TILE = 1` but a tile PNG is missing on disk, that slot falls
through to the vanilla TXD tile rather than leaving a hole.

---

## 5. Verification

1. Daytime: water reads as `#86aebf`, tiles crisp, no cyan fringe.
2. Night: water reads as `#5c5795`, tiles darkened, water matches.
3. Set game clock through 19:00–21:00: water and tiles crossfade together.
4. Edit `sea_night` in `6hud/colors.ini`, reload config: night water changes.
