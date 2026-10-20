# 21 - Top Right Module Accordion Collapse

## Objective
Eliminate phantom vertical gaps between wanted stars and the money counter when weapon UI is inactive or hidden.

## Problem Analysis
1. `s_wepSlotProgress` in `src/RadarRenderTopRight.inc` was pinned to 1.0f whenever `g_config.useWeaponUi` was true in `6hud.ini`, rather than tracking active weapon UI visibility (`showWep`).
2. `g_ammoSlideProgress` was pinned to 1.0f whenever the player had an armed weapon (`hasAmmo`), regardless of whether the weapon UI was idle and faded out.
3. This reserved ~78px of empty vertical space for invisible ammo and weapon icon slots, pushing the money counter far down and leaving a conspicuous gap beneath wanted stars.

## Changes
1. Bind `s_wepSlotProgress` expansion/collapse to `showWep` (eases to 0.0 when weapon UI idles out).
2. Bind `g_ammoSlideProgress` expansion/collapse to `(showWep && hasAmmo)` (eases to 0.0 when weapon UI idles out or weapon has no ammo).
3. Result: when weapon UI is not visible, ammo and icon vertical heights collapse to 0px, docking money cleanly under wanted stars or the top margin.
