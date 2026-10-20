# 09 - Top-Right HUD Accordion Slide Transitions

## Objective
Implement smooth vertical accordion slide transitions for top-right HUD elements, matching the motion behavior of the bottom-left minimap stacked cards.

## Stack Hierarchy (Top to Bottom)
1. **Wanted Level**: 6-star gauge / hollow star status (`Hooked_DrawWanted`).
2. **Criminal Profile**: 4-category heat badge strip (`DrawHeatBadges`).
3. **Weapon Ammo**: Ammo in clip / reserve clip counter (only when weapon has ammo).
4. **Weapon Icon**: Active weapon sprite / custom PNG icon.
5. **Money Counter**: Rolling account balance counter.

## Motion & Transition Specifications
- **Transition Duration**: 250ms - 300ms smoothstep interpolation (`dt / 0.25f`).
- **Dynamic Stack Offset**:
  - Each active layer contributes `(layerHeight + layerGap) * animProgress` to `curY`.
  - Inactive layers collapse to 0 height smoothly, causing all lower modules to glide up.
  - When higher layers activate (e.g. stars acquired, badges flagged, weapon equipped), lower modules glide down smoothly.
  - Gun to melee/fist switch collapses the Weapon Ammo slot smoothly, sliding Weapon Icon and Money up.
  - Weapon idle timeout smoothly collapses both Ammo and Icon, sliding Money up to anchor beneath Wanted/Badges.

## File Modifications
- `src/RadarWanted.inc`:
  - Track `s_wantedStarProgress` and `s_heatBadgeProgress`.
  - Render badges at dynamic `badgeY` driven by `s_wantedStarProgress`.
- `src/RadarRenderTopRight.inc`:
  - Calculate `curY` starting from `topMargin + (wantedH * s_wantedStarProgress) + (badgeH * s_heatBadgeProgress)`.
  - Track `s_ammoSlideProgress` and `s_wepIconSlideProgress`.
  - Smoothly advance `curY` by `(ammoH * s_ammoSlideProgress)` and `(iconH * s_wepIconSlideProgress)`.
  - Render Money at final accumulated `curY`.
