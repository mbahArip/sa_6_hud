# 37 - Save-Load Texture Loss, Interior Blank, Intro Blink

## Objective

Three bugs caused by the plugin replacing the vanilla radar by *suppression*
rather than by patching its draw code:

1. Loading a saved game left icons unloaded, while a new game worked.
2. The radar did not blink when it first appeared at game start.
3. Interiors showed map tiles instead of the vanilla blank disc.

---

## 1. Icons Lost on Save Load

### Cause

`d3dLostEvent` / `d3dResetEvent` are not sent for a save load. The game
recreates the D3D device on its own path, so:

- `Radar::Restart()` ran (via `restartGameEvent`), which calls `Shutdown()` →
  `DestroyShaders()`, releasing every icon, shader and cached map tile.
- Nothing rebuilt them. `CreateShaders()` was only reachable from `Init()` (boot
  only) and `OnD3dReset()` (never fired on this path).

New game happened to work because the frontend flow triggers a real device reset
on the way in, which does fire `d3dResetEvent`.

### Fix

Two layers, because a device swap can leave *non-null but dead* pointers that
null-guards cannot detect.

**a. Device generation guard.** `RadarState.inc` gained:

```cpp
static IDirect3DDevice9* g_textureDevice = nullptr;
```

`CreateShaders()` compares `RwD3D9GetCurrentD3DDevice()` against it. On mismatch
it calls `ForgetShaders()` — which nulls every cached pointer **without
releasing**, because the old resources died with the old device and `Release()`
would touch freed memory — then rebuilds everything against the new device.

**b. Reload on restart.** `Radar::Restart()` now calls `CreateShaders()` right
after `Shutdown()`, so the textures exist before the first gameplay frame rather
than being lazily discovered as missing.

**c. Per-frame safety net.** `Radar::Render()` calls `CreateShaders()` after its
early-outs. Every loader inside early-outs on a non-null pointer, so this is free
when warm and repairs anything the other two layers missed.

---

## 2. Intro Blink

`CRadar::DrawRadarMap` is patched to `ret`, so the vanilla blink never happens.
The plugin previously gated on `g_warmupFrames < 10` and then popped to full
opacity.

`g_radarBlinkTimer` is armed to 1.0s in `Radar::Restart()`. While it runs, the
radar alpha is forced to 0 on alternate 250 ms halves:

```cpp
const float kBlinkTotal = 1.0f;
const float kBlinkPeriod = 0.25f;
float elapsed = kBlinkTotal - g_radarBlinkTimer;
bool on = fmodf(elapsed, kBlinkPeriod * 2.0f) < kBlinkPeriod;
if (!on) g_radarAlphaFade = 0.0f;
```

Two on/off cycles over one second, matching the vanilla flash on spawn.

---

## 3. Interior Blank

Vanilla `CRadar::DrawRadarMap` draws an empty dark disc inside interiors. Our
renderer never consulted `CGame::currArea`, so it kept painting tiles.

- `Radar::DrawMap2D` returns immediately when `CGame::currArea > 0`. The backdrop
  disc is still drawn by `DrawBackdrop`, so the result is the vanilla blank disc.
- `DrawOverlay` skips blips, police blips and the north badge in interiors, but
  keeps the player puck, pin and GPS distance so navigation still works.

`CGame::currArea` was already used elsewhere for zoom and HUD-visibility
decisions, so this reuses an established signal rather than introducing one.

---

## Verification

1. Load a saved game: icons, wanted stars, gauges and map tiles all appear.
2. Start a new game: radar blinks on spawn, then settles to full opacity.
3. Walk into a house / shop: radar shows a blank disc, no tiles, no blips.
4. Walk back outside: tiles and blips return.
5. Alt-tab and change resolution: textures still render.
6. Player puck and GPS distance remain visible inside interiors.
