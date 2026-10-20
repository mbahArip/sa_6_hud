# Plan 01: Weapon Ammo Display

## Objective
Implement weapon ammo counter in the top-right HUD stack below Wanted Stars and Criminal Profile.

## Specifications
- **Placement**: Top-right corner below active wanted stars or criminal profile badges (dynamic Y offset). 16px screen padding.
- **Font**: Pricedown font (`FONT_PRICEDOWN`), 16px size (`sH * (16.0f / 720.0f)`), 1px black outline.
- **Format**: `[loaded ammo]   [total ammo]` (e.g. `30   9999`). Single-shot/throwable weapons display total ammo only.
- **Visibility**: Hidden for weapons without ammo (unarmed, melee weapons, parachute).
- **Colors**:
  - `weapon.ammo.current` = `#FFFFFF` (loaded in clip).
  - `weapon.ammo.total` = `#B0B0B0` (total ammo, muted light gray).
  - `weapon.ammo.stroke` = `#000000` (outline).
- **Vanilla Suppression**: Patch `0x58EAF0` (`CHud::DrawPlayerInfo`) to disable vanilla ammo, money, and health rendering.

## Safety & Audit
- Check `CWeaponInfo::GetWeaponInfo` and `CPed::m_aWeapons` against plugin-sdk.
- Tested on GTA San Andreas 1.0 US.
