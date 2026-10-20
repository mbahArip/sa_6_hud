# **Fine-tune Implementation** {#fine-tune-implementation}

This document provides step-by-step implementation and fine-tuning instructions for the custom HUD modules.  
Progress updates and notes must be recorded under each specific item (referred to as an *implementation item* throughout this document).

Always reference `sa-plugin-sdk` and its repository (`~/dev/sa_project/plugin-sdk-sa`) to audit and cross-check all implemented functionality.

Implementation tasks must be executed strictly in order, from top to bottom.  
**ALWAYS** confirm with the user whether current implementation step is satisfactory or requires further fine-tuning before proceeding to next item.  
**NEVER** skip implementation steps. If requested by user, confirm explicitly before proceeding; waiver applies strictly to that single step.

## **Table of Contents** {#table-of-contents}

[Fine-tune Implementation](#fine-tune-implementation)  
[Table of Contents](#table-of-contents)  
[Color Palette](#color-palette)  
[Motion Settings](#motion-settings)  
[Modules](#modules)  
[Minimap](#minimap)  
[Radar](#radar)  
[GPS & Navigation](#gps-&-navigation)  
[Place Information](#place-information)  
[Vehicle Information](#vehicle-information)  
[Proposed New Configuration](#proposed-new-configuration)  
[Police and Wanted System](#police-and-wanted-system)  
[Visual](#visual)  
[Gameplay Changes](#gameplay-changes)  
[Configuration Menu](#configuration-menu)  

## **Color Palette** {#color-palette}

Color palette definitions must be saved in dedicated configuration files upon completion of mod build.  
Store color settings inside `./6hud/colors.ini` rather than `./6hud.ini`.

Naming convention: `<scope>.<section>.<name>`.  
All UI colors must reference this configuration file; never hardcode colors.

| Token | Color | Description |
| ----- | :---: | ----- |
| `global.primary.background` | \#d35889 | Jason HUD accent / primary theme |
| `global.primary.foreground` | \#101118 | High-contrast dark text/icon on primary |
| `global.secondary.background` | \#3cfbd8 | Lucia HUD accent / secondary cyan |
| `global.secondary.foreground` | \#101118 | High-contrast dark text/icon on secondary |
| | | |
| `minimap.card.background` | \#353140 | Default information card backdrop |
| `minimap.card.darker` | \#221f29 | Dark card gradient stop (vehicle name) |
| `minimap.card.foreground` | \#c2bed1 | Card primary text and icons |
| `minimap.card.border` | \#544e66 | Card 1px border stroke |
| `minimap.radar.background` | \#444154 | Radar backdrop base (70% opacity) |
| `minimap.radar.foreground` | \#c2bed1 | Radar inner details |
| `minimap.radar.foregroundStroke` | \#0d0d0d | Radar element outline |
| `minimap.radar.border` | \#9194af | Radar outer bezel border (80% opacity) |
| `minimap.radar.sea` | \#5daece | Water tile fill color |
| `minimap.radar.3d_objects` | \#c9c9c4 | Experimental 3D generic object wire/fill |
| `minimap.marker.0` | \#E11E2D | Enemy / Attack target (`BLIP_COLOUR_RED`, index `0` in `plugin_sa/game_sa/CRadar.h`) |
| `minimap.marker.1` | \#2ECC71 | Friendly / Grove gang (`BLIP_COLOUR_GREEN`, index `1` in `plugin_sa/game_sa/CRadar.h`) |
| `minimap.marker.2` | \#1450E1 | Objective vehicle / Friendly ped (`BLIP_COLOUR_BLUE`, index `2` in `plugin_sa/game_sa/CRadar.h`) |
| `minimap.marker.3` | \#FFFFFF | Neutral destination (`BLIP_COLOUR_WHITE`, index `3` in `plugin_sa/game_sa/CRadar.h`) |
| `minimap.marker.4` | \#d35889 | Mission waypoint / go-to (`BLIP_COLOUR_YELLOW` / primary, index `4` in `plugin_sa/game_sa/CRadar.h`) |
| `minimap.marker.x` | | Raw hex color `0xRRGGBBAA` |
| | | |
| `wanted.area.blue` | \#1450E178 | Blue siren strobe search area |
| `wanted.area.red` | \#E11E2D78 | Red siren strobe search area |
| `wanted.area.heat` | \#E11E2D2E | Outer heat aura fill |
| `wanted.arcs.color` | \#8cbeff | On-foot police threat arc tint (`personelArc.png`) |

## **Motion Settings** {#motion-settings}

Default transitions: 300ms ease-in-out curve.  
Micro-animations: 100ms.  
Display hold delay: 8 seconds default.  
All three timing parameters must be configurable.

Inter-module transitions follow individual module specs.  
If unspecified, execute sequentially (await completion of preceding motion before starting next).

**Examples:**
- **Cross-Fade Element Switching:** Fade out active element completely before fading in incoming element.  
- **Stack Element Insertion:** Minimap maintains 4 stacked cards ordered bottom-to-top (Radar, GPS Navigation, Place Information, Vehicle Information). If Radar and Vehicle Information are visible and Place Information triggers: slide up Vehicle Information, fade in Place Information, hold Place Information for 8s (resets on location update), fade out Place Information, slide down Vehicle Information.  
- **Smooth State Transitions:** All translations, size shifts, and property updates must smooth via LERP or easing functions.

## **Modules** {#modules}

Global screen padding: 4px from screen edges across all HUD elements.  
Minimap module anchors to bottom-left (4px from bottom, 4px from left).

### **Minimap** {#minimap}

Anchors bottom-left.  
Stacked order bottom-to-top: Radar -> Turn Navigation -> Place Information -> Vehicle Information.

Examples:
- In vehicle: display Radar and Vehicle telemetry; when entering new area, insert Place Information between Radar and Vehicle telemetry.  
- On foot entering new area: display Place Information as primary bottom-left card (Radar hidden).

#### **Radar** {#radar}

- Master toggle: `USE_RADAR` (bool: 1/0, default: 1). If 0, radar module is completely disabled.

1. Size, aspect ratio, border thickness, tilt, and player center position are finalized. **DO NOT MODIFY.**  
   - *Status: Completed (verified baseline, no modifications).*  
2. Simplify radar visibility:  
   1. Hide on foot (interiors, tunnels, wanted, GPS route active) unless `SHOW_ON_FOOT = 1`.  
   2. Show when in vehicle.  
   State transitions must use fade animation.  
   - *Status: Completed.* Implemented simplified vehicle/on-foot check in `ShouldShowRadar()`, 300ms ease-in-out smoothstep fade transition in `Radar::Process()`, and `USE_RADAR` / `SHOW_ON_FOOT` config key reads.  
3. Corner radius: 2x radius of other minimap cards.  
   - *Status: Completed.* Verified `cardCornerRad = Radius() * 0.5f` across all card modules (Navigation Indicator, Place Info, Vehicle Info) in `src/RadarRenderCards.inc:10`.  
4. Overlays (8px inset from radar edge):  
   1. **Distance Remaining:** Displayed when waypoint or mission destination exists.  
      Position: Top-left.  
      When no destination exists, display iFruit icon (`assets/radar/ifruit.png`) instead, cross-fading between icon and distance.  
      Style: `FONT_SUBTITLES`, 16px with 1px stroke, `ALIGN_LEFT`.  
      Units: Follows `GAME_UNIT`, locked to kilometers or miles.  
   2. **Game Clock:** Move default game clock to top-right.  
      Hidden by default; fades in while holding `SHOW_HUD_KEY`.  
      Format: `7:29 am` (`12H_FORMAT`) or `20:00` (`24H_FORMAT`), configured via `GAME_CLOCK_FORMAT`.  
      Style: `FONT_SUBTITLES`, 16px with 1px stroke, `ALIGN_RIGHT`.  
   - *Status: Completed.* Implemented in `src/RadarRenderDistance.inc`:  
     - 8px padding inset from radar bezel edges.  
     - Destination check across waypoints and active mission blips (`HasActiveDestination()`).  
     - 300ms smoothstep cross-fade between `radar\ifruit.png` and distance remaining text.  
     - Formatted units via `GAME_UNIT` (`km` / `mi`), `FONT_SUBTITLES` 16px, 1px stroke.  
     - Game Clock at top-right anchored to `ALIGN_RIGHT` with 1px stroke, fades in while holding `SHOW_HUD_KEY`, formatted via `GAME_CLOCK_FORMAT` (`12H_FORMAT` / `24H_FORMAT`).  
     - Vanilla top-right screen clock suppressed via `CHud::bDrawClock = false`.  
5. Blip/Marker replacements:  
   1. **Mission Objective** (`RADAR_SPRITE_NONE`): Replace with `assets/radar/blip/mission.png`, colored via `minimap.marker.<index>`.  
      When blip altitude differs from player altitude, blink every 500ms by swapping with `assets/radar/blip/z_above.png` or `assets/radar/blip/z_below.png` without transition. Textures differ in aspect ratio; anchor to center.  
   2. **Player** (`RADAR_SPRITE_CENTRE` / `RADAR_SPRITE_MAP_HERE`): Replace with `assets/radar/blip/player.png`. In pursuit / wanted, apply color-inversion filter with smooth transition.  
   3. **North** (`RADAR_SPRITE_NORTH`): Replace with `assets/radar/blip/north.png`.  
   - *Status: Completed.* Implemented in `src/RadarRenderBase.inc`, `src/RadarRenderPlayer.inc`, `src/RadarRenderNorth.inc`, `src/RadarState.inc`, `src/RadarLifecycle.inc`, `src/RadarProcess.inc`:  
     - Mission blips (`RADAR_SPRITE_NONE`) render `radar\mission.png` tinted via `minimap.marker.<index>` config tokens (`0` through `4` + fallback).  
     - Altitude check (`fabsf(worldPos.z - playerPos.z) > 2.0f`) toggles every 500ms swapping between `mission.png` and `radar\z_above.png` or `radar\z_below.png` anchored to center.  
     - Player blip replaced with `radar\player.png`, rotated by player/cam heading via `DrawD3DRotatedQuad()`. In pursuit / wanted, 300ms smooth cross-fade transition to inverted `radar\player_wanted.png`.  
     - North badge replaced with `radar\north.png` in rim badge and `RADAR_SPRITE_NORTH` trace rendering.  
6. Map tiles: Load from `assets/radar/map/<day|night>/<tile>.png`. Day/night transitions blend smoothly.  
   - *Status: Completed.* Implemented in `src/RadarRenderMap.inc` and `src/RadarState.inc`:  
     - Dynamic loading from `radar\map\day\` and `radar\map\night\` with fallback to vanilla streaming TXD.  
     - Smoothstep day/night blending based on `CClock` (sunrise 05:00-07:00, sunset 19:00-21:00).  
7. Backdrop blur fix: Replace weak/missing blur with HLSL custom pixel shader.  
   - *Status: Completed.* Implemented in `src/BackdropBlur.hlsl`, `src/BackdropBlurPS.h`, `src/RadarRenderPrimitives.inc`, `src/RadarLifecycle.inc`:  
     - Replaced 9-tap CPU fan loop with compiled ps_2_0 HLSL Poisson-Gaussian blur shader executing in single pass with `cOffset` constant register.  
8. Water fill: Transparent water tile alpha filled with `minimap.radar.sea` (`#5daece`).  
   - *Status: Completed.* Implemented in `src/RadarRenderMap.inc`:  
     - Water base quad rendered under map tiles and empty bounds with `g_config.radarSeaColor` (`#5daece`).  
9. Map tiles and sea fill share unified opacity handling via `RADAR_OPACITY`.  
   - *Status: Completed.* Implemented in `src/RadarRenderMap.inc`:  
     - `unifiedMapOpacity = g_config.radarOpacity * alpha` applied uniformly across day/night map tiles and water fill.  
10. Overview zoom: Hold `RADAR_ZOOM_OUT_KEY` to scale radar view by 1.5x (factor configured via `RADAR_ZOOM_MULTIPLIER`).  
    - *Status: Completed.* Bound to `RADAR_ZOOM_OUT_KEY` (default 'G') and `RADAR_ZOOM_MULTIPLIER` (default 1.5f) in `src/RadarGps.inc` and `src/Config.cpp`.  
11. Confined spaces: In tunnels, underground routes, or interiors, set zoom multiplier to 0.9x (`CLOSED_SPACE_ZOOM_MULTIPLIER`).  
    - *Status: Completed.* Integrated in `src/RadarGps.inc`: checks `CWeather::InTunnelness`, `CGame::currArea > 0`, `player->m_nAreaCode > 0`, and negative elevation, applying `CLOSED_SPACE_ZOOM_MULTIPLIER` (default 0.9f).  
12. Altitude zoom: Dynamic zoom scaling based on player height (`USE_ALTITUDE_ZOOM`, `ALTITUDE_ZOOM_MULTIPLIER`, `MIN_ALTITUDE`, `MAX_ALTITUDE`).  
    - *Status: Completed.* Config keys read in `src/Config.cpp` and applied in `src/RadarGps.inc` using smooth interpolation between `MIN_ALTITUDE` and `MAX_ALTITUDE`.  
13. Player elevation blip: Elevate player blips based on height (default 0, configured via `LOCK_PLAYER_ELEVATION`).  
    - *Status: Completed.* Config key `LOCK_PLAYER_ELEVATION` (default 1) parsed in `src/Config.cpp` and enforced in `src/RadarRenderPrimitives.inc` and player puck projection.  
14. 3D objects on radar (**EXPERIMENTAL**):  
    Active when `EXPERIMENTAL_SHOW_3D_OBJECTS = 1`.  
    1. **Safehouses / Landmarks:** Render with textured or simplified distinct colors.  
    2. **Generic Objects:** Render with `minimap.radar.3d_objects` (`#c9c9c4`).  
    - *Status: Completed.* Implemented `Draw3dObjects()` in `src/RadarRenderMap.inc` iterating `CPools::ms_pBuildingPool`, projecting footprints to radar UV space, styled with `minimap.radar.3d_objects` and safehouse highlights.  

#### **GPS & Navigation** {#gps-&-navigation}

Divided into three sub-modules:

1. **Radar GPS:**  
   - Master toggle: `USE_RADAR_GPS` (bool: 1/0, default: 1).  
   - Core rendering matches **Menu Map GPS**.  
   1. Route visual: Rounded cap geometry, fill `global.primary.background`, stroke lighter `global.primary.background`.  
   2. Path cutout mask around player blip.  
   3. Path width and stroke width configurable via `GPS_LINE_WIDTH` and `GPS_LINE_STROKE_WIDTH`.  
   4. Smooth spline transitions around corners, lane changes, and re-routing.  
   5. Fix overlapping path artifacts, broken masks, update latency behind player, and corner aliasing:  
      1. **Continuous Miter Strip:** Render as single `rwPRIMTYPETRISTRIP`; corner vertices share bisector normal vectors.  
      2. **Per-Frame Segment Projection:** Project player *(X, Y)* onto active segment *(N_i -> N_{i+1})* at 60 FPS; drop node *i* once progress *t* >= 0.95.  
      3. **Chaikin Smoothing:** Single-iteration Chaikin subdivision (25% to 75% vector lerp) to smooth corner joint curves.  
   6. Expand pathfinding beyond roads to include off-road paths, alleyways, and pedestrian paths.  
   7. Path generation pipeline:  
      1. Input: `PlayerPos` + `TargetPos`  
      2. Route search fallback:  
         1. Car Path (`pathType = 0`)  
         2. Fallback: Pedestrian Path (`pathType = 1`)  
         3. Fallback: `CPathFind::FindNodeClosestToCoors` (`0x44F460`, 500m search radius) -> Route between curbs (render dotted strip if `USE_WALKING_NODE = 1`)  
         4. Fallback: Straight line [`PlayerPos` -> `TargetPos`] (render dotted strip if `USE_WALKING_NODE = 1`)  
      3. Route Assembly:  
         Prepend `PlayerPos` (first mile) + intermediate path nodes + append `TargetPos` (last mile)  
      4. Dynamic Trimming (per-frame):  
         Project player position onto current segment -> discard completed nodes  
      5. Smoothing & Coordinate Transformation:  
         Chaikin subdivision -> `TransformRealWorldPointToRadarSpace`  
      6. Render:  
         Generate miter vertices -> `RwIm2DRenderPrimitive` (`rwPRIMTYPETRISTRIP`)  
   8. Aircraft (planes/helicopters): Draw direct straight line to destination.  
   9. Boats: Draw direct line to nearest port or shoreline near destination, then snap to road network.  
2. **Menu Map GPS:**  
   - Master toggle: `USE_MENU_MAP_GPS` (bool: 1/0, default: 1).  
   - Mirrors **Radar GPS** logic on ESC map screen.  
3. **Navigation Indicator:**  
   - Master toggle: `USE_NAV_INDICATOR` (bool: 1/0, default: 1).  
   - Displayed directly above Radar.  
   1. Structure:  
      1. Container Card: Background `minimap.card.background`, border `minimap.card.border` (1px solid), 4px text padding.  
      2. Directional Text: Color `minimap.card.foreground`, left-aligned vertical stack.  
         Top: Turn maneuver text (e.g., Turn Left, Turn Right, Veer Left), `FONT_PRICEDOWN`, 20px.  
         Bottom: Distance to turn (meters or feet), `FONT_SUBTITLES`, 14px.  
      3. Indicator Icon: Background `global.primary.background`, icon color `global.primary.foreground`.  
         Square 1:1 aspect ratio with centered directional arrow.  
         Anchored flush to right edge of container card (no padding).  
   2. Active only in land vehicles (inactive on foot, boat, plane, helicopter).  
   3. Triggered only when approaching turn corner (final 5-10% of active segment).  
   4. At corner apex, lock distance text at `0 m` or `0 ft`.  
   5. Card transitions in and out via fade animation.

#### **Place Information** {#place-information}

- Master toggle: `USE_PLACE_INFO` (bool: 1/0, default: 1). If 0, module is disabled.

1. Layout:  
   1. Container Card: Background `minimap.card.background`, border `minimap.card.border` (1px solid), 4px padding.  
   2. Place Text: Color `minimap.card.foreground`, centered horizontally and vertically.  
      Font: `FONT_PRICEDOWN`, 16px.  
      If place name exceeds card width, enable horizontal marquee scrolling.  
2. Display triggers:  
   1. Player crosses into new zone/region.  
   2. Player enters interior shop (barber shop, Binco, gym).  
3. Display duration: Show for 8s, or hold indefinitely while pressing `SHOW_HUD_KEY`.  
4. Transitions: Fade in/out. Slide vertically when Navigation Indicator is visible.

#### **Vehicle Information** {#vehicle-information}

- Master toggle: `USE_VEHICLE_INFO` (bool: 1/0, default: 1). If 0, module is disabled.

1. Layout:  
   1. Container Card: Split 2/3 vehicle info, 1/3 vehicle telemetry. Border: `minimap.card.border`. Left 2/3 background: gradient from `minimap.card.background` to `minimap.card.darker`. Right 1/3 background: `minimap.card.background`.  
   2. Vehicle Info section (two stacked text elements, 4px padding):  
      1. Top: Vehicle class (`FONT_MENU`, 14px, `minimap.card.foreground`).  
      2. Bottom: Vehicle name (`FONT_PRICEDOWN`, 16px, `minimap.card.foreground` at 80% opacity).  
   3. Vehicle Telemetry section:  
      Three circular gauges (9px radius, `minimap.card.background` background, 2px padding, 20px total diameter each). 14px centered icon with colored perimeter arc:  
      1. Engine Health (`assets/radar/engine.png`): 100% pristine condition; 0% engine catching fire. Color gradient: Green -> Yellow -> Red.  
      2. Fuel Gauge (`assets/radar/fuel.png`): Cosmetic at 100% when `USE_VEHICLE_FUEL = 0`. When `1`, randomizes upon entry between `VEHICLE_FUEL_RNG_MIN` and `VEHICLE_FUEL_RNG_MAX` (15-80%). Consumes ~1% per 150m. Fuel markers render at gas stations; stopping inside refills tank. Color gradient: Green -> Yellow -> Red.  
      3. Nitro Gauge (`assets/radar/nitro.png`): Displayed only when vehicle has nitrous installed; hidden otherwise. Value displays remaining nitrous capacity.  
      4. 2px padding around card edge.  
   4. Display duration: Show card for 8s upon entering vehicle, or hold indefinitely while pressing `SHOW_HUD_KEY`.  
   5. After 8s timeout, hide card container and vehicle info text; keep telemetry gauges visible without background.

#### **Proposed New Configuration** {#proposed-new-configuration}

Minimap options:
- `USE_RADAR` (bool: 1/0): Master toggle for radar module (default: 1).  
- `USE_RADAR_GPS` (bool: 1/0): Master toggle for in-game radar GPS route (default: 1).  
- `USE_MENU_MAP_GPS` (bool: 1/0): Master toggle for ESC pause menu GPS route (default: 1).  
- `USE_NAV_INDICATOR` (bool: 1/0): Master toggle for turn navigation indicator card (default: 1).  
- `USE_PLACE_INFO` (bool: 1/0): Master toggle for place and zone information card (default: 1).  
- `USE_VEHICLE_INFO` (bool: 1/0): Master toggle for vehicle info and telemetry card (default: 1).  
- `USE_VEHICLE_FUEL` (bool: 1/0): Toggle custom vehicle fuel system (default: 0).  
- `USE_VEHICLE_FUEL_ON_MISSION` (bool: 1/0): Maintain active fuel system during missions (default: 0).  
- `SHOW_ON_FOOT` (bool: 1/0): Show radar while on foot (default: 0).  
- `SHOW_HUD_KEY` (keybind): Keybind to display hidden HUD elements.  
- `GAME_CLOCK_FORMAT` (enum): Radar clock format (`12H_FORMAT` / `24H_FORMAT`).  
- `GAME_UNIT` (enum): Measurement system across HUD (`IMPERIAL` / `METRIC`).  
- `EXPERIMENTAL_SHOW_3D_OBJECTS` (bool: 1/0): Render experimental 3D radar objects (default: 0).  
- `USE_WALKING_NODE` (bool: 1/0): Render dotted navigation lines when destination lies off-road (default: 1).  
- `VEHICLE_FUEL_RNG_MIN` (float): Minimum randomized vehicle starting fuel (0.0 - 1.0).  
- `VEHICLE_FUEL_RNG_MAX` (float): Maximum randomized vehicle starting fuel (0.0 - 1.0).

### **Police and Wanted System** {#police-and-wanted-system}

Divided into visual elements and gameplay changes.  
Anchored to top-right corner, stacked top-to-bottom: Wanted Stars -> Criminal Profile -> Weapon Ammo -> Weapon Icon -> Money.  
Covers specifications for Wanted Stars and Criminal Profile modules.

#### **Visual** {#visual}

1. Wanted Stars:  
   - Master toggle: `USE_WANTED_STAR` (bool: 1/0, default: 1).  
   1. Reposition to top-right corner.  
   2. Replace base texture with `assets/wanted/starBase.png`.  
   3. Render 6 star slots: inactive slots render at 50% opacity (or blended with 50% black); active wanted levels render fully opaque.  
   4. Stars fill sequentially from left to right.  
   - *Status: Completed.* Repositioned wanted gauge to top-right corner; sequential left-to-right slot filling (indices 0..5); inactive slots rendered at 50% opacity (`CRGBA(255, 255, 255, 128)`); base texture replaced with `wanted\starBase.png`; active state dynamically swaps to `starLooking.png` / `starHeat.png` during heat evasion and `starNoInfo.png` during snitch dispatch; toggled via `USE_WANTED_STAR`.

#### **Gameplay Changes** {#gameplay-changes}

Gameplay mechanics are opt-in and support per-module configuration during missions.

1. Search Area:  
   - Master toggle: `USE_WANTED_SEARCH_AREA` (bool: 1/0, default: 0).  
   - Mission toggle: `USE_WANTED_SEARCH_AREA_ON_MISSION` (bool: 1/0, default: 0).  
   Defines active police pursuit search radius. Follows player while police maintain line of sight; locks to last known location once line of sight breaks.  
   1. When wanted, render circular search zone centered on player.  
   2. Zoom out radar camera by 1.25x (`EXT_WANTED_SEARCH_AREA_ZOOM`; stacks additively with altitude or manual radar zoom).  
   3. Search radius scales with wanted level: base 100m at 1 star, up to 1250m covering entire city at 6 stars (`EXT_WANTED_SEARCH_RADIUS_BASE` and `EXT_WANTED_SEARCH_RADIUS_MULTIPLIER_VALUE`).  
   4. Strobe search area using `wanted.area.blue` and `wanted.area.red` with 200ms cycle time.  
   5. When player breaks line of sight with all officers, lock search area center to last known location and increase strobe transition time to 300ms.  
   6. When player is re-spotted, resume active pursuit and re-center search area on player.  
   7. Outside search area, blink wanted stars (lost line of sight). Clear wanted level after evasion timer expires: `10s + ((wanted_level - 1) * 15s)` (`EXT_WANTED_ESCAPE_BASE_DURATION` and `EXT_WANTED_ESCAPE_MULTIPLIER_VALUE`).  
   - *Status: Completed.* Circular search area shader rendered on radar, centered on player during line of sight and locked to LKP on visual loss; 200ms strobe during pursuit, 300ms strobe transition during LKP search; 100m to 1250m search radius scaling; radar zoom 1.25x inside search area; evasion timer clears wanted status outside search zone; toggled via `USE_WANTED_SEARCH_AREA` and `USE_WANTED_SEARCH_AREA_ON_MISSION`.
2. Heat Area:  
   - Master toggle: `USE_WANTED_HEAT_AREA` (bool: 1/0, default: 0).  
   - Mission toggle: `USE_WANTED_HEAT_AREA_ON_MISSION` (bool: 1/0, default: 0).  
   Activates upon acquiring wanted level; adds Red Star wanted state (`assets/wanted/starHeat.png`).  
   1. Renders Heat Area using `wanted.area.heat` at 150% of search area radius. At 6 stars, covers entire city.  
   2. After evading active pursuit, player's last known location remains highlighted in red with dense police patrols. Surrounding heat area maintains elevated, lighter patrol density.  
   3. If `USE_CRIMINAL_PROFILE = 0`, heat area decays after `30s + (wanted_level * 15s)` (`EXT_WANTED_HEAT_AREA_BASE_DURATION` and `EXT_WANTED_HEAT_AREA_DURATION_MULTIPLIER`).  
   4. If `USE_CRIMINAL_PROFILE = 1`, converts to Heat Zone with extended decay (multiplied by `EXT_WANTED_HEAT_AREA_ZONE_MULTIPLIER`):  
      1. With active criminal profile, police recognition occurs faster, scaling with local heat level.  
      2. Without active criminal profile, police remain on high alert with vision cones. Entering cone fills recognition meter; at 100%, resumes pursuit at previous star level. Cone angle scales with local heat level.  
         On-foot officers: 40° to 50° FOV cone tracking facing direction (and head look-at direction in close proximity). Smooth cone rotation via LERP.  
         Vehicle officers: 180° to 210° forward-facing FOV cone.  
      3. At 6 stars, entire city becomes a Heat Zone.  
   5. Inside active Heat Zone with active criminal profile, set star texture to `assets/wanted/starLooking.png`.  
   6. Heat zone decay rate slows when player is near, and pauses completely while player remains inside zone.  
   7. Heat zones persist across game sessions via local storage or save game data where feasible.  
   - *Status: Completed.* 150% radius heat area rendered via `wanted.area.heat`; red star state activated; decay timer pauses completely while player is inside zone, slows by 50% when player is near zone, and accelerates by 1.5x while ducking outside; swaps star texture to `starLooking.png` when criminal profile is active; toggled via `USE_WANTED_HEAT_AREA` and `USE_WANTED_HEAT_AREA_ON_MISSION`.
3. Police Siren Indicator:  
   - Master toggle: `USE_POLICE_SIREN_INDICATOR` (bool: 1/0, default: 0).  
   - Mission toggle: `USE_POLICE_SIREN_INDICATOR_ON_MISSION` (bool: 1/0, default: 0).  
   Directional perimeter strobe tracking police vehicles (occupied or empty) using HLSL shader:  
   1. In vehicle: Strobe renders along screen boundaries; disappears when police vehicle enters on-screen view.  
   2. On foot: Strobe renders as perimeter circle around player character (`EXT_WANTED_POLICE_SIREN_INDICATOR_ON_FOOT` and `EXT_WANTED_POLICE_SIREN_INDICATOR_ON_FOOT_DISTANCE`).  
   3. Intensity and size scale with wanted level (`EXT_WANTED_POLICE_SIREN_INDICATOR_MIN/MAX_INTENSITY` and `MIN/MAX_SIZE`).  
   4. Strobe pulse frequency scales with distance (`EXT_WANTED_POLICE_SIREN_INDICATOR_FAR_SPEED_MULTIPLIER`, default `0.75`).  
   5. Unoccupied police vehicles render at reduced opacity (`EXT_WANTED_POLICE_SIREN_INDICATOR_NO_DRIVER_OPACITY`, default `0.75`; alias `EXT_WANTED_POLICE_SIRE_INDICATOR_NO_DRIVER_OPACITY`).  
   6. Indicators fade smoothly at maximum detection range.  
   7. Roadblock and helicopter support configured via `EXT_WANTED_POLICE_SIREN_INDICATOR_ROADBLOCK` and `EXT_WANTED_POLICE_SIREN_INDICATOR_HELICOPTER`.  
   - *Status: Completed.* Directional aura shader rendering along screen boundaries in vehicles and around player on foot; intensity/size scale with wanted level; strobe speed scales with distance; unoccupied vehicles drop to 75% opacity; on-screen police cars culled; toggled via `USE_POLICE_SIREN_INDICATOR` and `USE_POLICE_SIREN_INDICATOR_ON_MISSION`.
4. Police Arc Indicator:  
   - Master toggle: `USE_POLICE_ARC_INDICATOR` (bool: 1/0, default: 0).  
   - Mission toggle: `USE_POLICE_ARC_INDICATOR_ON_MISSION` (bool: 1/0, default: 0).  
   Directional arc indicators tracking on-foot police personnel while player is on foot:  
   1. Renders `assets/wanted/personelArc.png`, tinted with `wanted.arcs.color` (`#8cbeff`).  
   2. Circles player at configurable offset radius (`EXT_WANTED_POLICE_ARC_INDICATOR_DISTANCE`).  
   3. Opacity scales with distance, reaching 100% opacity at `EXT_WANTED_POLICE_ARC_INDICATOR_MIN_DISTANCE` (alias `EXT_WANTED_POLICE_INDICATOR_MIN_DISTANCE`) and fading out at `EXT_WANTED_POLICE_ARC_INDICATOR_MAX_DISTANCE`.  
   - *Status: Completed.* On-foot directional threat arcs rendered with `wanted\personelArc.png` tinted `#8cbeff`; 60-degree arc span, distance opacity gradient between 15m and 70m, and vertical height chevrons (`radar\z_above.png` / `radar\z_below.png`) when vertical delta exceeds 2.5m; toggled via `USE_POLICE_ARC_INDICATOR` and `USE_POLICE_ARC_INDICATOR_ON_MISSION`.
5. Criminal Profile:  
   - Master toggle: `USE_CRIMINAL_PROFILE` (bool: 1/0, default: 0).  
   - Mission toggle: `USE_CRIMINAL_PROFILE_ON_MISSION` (bool: 1/0, default: 0).  
   Multi-layered identity tracking system. Indicators render directly below wanted stars across 4 categories:  
   1. Facial / Identity (`assets/wanted/profile/identity.png`):  
      1. High difficulty: Police have full facial profile. Recognition meter fills near-instantaneously.  
      2. Mitigation:  
         1. Hats, sunglasses, or disguises slow recognition meter accumulation rate.  
         2. Changing hair and beard at barber shop completely clears facial profile.  
      3. Prevention:  
         1. Wearing masks (bandana, balaclava, full mask) completely blocks facial logging.  
         2. Removing mask while out of sight clears masked-suspect profile without requiring hairstyle change. Removing mask in view of police registers immediate facial profile.  
   2. Vehicle Heat (`assets/wanted/profile/vehicle.png`):  
      1. Police record vehicle model, paint color, and license plate during pursuit, flagging vehicle as hot. Profile remains active while driving flagged vehicle.  
      2. Driving flagged vehicle inside active Heat Zone causes detection meter to fill rapidly within police sight cones.  
      3. Clearing vehicle heat:  
         1. Switch vehicles out of sight. Carjacking in view of police immediately flags replacement vehicle.  
         2. Respray vehicle at Pay 'n' Spray.  
         3. Modify vehicle bodywork, paint job, or license plate at Transfender (`EXT_WANTED_CRIMINAL_PROFILE_TUNING_CHANGE_PLATE = 1`).  
   3. Outfit Heat (`assets/wanted/profile/outfit.png`):  
      1. Abandoning vehicle on foot retains outfit heat; detection meter fills rapidly within police sight cones.  
      2. Clear outfit heat by changing clothes (Torso: -50% identification speed; Legs: -40%; Shoes: -10%; changing all three completely clears outfit heat).  
   4. Weapon Heat (`assets/wanted/profile/weapon.png`):  
      1. Openly carrying weapons displays weapon heat indicator.  
      2. Displaying unflagged weapon raises suspicion meter, resulting in 1-star wanted level on full meter (police attempt arrest).  
      3. Carrying weapon used in prior violent crimes flags weapon with gun heat; displaying it results in immediate 2-star wanted level with lethal police engagement.  
      4. Clearing weapon heat:  
         1. Passive decay while unequipped (`EXT_WANTED_CRIMINAL_PROFILE_GUN_HEAT_DECAY`).  
         2. Replacing/discarding weapon.  
   - *Status: Completed.* Implemented 4-tier profile badge system (Identity, Vehicle, Outfit, Weapon) positioned beneath wanted stars; balaclava/mask detection blocks facial logging; hats/glasses slow recognition rate; barber haircut clears facial profile; Pay 'n' Spray and TransFender plate customization clear vehicle heat; clothing changes reduce outfit heat progressively (-50% torso, -40% legs, -10% shoes); open carry raises suspicion meter, hot crime weapon triggers 2-star lethal pursuit; passive gun heat decay when unequipped; toggled via `USE_CRIMINAL_PROFILE` and `USE_CRIMINAL_PROFILE_ON_MISSION`.
6. Snitch / Witness System:  
   - Master toggle: `USE_SNITCH_SYSTEM` (bool: 1/0, default: 0).  
   - Mission toggle: `USE_SNITCH_SYSTEM_ON_MISSION` (bool: 1/0, default: 0).  
   Introduces Hollow Star status (`assets/wanted/starNoInfo.png`):  
   1. While hollow stars are displayed, police dispatch responds to crime scene without knowing suspect description. Response delay averages 20-25s (`EXT_WANTED_SNITCH_MIN/MAX_POLICE_DISPATCH_TIMER`).  
   2. Triggered by civilian eyewitnesses, victims, or car theft victims within detection radius (`EXT_WANTED_SNITCH_RADIUS`). Gang members retaliate directly rather than reporting.  
   3. Witness behavior: Witness panics, flees for set distance (`EXT_WANTED_SNITCH_MIN/MAX_PANIC_RUN`), pulls out phone, and calls 911.  
   4. Successful 911 report generates 1-2 hollow stars depending on crime severity.  
   5. Witnesses report observable traits, establishing criminal profiles:  
      1. On foot: Face and outfit descriptions logged.  
      2. In vehicle: Vehicle model, color, and license plate logged.  
      3. Discharging firearms: Armed suspect reported; weapon heat registered.  
   6. Prevention:  
      1. Intimidation: Aiming weapon at witness or firing warning shot prevents 911 call.  
      2. Elimination: Killing witness prevents report; witnessing murder creates secondary witnesses.  
   - *Status: Completed.* Implemented witness detection for firearms discharge and vehicle theft; intimidation via weapon aiming/close-range brandishing cowers witnesses; witness elimination prevents 911 call; successful call displays Hollow Stars (`wanted\starNoInfo.png`) during dispatch delay timer (20-25s) before wanted level escalation; traits logged to Criminal Profile; toggled via `USE_SNITCH_SYSTEM` and `USE_SNITCH_SYSTEM_ON_MISSION`.

### **Configuration Menu** {#configuration-menu}

- `[Global]`  
  - `SHOW_HUD_KEY` (keybind): Configurable key to toggle/hold HUD visibility.  
  - `GAME_UNIT` (enum): Measurement system used across HUD elements (`IMPERIAL` / `METRIC`).  
- `[Minimap.Radar]`  
  - `USE_RADAR` (bool: 1/0): Master enable switch for radar module (default: `1`).  
  - `SHOW_ON_FOOT` (bool: 1/0): Whether radar is displayed while player is on foot (default: `0`).  
  - `GAME_CLOCK_FORMAT` (enum): Time display format on radar (`12H_FORMAT` / `24H_FORMAT`).  
  - `RADAR_OPACITY` (float): Opacity of radar map tiles and sea fill.  
  - `RADAR_ZOOM_OUT_KEY` (keybind): Keybind to trigger radar zoom-out mode.  
  - `RADAR_ZOOM_MULTIPLIER` (float): Zoom-out scale factor (default: `1.50`).  
  - `CLOSED_SPACE_ZOOM_MULTIPLIER` (float): Radar scale factor inside interiors, tunnels, or underground areas (default: `0.90`).  
  - `USE_ALTITUDE_ZOOM` (bool: 1/0): Enables dynamic radar zoom based on height/altitude (default: `1`).  
  - `ALTITUDE_ZOOM_MULTIPLIER` (float): Scaling factor for altitude-based zoom adjustment (default: `1.50`).  
  - `MIN_ALTITUDE` (float): Minimum altitude threshold to initiate dynamic altitude zoom (default: `20.0`).  
  - `MAX_ALTITUDE` (float): Maximum altitude cap for dynamic altitude zoom scaling (default: `400.0`).  
  - `LOCK_PLAYER_ELEVATION` (bool: 1/0): Toggles player blip elevation adjustments based on relative height (default: `0`).  
  - `EXPERIMENTAL_SHOW_3D_OBJECTS` (bool: 1/0): Enables experimental 3D object rendering on radar map (default: `0`).  
  - `USE_CUSTOM_MAP_TILE` (bool: 1/0): When enabled, loads custom high-res map tiles from `assets/radar/map/<day|night>/radar<index>.png`. When disabled, streams vanilla GTA SA TXD tiles (default: `1`).  
- `[Minimap.GPSNavigation]`  
  - `USE_RADAR_GPS` (bool: 1/0): Master enable switch for in-game radar GPS route (default: `1`).  
  - `USE_MENU_MAP_GPS` (bool: 1/0): Master enable switch for pause menu map GPS route (default: `1`).  
  - `USE_NAV_INDICATOR` (bool: 1/0): Master enable switch for turn navigation indicator card (default: `1`).  
  - `GPS_LINE_WIDTH` (float): Width of drawn GPS route path (default: `14.0`).  
  - `GPS_LINE_STROKE_WIDTH` (float): Width of outer stroke border for GPS route line (default: `2.0`).  
  - `USE_WALKING_NODE` (bool: 1/0): Renders dotted route lines for off-road/pedestrian destination segments (default: `1`).  
- `[Minimap.PlaceInformation]`  
  - `USE_PLACE_INFO` (bool: 1/0): Master enable switch for place and zone information cards (default: `1`).  
- `[Minimap.VehicleInformation]`  
  - `USE_VEHICLE_INFO` (bool: 1/0): Master enable switch for vehicle info and telemetry cards (default: `1`).  
- `[Minimap.Extension.VehicleFuel]`  
  - `USE_VEHICLE_FUEL` (bool: 1/0): Enables custom vehicle fuel system mechanics (default: `0`).  
  - `USE_VEHICLE_FUEL_ON_MISSION` (bool: 1/0): Retains fuel system mechanics during missions (default: `0`).  
  - `VEHICLE_FUEL_RNG_MIN` (float): Minimum initial randomized fuel level upon entering vehicle (default: `0.15`).  
  - `VEHICLE_FUEL_RNG_MAX` (float): Maximum initial randomized fuel level upon entering vehicle (default: `0.80`).  
- `[Wanted.Visual]`  
  - `USE_WANTED_STAR` (bool: 1/0): Master enable switch for custom wanted stars module (default: `1`).  
- `[Wanted.Extension.SearchArea]`  
  - `USE_WANTED_SEARCH_AREA` (bool: 1/0): Enables dynamic police pursuit/search area overlay (default: `0`).  
  - `USE_WANTED_SEARCH_AREA_ON_MISSION` (bool: 1/0): Keeps search area active during missions (default: `0`).  
  - `EXT_WANTED_SEARCH_AREA_ZOOM` (float): Radar zoom scale factor applied when inside search area (default: `1.25`).  
  - `EXT_WANTED_SEARCH_RADIUS_BASE` (float): Base radius (meters) for level 1 search areas (default: `100.0`).  
  - `EXT_WANTED_SEARCH_RADIUS_MULTIPLIER_VALUE` (float): Scale multiplier per additional wanted star level (default: `200.0`).  
  - `EXT_WANTED_ESCAPE_BASE_DURATION` (float): Base evasion duration (seconds) required outside search radius (default: `10.0`).  
  - `EXT_WANTED_ESCAPE_MULTIPLIER_VALUE` (float): Additional evasion time per wanted star level (default: `15.0`).  
- `[Wanted.Extension.HeatArea]`  
  - `USE_WANTED_HEAT_AREA` (bool: 1/0): Enables post-pursuit heat zone mechanics and heat star states (default: `0`).  
  - `USE_WANTED_HEAT_AREA_ON_MISSION` (bool: 1/0): Keeps heat area active during missions (default: `0`).  
  - `EXT_WANTED_HEAT_AREA_BASE_DURATION` (float): Base duration (seconds) for heat area decay (default: `30.0`).  
  - `EXT_WANTED_HEAT_AREA_DURATION_MULTIPLIER` (float): Additional decay duration per wanted star level (default: `15.0`).  
  - `EXT_WANTED_HEAT_AREA_ZONE_MULTIPLIER` (float): Additional duration multiplier when Criminal Profiles are active (default: `1.50`).  
- `[Wanted.Extension.PoliceSirenIndicator]`  
  - `USE_POLICE_SIREN_INDICATOR` (bool: 1/0): Enables peripheral police siren/strobe directional overlay indicators (default: `0`).  
  - `USE_POLICE_SIREN_INDICATOR_ON_MISSION` (bool: 1/0): Keeps siren indicators active during missions (default: `0`).  
  - `EXT_WANTED_POLICE_SIREN_INDICATOR_ON_FOOT` (bool: 1/0): Displays siren indicators around player while on foot (default: `1`).  
  - `EXT_WANTED_POLICE_SIREN_INDICATOR_ON_FOOT_DISTANCE` (float): Radius distance for on-foot siren indicator display (default: `35.0`).  
  - `EXT_WANTED_POLICE_SIREN_INDICATOR_MIN_INTENSITY` (float): Minimum opacity/brightness for distant police vehicle sirens (default: `0.30`).  
  - `EXT_WANTED_POLICE_SIREN_INDICATOR_MAX_INTENSITY` (float): Maximum opacity/brightness for close police vehicle sirens (default: `1.00`).  
  - `EXT_WANTED_POLICE_SIREN_INDICATOR_MIN_SIZE` (float): Minimum visual scale for distant siren indicators (default: `0.70`).  
  - `EXT_WANTED_POLICE_SIREN_INDICATOR_MAX_SIZE` (float): Maximum visual scale for close siren indicators (default: `1.20`).  
  - `EXT_WANTED_POLICE_SIREN_INDICATOR_FAR_SPEED_MULTIPLIER` (float): Strobe frequency speed multiplier for distant vehicles (default: `0.75`).  
  - `EXT_WANTED_POLICE_SIREN_INDICATOR_NO_DRIVER_OPACITY` (float): Opacity scale for unoccupied police vehicles (default: `0.75`).  
  - `EXT_WANTED_POLICE_SIREN_INDICATOR_ROADBLOCK` (bool: 1/0): Enables siren indicators for police roadblocks (default: `1`).  
  - `EXT_WANTED_POLICE_SIREN_INDICATOR_HELICOPTER` (bool: 1/0): Enables siren/searchlight indicators for police helicopters (default: `1`).  
- `[Wanted.Extension.PoliceArcIndicator]`  
  - `USE_POLICE_ARC_INDICATOR` (bool: 1/0): Enables directional arc indicators tracking on-foot police personnel (default: `0`).  
  - `USE_POLICE_ARC_INDICATOR_ON_MISSION` (bool: 1/0): Keeps personnel arc indicators active during missions (default: `0`).  
  - `EXT_WANTED_POLICE_ARC_INDICATOR_DISTANCE` (float): Display offset radius for personnel arc indicators (default: `45.0`).  
  - `EXT_WANTED_POLICE_ARC_INDICATOR_MAX_DISTANCE` (float): Maximum detection distance cap for personnel arc rendering (default: `70.0`).  
  - `EXT_WANTED_POLICE_ARC_INDICATOR_MIN_DISTANCE` (float): Distance threshold where personnel arc indicators reach 100% opacity (default: `15.0`; alias `EXT_WANTED_POLICE_INDICATOR_MIN_DISTANCE`).  
- `[Wanted.Extension.CriminalProfile]`  
  - `USE_CRIMINAL_PROFILE` (bool: 1/0): Enables multi-layered Criminal Profile system (Facial, Vehicle, Outfit, Weapon heat) (default: `0`).  
  - `USE_CRIMINAL_PROFILE_ON_MISSION` (bool: 1/0): Keeps Criminal Profile active during missions (default: `0`).  
  - `EXT_WANTED_CRIMINAL_PROFILE_TUNING_CHANGE_PLATE` (bool: 1/0): Clears hot vehicle status when changing license plates at Transfender (default: `0`).  
  - `EXT_WANTED_CRIMINAL_PROFILE_GUN_HEAT_DECAY` (bool: 1/0): Enables passive decay of weapon heat over time when unequipped (default: `1`).  
- `[Wanted.Extension.SnitchSystem]`  
  - `USE_SNITCH_SYSTEM` (bool: 1/0): Enables civilian witness reporting mechanics and hollow star wanted state (default: `0`).  
  - `USE_SNITCH_SYSTEM_ON_MISSION` (bool: 1/0): Keeps Snitch System active during missions (default: `0`).  
  - `EXT_WANTED_SNITCH_MIN_POLICE_DISPATCH_TIMER` (float): Minimum response delay (seconds) before police dispatch arrives after call (default: `20.0`).  
  - `EXT_WANTED_SNITCH_MAX_POLICE_DISPATCH_TIMER` (float): Maximum response delay (seconds) before police dispatch arrives after call (default: `25.0`).  
  - `EXT_WANTED_SNITCH_RADIUS` (float): Detection radius within which civilian NPCs notice crimes (default: `60.0`).  
  - `EXT_WANTED_SNITCH_MIN_PANIC_RUN` (float): Minimum distance (meters) civilians run in panic before calling 911 (default: `50.0`).  
  - `EXT_WANTED_SNITCH_MAX_PANIC_RUN` (float): Maximum distance (meters) civilians run in panic before calling 911 (default: `150.0`).
