# 31 - Map Tile Seam (Half-Texel UV Inset)

## Objective
Custom radar tiles showed a 1px transparent strip along every tile boundary,
visible as a faint grid over the map.

---

## 1. Cause

`drawCustomMapTile` sampled the full 0.0–1.0 UV range with
`D3DTADDRESS_CLAMP` and `D3DTEXF_LINEAR` filtering.

At UV exactly 0.0 or 1.0 the LINEAR filter averages the outermost texel with
whatever lies outside the image. Because the PNGs are transparent around the
edges, that average pulls alpha toward zero, producing a translucent line on
every shared edge between two tiles.

The `sx0 - 0.5f` half-pixel offset on the vertex positions was unrelated; it
compensates for D3D9's texel-center convention and does not address the edge
bleed.

---

## 2. Fix

Inset the UVs by half a texel so the filter never reaches the boundary:

```cpp
const float uv0 = 0.5f / 512.0f;   // 512 = tile resolution
const float uv1 = 1.0f - uv0;
```

Applied to the four quad corners instead of `0.0f` / `1.0f`.

`MINFILTER`/`MAGFILTER` stay `LINEAR`, which is still correct for the sub-pixel
downscale and avoids aliasing. `MIPFILTER = NONE` from plan 30 is unchanged.

---

## 3. Code Shape

The tile blit was extracted out of `DrawMap2D` into two file-scope helpers in
`src/RadarRenderMap.inc` so the minimap and the pause map share one
implementation:

| Helper | Purpose |
|---|---|
| `BlitMapTileTexture` | Sets D3D9 sampler/stage/render state and draws one textured quad with the inset UVs |
| `BlitMapTileBlended` | Resolves day/night textures for a tile index and crossfades them with `GetDayNightBlend()` |

`DrawMap2D` previously carried ~50 lines of inline D3D setup inside a lambda;
that is now a single `BlitMapTileBlended(...)` call.

---

## 4. Assumption

`0.5f / 512.0f` hardcodes the tile resolution. If tiles are ever re-exported at a
different size, this constant must change with them. A `GetLevelDesc` lookup
would remove the assumption at the cost of a D3D call per tile per frame.

---

## 5. Verification

1. Open the radar. No faint grid or transparent lines between tiles.
2. Pan across several tile boundaries at default zoom: seams stay invisible.
3. Zoom the radar out: seams still invisible (UV inset is resolution-relative).
4. Day/night crossfade still works; no seam appears mid-transition.
