# 34 - Remove Cop AI (Police Behavior Director)

## Objective

Delete the whole `[cop_ai]` subsystem. It never delivered better police behavior,
and its supplemental dispatch caused a world-population bug: after finishing a
mission (reported on "Burning Desire"), ambient NPCs stopped spawning on the
streets.

## Root Cause of the NPC Bug

`CreatePoliceGroundVehicle` in the deleted `src/RadarCopAi.inc` called:

```cpp
CTheScripts::ClearSpaceForMissionEntity(vehicle->GetPosition(), vehicle);
```

That routine deletes nearby peds and vehicles so a mission script can place its
own entity. Calling it for a plugin-spawned supplemental cop car wiped the
ambient population around the player on every dispatch. Wiped peds do not
repopulate immediately, so the streets stayed empty after the mission ended.

## Removed

### File

`src/RadarCopAi.inc` deleted in full, and its `#include` dropped from
`src/Radar.cpp`.

Deleted functions:

- `IsValidPoliceUnit`
- `CountActivePoliceVehicles`
- `IsDispatchPositionAllowed`
- `IsDispatchPositionAroundCorner`
- `FindDispatchPosition`
- `CreatePoliceGroundVehicle` (the `ClearSpaceForMissionEntity` caller)
- `RegisterPoliceVehicleForPursuit`
- `SpawnEmergencyVehicleAt`
- `ReleasePluginPoliceUnits`
- `UpdateNativeWantedComposition`
- `TryDispatchSupplementalPolice`
- `UpdateCopAi`

### Kept and Relocated

Three pure query helpers had a surviving consumer in `RadarWantedSearch.inc`
(the wanted-search line-of-sight scan), so they moved there instead of being
deleted:

- `IsPoliceVehicleModel`
- `HasValidPoliceDriver`
- `IsPoliceHelicopter`

### Globals (`src/RadarState.inc`)

- `g_dispatchTimer`
- `g_lastCopCompositionWanted`
- `g_pluginPoliceUnitRefs`

`g_copHasVisual`, `g_visualLossTimer` and the search/heat stage state stay —
those belong to the wanted-search feature, not to cop AI.

### Config (`src/Config.h`, `src/Config.cpp`)

The whole `[cop_ai]` block: 16 struct fields, their 20 `readInt`/`readFloat`
reads and clamps, and the `WritePrivateProfileStringA("cop_ai", ...)` cleanup
in the save path.

No `6hud.ini` edit was needed — the shipped INI never contained a `[cop_ai]`
section, so every option was silently running on defaults.

## Behavior Reverted in `RadarWantedSearch.inc`

With the director gone, these tweaks reverted to vanilla detection:

| Removed | Effect |
|---|---|
| `copAiMultiLevelAwarenessEnabled` + `copAiMaxDirectVerticalDifference` checks (ped scan, vehicle scan) | Cops on a different floor/level no longer count as "not seeing you"; vertical distance is ignored again |
| `copAiVisualLossGracePeriod` block | Losing line of sight drops you out of pursuit immediately, no grace window |
| `UpdateCopAi(player, wanted, dt)` call | No more plugin dispatch |
| `copAiHiddenSearchGracePeriod` term in `isHiding` | Hiding only counts while ducking, no time-based grace |
| `copAiEscapeDifficultyScale` on `lkpTargetEscapeTime` | Escape timer uses the configured duration directly |

## Verification

1. After completing "Burning Desire" (or any mission), ambient peds and traffic
   keep spawning on the streets.
2. Wanted level still works: stars, search area, heat area, LKP blink and
   escape timers all behave as before.
3. No plugin-spawned police vehicles appear; only the game's own dispatch.
4. `6hud.log` no longer prints `cop_ai:` lines.
5. INI save no longer strips a `[cop_ai]` section (it no longer exists).

## Related

Icon rendering issue is unrelated and asset-side: seven icons are still
240x240 (non power-of-two), which GTA SA's D3D9 cannot sample as DXT-compressed
textures. They need re-export to 256x256:

- `assets/icons/engine.dds`
- `assets/icons/fuel.dds`
- `assets/icons/nitro.dds`
- `assets/icons/profile/identity.dds`
- `assets/icons/profile/outfit.dds`
- `assets/icons/profile/vehicle.dds`
- `assets/icons/profile/weapon.dds`
