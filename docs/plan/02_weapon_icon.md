# Plan 02: Weapon Icon Display

## Objective
Display current active weapon icon directly beneath the weapon ammo counter in the top-right HUD stack.

## Specifications
- **Placement**: Top-right corner directly below weapon ammo counter.
- **Rendering**:
  - Direct call to native `CHud::DrawWeaponIcon(CPed* ped, int x, int y, float alpha)` (address `0x58D7D0`), or sprite drawing from model TXD.
  - Sized and aligned with top-right margins.
- **Alpha**: Follows overall HUD fade/visibility state.

## Safety & Audit
- Check `CHud::DrawWeaponIcon` calling convention and parameter stack in `C:\Users\me\dev\sa_project\plugin-sdk-sa`.
