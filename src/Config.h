#pragma once

#include <CRGBA.h>
#include "Layout.h"

namespace hud6 {

// Gamepad buttons addressable from the [KEYS] section. Stored as a bitmask in
// Config so a bind can be keyboard-only, pad-only, or both.
enum ePadButton : int {
    PADBTN_NONE       = 0,
    PADBTN_A          = 1 << 0,   // Cross / Square (PS)
    PADBTN_B          = 1 << 1,   // Circle
    PADBTN_X          = 1 << 2,   // Square
    PADBTN_Y          = 1 << 3,   // Triangle
    PADBTN_LB         = 1 << 4,   // L1
    PADBTN_LT         = 1 << 5,   // L2
    PADBTN_RB         = 1 << 6,   // R1
    PADBTN_RT         = 1 << 7,   // R2
    PADBTN_START      = 1 << 8,
    PADBTN_BACK       = 1 << 9,   // Select
    PADBTN_DPAD_UP    = 1 << 10,
    PADBTN_DPAD_DOWN  = 1 << 11,
    PADBTN_DPAD_LEFT  = 1 << 12,
    PADBTN_DPAD_RIGHT = 1 << 13
};

struct Config {
    Config() {
        for (int i = 0; i < (int)HudModuleId::Count; ++i) {
            modulePositions[i] = GetDefaultModulePosition((HudModuleId)i);
        }
    }
    // =========================================================================
    // [palette] Unified Design System & Color Tokens
    // =========================================================================
    CRGBA  palettePrimary           = { 211,  88, 137, 255 }; // #d35889 global.primary.background
    CRGBA  palettePrimaryForeground = {  16,  17,  24, 255 }; // #101118 global.primary.foreground
    CRGBA  paletteBackdrop          = {  14,  18,  24, 230 }; // Dark Acrylic Glass Frosted Backdrop
    CRGBA  paletteBorder            = { 205, 205, 205, 170 }; // High-contrast Sleek Silver Border
    CRGBA  paletteTextPrimary       = { 255, 255, 255, 255 }; // Pure White Primary Label Text
    CRGBA  paletteTextAccent        = { 244,  63, 142, 255 }; // Subtitle & Distance Accent Pink

    // =========================================================================
    // [Global] System & Keybind Settings
    // =========================================================================
    int    showHudKey               = 'J';                   // [KEYS] SHOW_HUD_KEY keyboard key to display hidden HUD elements
    int    showHudPad               = PADBTN_NONE;           // [KEYS] SHOW_HUD_PAD gamepad button for the same action
    int    gameUnit                 = 0;                     // 0 = Metric (km), 1 = Imperial (mi) (GAME_UNIT)
    int    gameClockFormat          = 1;                     // 0 = 12H_FORMAT ("7:29 am"), 1 = 24H_FORMAT ("20:00") (GAME_CLOCK_FORMAT)
    float  uiIdleTimeout            = 4.0f;                  // Global UI auto-hide timeout in seconds (UI_IDLE_TIMEOUT)

    // =========================================================================
    // [minimap.marker] Marker Colors
    // =========================================================================
    CRGBA  markerColors[5]          = {
        { 225,  30,  45, 255 }, // 0: Enemy / Attack (#E11E2D, BLIP_COLOUR_RED)
        {  46, 204, 113, 255 }, // 1: Friendly (#2ECC71, BLIP_COLOUR_GREEN)
        {  20,  80, 225, 255 }, // 2: Objective vehicle / Friendly ped (#1450E1, BLIP_COLOUR_BLUE)
        { 255, 255, 255, 255 }, // 3: Neutral destination (#FFFFFF, BLIP_COLOUR_WHITE)
        { 211,  88, 137, 255 }  // 4: Mission waypoint (#d35889, BLIP_COLOUR_YELLOW)
    };

    // =========================================================================
    // [radar] Radar Shell & Minimap Perspective
    // =========================================================================
    bool   enabled                  = true;                  // Enable / disable 6hud radar overlay (1 = on, 0 = off)
    float  radarSizeScale           = 1.0f;                  // Overall HUD radar size multiplier (1.0 = 100%, 1.2 = 120%)
    float  radarCornerRadius        = 6.5f;                  // Radar bezel corner radius percentage (0 = sharp rectangle, 6.5 = sleek modern, 18 = circular)
    float  borderThickness          = 1.0f;                  // Outer frame bezel stroke thickness in pixels
    CRGBA  borderColor              = { 205, 205, 205, 170 };// Radar frame bezel outline color (R, G, B, Alpha)
    int    mapStreamGrid            = 3;                     // 2D map streaming grid size (3 = 3x3 tiles, 4 = 4x4 tiles)
    float  fadeSpeed                = 0.03f;                 // Radar show / hide smooth fade speed (0.01 = slow, 0.03 = default, 0.10 = fast)
    bool   showOnFoot               = false;                 // Render radar when player is on foot (1 = on, 0 = off/hidden)
    bool   showInVehicle            = true;                  // Render radar when driving vehicle/car/bike/heli/boat (1 = on, 0 = off)
    bool   showInInterior           = true;                  // Render radar inside shops, casinos, interiors (1 = on, 0 = off)
    bool   showInTunnel             = true;                  // Render radar inside tunnels & underground (1 = on, 0 = off)
    bool   showWhenWanted           = true;                  // Force render radar when wanted level > 0 (1 = on, 0 = off)
    bool   showWhenGpsActive        = true;                  // Force render radar when destination waypoint is set (1 = on, 0 = off)
    bool   lockPlayerElevation      = true;                  // Lock player Z elevation to flat 2D plane (1 = locked flat, 0 = 3D relative)
    float  radarZoom                = 50.0f;                 // Base camera zoom distance in meters
    float  radarMaxZoom             = 70.0f;                 // Max dynamic camera zoom distance at high speed
    float  speedZoomThreshold       = 70.0f;                 // Vehicle speed threshold in km/h before dynamic zoom begins
    float  radarTiltDeg             = 35.0f;                 // Camera elevation angle in degrees (90 = top-down, 35 = 2.5D perspective)
    float  transitionSpeed          = 0.025f;                // Smooth camera zoom/mode interpolation speed (0.01 = slow, 0.05 = fast)
    bool   tiltPlayerBlip           = false;                 // Tilt player puck in 2.5D perspective with camera pitch (1 = on, 0 = off)
    int    peekKey                  = 'G';                   // [KEYS] MAP_PEEK_KEY keyboard key to hold for overview peek zoom-out
    int    peekPad                  = PADBTN_NONE;           // [KEYS] MAP_PEEK_PAD gamepad button for peek zoom-out
    float  peekZoomMult             = 1.5f;                  // Zoom range multiplier during peek mode (keeps same tilt angle)
    bool   enableTunnelZoom         = true;                  // Dynamic radar zoom in tunnels and underground (1 = on, 0 = off)
    float  tunnelZoomMult           = 0.65f;                 // Radar zoom multiplier inside tunnels (0.65 = 35% closer zoom)
    bool   enableAltitudeZoom       = true;                  // Dynamic altitude/height Z-position zoom (1 = on, 0 = off)
    float  altitudeMaxZoomMult      = 1.50f;                 // Maximum altitude zoom multiplier at high heights (e.g. 1.5x)
    float  altitudeMinHeight        = 20.0f;                 // Minimum player Z height in meters before altitude zoom begins
    float  altitudeMaxHeight        = 400.0f;                // Altitude in meters where maximum altitude zoom multiplier is reached
    float  radarOpacity             = 0.65f;                 // Radar master opacity multiplier (0.10 = 10%, 0.65 = 65% default, 1.00 = 100%)
    CRGBA  backdropColor            = { 14, 18, 24, 255 };   // Dark acrylic radar backdrop color (R, G, B)
    float  backdropOpacity          = 1.00f;                 // Radar background opacity (0.0 = fully transparent, 1.0 = solid)
    bool   useCustomMapTile         = true;                  // USE_CUSTOM_MAP_TILE (load custom png tiles from assets/radar/map if true; vanilla TXD if false)
    bool   useCustomPauseMap        = true;                  // USE_CUSTOM_PAUSE_MAP (also use custom png tiles on the ESC pause map; sea color fills transparent water)
    bool   fillEmptySea             = true;                  // Fill empty/unstreamed radar map bounds outside 12x12 grid with sea water color (1 = on, 0 = off)
    CRGBA  radarSeaColor            = { 134, 174, 191, 255 }; // #86aebf (minimap.radar.sea) - daytime water
    CRGBA  radarSeaColorNight       = { 92, 87, 149, 255 };   // #5c5795 (minimap.radar.sea_night) - night water
    float  closedSpaceZoomMultiplier= 0.9f;                  // CLOSED_SPACE_ZOOM_MULTIPLIER
    bool   experimentalShow3dObjects= false;                 // EXPERIMENTAL_SHOW_3D_OBJECTS
    CRGBA  color3dObjects           = { 201, 201, 196, 180 };// #c9c9c4 (minimap.radar.3d_objects)
    bool   enableBackdropBlur       = true;                  // Live D3D9 shader background blur behind radar (1 = on, 0 = off/acrylic tint)
    int    blurQuality              = 3;                     // Backdrop blur quality downscale factor (1 = Low 8x, 2 = Medium 4x, 3 = High 2x, 4 = Ultra 1x)
    float  blurIntensity            = 1.50f;                 // Backdrop blur radius intensity multiplier (0.5 = subtle, 1.5 = default, 3.0 = heavy)

    // Radar Blips & Icons
    bool   showPoliceFoot           = true;                  // Show police on foot blips on radar when wanted (1 = on, 0 = off)
    bool   showPoliceVehicle        = true;                  // Show police vehicle blips on radar when wanted (1 = on, 0 = off)
    float  blipScale                = 2.00f;                 // World icon sprites scale multiplier (CJ, shops, spray, etc.)
    float  targetBlipScale          = 1.10f;                 // Target destination waypoint blip scale multiplier
    float  missionBlipScale         = 1.00f;                 // Colored mission objective dots scale multiplier
    float  playerPuckScale          = 1.25f;                 // Player center navigation arrow puck scale multiplier
    float  northBadgeScale          = 1.15f;                 // North compass badge scale multiplier
    float  policePedScale           = 0.95f;                 // Cop on foot blip scale multiplier
    float  policeBikeScale          = 1.10f;                 // Cop motorcycle blip scale multiplier
    float  policeCarScale           = 1.35f;                 // Cop squad cruiser blip scale multiplier
    float  policeHeliTruckScale     = 1.50f;                 // Police helicopter, enforcer, SWAT, boat blip scale multiplier
    float  policeTankHydraScale     = 1.75f;                 // Military tank and Hydra jet blip scale multiplier
    bool   showPlayerPuckStroke     = true;                  // Outer stroke on player puck (1 = on, 0 = off)
    float  playerPuckStroke         = 0.5f;                  // Player puck stroke thickness in pixels
    CRGBA  playerPuckStrokeColor    = { 0, 0, 0, 220 };      // Player puck stroke color (default: black)
    CRGBA  playerPuckDiscColor      = { 0, 0, 0, 128 };      // Player puck circular disc background (default: black 50% opacity)
    CRGBA  playerPuckArrowColor     = { 255, 255, 255, 255 };// Player puck arrow pointer color (default: pure white)
    bool   showNorthBadgeStroke     = false;                 // Outer white stroke on north badge (1 = on, 0 = off)
    float  northBadgeStroke         = 0.5f;                  // North badge stroke thickness in pixels

    // =========================================================================
    // [gps] / [Minimap.Gps] Navigation & Waypoints
    // =========================================================================
    bool   enableGps                = true;                  // USE_RADAR_GPS (1 = on, 0 = off)
    bool   enableMenuMapGps         = true;                  // USE_MENU_MAP_GPS (1 = on, 0 = off)
    bool   enableGpsNavigator       = true;                  // USE_NAV_INDICATOR (1 = on, 0 = off)
    bool   useWalkingNode           = true;                  // USE_WALKING_NODE (1 = on, 0 = off)
    CRGBA  gpsColor                 = { 211,  88, 137, 240 }; // #d35889 Core GPS road ribbon fill
    CRGBA  gpsCasingColor           = { 230, 125, 168, 200 }; // Lighter primary stroke casing
    float  gpsLineWidth             = 6.5f;                  // GPS_LINE_WIDTH (pixels)
    float  gpsCasingWidth           = 1.2f;                  // GPS_LINE_STROKE_WIDTH (pixels)
    float  mapGpsLineWidth          = 12.0f;                 // ESC Menu map core road ribbon width in pixels
    float  mapGpsCasingWidth        = 2.0f;                  // ESC Menu map outer casing stroke thickness in pixels
    float  gpsMaskInner             = 12.0f;                 // Radial mask inner radius in meters (0% opacity around player)
    float  gpsMaskOuter             = 50.0f;                 // Radial mask outer radius in meters (100% opacity road fade)
    CRGBA  targetBlipColor          = { 211,  88, 137, 255 }; // Waypoint dot fill color (R, G, B, Alpha)
    bool   showTargetBlipStroke     = true;                  // Outer darker stroke on waypoint blip (1 = on, 0 = off)
    float  targetBlipStroke         = 1.0f;                  // Waypoint blip stroke thickness in pixels
    float  waypointReachRadius      = 9.9f;                  // Distance in meters to auto-clear destination waypoint on arrival
    bool   showGpsDistance          = true;                  // Show / hide remaining route distance text in radar (1 = on, 0 = off)
    int    distanceUnit             = 0;                     // Measurement unit: 0 = Metric (km), 1 = Imperial (mi)
    float  distanceTextScale        = 1.0f;                  // Distance text size multiplier (1.0 = 100%)
    float  distanceStrokeThickness  = 1.5f;                  // Distance text outline stroke thickness in pixels
    CRGBA  distanceTextColor        = { 255, 255, 255, 255 };// Distance text foreground color (R, G, B, Alpha)
    CRGBA  distanceStrokeColor      = { 0, 0, 0, 220 };      // 4-way stroke outline color (R, G, B, Alpha)

    // =========================================================================
    // [info_cards] Location & Vehicle Telemetry Cards
    // =========================================================================
    bool   showInfoCards            = true;                  // Master enable for GTA 6 stacked info cards (1 = on, 0 = off)
    bool   usePlaceInfo             = true;                  // USE_PLACE_INFO (1 = on, 0 = off)
    bool   useVehicleInfo           = true;                  // USE_VEHICLE_INFO (1 = on, 0 = off)
    bool   useVehicleFuel           = false;                 // USE_VEHICLE_FUEL (1 = on, 0 = off)
    bool   useVehicleFuelOnMission  = false;                 // USE_VEHICLE_FUEL_ON_MISSION (1 = on, 0 = off)
    float  vehicleFuelConsumption   = 1.0f;                  // VEHICLE_FUEL_CONSUMPTION rate multiplier (1.0 = default)
    float  vehicleFuelRngMin        = 0.15f;                 // VEHICLE_FUEL_RNG_MIN (15%)
    float  vehicleFuelRngMax        = 0.80f;                 // VEHICLE_FUEL_RNG_MAX (80%)
    int    infoHotkey               = 'J';                   // [KEYS] INFO_KEY keyboard hotkey to show/refresh info cards on demand
    int    infoPad                  = PADBTN_NONE;           // [KEYS] INFO_KEY_PAD gamepad button for the info-card refresh
    CRGBA  cardBackdropColor        = {  53,  49,  64, 230 }; // #353140 minimap.card.background
    CRGBA  cardDarkerColor          = {  34,  31,  41, 255 }; // #221f29 minimap.card.darker
    CRGBA  cardTextColor            = { 194, 190, 209, 255 }; // #c2bed1 minimap.card.foreground
    CRGBA  cardBorderColor          = {  84,  78, 102, 255 }; // #544e66 minimap.card.border
    float  cardBackdropOpacity      = 0.90f;                 // Info cards background opacity multiplier
    float  cardTransitionSpeed      = 0.08f;                 // Info cards slide / fade transition animation speed
    float  locationDuration         = 4.0f;                  // Location card display duration on entering a new area (seconds)
    float  vehicleNameDuration      = 4.0f;                  // Vehicle name banner display duration on entering vehicle (seconds)
    float  vehicleNameScale         = 1.0f;                  // Vehicle name text scale multiplier (1.0 = 100%)
    float  vehicleClassScale        = 1.0f;                  // Vehicle class subtitle text scale multiplier (1.0 = 100%)
    float  vehicleTextGap           = 1.0f;                  // Vehicle name to class vertical spacing multiplier (1.0 = 100%)
    float  gaugeScale               = 1.0f;                  // Circular vector gauges size scale multiplier (1.0 = 100%)
    float  gaugeSpacing             = 1.0f;                  // Circular vector gauges horizontal spacing multiplier (1.0 = 100%)
    bool   showGaugeWaypoint        = false;                 // Show waypoint destination progress gauge (1 = on, 0 = off)
    bool   showGaugeHealth          = true;                  // Show vehicle engine/health gauge (1 = on, 0 = off)
    bool   showGaugeFuel            = true;                  // Show vehicle fuel level gauge (1 = on, 0 = off)
    bool   showGaugeNitro           = true;                  // Show vehicle nitro boost gauge when nitro installed (1 = on, 0 = off)
    int    nitroGaugeType           = 0;                     // Nitro gauge style (0 = Vanilla with bottle badge, 1 = Controlled continuous bar)
    bool   enableControllableNitro  = false;                 // Enable hold-to-boost controllable nitrous gameplay (1 = on, 0 = off)
    CRGBA  gaugeWaypointColor       = { 255, 145, 30, 255 }; // Waypoint gauge accent color (default: orangish)
    CRGBA  gaugeFuelColor           = { 70, 215, 190, 255 }; // Fuel gauge accent color (default: mint-cyan)
    CRGBA  gaugeNitroColor          = { 140, 190, 255, 255 };// Nitro gauge accent color (default: silver-cyan)
    float  cardBorderThickness      = 1.0f;                  // Info cards bezel border stroke thickness in pixels

    // =========================================================================
    // [search_radius] / [Wanted.Extension.SearchArea]
    // =========================================================================
    bool   useWantedStar            = true;                  // [Wanted.Visual] USE_WANTED_STAR (1 = on, 0 = off)
    bool   useWantedSearchArea      = false;                 // [Wanted.Extension.SearchArea] USE_WANTED_SEARCH_AREA (default: 0)
    bool   useWantedSearchAreaOnMission = false;             // USE_WANTED_SEARCH_AREA_ON_MISSION (default: 0)
    float  extWantedSearchAreaZoom  = 1.25f;                 // EXT_WANTED_SEARCH_AREA_ZOOM (default: 1.25)
    float  extWantedSearchRadiusBase= 100.0f;                // EXT_WANTED_SEARCH_RADIUS_BASE (default: 100.0)
    float  extWantedSearchRadiusMultiplier = 230.0f;         // EXT_WANTED_SEARCH_RADIUS_MULTIPLIER_VALUE (default: 230.0)
    float  extWantedEscapeBaseDuration = 10.0f;              // EXT_WANTED_ESCAPE_BASE_DURATION (default: 10.0)
    float  extWantedEscapeMultiplier= 15.0f;                 // EXT_WANTED_ESCAPE_MULTIPLIER_VALUE (default: 15.0)

    bool   enableWantedSearchRadius = true;                  // Legacy master enable for search circle
    bool   useNewWantedSystem       = true;                  // Legacy master enable for wanted evasion & star system
    float  searchRadiusBase         = 100.0f;                // Base search radius in meters at 1-star wanted level
    float  searchRadiusPerStar      = 230.0f;                // Additional search radius meters per star level (scales to 1250m at 6 stars)
    float  wantedEscapeTime         = 10.0f;                 // Base evasion duration in seconds at 1-star wanted level
    float  wantedEscapeTimePerStar  = 15.0f;                 // Additional evasion duration seconds per star level
    bool   allowHideInSearchArea    = true;                  // Allow evasion countdown while hiding inside search area
    float  searchRadiusBlur         = 0.10f;                 // Inner search area edge blur feather (0.01 = sharp, 0.40 = soft glow)
    float  wantedDuration           = 1.5f;                  // Siren alternating flash cycle period in seconds
    CRGBA  wantedColorBlue          = { 20, 80, 225, 120 };  // Police siren blue tint color (R, G, B, Alpha)
    CRGBA  wantedColorRed           = { 225, 30, 45, 120 };  // Police siren red tint color (R, G, B, Alpha)

    // =========================================================================
    // [heat_area] / [Wanted.Extension.HeatArea]
    // =========================================================================
    bool   useWantedHeatArea        = false;                 // [Wanted.Extension.HeatArea] USE_WANTED_HEAT_AREA (default: 0)
    bool   useWantedHeatAreaOnMission = false;               // USE_WANTED_HEAT_AREA_ON_MISSION (default: 0)
    float  extWantedHeatAreaBaseDuration = 30.0f;            // EXT_WANTED_HEAT_AREA_BASE_DURATION (default: 30.0)
    float  extWantedHeatAreaDurationMultiplier = 15.0f;      // EXT_WANTED_HEAT_AREA_DURATION_MULTIPLIER (default: 15.0)
    float  extWantedHeatAreaZoneMultiplier = 1.50f;          // EXT_WANTED_HEAT_AREA_ZONE_MULTIPLIER (default: 1.50)
    CRGBA  wantedAreaHeatColor      = { 200, 30, 40, 90 };   // wanted.area.heat

    bool   enableHeatArea           = true;                  // Legacy master enable for outer heat area rendering
    bool   enableHeatSearchStage    = true;                  // 2-stage wide heat area evasion gameplay
    bool   showHeatOnMap            = true;                  // Draw search radius and heat area on pause ESC menu map
    float  heatRadiusScale          = 1.50f;                 // Outer heat radius scale (150% of search radius)
    float  heatRadiusOpacity        = 0.33f;                 // Outer heat aura maximum opacity
    float  heatRadiusBlur           = 0.80f;                 // Outer heat perimeter edge blur feather
    float  heatEscapeTime           = 30.0f;                 // Base stage-3 heat area evasion duration in seconds
    float  heatEscapeTimePerStar    = 15.0f;                 // Additional heat area evasion duration seconds per star level

    // =========================================================================
    // [cop_auras] / [Wanted.Extension.PoliceSirenIndicator]
    // =========================================================================
    bool   usePoliceSirenIndicator  = false;                 // [Wanted.Extension.PoliceSirenIndicator] USE_POLICE_SIREN_INDICATOR (default: 0)
    bool   usePoliceSirenIndicatorOnMission = false;         // USE_POLICE_SIREN_INDICATOR_ON_MISSION (default: 0)
    bool   extWantedPoliceSirenOnFoot = true;                // EXT_WANTED_POLICE_SIREN_INDICATOR_ON_FOOT (default: 1)
    float  extWantedPoliceSirenOnFootDist = 35.0f;           // EXT_WANTED_POLICE_SIREN_INDICATOR_ON_FOOT_DISTANCE (default: 35.0)
    float  extWantedPoliceSirenMinIntensity = 0.30f;         // EXT_WANTED_POLICE_SIREN_INDICATOR_MIN_INTENSITY (default: 0.30)
    float  extWantedPoliceSirenMaxIntensity = 1.00f;         // EXT_WANTED_POLICE_SIREN_INDICATOR_MAX_INTENSITY (default: 1.00)
    float  extWantedPoliceSirenMinSize = 0.70f;              // EXT_WANTED_POLICE_SIREN_INDICATOR_MIN_SIZE (default: 0.70)
    float  extWantedPoliceSirenMaxSize = 1.20f;              // EXT_WANTED_POLICE_SIREN_INDICATOR_MAX_SIZE (default: 1.20)
    float  extWantedPoliceSirenFarSpeedMult = 0.75f;         // EXT_WANTED_POLICE_SIREN_INDICATOR_FAR_SPEED_MULTIPLIER (default: 0.75)
    float  extWantedPoliceSirenNoDriverOpacity = 0.75f;      // EXT_WANTED_POLICE_SIRE_INDICATOR_NO_DRIVER_OPACITY (default: 0.75)
    bool   extWantedPoliceSirenRoadblock = true;             // EXT_WANTED_POLICE_SIREN_INDICATOR_ROADBLOCK (default: 1)
    bool   extWantedPoliceSirenHelicopter = true;            // EXT_WANTED_POLICE_SIREN_INDICATOR_HELICOPTER (default: 1)

    bool   enableCopAuras           = true;                  // Legacy master enable for screen edge vehicle lightbars
    bool   copAuraVehCull           = true;                  // Disappear when vehicle is fully on player screen
    float  copAuraVehCullMargin     = 0.08f;                 // Screen margin fraction for visibility culling
    float  copAuraFootPadX          = 0.45f;                 // On-foot circular radius fraction from center
    float  copAuraFootPadY          = 0.38f;                 // Reserved / unused
    float  copAuraFootAspectRatio   = 2.00f;                 // On-foot pod aspect ratio
    float  copAuraMaxDist           = 150.0f;                // Maximum world distance in meters
    float  copAuraIntensity         = 1.50f;                 // Emergency siren edge aura brightness multiplier
    float  copAuraBaseRotation      = 0.0f;                  // Base orientation rotation offset in degrees
    float  copAuraAspectRatio       = 1.66f;                 // In-vehicle pod aspect ratio
    float  copAuraVehHiddenPercent  = 0.55f;                 // Fraction of vehicle aura clipped by screen edge
    float  copAuraSize              = 2.75f;                 // Aura overall size scale multiplier
    float  copAuraBoundaryTop       = 0.00f;                 // Boundary margin from top edge
    float  copAuraBoundarySide      = 0.00f;                 // Boundary margin from side edges
    float  copAuraSquirclePower     = 7.00f;                 // Bottom squircle curvature power
    float  copAuraMinIntensity      = 0.15f;                 // Minimum intensity floor
    float  copAuraMaxIntensity      = 0.90f;                 // Maximum intensity ceiling
    float  copAuraStrobeSpeed       = 0.20f;                 // Siren alternating flash rate multiplier
    float  copAuraChromaticDepth    = 2.50f;                 // Transition overlap multiplier
    float  copAuraTurbulenceStrength= 5.00f;                 // Noise amplitude
    float  copAuraWhiteCoreStrength = 0.80f;                 // Hotspot intensity multiplier

    // =========================================================================
    // [cop_arcs] / [Wanted.Extension.PoliceArcIndicator]
    // =========================================================================
    bool   usePoliceArcIndicator    = false;                 // [Wanted.Extension.PoliceArcIndicator] USE_POLICE_ARC_INDICATOR (default: 0)
    bool   usePoliceArcIndicatorOnMission = false;           // USE_POLICE_ARC_INDICATOR_ON_MISSION (default: 0)
    float  extWantedPoliceArcDistance = 45.0f;               // EXT_WANTED_POLICE_ARC_INDICATOR_DISTANCE (default: 45.0)
    float  extWantedPoliceArcMaxDist = 70.0f;                // EXT_WANTED_POLICE_ARC_INDICATOR_MAX_DISTANCE (default: 70.0)
    float  extWantedPoliceArcMinDist = 15.0f;                // EXT_WANTED_POLICE_INDICATOR_MIN_DISTANCE (default: 15.0)
    CRGBA  wantedArcColor           = { 140, 190, 255, 255 };// wanted.arcs.color (#8cbeff)

    bool   enableCopArcs            = true;                  // Legacy master enable for on-foot directional threat arcs
    float  copArcMaxDist            = 70.0f;                 // Maximum world distance in meters
    float  copArcScale              = 1.0f;                  // Scale multiplier
    bool   copArcUseCircularPerimeter = true;                // Track on-foot circular perimeter matching aura

    // =========================================================================
    // [heat_identification] / [Wanted.Extension.CriminalProfile]
    // =========================================================================
    bool   useCriminalProfile       = false;                 // [Wanted.Extension.CriminalProfile] USE_CRIMINAL_PROFILE (default: 0)
    bool   useCriminalProfileOnMission = false;             // USE_CRIMINAL_PROFILE_ON_MISSION (default: 0)
    bool   extWantedChangePlateTransfender = false;          // EXT_WANTED_CRIMINAL_PROFILE_TUNING_CHANGE_PLATE (default: 0)
    bool   extWantedGunHeatDecay    = true;                  // EXT_WANTED_CRIMINAL_PROFILE_GUN_HEAT_DECAY (default: 1)
    std::string maskModelNames      = "bandmask, hockeymask, helmet, moto, bikerhelmet, zorromask, balaclava";
    std::string disguiseModelNames  = "grouchos, glasses01, glasses03, glasses04, glasses05";
    std::vector<unsigned int> maskModelKeys;
    std::vector<unsigned int> disguiseModelKeys;

    bool   enableHeatIdentification = true;                  // Master enable for 3-tier heat (Clothes, Vehicle, Gun)
    float  footCopScanDist          = 10.0f;                 // On-foot cop visual scanning range in meters (10m)
    float  footCopScanTime          = 3.5f;                  // Time in seconds for foot cop to confirm identity
    float  vehCopScanDist           = 25.0f;                 // Cruiser cop visual scanning range in meters (25m)
    float  vehCopScanTime           = 5.0f;                  // Time in seconds for cruiser cop to confirm identity
    float  gunBaseCooldown          = 90.0f;                 // Base gun heat cooldown in seconds out of combat
    float  gunCooldownPerStar       = 30.0f;                 // Additional gun cooldown seconds per wanted star
    bool   payNSprayRandomizePlate  = true;                  // Randomize license plate on Pay 'n' Spray respray
    int    payNSprayCost            = 500;                   // Custom Pay 'n' Spray price in dollars
    bool   transfenderPlateMenu     = true;                  // Native CMenuSystem license plate menu in TransFender
    int    plateModCost             = 150;                   // TransFender license plate customization fee

    // =========================================================================
    // [Wanted.Extension.SnitchSystem]
    // =========================================================================
    bool   useSnitchSystem          = false;                 // [Wanted.Extension.SnitchSystem] USE_SNITCH_SYSTEM (default: 0)
    bool   useSnitchSystemOnMission = false;                 // USE_SNITCH_SYSTEM_ON_MISSION (default: 0)
    float  extWantedSnitchMinDispatchTime = 20.0f;           // EXT_WANTED_SNITCH_MIN_POLICE_DISPATCH_TIMER (default: 20.0)
    float  extWantedSnitchMaxDispatchTime = 25.0f;           // EXT_WANTED_SNITCH_MAX_POLICE_DISPATCH_TIMER (default: 25.0)
    float  extWantedSnitchRadius    = 60.0f;                 // EXT_WANTED_SNITCH_RADIUS (default: 60.0)
    float  extWantedSnitchMinPanicRun = 50.0f;               // EXT_WANTED_SNITCH_MIN_PANIC_RUN (default: 50.0)
    float  extWantedSnitchMaxPanicRun = 150.0f;              // EXT_WANTED_SNITCH_MAX_PANIC_RUN (default: 150.0)

    // Heat cleanup and identification tuning
    int    heatCleanupMode          = 2;                     // 0=simple, 1=weighted, 2=hybrid
    bool   vehicleWeightingEnabled  = true;
    bool   clothingWeightingEnabled = false;
    bool   weaponWeightingEnabled   = false;
    int    vehicleCleanupThreshold  = 100;
    int    clothingCleanupThreshold = 100;
    int    weaponCleanupThreshold   = 100;
    int    vehicleWeightPlate       = 100;
    int    vehicleWeightPaintjob    = 60;
    int    vehicleWeightRespray     = 50;
    int    vehicleWeightMinorMod    = 15;
    int    clothingWeightFullOutfit = 100;
    int    clothingWeightPerSlot    = 20;
    int    weaponWeightPistol       = 20;
    int    weaponWeightSmg          = 30;
    int    weaponWeightShotgun      = 45;
    int    weaponWeightRifle        = 55;
    int    weaponWeightSniper       = 70;
    int    weaponWeightHeavy        = 80;
    int    weaponWeightExplosive    = 100;
    int    weaponWeightPerShot      = 1;
    int    weaponSwitchCleanupMode  = 1;                     // 0=preserve signature, 1=change signature, 2=clear GUN
    int    weaponSwitchCleanupWeight = 25;
    float  weaponHeatDecayPerSecond = 1.0f;
    int    debugMode                = 0;                     // 0=off, 1=basic, 2=heat transitions, 3=verbose
    bool   vehicleHeatAllowHistoryFallback = false;           // Model-only vehicle matches are disabled by default
    bool   vehicleHeatRequireCurrentVisual = true;             // Vehicle heat requires current police visual confirmation

    // =========================================================================
    // [MONEY] / [WEAPON] / [PLACE_INFO] / [VEH_INFO] / [GPS_NAV] Auto-hide
    // =========================================================================
    // When set, the element ignores UI_IDLE_TIMEOUT and never fades out.
    bool   alwaysShowMoney          = false;                 // [MONEY] ALWAYS_SHOW
    bool   alwaysShowWeapon         = false;                 // [WEAPON] ALWAYS_SHOW
    bool   alwaysShowPlaceInfo      = false;                 // [PLACE_INFO] ALWAYS_SHOW
    bool   alwaysShowVehInfo        = false;                 // [VEH_INFO] ALWAYS_SHOW

    // =========================================================================
    // [weapon] & [money] Top-Right HUD Options
    // =========================================================================
    bool   useWeaponUi              = true;                  // [weapon] USE_WEAPON_UI (1 = on, 0 = off)
    int    weaponTotalAmmoType      = 0;                     // [weapon] WEAPON_UI_TOTAL_AMMO_TYPE: 0 = TOTAL_AMMO, 1 = TOTAL_CLIP
    bool   useMoneyUi               = true;                  // [money] USE_MONEY_UI (1 = on, 0 = off)

    // =========================================================================
    // [weapon.ammo] & [money] Top-Right HUD Colors
    // =========================================================================
    CRGBA  weaponAmmoCurrentColor   = { 255, 255, 255, 255 }; // weapon.ammo.current (#FFFFFF)
    CRGBA  weaponAmmoTotalColor     = { 176, 176, 176, 255 }; // weapon.ammo.total (#B0B0B0)
    CRGBA  weaponAmmoStrokeColor    = {   0,   0,   0, 255 }; // weapon.ammo.stroke (#000000)
    CRGBA  moneyTextColor           = { 255, 255, 255, 255 }; // money.text (#FFFFFF)
    CRGBA  moneyStrokeColor         = {   0,   0,   0, 255 }; // money.stroke (#000000)
    CRGBA  moneyGaugeColor          = {  46, 204, 113, 255 }; // money.gauge (#2ECC71)

    // =========================================================================
    // [menu] In-Game Configuration UI
    // =========================================================================
    int    menuToggleKey            = 0xBF;                  // Virtual key code to open GUI menu (0xBF = '/' key, VK_OEM_2)

    // =========================================================================
    // [layout.<module>] Modular HUD Positioning
    // =========================================================================
    ModulePositionConfig modulePositions[(int)HudModuleId::Count];
};

extern Config g_config;
extern Config g_pendingConfig;

void LoadConfig();
void SaveConfig();
void ResetConfig();
void CheckHotReload();
void ApplyPendingConfig();
void RequestConfigReload();
void LogWrite(const char* fmt, ...);
unsigned int HashUppercaseKey(const char* str);

} // namespace hud6
