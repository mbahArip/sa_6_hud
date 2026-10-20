# 26 - Comprehensive INI Comments & Specification Documentation

## Objective
Add clear, comprehensive inline documentation comments to every section and setting in `6hud.ini`.
For every setting, document:
1. Setting purpose & description
2. Available options, enums, or boolean values
3. Min / Max numeric bounds (where applicable)
4. Default value

## Structure & Guidelines
- Use `#` for comment blocks and header dividers.
- Format documentation blocks uniformly:
  ```ini
  # Setting description
  # Type: [bool | enum | float | int | string]
  # Options / Range: [...]
  # Default: ...
  KEY = VALUE
  ```
- Ensure comments do not disrupt `WritePrivateProfileStringA` or `LoadConfig()`.
- Do not touch or modify the game installation directory `D:\Games\gta sa\GTA San Andreas\6hud.ini`. Only modify repo `6hud.ini`.

## Sections Covered
- `[Global]`
- `[weapon]`
- `[money]`
- `[Minimap.Radar]`
- `[Minimap.GPSNavigation]`
- `[Minimap.PlaceInformation]`
- `[Minimap.VehicleInformation]`
- `[Minimap.Extension.VehicleFuel]`
- `[Wanted.Visual]`
- `[Wanted.Extension.SearchArea]`
- `[Wanted.Extension.HeatArea]`
- `[Wanted.Extension.PoliceSirenIndicator]`
- `[Wanted.Extension.PoliceArcIndicator]`
- `[Wanted.Extension.CriminalProfile]`
- `[Wanted.Extension.SnitchSystem]`
- `[layout.<module>]` (radar, gps_nav, place_info, vehicle_info, health_armor, wanted, weapon_hud, money_hud)
