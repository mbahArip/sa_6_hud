/*
    GTA SA Plugin SDK — Weapon Management
    Demonstrates: CPed::GiveWeapon, SetCurrentWeapon, CStreaming model load
    Source: examples/PlayerWeapon/Main.cpp
*/
#include <plugin.h>
#include "CStreaming.h"
#include "CTimer.h"

using namespace plugin;

class WeaponManager {
public:
    WeaponManager() {
        static int lastKeyTime = 0;

        Events::gameProcessEvent += [] {
            CPed* player = FindPlayerPed();
            if (!player) return;
            if (!KeyPressed(VK_TAB)) return;
            if (CTimer::m_snTimeInMilliseconds - lastKeyTime < 500) return;

            lastKeyTime = CTimer::m_snTimeInMilliseconds;

            // Must load weapon model before giving
            CStreaming::RequestModel(MODEL_M4, 2);
            CStreaming::LoadAllRequestedModels(false);

            player->GiveWeapon(WEAPONTYPE_M4, 300, true);   // type, ammo, forceAdd
            player->SetCurrentWeapon(WEAPONTYPE_M4);

            // Allow engine to unload weapon model when not needed
            CStreaming::SetModelIsDeletable(MODEL_M4);
        };
    }
} weaponManager;

/*
  WEAPON SLOTS (m_aWeapons[13], offset 0x5A0 on CPed):
  Slot 0: Fist / Brass knuckles
  Slot 1: Melee (bat, knife, etc.)
  Slot 2: Handgun (pistol, silenced, deagle)
  Slot 3: Shotgun
  Slot 4: SMG (micro-uzi, mp5, tec9)
  Slot 5: Assault rifle (AK47, M4)
  Slot 6: Rifle (country rifle, sniper)
  Slot 7: Heavy (rocket, minigun, flamethrower)
  Slot 8: Thrown (grenade, molotov, satchel)
  Slot 9: Special (spraycan, fire extinguisher, camera)
  Slot 10: Gift
  Slot 11: Parachute
  Slot 12: Detonator

  Selected slot: player->m_nSelectedWepSlot (offset 0x718)
  Current weapon: player->m_aWeapons[player->m_nSelectedWepSlot]
*/
