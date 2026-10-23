# 38 - Keybinds, Config Regroup, Lazy Hooks, i18n

One release, four workstreams. Phases ordered by dependency. Each phase ends with
a build + in-game check.

---

## Phase 1 - Dual keybinds (keyboard + gamepad)

Current state: `showHudKey='J'`, `peekKey='G'`, `infoHotkey='J'` are single chars
read via `GetAsyncKeyState`. Keyboard-only, so a controller player must reach for
the keyboard.

### Format

One action, two fields: a keyboard key and an optional pad button.

```ini
[KEYS]
SHOW_HUD_KEY=J
SHOW_HUD_PAD=LB
MAP_PEEK_KEY=G
MAP_PEEK_PAD=RB
INFO_KEY=H
INFO_KEY_PAD=Y
```

- Key accepts a single char (`J`), a `VK_` name (`VK_F5`), or hex (`0x74`).
- Pad accepts `A/B/X/Y/LB/RB/LT/RT/START/BACK/L3/R3/DUP/DDOWN/DLEFT/DRIGHT`.
- An empty or `NONE` pad value keeps the action keyboard-only, so existing
  configs behave exactly as before.

### Wiring

- `RadarGps.inc` peek: OR the pad button into `s_peekActive`.
- `RadarGps.inc` info hotkey: same for the info-card refresh.
- `RadarProcess.inc` show-HUD: same, alongside the existing
  `CPad::NewKeyState.standardKeys[]` check.
- The plate-customise `'P'` key in `RadarHeatTracking.inc` is gameplay, not a
  user bind. Left alone.

---

## Phase 2 - Config regroup + auto-migration

### New layout

Master toggles grouped by UI surface. Behavioural knobs stay under their own
group. Extension modules each get a presence toggle.

```ini
[UI]
USE_RADAR=1
USE_GPS_NAV=1
USE_PLACE_INFO=1
USE_VEH_INFO=1
USE_WANTED_LEVEL=1
USE_WEAPON=1
USE_MONEY=1

[EXT]
USE_FUEL=0
USE_NITRO_BOOST=0
USE_HEAT_AREA=1
USE_SEARCH_AREA=1
USE_CRIMINAL_PROFILE=0
USE_SNITCH=0
USE_SIREN_INDICATOR=0
USE_ARC_INDICATOR=0
```

Mapping from the legacy keys:

| new | legacy fallback |
|---|---|
| `[UI] USE_RADAR` | `Minimap.Radar/USE_RADAR`, `radar/enabled` |
| `[UI] USE_GPS_NAV` | `Minimap.GPSNavigation/USE_RADAR_GPS`, `gps/enabled` |
| `[UI] USE_PLACE_INFO` | `Minimap.PlaceInformation/USE_PLACE_INFO` |
| `[UI] USE_VEH_INFO` | `Minimap.VehicleInformation/USE_VEHICLE_INFO` |
| `[UI] USE_WANTED_LEVEL` | `Wanted.Visual/USE_WANTED_STAR` |
| `[UI] USE_WEAPON` | `weapon/USE_WEAPON_UI` |
| `[UI] USE_MONEY` | `money/USE_MONEY_UI` |
| `[EXT] *` | the 8 `use*` / `enable*` extension flags |

### Migration

`LoadConfig` checks whether `6hud.ini` already has a `[UI]` section.

- Has `[UI]`: normal load.
- No `[UI]` but `6hud.old.ini` exists next to it: read legacy values from the old
  file, write the new layout to `6hud.ini`, then delete `6hud.old.ini`.
- No `[UI]` and no old file: write fresh defaults.

Legacy read-chains stay for one release so a hand-edited old INI still parses.
They are marked `// LEGACY` and dropped next release.

### `ALWAYS_SHOW`

Added to every element that idle-fades:

- `[MONEY] ALWAYS_SHOW`
- `[WEAPON] ALWAYS_SHOW`
- `[PLACE_INFO] ALWAYS_SHOW`
- `[VEH_INFO] ALWAYS_SHOW`
- `[GPS_NAV] ALWAYS_SHOW`

When set, the element's idle decay is skipped so it never fades out.

---

## Phase 3 - Folder split + lazy hooks

### Folder split

`.inc` files regrouped by feature. Includes stay in the single `Radar.cpp`
translation unit, so file-statics and the zero-call-site-churn property hold.

```
src/core/           RadarPatches.inc RadarState.inc RadarHeatCore.inc
src/ui/primitives/  RadarRenderPrimitives.inc
src/ui/radar/       RadarLifecycle.inc RadarGps.inc RadarProcess.inc
                    RadarRenderMap.inc RadarRenderBase.inc RadarRenderNorth.inc
                    RadarRenderPlayer.inc RadarRenderPin.inc
                    RadarRenderOverlay.inc RadarRenderMenuMap.inc
                    RadarRenderLifecycle.inc
src/ui/gps/         RadarRenderGps.inc RadarRenderDistance.inc
src/ui/cards/       RadarInfoCards.inc RadarInfoCardsUpdate.inc
                    RadarRenderCards.inc
src/ui/topright/    RadarRenderTopRight.inc
src/ui/wanted/      RadarWanted.inc RadarWantedSearch.inc
                    RadarRenderWanted.inc RadarRenderAuras.inc
                    RadarRenderArcs.inc RadarRenderPolice.inc
src/ext/heat/       RadarHeatTracking.inc
```

`ext/heat` still carries the criminal-profile, snitch and TransFender code
because those grew inside the same unit. Splitting them into `ext/criminal`,
`ext/snitch` and `ext/fuel` is the next pass, done together with the per-module
fine-tuning.

Include order is preserved exactly as before: the units share file-static state,
so a unit may only be included after everything it references. Only the paths
changed.

Textures load on first frame regardless, so nothing changes visually.

### Lazy patch registry

`src/core/RadarPatches.inc` now owns a registry of every hook site.

- `SnapshotPatch(address, size, module)` copies the original bytes before the
  first write, keyed by module.
- `RestoreModulePatches(module)` writes those bytes back and flushes the
  instruction cache.
- `SyncModulePatches()` compares each module's config flag against its live
  state and applies or restores accordingly. It runs from `Radar::Init` and once
  per frame from `OnGameProcess`, where every branch early-outs, so a menu
  toggle takes effect immediately.

| module | live restore |
|---|---|
| nitro (`0x6A3EA0`) | yes |
| wanted stars (`0x58D9A0`) | yes |
| GPS pathfinder buffers | no, restart-only |

The GPS buffer swap is restart-only because the engine keeps a pointer to a fixed
node array; swapping it while a search is in flight corrupts the walk. It is
applied once at boot when `[UI] USE_GPS_NAV` is on, and
`RestoreModulePatches` refuses to touch it, logging that a restart is needed.

---

## Phase 4 - i18n

### Scope

- GPS turn strings: `Make U-Turn Left/Right`, `Turn Right/Left`,
  `Bear Right/Left`, `Continue`, and the `In <n>km/m/mi/ft` distance lines.
- Vehicle class strings and the `Vehicle` fallback.
- Settings menu tab names and labels.
- Out of scope: heat badge labels (`ID/VEH/CLO/GUN`) are language-free icon
  shorthand; zone names already come from the game's own GXT via `TheText.Get`.

### Files

`assets/i18n/<lang>.ini`, flat keys, printf placeholders preserved:

```ini
GPS_TURN_LEFT=Turn Left
GPS_UTURN_LEFT=Make U-Turn Left
GPS_IN_KM=In %.1fkm
VEHICLE_FALLBACK=Vehicle
MENU_TAB_RADAR=Radar
```

Shipped: `en.ini`, `fr.ini`, `de.ini`, `it.ini`, `es.ini`.

### Implementation

`src/I18n.h` / `src/I18n.cpp` hold a fixed `eI18nKey` table. Each key has an
English default compiled in, so a missing file or a partial translation degrades
to English instead of printing blanks. Strings are copied into a static array at
load time, so lookups are pointer returns and cost nothing per frame.

### Language source

`FrontEndMenuManager.m_nPrefsLanguage` (0 EN, 1 FR, 2 DE, 3 IT, 4 ES), read once
in `LoadI18n()`. The game fixes its language before boot, so no hot-swap is
needed; a second call is a no-op unless the index changed.

Translated so far: the GPS turn and distance strings, the vehicle-name fallback,
and the settings menu tab names. Remaining menu labels are still English and
belong to the per-module fine-tuning pass.

---

## Verification

1. Keyboard `J`/`G`/`H` behave as before; pad `LB`/`RB`/`Y` drive the same state
   machines.
2. Fresh boot writes the new INI layout; an old INI migrates and is deleted.
3. Every `ALWAYS_SHOW=1` element stays visible indefinitely.
4. Disabling a module mid-game restores vanilla behaviour where supported, and
   shows `(restart)` where not.
5. GPS text follows the game's language setting.
