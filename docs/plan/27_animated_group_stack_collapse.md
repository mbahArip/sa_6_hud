# 27 - Animated Group Stack Collapse

## Objective
Group-mode modules must behave as a real stack: when a module in the middle of a
group is hidden, the modules around it slide to close the gap instead of leaving a
hole. Previously `SolveLayout` reserved a slot for every module unconditionally, so
hiding GPS Navigation and Place Information still left Vehicle Information floating
high above the radar with two empty slots between them.

Applies to **all four groups**, not only Minimap.

---

## 1. Problem

`SolveLayout` is a pure, static solver. It has no knowledge of which modules are
currently visible, so every group slot reserves its full `height + gap`:

```
[Vehicle Information]   <- index 3
[Place Information]     <- index 2  (hidden, still reserving space)
[GPS Navigation]        <- index 1  (hidden, still reserving space)
[Radar]                 <- index 0
```

The gap stays even though nothing is drawn in it.

The top-right HUD previously worked around this with a hand-rolled accordion
(`curY += slotH * ease`), but that logic existed only for the Criminal group and
duplicated layout knowledge that belongs in one place.

---

## 2. Solution

Add a second solver, `SolveAnimatedLayout`, in `Layout.h`. Same group anchoring and
dimension rules as `SolveLayout`, but each slot only reserves its share of the stack
proportional to its current visibility alpha:

```
curBottom -= (item.height + gap) * item.alpha
```

- `alpha == 1.0` → full slot reserved (module visible)
- `alpha == 0.0` → slot collapses entirely, neighbours slide in
- Fractional alpha → smooth slide during fade in/out

Free-mode modules are unaffected: absolute anchors do not move with alpha.

### Signature

```cpp
inline LayoutResult SolveAnimatedLayout(
    const ModulePositionConfig configs[(int)HudModuleId::Count],
    const float alphas[(int)HudModuleId::Count],
    float sW,
    float sH,
    float radarScale = 1.0f
);
```

---

## 3. Per-Module Alpha Source

Renderers publish their live visibility into `g_moduleAlpha[]` each frame before the
solver consumes it.

| Module | Alpha source |
|---|---|
| `Radar` | `1.0f` (always present while radar is enabled) |
| `GpsNav` | `s_navAlpha` |
| `PlaceInfo` | `s_locAlpha` |
| `VehicleInfo` | `s_vehCardAlpha` |
| `HealthArmor` | `1.0f` |
| `Wanted` | `max(wantedEase, badgeEase)` |
| `WeaponHud` | `max(ammoEase, iconEase)` |
| `MoneyHud` | `s_moneyFadeAlpha` |

`g_moduleAlpha[]` is initialised to `1.0f` in `Radar::Init()` so the very first frame
does not collapse every group to a single point.

---

## 4. State

Added to `src/RadarState.inc`:

```cpp
static float g_moduleAlpha[(int)HudModuleId::Count];
static CRect g_animatedModuleRects[(int)HudModuleId::Count];
```

`UpdateRadarRect()` now solves both layouts every frame:

```cpp
LayoutResult layout = SolveLayout(activeCfg.modulePositions, sW, sH, activeCfg.radarSizeScale);
LayoutResult anim   = SolveAnimatedLayout(activeCfg.modulePositions, g_moduleAlpha, sW, sH, activeCfg.radarSizeScale);
```

- `g_moduleRects[]` → static resting layout (preview, Free-mode anchors)
- `g_animatedModuleRects[]` → alpha-collapsed stack (gameplay)

---

## 5. Renderer Changes

| File | Change |
|---|---|
| `RadarRenderCards.inc` | Publishes `GpsNav`/`PlaceInfo`/`VehicleInfo` alphas. Reads `g_animatedModuleRects` in gameplay, `g_moduleRects` in preview. |
| `RadarRenderTopRight.inc` | Publishes `Wanted`/`WeaponHud`/`MoneyHud` alphas. Hand-rolled accordion Y math deleted (33 lines); positions come from the solver. |
| `RadarWanted.inc` | All three `wantedRect` lookups use `g_animatedModuleRects`. |
| `RadarRenderTopRight.inc` | `DrawTopLeftHealthBar` uses `g_animatedModuleRects`. |

Preview (`Menu::IsLayoutPreview()`) always reads `g_moduleRects`: every alpha is
effectively 1 there, so the resting layout is the correct thing to show.

---

## 6. Verification

1. Group mode, Minimap: hide GPS Navigation and Place Information → Vehicle
   Information sits directly above the radar with no gap.
2. Show them again → Vehicle Information slides back up as they fade in.
3. Same check for Criminal group: hide Wanted → Weapon and Money slide up.
4. Free mode: modules stay pinned to their anchors regardless of visibility.
5. Layout tab preview: all modules at resting positions, no overlap.
