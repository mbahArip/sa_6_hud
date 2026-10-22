# 30 - Map Tile Orientation Fix & Sharper Custom Tiles

## Objective
Two defects in the custom radar tile pipeline:

1. The PNG files were named with row 0 at the **bottom** of the map, while the
   renderer indexes row 0 at the **top**. Every tile was drawn 11 rows off,
   producing a vertically mirrored map.
2. The tiles are 512x512 but displayed at roughly 150px. D3D picked a
   downsampled mip level, so the map looked blurry despite adequate source
   resolution.

---

## 1. Tile Orientation

### Coordinate mapping in the renderer

`RadarRenderMap.inc` walks the 12x12 grid and derives world coordinates as:

```cpp
float wx0 = -3000.0f + (float)tx * 500.0f;      // tx: west -> east
float wy0 =  3000.0f - (float)(ty + 1) * 500.0f; // ty: north -> south
int tileIndex = tx + 12 * ty;
```

- `ty == 0` → `wy0` near `+2500` → **north / top of map**
- `tx == 0` → `wx0` near `-3000` → **west / left of map**
- Therefore `tileIndex == 0` is the **top-left** tile, and `143` is **bottom-right**.

### The mismatch

The supplied PNGs used row 0 at the bottom, so `radar00.png` was the bottom-left
tile. The renderer drew it in the top-left slot, mirroring the whole map
vertically.

### The fix

Rename every file, flipping only the row axis:

```
n   = col + 12 * row_from_bottom
n'  = col + 12 * (11 - row_from_bottom)
```

Two-pass rename so no target name collides with a source that has not been moved
yet:

1. `radar<NN>.png` → `tmp_<NN>.png`
2. `tmp_<NN>.png` → `radar<NN'>.png` using the formula above

Applied to both `assets/radar/map/day/` and `assets/radar/map/night/`.
Verified: both sets remain contiguous `0..143` with no gaps or duplicates.

Worked example: `radar00` (bottom-left) → `radar132` (row 11, col 0).
`radar143` (top-right) → `radar11` (row 0, col 11).

---

## 2. Blurry Tiles

### Cause

`LoadPngTexture` creates the texture with `D3DX_DEFAULT` mip levels, so a full mip
chain down to 1x1 exists. `drawCustomMapTile` set `MINFILTER`/`MAGFILTER` to
`D3DTEXF_LINEAR` but left `D3DSAMP_MIPFILTER` at its default, allowing the
hardware to sample a much smaller mip. A 512px tile drawn at ~150px lands on mip
level 2 or 3, which reads as soft.

### Fix

`src/RadarRenderMap.inc`, inside `drawCustomMapTile`:

```cpp
dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
```

This forces sampling from the top mip (the full 512x512 image) regardless of
on-screen size. The mip chain still exists and is harmless; it is simply never
selected.

`MINFILTER`/`MAGFILTER` stay `LINEAR`, which is correct for the sub-pixel
downscale and avoids aliasing.

---

## 3. Verification

1. Enter the game, open the radar. Land shapes match the world: coastline and
   roads line up with the actual geography, no vertical mirroring.
2. Compare a known landmark's radar position against the in-game world.
3. Text and road edges on the radar read crisply rather than soft.
4. Toggle day/night: both tile sets are correctly oriented and equally sharp.
5. `assets/radar/map/day` and `assets/radar/map/night` each contain exactly 144
   files named `radar00.png` through `radar143.png`.
