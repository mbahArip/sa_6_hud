# 15 - Ammo Fade Transition on Weapon Switch

## Objective
Fix ammo text fading when switching from an armed weapon to a weapon without ammo (e.g. Pistol to Knife/Fists). Both loaded clip ammo and reserve ammo/total clips must fade out smoothly together instead of clip text vanishing instantly while reserve text lingers or reformats.

## Root Cause Analysis
1. When switching from Pistol to Knife, the player's weapon slot immediately points to Knife.
2. Knife has `clipSize = 0` and `totalAmmo = 1` (GTA SA melee default).
3. The formatting code evaluated `clipSize <= 1` as true for Knife, immediately formatting Knife's totalAmmo ("1") into `curBuf` while clearing `totBuf` to `""`.
4. As a result, the left number instantly vanished and the right number was replaced with "1" which then faded out over 0.25s.
5. Fix: Maintain persistent display buffers (`s_lastCurAmmoStr` and `s_lastTotAmmoStr`). Update them only when `hasAmmo` is `true`. When `hasAmmo` becomes `false`, retain the last valid weapon's ammo strings during the 0.25s collapse/fade transition so both numbers fade out together cleanly.
