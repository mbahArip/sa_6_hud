# 23 - Show HUD Key Accordion Slide Disable

## Objective
Prevent top-right HUD elements (weapon ammo, weapon icon, money counter) from sliding down from Y=0 when the Show HUD key ('J') is pressed.

## Problem Analysis
1. When the HUD was idle and hidden, `s_wepSlotProgress` and `g_ammoSlideProgress` were resting at 0.0f.
2. Pressing the Show HUD key ('J') activates `g_hudShowAll`, simultaneously starting the fade-in and the accordion height expansion (0.0f -> 1.0f over 250ms).
3. This pushed the money counter and weapon elements downward while they were fading in, producing an unwanted downward sliding motion.

## Changes
1. **Pre-expand Slot Heights While Invisible**:
   - When `g_hudShowAll` is triggered or active while elements are hidden (`s_weaponFadeAlpha <= 0.05f`), immediately set `s_wepSlotProgress = 1.0f` and `g_ammoSlideProgress = (hasAmmo ? 1.0f : 0.0f)`.
   - Elements are at 0% opacity during this placement, making the instant position snap completely invisible.
2. **Fade-In In Place**:
   - As opacity ramps from 0% -> 100%, vertical slot offsets are already fully expanded at their final resting positions, eliminating any downward slide.
3. **Fade-Out Before Collapse**:
   - When `g_hudShowAll` ends, elements fade out to 0% opacity before slot progress collapses, preventing upward sliding during dismissal.
