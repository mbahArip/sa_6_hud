# 12 - Top-Right HUD Motion & Empty Ammo Slot Fix

## Objective
Fix top-right HUD animations so pressing Show HUD fades elements in place rather than sliding/flying Money from top to bottom. Ensure weapons with 0 ammo or non-ammo slots (unarmed, melee, gifts, goggles, detonators) completely collapse the ammo slot without reserving space.

## Root Cause Analysis
1. **Money Flying on Show HUD**:
   - `s_weaponFadeAlpha` and `g_ammoSlideProgress` were used for both visibility opacity AND vertical accordion slot expansion.
   - When idle, slot heights collapsed to 0; when Show HUD was pressed, slots expanded 0->1, driving Money downwards 78px while fading.
   - Fix: Decouple layout structural height from fade alpha. If weapon UI is active, the icon slot is structural (`1.0`). If weapon has ammo, `g_ammoSlideProgress` is structural (`1.0`). Elements fade in-place at their resting slots without vertical motion on Show HUD. Slide transitions only trigger on dynamic state mutations (wanted stars change, weapon switch gun <-> melee, ammo depletion).
2. **Ammo Slot Taking Space with 0 Ammo**:
   - Condition `else if (totalAmmo <= 0 && clipSize <= 0) hasAmmo = false;` failed because firearms always have `clipSize > 0`.
   - Also thermal/nightvision goggles and detonators were not excluded.
   - Fix: Strict slot check: `bool hasAmmo = (wepSlot >= 2 && wepSlot <= 9 && totalAmmo > 0);`. Non-ammo weapons and 0-ammo firearms strictly return `false`, immediately collapsing the ammo slot.
