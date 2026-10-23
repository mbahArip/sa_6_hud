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
src/core/           RadarState.inc RadarPatches.inc RadarHeatCore.inc
                    Config.* Layout.h PatchRegistry
src/ui/radar/       map, border, overlay, north, player, pin, menu map, lifecycle
src/ui/gps/         GPS path render, distance, navigator
src/ui/cards/       info cards, vehicle info
src/ui/topright/    weapon, ammo, money
src/ui/wanted/      stars, search area, heat area render
src/ext/fuel/       consumption, pay-n-spray
src/ext/heat_area/  escape stages
src/ext/criminal/   profile, badges, identification
src/ext/snitch/     civilian witness system
src/ext/siren/      auras
src/ext/arc/        threat arcs
```

Textures load on first frame regardless, so nothing changes visually.

### Lazy patch registry

Today `Radar::Init` applies the GPS-engine, nitro and wanted patches
unconditionally. New rule: a module's game-code hook is only installed while the
module is enabled.

- `PatchRegistry` records the original bytes at each patch address before the
  first write.
- Enabling a module writes its patch set; disabling restores the recorded bytes
  and flushes the instruction cache.
- Live restore is used where it is safe: the wanted-star jump, nitro hook,
  radar suppression patches.
- Restart-only: the GPS pathfinder buffer resize, because reallocating the node
  array mid-route corrupts an in-flight search. The menu tags those
  `(restart)`.

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

Ship `en.ini` plus `fr/de/it/es.ini`. Non-English files start as copies of the
English set for the community to fill in.

### Language source

`FrontEndMenuManager.m_nPrefsLanguage` (0 EN, 1 FR, 2 DE, 3 IT, 4 ES), read once
at `Init`. The game fixes its language before boot, so no hot-swap is needed.
Missing file falls back to `en.ini`.

---

## Verification

1. Keyboard `J`/`G`/`H` behave as before; pad `LB`/`RB`/`Y` drive the same state
   machines.
2. Fresh boot writes the new INI layout; an old INI migrates and is deleted.
3. Every `ALWAYS_SHOW=1` element stays visible indefinitely.
4. Disabling a module mid-game restores vanilla behaviour where supported, and
   shows `(restart)` where not.
5. GPS text follows the game's language setting.
