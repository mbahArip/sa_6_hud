# 29 - Layout Preview Rendering Fixes

## Objective
The in-game **Layout** tab (`Menu.cpp`, tab index 7) is supposed to show a live mock
of every HUD element so positions can be tuned without leaving the menu. It was
broken in four separate ways. Fix all of them.

---

## 1. Bug: preview never rendered at all

### Cause
`Radar::Render()` guarded on `GameplayRunning()`, which returns `false` whenever
`FrontEndMenuManager.m_bMenuActive` is set. Opening the in-game menu sets that flag,
so `DrawOverlay()` was never reached while the menu was open — the preview was
entirely blank.

### Fix
`src/RadarRenderLifecycle.inc`

```cpp
if ((!GameplayRunning() && !Menu::IsLayoutPreview()) || g_warmupFrames < 10) return;
```

Preview bypasses the gameplay gate.

---

## 2. Bug: wanted stars, heat badges and money gauge missing from preview

### Cause
All three draw from the game's own HUD pass through the `0x58D9A0` hook
(`Hooked_DrawWanted`). The open menu suppresses that pass, so nothing drew.

### Fix
`src/RadarRenderOverlay.inc`

```cpp
// Wanted gauge normally renders from the game's HUD pass (hook on 0x58D9A0), which the
// open menu suppresses — draw it here so the Layout tab preview is complete.
if (isPreview) Hooked_DrawWanted();
```

The star PNGs themselves were never missing; `6hud.log` confirms all four
(`starBase`, `starHeat`, `starLooking`, `starNoInfo`) load from
`6hud\assets\wanted\`.

---

## 3. Bug: info cards overlapped in preview

### Cause
`DrawInfoCards` chose between two position sources per card:

```cpp
CRect navRect = freeNav ? g_moduleRects[GpsNav] : CRect(radarX, navY, ...);
```

The Group-mode branch hardcoded radar-relative positions instead of using the values
`SolveLayout` had already produced. In preview those hardcoded positions collapsed
onto each other.

### Fix
All three cards read from the solver. Preview reads `g_moduleRects`, gameplay reads
`g_animatedModuleRects` (see plan 27). Dead variables (`radarW`, `radarX`, `radarY`,
`navY`, `locBaseY`, `locY`, `pad`, `gap`, `sW`, `activeCfg`) removed.

---

## 4. Bug: top-right HUD overlapped after idle timeout

### Cause
`DrawTopRightHUD` computed `ammoY` / `iconY` / `moneyY` from accordion math driven by
live idle timers. After the idle timeout those timers decayed to zero, so all three
resolved to `topMargin` and drew on top of each other.

### Fix
The hand-rolled accordion Y math is gone. Positions come from the group solver, and
the accordion state now only feeds `g_moduleAlpha[]` so the stack slides smoothly
during fade in/out.

---

## 5. Bug: Radar collapsible labelled "Radar / Minimap"

### Cause
`GetModuleDisplayName(HudModuleId::Radar)` returned `"Radar / Minimap"`.

### Fix
`src/Layout.h` → returns `"Radar"`.

---

## 6. Custom map tiles defaulted on with no tiles shipped

### Cause
`USE_CUSTOM_MAP_TILE` defaulted to `1`, but `assets/radar/map/` did not exist. Every
tile load failed (576 attempts per session) and the map fell back to vanilla TXD.
Harmless but noisy.

### Status
The tile assets are now committed (288 PNGs across `day/` and `night/`), so the
default is back to `1`. Missing individual tiles still fall through to the vanilla
TXD path.

---

## 7. Verification

1. Open Layout tab → every HUD element visible at its configured position.
2. No overlap anywhere, including after sitting in the menu past the idle timeout.
3. Radar collapsible reads "Radar".
4. Wanted stars, heat badges and money gauge all render in preview.
5. Drag a module to Free mode → it detaches and stays where dropped.
6. Back to Group mode → it rejoins the stack and slides with its neighbours.
