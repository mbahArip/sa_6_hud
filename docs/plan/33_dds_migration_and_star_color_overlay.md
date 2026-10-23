# 33 - DDS Asset Migration & Wanted Star Color Overlay

## Objective
Move every shipped texture from PNG to DDS, relocate them into a
purpose-grouped folder layout, and collapse the four wanted-star textures into a
single texture plus per-state diffuse tinting.

---

## 1. New Asset Layout

| Old | New |
|---|---|
| `assets/wallet.png` | `assets/icons/wallet.dds` |
| `assets/radar/engine.png` | `assets/icons/engine.dds` |
| `assets/radar/fuel.png` | `assets/icons/fuel.dds` |
| `assets/radar/nitro.png` | `assets/icons/nitro.dds` |
| `assets/radar/ifruit.png` | `assets/icons/ifruit.dds` |
| `assets/radar/mission.png` | `assets/blips/mission.dds` |
| `assets/radar/north.png` | `assets/blips/north.dds` |
| `assets/radar/player.png` | `assets/blips/player.dds` |
| `assets/radar/player_wanted.png` | `assets/blips/player_wanted.dds` |
| `assets/radar/z_above.png` | `assets/blips/z_above.dds` |
| `assets/radar/z_below.png` | `assets/blips/z_below.dds` |
| `assets/wanted/personelArc.png` | `assets/ui/cops_arrow.dds` |
| `assets/wanted/profile/*.png` | `assets/icons/profile/*.dds` |
| `assets/wanted/starBase.png` | `assets/icons/wantedStar.dds` |
| — | `assets/icons/wantedStarBorder.dds` |
| `assets/wanted/starHeat.png` | removed (color overlay) |
| `assets/wanted/starLooking.png` | removed (color overlay) |
| `assets/wanted/starNoInfo.png` | removed (replaced by `wantedStarBorder.dds`) |
| `assets/radar/map/day/*.png` | `assets/maps/day/*.dds` |
| `assets/radar/map/night/*.png` | `assets/maps/night/*.dds` |

Stale `assets/radar/` and `assets/wanted/` trees were deleted from the game
directory. `xcopy /E` never deletes, so leftovers had to be removed manually.

---

## 2. Loader

`LoadPngTexture` renamed to `LoadTexture`, since it no longer implies a format.

Two loader changes:

- `D3DFMT_A8R8G8B8` → `D3DFMT_UNKNOWN`. DDS files carry their own pixel format
  (DXT1/DXT5); forcing A8R8G8B8 would decompress and re-upload them.
- Size/mip args stay `D3DX_DEFAULT_NONPOW2` / `D3DX_DEFAULT` so a DDS's baked
  mip chain is preserved.

`D3DXCreateTextureFromFileExA` reads DDS natively, so nothing else changed.

Per-tile multi-extension fallback chains in `GetMapTileTextures` were deleted;
the files are now at one known path:

```cpp
snprintf(name, sizeof(name), "maps\\day\\radar%02d.dds", tileIndex);
snprintf(name, sizeof(name), "maps\\night\\radar%02d.dds", tileIndex);
```

Custom weapon icons also switched to `.dds`.

---

## 3. Wanted Star Color Overlay

Previously four separate textures encoded four states. Now one texture,
`wantedStar.dds`, is drawn in every state and recolored through the diffuse
vertex color, which `DrawD3DTexturedQuad` combines via `D3DTOP_MODULATE`.

```cpp
static const CRGBA kStarTintActive  (255, 255, 255);
static const CRGBA kStarTintSlot    ( 50,  50,  50);
static const CRGBA kStarTintHeat    (255,  31,  38);
static const CRGBA kStarTintLooking (147, 149, 152);
```

| State | Texture | Tint |
|---|---|---|
| Inactive slot | `wantedStar` | `50, 50, 50` @ `inactiveA` |
| Active | `wantedStar` | `255, 255, 255` @ `activeA` |
| Heat (pursuit) | `wantedStar` | `255, 31, 38` @ `activeA` |
| Looking (profile active) | `wantedStar` | `147, 149, 152` @ `activeA` |
| No info | `wantedStarBorder` | `255, 255, 255` @ `activeA` |

Alpha handling is unchanged: `activeA` / `inactiveA` still derive from
`g_wantedAnimProgress`, so the slide and glow animations keep working.

### No-info slots

`wantedStarBorder` replaces the filled star entirely. The inactive background
star is skipped for those slots, otherwise the filled star would show behind
the hollow border:

```cpp
bool isHollowSlot = isActive && isHollowState;
if (!isHollowSlot) { /* draw inactive base star */ }
```

### Removed globals

`g_pTexStarHeat`, `g_pTexStarLooking`, `g_pTexStarNoInfo` deleted from
`RadarState.inc`, along with their load and release calls.
`g_pTexStarBorder` added.

---

## 4. D3D State Corruption Fix

Symptom: after the pause map was opened, other HUD icons drew jagged or
untextured.

Cause: `BlitMapTileTexture` uses raw D3D9 calls and sets `FVF`,
`ALPHABLENDENABLE`, `SRCBLEND`, `DESTBLEND`, `ZENABLE`, `LIGHTING`, `CULLMODE`,
`PIXELSHADER`, sampler 0 and texture-stage 0 state. None of it was restored, so
whatever the game drew next inherited our state.

Fix: `MapDrawState` in `src/RadarRenderMap.inc` snapshots 21 pieces of state
before the tile loop and restores them after. Wrapped around both tile draw
sites:

- `Radar::DrawMap2D` (minimap)
- `Radar::DrawMenuMapOverlay` (pause map)

The struct holds its own `IDirect3DBaseTexture9*` reference from `GetTexture`,
so it `Release()`s it in `Restore()`.

---

## 5. Verification

1. HUD icons render correctly (not jagged, not untextured) after opening and
   closing the pause map.
2. Minimap tiles and pause map tiles both load from `assets/maps/{day,night}`.
3. Wanted stars: inactive slots dark grey, active white, heat red, looking grey.
4. No-info slots show a hollow border with no filled star behind it.
5. Star slide-in and glow pulse animations still play.
6. No `radar/` or `wanted/` folder remains under `6hud/assets/` in the game dir.
