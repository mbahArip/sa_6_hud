# Plan 04: Temporary Top-Left Health and Armor Bar

## Objective
Relocate player health and armor bars to the top-left corner of the screen while top-right HUD elements are being reconstructed.

## Specifications
- **Placement**: Top-left corner (`x = 16px`, `y = 16px` scaled).
- **Health Bar**:
  - Horizontal bar showing `player->m_fHealth / player->m_fMaxHealth`.
  - Color: Red with dark background track.
- **Armor Bar**:
  - Horizontal bar showing `player->m_fArmour / 100.0f`.
  - Color: White / light blue with dark background track.
  - Rendered only if armor > 0.
- **Dimensions**: Clean 120px x 8px bar per component.

## Safety & Audit
- Check `CPed::m_fHealth`, `CPed::m_fArmour`, and `CPed::m_fMaxHealth` in `plugin-sdk-sa`.
