# Plan 07: Weapon HUD Finetune

## Objective
Add module controls, 20px font, total ammo vs reserve clip toggle, idle auto-hide, and named custom icon loading.

## Specifications
- **Config**:
  - `[weapon]` -> `USE_WEAPON_UI = 1` (boolean toggle for weapon ammo + icon)
  - `[weapon]` -> `WEAPON_UI_TOTAL_AMMO_TYPE = TOTAL_AMMO` (`TOTAL_AMMO` or `TOTAL_CLIP`)
- **Font Size**:
  - Scale Pricedown font to 20px (`sH * (20.0f / 720.0f)`).
- **Total Ammo vs Total Clip**:
  - `TOTAL_AMMO`: raw reserve count (`totalAmmo`).
  - `TOTAL_CLIP`: reserve clips `(totalAmmo - ammoInClip) / clipSize`. Single-shot / throwable weapons show total count.
- **Auto-Hide & Activity Tracking**:
  - Active triggers: weapon slot switch, weapon type change, aiming (`m_bIsAimingGun`), firing (`m_nState == WEAPONSTATE_FIRING`), ammo count delta, 'J' key peek hold.
  - Idle timeout: fades to 0.0 alpha over 0.25s after `UI_IDLE_TIMEOUT` seconds idle.
- **Custom Weapon Icons by Name**:
  - Name lookup: `assets/weapon/<name>.png` (e.g. `colt45.png`, `ak47.png`, `m4.png`).
  - Fallback: TXD `"ICON"` sprite -> `hud.txd` fist sprite.
