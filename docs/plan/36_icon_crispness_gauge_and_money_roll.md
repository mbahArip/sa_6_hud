# 36 - Icon Crispness, Gauge Smoothness, Money Roll Cap

## Objective

Three polish fixes on the top-right HUD and the vehicle telemetry gauges:

1. Icons looked soft.
2. The money ring and the vehicle gauges still looked faceted after the
   previous arc-segment change.
3. The money counter could roll for tens of seconds, and a new game inherited
   the previous session's balance.

---

## 1. Icon Crispness

### Codec

All icons are now **DXT3**, 256x256, power-of-two:

`wallet`, `engine`, `fuel`, `nitro`, `ifruit`, `wantedStar`,
`wantedStarBorder`, `profile/{identity,outfit,vehicle,weapon}`,
and `ui/cops_arrow` (512x256).

DXT3 was chosen over DXT5 deliberately. Both cost 16 bytes per 4x4 block, but
DXT3 stores an explicit 4-bit alpha per texel while DXT5 stores interpolated
alpha endpoints. For a hard-edged glyph on a flat HUD background, DXT3's hard
cutout reads crisper; DXT5's interpolated alpha smears the silhouette edge into
the transparent neighbour. DXT5 only wins for smooth gradients, which none of
these icons are.

### Filter

`DrawD3DTexturedQuad` previously used trilinear (MIN/MAG/MIP all `LINEAR`).
Icons are 256x256 but draw at roughly 20-60 px, so the sampler picked mip level
2-3 and the glyph was read from a 64 or 32 px copy — that, not the codec, was
the visible blur.

Mipmaps are kept (they still help the glow texture and any large draw), but the
mip filter is now off for this path:

```cpp
dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
```

Top mip only. For small hard-edged HUD glyphs the aliasing this introduces is
much less noticeable than the blur it removes.

---

## 2. Gauge Smoothness (option A)

The earlier fix scaled `DrawArcStrip` segments by swept angle, but the gauges
were still faceted because their surrounding primitives had their own low
segment counts:

| primitive | before | after |
|---|---|---|
| `DrawCircle` default | 24 (cap 48) | 48 (cap 96) |
| `DrawRing` default | 32 (cap 64) | 64 (cap 96) |
| money gauge bg circle | 24 | 48 |
| money gauge track + value arc | 24 | 48 |
| vehicle gauge bg circle | 20 | 48 |
| vehicle gauge track + arc | 20 / 24 | 48 |
| vehicle gauge end caps | 12 | 32 |

Vertex buffer sizes were checked against the new ceilings:

- `DrawCircle`: 128 verts, writes at most `96 + 2 = 98`.
- `DrawRing`: 196 verts, writes at most `(96 + 1) * 2 = 194`.
- `DrawArcStrip`: `(96 + 1) * 2 = 194`.

All within bounds.

---

## 3. Money Roll

### Before

```cpp
float speed = std::clamp(std::abs(diff) * 8.0f, 60.0f, 5000000.0f);
float step = speed * dt;
```

Speed was proportional to the gap with a 5M/s ceiling, so a large balance change
took tens of seconds to settle. Worse, `s_renderMoney` was never reset between
sessions: loading a rich save and then starting a new game made the counter
count *down* from the old maximum to the starting cash.

### After

A fixed-duration eased tween replaces the speed clamp:

```cpp
static const float kMoneyAnimDuration = 3.0f;
static float s_moneyAnimFrom, s_moneyAnimTo, s_moneyAnimT;

if (s_moneyAnimT < 1.0f) {
    s_moneyAnimT = std::min(1.0f, s_moneyAnimT + dt / kMoneyAnimDuration);
    float inv = 1.0f - s_moneyAnimT;
    float ease = 1.0f - inv * inv * inv;   // ease-out cubic
    s_renderMoney = s_moneyAnimFrom + (s_moneyAnimTo - s_moneyAnimFrom) * ease;
    if (s_moneyAnimT >= 1.0f) s_renderMoney = s_moneyAnimTo;
}
```

Any balance change now completes in exactly 3 seconds regardless of size.

### Session reset

`ResetMoneyAnimation()` clears the readout back to 0 and is called from
`Radar::Restart()`, which the `restartGameEvent` hook fires on both new game and
save load. The counter therefore always rolls **up from 0** to the session's
starting balance, never down from a previous session's figure.

The first frame after a restart seeds `s_moneyAnimFrom = 0` and
`s_moneyAnimTo = curMoney`, so the 3 s roll starts immediately rather than
snapping.

`ResetMoneyAnimation` lives in `RadarRenderTopRight.inc`, which is included after
`RadarLifecycle.inc`. A forward declaration in `RadarState.inc` (included first)
lets `Radar::Restart` call it without reordering the include list.

---

## Verification

1. Icons are visibly sharper than before; no soft halo on the glyph edges.
2. Money ring, vehicle gauges and their end caps are smooth circles, no facets.
3. Changing cash rolls the counter and settles within 3 seconds.
4. Load a save with a high balance, then start a new game: the counter starts at
   0 and rolls up to the starting cash, it does not count down.
5. Map tiles, blips, radar bezel and the pause map are unchanged.
