# 35 - Fix Raw D3D9 Quad State and Jagged Gauge Arcs

## Objective

Two rendering bugs:

1. Textured icons drawn through raw D3D9 (`wallet`, `engine`, `fuel`, `nitro`,
   `profile/*`, `wantedStarBorder`, `cops_arrow`) loaded fine but did not appear.
2. The money gauge ring and the vehicle-info gauges looked jagged.

---

## 1. Icons Missing — Vertex Shader Left Bound by RenderWare

### Why the assets were innocent

Every suspicious texture was checked and cleared:

- All DDS files are power-of-two (256x256, 512x256, 512x512).
- `6hud.log` shows every one of them loading with a valid pointer.
- A DXT5/DXT3 block decoder confirmed real pixel content with full alpha
  (`wallet.dds`: 32681 opaque texels, max alpha 255).

So the files, the codec and the loader were never the problem.

### Actual cause

The plugin mixes two rendering paths in the same frame:

- `RwIm2DRenderPrimitive` (RenderWare) — circles, rings, rounded fills, arcs.
- `IDirect3DDevice9::DrawPrimitiveUP` (raw D3D9) — every `DrawD3DTexturedQuad`.

RenderWare's D3D9 backend has its own vertex-shader abstraction
(`_rwD3D9SetVertexShader`, declared in `plugin_sa/game_sa/rw/rwcore.h`), and its
2D path binds a vertex shader. In D3D9 a bound vertex shader **takes precedence
over `SetFVF`** — `SetFVF` only affects the fixed-function path. So a raw quad
submitted after `RwIm2D` was still being fed through RenderWare's 2D shader with
an input layout it does not match, and the geometry was discarded.

This explains the exact failure set: the icons that vanished were all drawn
immediately after `DrawCircle` / `DrawArcStrip` calls in the same function —
the money gauge icon right after the gauge ring, and the vehicle gauges' icons
right after their arcs.

### Fix

New `BeginRawD3DQuad(IDirect3DDevice9*)` in `src/RadarState.inc`, called at the
top of every raw-D3D draw:

```cpp
dev->SetVertexShader(nullptr);
dev->SetVertexDeclaration(nullptr);
dev->SetFVF(D3DFVF_2DVERT);
dev->SetPixelShader(nullptr);

for (DWORD stage = 1; stage < 4; ++stage) {
    dev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_DISABLE);
    dev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
dev->SetRenderState(D3DRS_ZENABLE, FALSE);
dev->SetRenderState(D3DRS_LIGHTING, FALSE);
dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
```

Applied to every raw-D3D entry point, replacing the duplicated per-function
state blocks:

| file | function |
|---|---|
| `src/RadarState.inc` | `DrawD3DTexturedQuad` |
| `src/RadarState.inc` | `DrawD3DTexturedQuadAdditive` |
| `src/RadarState.inc` | `DrawD3DRotatedQuad` |
| `src/RadarRenderMap.inc` | `BlitMapTileTexture` |
| `src/RadarRenderPrimitives.inc` | `DrawD3DTexturedRoundedFill` |
| `src/RadarRenderMenuMap.inc` | pause-map sea fill |

Stages 1..3 are disabled because RenderWare can leave them enabled, which would
blend extra texture stages into our single-texture quads.

### Diagnostic

A three-shot log was added to the money-gauge wallet draw so one run confirms
the state at draw time:

```
wallet: draw tex=<ptr> rect=(x,y,w,h) alpha=<a> vs=<vertexshader> decl=<decl>
```

`vs=` must be `00000000` after the fix. If it is non-null, the reset is not
reaching that draw site.

---

## 2. Jagged Gauges — Fixed Segment Count

`DrawArcStrip` used a caller-supplied segment count capped at 36. A full ring at
20-24 segments is a visible polygon, and a short value arc got the same 24
segments spread over a few degrees, so the curve read as straight facets.

Segments are now derived from the swept angle:

```cpp
const int kMaxSegments = 96;
float span = fabsf(endAngleRad - startAngleRad);
int autoSegments = (int)ceilf(span / (2.0f * kPi) * (float)kMaxSegments);
if (autoSegments > segments) segments = autoSegments;
```

A full ring now gets 96 segments; a quarter arc gets 24; a 5-degree sliver still
gets 2 and is clamped up to the 3-segment minimum. The caller's value acts as a
floor, so existing call sites keep working unchanged.

Affected gauges: money (`RadarRenderTopRight.inc`) and the engine / fuel / nitro
telemetry gauges (`RadarRenderCards.inc`), plus every other arc consumer
(police threat arcs, player puck stroke, GPS caps).

---

## Verification

1. Wallet icon visible inside the money gauge.
2. Engine / fuel / nitro icons visible inside the vehicle-info gauges.
3. Wanted stars and the criminal-profile card icons render.
4. Money ring and vehicle gauges are smooth, no visible facets.
5. Map tiles, blips and the pause map are unchanged.
6. `6hud.log` shows `vs=00000000` in the `wallet: draw` line.

## Note

`cops_arrow.dds` is loaded but its consumers are gated by
`[wanted] ENABLE_COP_ARCS` / `ENABLE_COP_AURAS`, both off in the shipped
`6hud.ini` (`arcs=0, auras=0` in the log). It will not draw until those are
enabled — that is expected, not a bug.
