#include "Config.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sstream>
#include <CKeyGen.h>

namespace hud6 {

Config g_config;
Config g_pendingConfig;

static char s_logPath[MAX_PATH] = {};
static bool s_logInit = false;
static FILETIME s_lastWriteTime = {};
static bool s_reloadRequested = false;

static void InitLogPath() {
    if (s_logInit) return;
    char dir[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, dir, MAX_PATH);
    char* slash = strrchr(dir, '\\');
    if (slash) *slash = '\0';
    snprintf(s_logPath, sizeof(s_logPath), "%s\\6hud.log", dir);
    s_logInit = true;
    FILE* f = fopen(s_logPath, "w");
    if (f) {
        fprintf(f, "[6hud] initialized\n");
        fclose(f);
    }
}

void LogWrite(const char* fmt, ...) {
    InitLogPath();
    FILE* f = fopen(s_logPath, "a");
    if (!f) return;
    va_list args;
    va_start(args, fmt);
    fprintf(f, "[6hud] ");
    vfprintf(f, fmt, args);
    fprintf(f, "\n");
    va_end(args);
    fclose(f);
}

static void GetIniPath(char* outPath, size_t outSize) {
    char dir[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, dir, MAX_PATH);
    char* slash = strrchr(dir, '\\');
    if (slash) *slash = '\0';
    snprintf(outPath, outSize, "%s\\6hud.ini", dir);
}

static void GetColorsIniPath(char* outPath, size_t outSize) {
    char dir[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, dir, MAX_PATH);
    char* slash = strrchr(dir, '\\');
    if (slash) *slash = '\0';
    snprintf(outPath, outSize, "%s\\6hud\\colors.ini", dir);
}

static const unsigned int s_jamCrcTable[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

unsigned int HashUppercaseKey(const char* str) {
    if (!str) return 0;
    unsigned int hash = 0xFFFFFFFF;
    while (*str) {
        char c = *str++;
        if (c >= 'a' && c <= 'z') c -= ('a' - 'A');
        hash = s_jamCrcTable[(unsigned char)(hash ^ (unsigned char)c)] ^ (hash >> 8);
    }
    return hash;
}

static std::vector<unsigned int> ParseModelKeys(const std::string& csv) {
    std::vector<unsigned int> keys;
    std::stringstream ss(csv);
    std::string item;
    while (std::getline(ss, item, ',')) {
        size_t first = item.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        size_t last = item.find_last_not_of(" \t\r\n");
        std::string token = item.substr(first, (last - first + 1));
        if (!token.empty()) {
            keys.push_back(HashUppercaseKey(token.c_str()));
        }
    }
    return keys;
}

void LoadConfig() {
    Config activeConfig = g_config;
    char path[MAX_PATH] = {};
    GetIniPath(path, sizeof(path));
    char colorsPath[MAX_PATH] = {};
    GetColorsIniPath(colorsPath, sizeof(colorsPath));

    auto readStr = [&](const char* section, const char* key, const char* def) -> std::string {
        char s[128] = {};
        if (GetPrivateProfileStringA(section, key, "", s, sizeof(s), path) > 0) {
            return std::string(s);
        }
        if (section && strcmp(section, "radar") != 0) {
            if (GetPrivateProfileStringA("radar", key, "", s, sizeof(s), path) > 0) {
                return std::string(s);
            }
        }
        return std::string(def);
    };

    auto readInt = [&](const char* section, const char* key, int def) -> int {
        int v = GetPrivateProfileIntA(section, key, -999999, path);
        if (v != -999999) return v;
        if (section && strcmp(section, "radar") != 0) {
            v = GetPrivateProfileIntA("radar", key, -999999, path);
            if (v != -999999) return v;
        }
        return def;
    };

    auto readFloat = [&](const char* section, const char* key, float def) -> float {
        std::string s = readStr(section, key, "");
        if (!s.empty()) {
            return (float)atof(s.c_str());
        }
        return def;
    };

    auto readScale = [&](const char* section, const char* key, float defPct) -> float {
        float pct = readFloat(section, key, defPct);
        return pct > 10.0f ? (pct / 100.0f) : pct;
    };

    auto readColor = [&](const char* section, const char* key, CRGBA def) -> CRGBA {
        char sBuf[128] = {};
        std::string s = "";
        // First priority: 6hud/colors.ini
        if (GetPrivateProfileStringA(section, key, "", sBuf, sizeof(sBuf), colorsPath) > 0) {
            s = sBuf;
        }
        // Fallback: 6hud.ini
        if (s.empty()) {
            s = readStr(section, key, "");
        }
        if (!s.empty()) {
            if (s[0] == '#' || (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))) {
                const char* hexStr = (s[0] == '#') ? (s.c_str() + 1) : (s.c_str() + 2);
                unsigned int val = 0;
                if (sscanf(hexStr, "%x", &val) == 1) {
                    size_t hexLen = strlen(hexStr);
                    if (hexLen <= 6) {
                        return CRGBA((val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF, 255);
                    } else {
                        return CRGBA((val >> 24) & 0xFF, (val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
                    }
                }
            }
            int r = 0, g = 0, b = 0, a = 255;
            if (sscanf(s.c_str(), "%d , %d , %d , %d", &r, &g, &b, &a) >= 3) {
                return CRGBA((unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a);
            }
        }
        return def;
    };

    // =========================================================================
    // [palette] Design System & Color Tokens
    // =========================================================================
    g_config.palettePrimary      = readColor("palette", "primary_color", { 232, 74, 213, 255 });
    g_config.paletteBackdrop     = readColor("palette", "backdrop_color", { 14, 18, 24, 230 });
    g_config.paletteBorder       = readColor("palette", "border_color", { 205, 205, 205, 170 });
    g_config.paletteTextPrimary  = readColor("palette", "text_primary", { 255, 255, 255, 255 });
    g_config.paletteTextAccent   = readColor("palette", "text_accent", { 244, 63, 142, 255 });

    // =========================================================================
    // [Global] System & Keybind Settings
    // =========================================================================
    std::string hudKey = readStr("Global", "SHOW_HUD_KEY", "");
    if (!hudKey.empty()) {
        char k = hudKey[0];
        if (k >= 'a' && k <= 'z') k -= ('a' - 'A');
        g_config.showHudKey = (int)(unsigned char)k;
    } else {
        g_config.showHudKey = 'J';
    }

    std::string globalUnit = readStr("Global", "GAME_UNIT", "");
    if (!globalUnit.empty()) {
        g_config.gameUnit = (globalUnit[0] == 'I' || globalUnit[0] == 'i') ? 1 : 0;
    } else {
        std::string oldUnit = readStr("gps", "unit", "km");
        g_config.gameUnit = (oldUnit[0] == 'f' || oldUnit[0] == 'i' || (oldUnit[0] == 'm' && oldUnit[1] == 'i')) ? 1 : 0;
    }
    g_config.distanceUnit = g_config.gameUnit;

    g_config.uiIdleTimeout = readFloat("Global", "UI_IDLE_TIMEOUT", -999999.0f);
    if (g_config.uiIdleTimeout == -999999.0f) {
        g_config.uiIdleTimeout = readFloat("global", "idle_timeout", 4.0f);
    }
    if (g_config.uiIdleTimeout < 0.5f) g_config.uiIdleTimeout = 0.5f;

    // =========================================================================
    // [minimap.marker] Marker Colors
    // =========================================================================
    g_config.markerColors[0] = readColor("minimap.marker", "0", { 225,  30,  45, 255 });
    g_config.markerColors[1] = readColor("minimap.marker", "1", {  46, 204, 113, 255 });
    g_config.markerColors[2] = readColor("minimap.marker", "2", {  20,  80, 225, 255 });
    g_config.markerColors[3] = readColor("minimap.marker", "3", { 255, 255, 255, 255 });
    g_config.markerColors[4] = readColor("minimap.marker", "4", { 211,  88, 137, 255 });

    // =========================================================================
    // [radar] / [Minimap.Radar]
    // =========================================================================
    int radarEnabled = readInt("Minimap.Radar", "USE_RADAR", -999999);
    if (radarEnabled == -999999) radarEnabled = readInt("radar", "enabled", 1);
    g_config.enabled             = (radarEnabled != 0);

    std::string clockFmt = readStr("Minimap.Radar", "GAME_CLOCK_FORMAT", "");
    if (!clockFmt.empty()) {
        g_config.gameClockFormat = (clockFmt.find("12") != std::string::npos) ? 0 : 1;
    } else {
        g_config.gameClockFormat = 1; // 24H_FORMAT
    }

    float radarScale = readFloat("Minimap.Radar", "RADAR_SIZE_SCALE", -999999.0f);
    if (radarScale < -900000.0f) radarScale = readFloat("radar", "radar_size_scale", 0.90f);
    g_config.radarSizeScale      = radarScale;
    g_config.radarCornerRadius   = readFloat("radar", "radar_corner_radius", 6.5f);
    if (g_config.radarCornerRadius < 0.0f) g_config.radarCornerRadius = 0.0f;
    if (g_config.radarCornerRadius > 50.0f) g_config.radarCornerRadius = 50.0f;

    g_config.borderThickness     = readFloat("radar", "border_thickness", 1.0f);
    g_config.borderColor         = readColor("radar", "border_color", g_config.paletteBorder);
    g_config.mapStreamGrid       = readInt("radar", "map_stream_grid", 3);
    if (g_config.mapStreamGrid < 1) g_config.mapStreamGrid = 1;
    if (g_config.mapStreamGrid > 6) g_config.mapStreamGrid = 6;

    g_config.fadeSpeed           = readFloat("radar", "fade_speed", 0.03f);

    int radarOnFoot = readInt("Minimap.Radar", "SHOW_ON_FOOT", -999999);
    if (radarOnFoot == -999999) radarOnFoot = readInt("radar", "show_on_foot", 0);
    g_config.showOnFoot          = (radarOnFoot != 0);
    g_config.showInVehicle       = readInt("radar", "show_in_vehicle", 1) != 0;
    g_config.showInInterior      = readInt("radar", "show_in_interior", 1) != 0;
    g_config.showInTunnel        = readInt("radar", "show_in_tunnel", 1) != 0;
    g_config.showWhenWanted      = readInt("radar", "show_when_wanted", 1) != 0;
    g_config.showWhenGpsActive   = readInt("radar", "show_when_gps_active", 1) != 0;

    int lockElevation = readInt("Minimap.Radar", "LOCK_PLAYER_ELEVATION", -999999);
    if (lockElevation == -999999) lockElevation = readInt("radar", "lock_player_elevation", 1);
    g_config.lockPlayerElevation = (lockElevation != 0);

    g_config.radarZoom           = readFloat("radar", "radar_zoom", 50.0f);
    g_config.radarMaxZoom        = readFloat("radar", "radar_max_zoom", 70.0f);
    g_config.speedZoomThreshold  = readFloat("radar", "speed_zoom_threshold", 70.0f);
    g_config.radarTiltDeg        = readFloat("radar", "radar_tilt", 35.0f);
    g_config.transitionSpeed     = readFloat("radar", "transition_speed", 0.025f);
    g_config.tiltPlayerBlip      = readInt("radar", "tilt_player_blip", 0) != 0;

    std::string pKey = readStr("Minimap.Radar", "RADAR_ZOOM_OUT_KEY", "");
    if (pKey.empty()) pKey = readStr("radar", "peek_key", "G");
    if (!pKey.empty()) {
        char k = pKey[0];
        if (k >= 'a' && k <= 'z') k -= ('a' - 'A');
        g_config.peekKey = (int)(unsigned char)k;
    }
    float zoomMult = readFloat("Minimap.Radar", "RADAR_ZOOM_MULTIPLIER", -999999.0f);
    if (zoomMult < 0.0f) zoomMult = readFloat("radar", "peek_zoom_mult", 1.5f);
    g_config.peekZoomMult        = zoomMult;

    g_config.closedSpaceZoomMultiplier = readFloat("Minimap.Radar", "CLOSED_SPACE_ZOOM_MULTIPLIER", 0.9f);
    g_config.enableTunnelZoom    = readInt("radar", "enable_tunnel_zoom", 1) != 0;
    g_config.tunnelZoomMult      = readFloat("radar", "tunnel_zoom_mult", 0.65f);

    int useAltZoom = readInt("Minimap.Radar", "USE_ALTITUDE_ZOOM", -999999);
    if (useAltZoom == -999999) useAltZoom = readInt("radar", "enable_altitude_zoom", 0);
    g_config.enableAltitudeZoom  = (useAltZoom != 0);

    float altMult = readFloat("Minimap.Radar", "ALTITUDE_ZOOM_MULTIPLIER", -999999.0f);
    if (altMult < 0.0f) altMult = readFloat("radar", "altitude_max_zoom_mult", 1.50f);
    g_config.altitudeMaxZoomMult = altMult;

    float minAlt = readFloat("Minimap.Radar", "MIN_ALTITUDE", -999999.0f);
    if (minAlt < 0.0f) minAlt = readFloat("radar", "altitude_min_height", 20.0f);
    g_config.altitudeMinHeight   = minAlt;

    float maxAlt = readFloat("Minimap.Radar", "MAX_ALTITUDE", -999999.0f);
    if (maxAlt < 0.0f) maxAlt = readFloat("radar", "altitude_max_height", 250.0f);
    g_config.altitudeMaxHeight   = maxAlt;

    g_config.radarSeaColor       = readColor("minimap.radar", "sea", { 134, 174, 191, 255 });
    g_config.radarSeaColorNight  = readColor("minimap.radar", "sea_night", { 92, 87, 149, 255 });
    g_config.experimentalShow3dObjects = readInt("Minimap.Radar", "EXPERIMENTAL_SHOW_3D_OBJECTS", 0) != 0;
    g_config.color3dObjects      = readColor("minimap.radar", "3d_objects", { 201, 201, 196, 180 });

    g_config.backdropColor       = readColor("radar", "backdrop_color", g_config.paletteBackdrop);
    g_config.backdropOpacity     = readFloat("radar", "backdrop_opacity", 100.0f);
    if (g_config.backdropOpacity > 1.0f) g_config.backdropOpacity /= 100.0f;
    g_config.radarOpacity        = readFloat("radar", "radar_opacity", 65.0f);
    float radOpacity = readFloat("Minimap.Radar", "RADAR_OPACITY", -999999.0f);
    if (radOpacity >= 0.0f) g_config.radarOpacity = radOpacity;
    if (g_config.radarOpacity > 1.0f) g_config.radarOpacity /= 100.0f;
    if (g_config.radarOpacity < 0.1f) g_config.radarOpacity = 0.10f;
    g_config.fillEmptySea        = readInt("radar", "fill_empty_sea", 1) != 0;
    int customTileVal = readInt("Minimap.Radar", "USE_CUSTOM_MAP_TILE", -999999);
    if (customTileVal == -999999) customTileVal = readInt("radar", "use_custom_map_tile", 1);
    g_config.useCustomMapTile    = (customTileVal != 0);
    g_config.useCustomPauseMap   = readInt("radar", "use_custom_pause_map", g_config.useCustomMapTile ? 1 : 0) != 0;
    g_config.enableBackdropBlur  = readInt("radar", "enable_backdrop_blur", 1) != 0;
    int bq = readInt("Global", "BACKDROP_BLUR_QUALITY", -999999);
    if (bq == -999999) bq = readInt("radar", "blur_quality", 3);
    if (bq < 1) bq = 1;
    if (bq > 4) bq = 4;
    g_config.blurQuality         = bq;

    float bi = readFloat("Global", "BACKDROP_BLUR_INTENSITY", -999999.0f);
    if (bi < -900000.0f) bi = readFloat("radar", "blur_intensity", 1.50f);
    g_config.blurIntensity       = bi;

    // Radar Blips
    g_config.showPoliceFoot      = readInt("radar", "show_police_foot", 1) != 0;
    g_config.showPoliceVehicle   = readInt("radar", "show_police_vehicle", 1) != 0;
    g_config.blipScale           = readScale("radar", "blip_scale", 200.0f);
    g_config.targetBlipScale     = readScale("radar", "target_blip_scale", 110.0f);
    g_config.missionBlipScale    = readScale("radar", "mission_blip_scale", 100.0f);
    g_config.playerPuckScale     = readScale("radar", "player_puck_scale", 125.0f);
    g_config.northBadgeScale     = readScale("radar", "north_badge_scale", 115.0f);
    g_config.policePedScale      = readScale("radar", "police_ped_scale", 95.0f);
    g_config.policeBikeScale     = readScale("radar", "police_bike_scale", 110.0f);
    g_config.policeCarScale      = readScale("radar", "police_car_scale", 135.0f);
    g_config.policeHeliTruckScale= readScale("radar", "police_heli_truck_scale", 150.0f);
    g_config.policeTankHydraScale= readScale("radar", "police_tank_hydra_scale", 175.0f);
    g_config.showPlayerPuckStroke= readInt("radar", "show_player_puck_stroke", 1) != 0;
    g_config.playerPuckStroke    = readFloat("radar", "player_puck_stroke", 0.5f);
    g_config.playerPuckStrokeColor = readColor("radar", "player_puck_stroke_color", { 0, 0, 0, 220 });
    g_config.playerPuckDiscColor   = readColor("radar", "player_puck_disc_color", { 0, 0, 0, 128 });
    g_config.playerPuckArrowColor  = readColor("radar", "player_puck_arrow_color", g_config.paletteTextPrimary);
    g_config.showNorthBadgeStroke= readInt("radar", "show_north_badge_stroke", 0) != 0;
    g_config.northBadgeStroke    = readFloat("radar", "north_badge_stroke", 0.5f);

    // =========================================================================
    // [gps] / [Minimap.GPSNavigation] / [Minimap.Gps]
    // =========================================================================
    int radarGpsVal = readInt("Minimap.GPSNavigation", "USE_RADAR_GPS", -999999);
    if (radarGpsVal == -999999) radarGpsVal = readInt("Minimap.Gps", "USE_RADAR_GPS", -999999);
    if (radarGpsVal == -999999) radarGpsVal = readInt("gps", "enabled", 1);
    g_config.enableGps           = (radarGpsVal != 0);

    int menuMapGpsVal = readInt("Minimap.GPSNavigation", "USE_MENU_MAP_GPS", -999999);
    if (menuMapGpsVal == -999999) menuMapGpsVal = readInt("Minimap.Gps", "USE_MENU_MAP_GPS", -999999);
    if (menuMapGpsVal == -999999) menuMapGpsVal = readInt("gps", "enable_menu_map", 1);
    g_config.enableMenuMapGps    = (menuMapGpsVal != 0);

    int navIndicatorVal = readInt("Minimap.GPSNavigation", "USE_NAV_INDICATOR", -999999);
    if (navIndicatorVal == -999999) navIndicatorVal = readInt("Minimap.Gps", "USE_NAV_INDICATOR", -999999);
    if (navIndicatorVal == -999999) navIndicatorVal = readInt("gps", "enable_navigator", 1);
    g_config.enableGpsNavigator  = (navIndicatorVal != 0);

    int walkNodeVal = readInt("Minimap.GPSNavigation", "USE_WALKING_NODE", -999999);
    if (walkNodeVal == -999999) walkNodeVal = readInt("Minimap.Gps", "USE_WALKING_NODE", -999999);
    if (walkNodeVal == -999999) walkNodeVal = readInt("gps", "use_walking_node", 1);
    g_config.useWalkingNode      = (walkNodeVal != 0);

    CRGBA gpsRibbonDefault       = { g_config.palettePrimary.r, g_config.palettePrimary.g, g_config.palettePrimary.b, 240 };
    g_config.gpsColor            = readColor("gps", "gps_color", gpsRibbonDefault);
    g_config.gpsCasingColor      = readColor("gps", "gps_casing_color", { 230, 125, 168, 200 });

    float lineWidthVal = readFloat("Minimap.GPSNavigation", "GPS_LINE_WIDTH", -999999.0f);
    if (lineWidthVal < 0.0f) lineWidthVal = readFloat("Minimap.Gps", "GPS_LINE_WIDTH", -999999.0f);
    if (lineWidthVal < 0.0f) lineWidthVal = readFloat("gps", "gps_line_width", 6.5f);
    g_config.gpsLineWidth        = lineWidthVal;

    float lineStrokeVal = readFloat("Minimap.GPSNavigation", "GPS_LINE_STROKE_WIDTH", -999999.0f);
    if (lineStrokeVal < 0.0f) lineStrokeVal = readFloat("Minimap.Gps", "GPS_LINE_STROKE_WIDTH", -999999.0f);
    if (lineStrokeVal < 0.0f) lineStrokeVal = readFloat("gps", "gps_casing_width", 1.2f);
    g_config.gpsCasingWidth      = lineStrokeVal;

    g_config.mapGpsLineWidth     = readFloat("gps", "map_gps_line_width", 12.0f);
    g_config.mapGpsCasingWidth   = readFloat("gps", "map_gps_casing_width", 2.0f);
    g_config.gpsMaskInner        = readFloat("gps", "gps_mask_inner", 12.0f);
    g_config.gpsMaskOuter        = readFloat("gps", "gps_mask_outer", 50.0f);
    g_config.targetBlipColor     = readColor("gps", "target_blip_color", g_config.palettePrimary);
    g_config.showTargetBlipStroke= readInt("gps", "show_target_blip_stroke", 1) != 0;
    g_config.targetBlipStroke    = readFloat("gps", "target_blip_stroke", 1.0f);
    g_config.waypointReachRadius = readFloat("gps", "waypoint_reach_radius", 9.9f);
    g_config.showGpsDistance     = readInt("gps", "show_gps_distance", 1) != 0;

    std::string unitStr = readStr("gps", "unit", "km");
    g_config.distanceUnit        = (unitStr[0] == 'f' || unitStr[0] == 'i' || (unitStr[0] == 'm' && unitStr[1] == 'i')) ? 1 : 0;
    g_config.distanceTextScale   = readScale("gps", "distance_text_scale", 100.0f);
    g_config.distanceStrokeThickness = readFloat("gps", "distance_stroke_thickness", 1.5f);
    g_config.distanceTextColor   = readColor("gps", "distance_text_color", g_config.paletteTextPrimary);
    g_config.distanceStrokeColor = readColor("gps", "distance_stroke_color", { 0, 0, 0, 220 });

    // =========================================================================
    // [info_cards] / [minimap.card] / [Minimap.PlaceInformation] / [Minimap.VehicleInformation] / [Minimap.Extension.VehicleFuel]
    // =========================================================================
    g_config.showInfoCards       = readInt("info_cards", "enabled", 1) != 0;
    int placeInfoVal = readInt("Minimap.PlaceInformation", "USE_PLACE_INFO", -999999);
    if (placeInfoVal == -999999) placeInfoVal = readInt("Minimap.Place", "USE_PLACE_INFO", -999999);
    if (placeInfoVal == -999999) placeInfoVal = readInt("info_cards", "use_place_info", 1);
    g_config.usePlaceInfo        = (placeInfoVal != 0);

    int vehInfoVal = readInt("Minimap.VehicleInformation", "USE_VEHICLE_INFO", -999999);
    if (vehInfoVal == -999999) vehInfoVal = readInt("Minimap.Vehicle", "USE_VEHICLE_INFO", -999999);
    if (vehInfoVal == -999999) vehInfoVal = readInt("info_cards", "use_vehicle_info", 1);
    g_config.useVehicleInfo      = (vehInfoVal != 0);

    int fuelVal = readInt("Minimap.Extension.VehicleFuel", "USE_VEHICLE_FUEL", -999999);
    if (fuelVal == -999999) fuelVal = readInt("Minimap.Vehicle", "USE_VEHICLE_FUEL", -999999);
    if (fuelVal == -999999) fuelVal = readInt("info_cards", "use_vehicle_fuel", 0);
    g_config.useVehicleFuel      = (fuelVal != 0);

    int fuelMissionVal = readInt("Minimap.Extension.VehicleFuel", "USE_VEHICLE_FUEL_ON_MISSION", -999999);
    if (fuelMissionVal == -999999) fuelMissionVal = readInt("Minimap.Vehicle", "USE_VEHICLE_FUEL_ON_MISSION", -999999);
    if (fuelMissionVal == -999999) fuelMissionVal = readInt("info_cards", "use_vehicle_fuel_on_mission", 0);
    g_config.useVehicleFuelOnMission = (fuelMissionVal != 0);

    g_config.vehicleFuelConsumption = readFloat("Minimap.Extension.VehicleFuel", "VEHICLE_FUEL_CONSUMPTION",
        readFloat("Minimap.Vehicle", "VEHICLE_FUEL_CONSUMPTION", readFloat("info_cards", "vehicle_fuel_consumption", 1.0f)));
    g_config.vehicleFuelRngMin   = readFloat("Minimap.Extension.VehicleFuel", "VEHICLE_FUEL_RNG_MIN",
        readFloat("Minimap.Vehicle", "VEHICLE_FUEL_RNG_MIN", readFloat("info_cards", "vehicle_fuel_rng_min", 0.15f)));
    g_config.vehicleFuelRngMax   = readFloat("Minimap.Extension.VehicleFuel", "VEHICLE_FUEL_RNG_MAX",
        readFloat("Minimap.Vehicle", "VEHICLE_FUEL_RNG_MAX", readFloat("info_cards", "vehicle_fuel_rng_max", 0.80f)));

    std::string iKey = readStr("info_cards", "info_hotkey", "J");
    if (!iKey.empty()) {
        char k = iKey[0];
        if (k >= 'a' && k <= 'z') k -= ('a' - 'A');
        g_config.infoHotkey = (int)(unsigned char)k;
    }
    g_config.cardBackdropColor   = readColor("minimap.card", "background", readColor("info_cards", "card_backdrop_color", { 53, 49, 64, 230 }));
    g_config.cardDarkerColor     = readColor("minimap.card", "darker", { 34, 31, 41, 255 });
    g_config.cardTextColor       = readColor("minimap.card", "foreground", readColor("info_cards", "text_color", { 194, 190, 209, 255 }));
    g_config.cardBorderColor     = readColor("minimap.card", "border", readColor("info_cards", "card_border_color", { 84, 78, 102, 255 }));
    g_config.cardBackdropOpacity = readFloat("info_cards", "card_backdrop_opacity", 90.0f);
    if (g_config.cardBackdropOpacity > 1.0f) g_config.cardBackdropOpacity /= 100.0f;
    g_config.cardTransitionSpeed = readFloat("info_cards", "card_transition_speed", 0.08f);
    float locDur = readFloat("Minimap.PlaceInformation", "PLACE_INFO_DURATION", -999999.0f);
    if (locDur < -900000.0f) locDur = readFloat("info_cards", "location_duration", g_config.uiIdleTimeout);
    g_config.locationDuration    = locDur;

    float vehDur = readFloat("Minimap.VehicleInformation", "VEHICLE_INFO_DURATION", -999999.0f);
    if (vehDur < -900000.0f) vehDur = readFloat("info_cards", "vehicle_name_duration", g_config.uiIdleTimeout);
    g_config.vehicleNameDuration = vehDur;
    g_config.vehicleNameScale    = readScale("info_cards", "vehicle_name_scale", 100.0f);
    g_config.vehicleClassScale   = readScale("info_cards", "vehicle_class_scale", 100.0f);
    g_config.vehicleTextGap      = readScale("info_cards", "vehicle_text_gap", 100.0f);
    g_config.gaugeScale          = readScale("info_cards", "gauge_scale", 100.0f);
    g_config.gaugeSpacing        = readScale("info_cards", "gauge_spacing", 100.0f);
    g_config.showGaugeWaypoint   = readInt("info_cards", "show_gauge_waypoint", 0) != 0;
    g_config.showGaugeHealth     = readInt("info_cards", "show_gauge_health", 1) != 0;
    g_config.showGaugeFuel       = readInt("info_cards", "show_gauge_fuel", 1) != 0;
    g_config.showGaugeNitro      = readInt("info_cards", "show_gauge_nitro", 1) != 0;
    g_config.nitroGaugeType      = readInt("info_cards", "nitro_gauge_type", 0);
    g_config.enableControllableNitro = readInt("info_cards", "enable_controllable_nitro", 0) != 0;
    g_config.gaugeWaypointColor  = readColor("info_cards", "gauge_waypoint_color", { 255, 145, 30, 255 });
    g_config.gaugeFuelColor      = readColor("info_cards", "gauge_fuel_color", { 70, 215, 190, 255 });
    g_config.gaugeNitroColor     = readColor("info_cards", "gauge_nitro_color", { 140, 190, 255, 255 });
    g_config.cardBorderThickness = readFloat("info_cards", "card_border_thickness", 1.0f);

    // =========================================================================
    // [search_radius]
    // =========================================================================
    g_config.enableWantedSearchRadius = readInt("search_radius", "enabled", 1) != 0;
    g_config.useNewWantedSystem  = readInt("search_radius", "use_new_wanted_system", 1) != 0;
    g_config.searchRadiusBase    = readFloat("search_radius", "radius_base", 50.0f);
    g_config.searchRadiusPerStar = readFloat("search_radius", "radius_per_star", 90.0f);
    g_config.wantedEscapeTime    = readFloat("search_radius", "escape_time", 6.0f);
    g_config.wantedEscapeTimePerStar = readFloat("search_radius", "escape_time_per_star", 5.0f);
    // =========================================================================
    // [Wanted.Visual]
    // =========================================================================
    int wantedStarVal = readInt("Wanted.Visual", "USE_WANTED_STAR", -999999);
    if (wantedStarVal == -999999) wantedStarVal = readInt("search_radius", "use_new_wanted_system", 1);
    g_config.useWantedStar = (wantedStarVal != 0);
    g_config.useNewWantedSystem = g_config.useWantedStar;

    // =========================================================================
    // [search_radius] / [Wanted.Extension.SearchArea]
    // =========================================================================
    int searchAreaVal = readInt("Wanted.Extension.SearchArea", "USE_WANTED_SEARCH_AREA", -999999);
    if (searchAreaVal == -999999) searchAreaVal = readInt("search_radius", "enabled", 1);
    g_config.useWantedSearchArea = (searchAreaVal != 0);
    g_config.enableWantedSearchRadius = g_config.useWantedSearchArea;

    g_config.useWantedSearchAreaOnMission = readInt("Wanted.Extension.SearchArea", "USE_WANTED_SEARCH_AREA_ON_MISSION", 0) != 0;
    g_config.extWantedSearchAreaZoom = readFloat("Wanted.Extension.SearchArea", "EXT_WANTED_SEARCH_AREA_ZOOM", 1.25f);
    g_config.extWantedSearchRadiusBase = readFloat("Wanted.Extension.SearchArea", "EXT_WANTED_SEARCH_RADIUS_BASE", readFloat("search_radius", "radius_base", 100.0f));
    g_config.extWantedSearchRadiusMultiplier = readFloat("Wanted.Extension.SearchArea", "EXT_WANTED_SEARCH_RADIUS_MULTIPLIER_VALUE", readFloat("search_radius", "radius_per_star", 230.0f));
    g_config.extWantedEscapeBaseDuration = readFloat("Wanted.Extension.SearchArea", "EXT_WANTED_ESCAPE_BASE_DURATION", readFloat("search_radius", "escape_time", 10.0f));
    g_config.extWantedEscapeMultiplier = readFloat("Wanted.Extension.SearchArea", "EXT_WANTED_ESCAPE_MULTIPLIER_VALUE", readFloat("search_radius", "escape_time_per_star", 15.0f));

    g_config.searchRadiusBase    = g_config.extWantedSearchRadiusBase;
    g_config.searchRadiusPerStar = g_config.extWantedSearchRadiusMultiplier;
    g_config.wantedEscapeTime    = g_config.extWantedEscapeBaseDuration;
    g_config.wantedEscapeTimePerStar = g_config.extWantedEscapeMultiplier;
    g_config.allowHideInSearchArea = readInt("search_radius", "allow_hide_in_search", 1) != 0;
    g_config.searchRadiusBlur    = readFloat("search_radius", "search_blur", 10.0f);
    if (g_config.searchRadiusBlur > 1.0f) g_config.searchRadiusBlur /= 100.0f;
    g_config.wantedDuration      = readFloat("search_radius", "wanted_duration", 1.5f);
    g_config.wantedColorBlue     = readColor("wanted.area", "blue", readColor("search_radius", "wanted_color_blue", { 20, 80, 225, 120 }));
    g_config.wantedColorRed      = readColor("wanted.area", "red", readColor("search_radius", "wanted_color_red", { 225, 30, 45, 120 }));

    // =========================================================================
    // [heat_area] / [Wanted.Extension.HeatArea]
    // =========================================================================
    int heatAreaVal = readInt("Wanted.Extension.HeatArea", "USE_WANTED_HEAT_AREA", -999999);
    if (heatAreaVal == -999999) heatAreaVal = readInt("heat_area", "enabled", 1);
    g_config.useWantedHeatArea   = (heatAreaVal != 0);
    g_config.enableHeatArea      = g_config.useWantedHeatArea;

    g_config.useWantedHeatAreaOnMission = readInt("Wanted.Extension.HeatArea", "USE_WANTED_HEAT_AREA_ON_MISSION", 0) != 0;
    g_config.extWantedHeatAreaBaseDuration = readFloat("Wanted.Extension.HeatArea", "EXT_WANTED_HEAT_AREA_BASE_DURATION", readFloat("heat_area", "heat_escape_time", 30.0f));
    g_config.extWantedHeatAreaDurationMultiplier = readFloat("Wanted.Extension.HeatArea", "EXT_WANTED_HEAT_AREA_DURATION_MULTIPLIER", readFloat("heat_area", "heat_escape_time_per_star", 15.0f));
    g_config.extWantedHeatAreaZoneMultiplier = readFloat("Wanted.Extension.HeatArea", "EXT_WANTED_HEAT_AREA_ZONE_MULTIPLIER", 1.50f);
    g_config.wantedAreaHeatColor = readColor("wanted.area", "heat", { 200, 30, 40, 90 });

    g_config.enableHeatSearchStage = readInt("heat_area", "enable_heat_search_stage", 1) != 0;
    g_config.showHeatOnMap       = readInt("heat_area", "show_heat_on_map", 1) != 0;
    g_config.heatRadiusScale     = readFloat("heat_area", "heat_radius_scale", 1.50f);
    g_config.heatRadiusOpacity   = readFloat("heat_area", "heat_radius_opacity", 33.0f);
    if (g_config.heatRadiusOpacity > 1.0f) g_config.heatRadiusOpacity /= 100.0f;
    g_config.heatRadiusBlur      = readFloat("heat_area", "heat_radius_blur", 80.0f);
    if (g_config.heatRadiusBlur > 1.0f) g_config.heatRadiusBlur /= 100.0f;
    g_config.heatEscapeTime      = g_config.extWantedHeatAreaBaseDuration;
    g_config.heatEscapeTimePerStar = g_config.extWantedHeatAreaDurationMultiplier;

    // =========================================================================
    // [cop_auras] / [Wanted.Extension.PoliceSirenIndicator]
    // =========================================================================
    int sirenVal = readInt("Wanted.Extension.PoliceSirenIndicator", "USE_POLICE_SIREN_INDICATOR", -999999);
    if (sirenVal == -999999) sirenVal = readInt("cop_auras", "enabled", 1);
    g_config.usePoliceSirenIndicator = (sirenVal != 0);
    g_config.enableCopAuras      = g_config.usePoliceSirenIndicator;

    g_config.usePoliceSirenIndicatorOnMission = readInt("Wanted.Extension.PoliceSirenIndicator", "USE_POLICE_SIREN_INDICATOR_ON_MISSION", 0) != 0;
    g_config.extWantedPoliceSirenOnFoot = readInt("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_ON_FOOT", 1) != 0;
    g_config.extWantedPoliceSirenOnFootDist = readFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_ON_FOOT_DISTANCE", 35.0f);
    g_config.extWantedPoliceSirenMinIntensity = readFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_MIN_INTENSITY", 0.30f);
    g_config.extWantedPoliceSirenMaxIntensity = readFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_MAX_INTENSITY", 1.00f);
    g_config.extWantedPoliceSirenMinSize = readFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_MIN_SIZE", 0.70f);
    g_config.extWantedPoliceSirenMaxSize = readFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_MAX_SIZE", 1.20f);
    g_config.extWantedPoliceSirenFarSpeedMult = readFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_FAR_SPEED_MULTIPLIER", 0.75f);
    g_config.extWantedPoliceSirenNoDriverOpacity = readFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_NO_DRIVER_OPACITY",
        readFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIRE_INDICATOR_NO_DRIVER_OPACITY", 0.75f));
    g_config.extWantedPoliceSirenRoadblock = readInt("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_ROADBLOCK", 1) != 0;
    g_config.extWantedPoliceSirenHelicopter = readInt("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_HELICOPTER", 1) != 0;

    g_config.copAuraVehCull      = readInt("cop_auras", "veh_cull_on_screen", 1) != 0;
    g_config.copAuraVehCullMargin= readFloat("cop_auras", "veh_cull_margin", 8.0f);
    if (g_config.copAuraVehCullMargin > 1.0f) g_config.copAuraVehCullMargin /= 100.0f;

    g_config.copAuraFootPadX     = readFloat("cop_auras", "foot_pad_x", 45.0f);
    if (g_config.copAuraFootPadX > 1.0f) g_config.copAuraFootPadX /= 100.0f;
    g_config.copAuraFootPadY     = readFloat("cop_auras", "foot_pad_y", 38.0f);
    if (g_config.copAuraFootPadY > 1.0f) g_config.copAuraFootPadY /= 100.0f;
    g_config.copAuraFootAspectRatio = readFloat("cop_auras", "foot_aspect_ratio", 2.00f);

    g_config.copAuraMaxDist      = readFloat("cop_auras", "max_dist", 150.0f);
    g_config.copAuraIntensity    = readScale("cop_auras", "intensity", 150.0f);
    g_config.copAuraBaseRotation = readFloat("cop_auras", "base_rotation", 0.0f);
    g_config.copAuraAspectRatio  = readFloat("cop_auras", "aspect_ratio", 1.66f);
    if (g_config.copAuraAspectRatio < 0.1f) g_config.copAuraAspectRatio = 1.66f;
    g_config.copAuraVehHiddenPercent = readFloat("cop_auras", "veh_hidden_percent", 25.0f);
    if (g_config.copAuraVehHiddenPercent > 1.0f) g_config.copAuraVehHiddenPercent /= 100.0f;
    g_config.copAuraSize         = readScale("cop_auras", "size", 275.0f);
    g_config.copAuraBoundaryTop  = readFloat("cop_auras", "boundary_top", 0.0f);
    if (g_config.copAuraBoundaryTop > 1.0f) g_config.copAuraBoundaryTop /= 100.0f;
    g_config.copAuraBoundarySide = readFloat("cop_auras", "boundary_side", 0.0f);
    if (g_config.copAuraBoundarySide > 1.0f) g_config.copAuraBoundarySide /= 100.0f;
    g_config.copAuraSquirclePower= readFloat("cop_auras", "squircle_power", 7.0f);
    if (g_config.copAuraSquirclePower < 2.0f) g_config.copAuraSquirclePower = 7.0f;

    g_config.copAuraMinIntensity = readScale("cop_auras", "min_intensity", 15.0f);
    g_config.copAuraMaxIntensity = readScale("cop_auras", "max_intensity", 90.0f);
    g_config.copAuraStrobeSpeed  = readScale("cop_auras", "strobe_speed", 20.0f);
    g_config.copAuraChromaticDepth = readScale("cop_auras", "chromatic_depth", 250.0f);
    g_config.copAuraTurbulenceStrength = readScale("cop_auras", "turbulence_strength", 500.0f);
    g_config.copAuraWhiteCoreStrength  = readScale("cop_auras", "white_core_strength", 80.0f);

    // =========================================================================
    // [cop_arcs] / [Wanted.Extension.PoliceArcIndicator]
    // =========================================================================
    int arcVal = readInt("Wanted.Extension.PoliceArcIndicator", "USE_POLICE_ARC_INDICATOR", -999999);
    if (arcVal == -999999) arcVal = readInt("cop_arcs", "enabled", 1);
    g_config.usePoliceArcIndicator = (arcVal != 0);
    g_config.enableCopArcs       = g_config.usePoliceArcIndicator;

    g_config.usePoliceArcIndicatorOnMission = readInt("Wanted.Extension.PoliceArcIndicator", "USE_POLICE_ARC_INDICATOR_ON_MISSION", 0) != 0;
    g_config.extWantedPoliceArcDistance = readFloat("Wanted.Extension.PoliceArcIndicator", "EXT_WANTED_POLICE_ARC_INDICATOR_DISTANCE", 45.0f);
    g_config.extWantedPoliceArcMaxDist = readFloat("Wanted.Extension.PoliceArcIndicator", "EXT_WANTED_POLICE_ARC_INDICATOR_MAX_DISTANCE", 70.0f);
    g_config.extWantedPoliceArcMinDist = readFloat("Wanted.Extension.PoliceArcIndicator", "EXT_WANTED_POLICE_ARC_INDICATOR_MIN_DISTANCE",
        readFloat("Wanted.Extension.PoliceArcIndicator", "EXT_WANTED_POLICE_INDICATOR_MIN_DISTANCE", 15.0f));
    g_config.wantedArcColor      = readColor("wanted.arcs", "color", { 140, 190, 255, 255 });

    g_config.copArcMaxDist       = g_config.extWantedPoliceArcMaxDist;
    g_config.copArcScale         = readScale("cop_arcs", "scale", 100.0f);
    g_config.copArcUseCircularPerimeter = readInt("cop_arcs", "use_circular_perimeter", 1) != 0;

    // =========================================================================
    // [heat_identification] / [Wanted.Extension.CriminalProfile]
    // =========================================================================
    int profileVal = readInt("Wanted.Extension.CriminalProfile", "USE_CRIMINAL_PROFILE", 0);
    g_config.useCriminalProfile  = (profileVal != 0);
    g_config.enableHeatIdentification = g_config.useCriminalProfile;

    g_config.useCriminalProfileOnMission = readInt("Wanted.Extension.CriminalProfile", "USE_CRIMINAL_PROFILE_ON_MISSION", 0) != 0;
    g_config.extWantedChangePlateTransfender = readInt("Wanted.Extension.CriminalProfile", "EXT_WANTED_CRIMINAL_PROFILE_TUNING_CHANGE_PLATE", 0) != 0;
    g_config.extWantedGunHeatDecay = readInt("Wanted.Extension.CriminalProfile", "EXT_WANTED_CRIMINAL_PROFILE_GUN_HEAT_DECAY", 1) != 0;
    g_config.maskModelNames = readStr("Wanted.Extension.CriminalProfile", "MASK_MODEL_NAMES", "bandmask, hockeymask, helmet, moto, bikerhelmet, zorromask, balaclava");
    g_config.disguiseModelNames = readStr("Wanted.Extension.CriminalProfile", "DISGUISE_MODEL_NAMES", "grouchos, glasses01, glasses03, glasses04, glasses05");
    g_config.maskModelKeys = ParseModelKeys(g_config.maskModelNames);
    g_config.disguiseModelKeys = ParseModelKeys(g_config.disguiseModelNames);

    // =========================================================================
    // [Wanted.Extension.SnitchSystem]
    // =========================================================================
    g_config.useSnitchSystem     = readInt("Wanted.Extension.SnitchSystem", "USE_SNITCH_SYSTEM", 0) != 0;
    g_config.useSnitchSystemOnMission = readInt("Wanted.Extension.SnitchSystem", "USE_SNITCH_SYSTEM_ON_MISSION", 0) != 0;
    g_config.extWantedSnitchMinDispatchTime = readFloat("Wanted.Extension.SnitchSystem", "EXT_WANTED_SNITCH_MIN_POLICE_DISPATCH_TIMER", 20.0f);
    g_config.extWantedSnitchMaxDispatchTime = readFloat("Wanted.Extension.SnitchSystem", "EXT_WANTED_SNITCH_MAX_POLICE_DISPATCH_TIMER", 25.0f);
    g_config.extWantedSnitchRadius = readFloat("Wanted.Extension.SnitchSystem", "EXT_WANTED_SNITCH_RADIUS", 60.0f);
    g_config.extWantedSnitchMinPanicRun = readFloat("Wanted.Extension.SnitchSystem", "EXT_WANTED_SNITCH_MIN_PANIC_RUN", 50.0f);
    g_config.extWantedSnitchMaxPanicRun = readFloat("Wanted.Extension.SnitchSystem", "EXT_WANTED_SNITCH_MAX_PANIC_RUN", 150.0f);

    // =========================================================================
    // [heat_identification]
    // =========================================================================
    g_config.footCopScanDist         = readFloat("heat_identification", "foot_cop_scan_dist", 10.0f);
    g_config.footCopScanTime         = readFloat("heat_identification", "foot_cop_scan_time", 3.5f);
    g_config.vehCopScanDist          = readFloat("heat_identification", "veh_cop_scan_dist", 25.0f);
    g_config.vehCopScanTime          = readFloat("heat_identification", "veh_cop_scan_time", 5.0f);
    g_config.gunBaseCooldown         = readFloat("heat_identification", "gun_base_cooldown", 90.0f);
    g_config.gunCooldownPerStar      = readFloat("heat_identification", "gun_cooldown_per_star", 30.0f);
    g_config.payNSprayRandomizePlate = readInt("heat_identification", "pay_n_spray_randomize_plate", 1) != 0;
    g_config.payNSprayCost           = readInt("heat_identification", "pay_n_spray_cost", 500);
    g_config.transfenderPlateMenu    = readInt("heat_identification", "transfender_plate_menu", 1) != 0;
    g_config.plateModCost            = readInt("heat_identification", "plate_mod_cost", 150);
    g_config.heatCleanupMode         = readInt("heat_identification", "cleanup_mode", 2);
    g_config.vehicleWeightingEnabled = readInt("heat_identification", "vehicle_weighting_enabled", 1) != 0;
    g_config.clothingWeightingEnabled = readInt("heat_identification", "clothing_weighting_enabled", 0) != 0;
    g_config.weaponWeightingEnabled  = readInt("heat_identification", "weapon_weighting_enabled", 0) != 0;
    g_config.vehicleCleanupThreshold = readInt("heat_identification", "vehicle_cleanup_threshold", 100);
    g_config.clothingCleanupThreshold = readInt("heat_identification", "clothing_cleanup_threshold", 100);
    g_config.weaponCleanupThreshold  = readInt("heat_identification", "weapon_cleanup_threshold", 100);
    g_config.vehicleWeightPlate      = readInt("heat_identification", "vehicle_weight_plate", 100);
    g_config.vehicleWeightPaintjob   = readInt("heat_identification", "vehicle_weight_paintjob", 60);
    g_config.vehicleWeightRespray    = readInt("heat_identification", "vehicle_weight_respray", 50);
    g_config.vehicleWeightMinorMod   = readInt("heat_identification", "vehicle_weight_minor_mod", 15);
    g_config.clothingWeightFullOutfit = readInt("heat_identification", "clothing_weight_full_outfit", 100);
    g_config.clothingWeightPerSlot   = readInt("heat_identification", "clothing_weight_per_changed_slot", 20);
    g_config.weaponWeightPistol      = readInt("heat_identification", "weapon_weight_pistol", 20);
    g_config.weaponWeightSmg         = readInt("heat_identification", "weapon_weight_smg", 30);
    g_config.weaponWeightShotgun     = readInt("heat_identification", "weapon_weight_shotgun", 45);
    g_config.weaponWeightRifle       = readInt("heat_identification", "weapon_weight_rifle", 55);
    g_config.weaponWeightSniper      = readInt("heat_identification", "weapon_weight_sniper", 70);
    g_config.weaponWeightHeavy       = readInt("heat_identification", "weapon_weight_heavy", 80);
    g_config.weaponWeightExplosive   = readInt("heat_identification", "weapon_weight_explosive", 100);
    g_config.weaponWeightPerShot     = readInt("heat_identification", "weapon_weight_per_shot", 1);
    g_config.weaponSwitchCleanupMode = readInt("heat_identification", "weapon_switch_cleanup_mode", 1);
    g_config.weaponSwitchCleanupWeight = readInt("heat_identification", "weapon_switch_cleanup_weight", 25);
    g_config.weaponHeatDecayPerSecond = readFloat("heat_identification", "weapon_heat_decay_per_second", 1.0f);
    g_config.debugMode               = readInt("debug", "debug_mode", 0);
    g_config.vehicleHeatAllowHistoryFallback = readInt("heat_identification", "vehicle_heat_allow_history_fallback", 0) != 0;
    g_config.vehicleHeatRequireCurrentVisual = readInt("heat_identification", "vehicle_heat_require_current_visual", 1) != 0;

    // =========================================================================
    // [weapon.ammo] & [money]
    // =========================================================================
    g_config.useWeaponUi            = readInt("weapon", "USE_WEAPON_UI", 1) != 0;
    std::string ammoTypeStr         = readStr("weapon", "WEAPON_UI_TOTAL_AMMO_TYPE", "TOTAL_AMMO");
    if (ammoTypeStr == "TOTAL_CLIP" || ammoTypeStr == "clip" || ammoTypeStr == "CLIP") {
        g_config.weaponTotalAmmoType = 1;
    } else {
        g_config.weaponTotalAmmoType = 0;
    }
    g_config.useMoneyUi             = readInt("money", "USE_MONEY_UI", 1) != 0;

    g_config.weaponAmmoCurrentColor = readColor("weapon.ammo", "current", { 255, 255, 255, 255 });
    g_config.weaponAmmoTotalColor   = readColor("weapon.ammo", "total",   { 176, 176, 176, 255 });
    g_config.weaponAmmoStrokeColor  = readColor("weapon.ammo", "stroke",  {   0,   0,   0, 255 });
    g_config.moneyTextColor         = readColor("money", "text",           { 255, 255, 255, 255 });
    g_config.moneyStrokeColor       = readColor("money", "stroke",         {   0,   0,   0, 255 });
    g_config.moneyGaugeColor        = readColor("money", "gauge",          {  46, 204, 113, 255 });

    // =========================================================================
    // [menu]
    // =========================================================================
    g_config.menuToggleKey       = readInt("menu", "toggle_key", 0xBF);

    // =========================================================================
    // [layout.<module>]
    // =========================================================================
    for (int i = 0; i < (int)HudModuleId::Count; ++i) {
        HudModuleId modId = (HudModuleId)i;
        const char* modName = GetModuleName(modId);
        std::string sec = std::string("layout.") + modName;
        ModulePositionConfig def = GetDefaultModulePosition(modId);

        std::string modeStr = readStr(sec.c_str(), "position_mode", PositionModeToString(def.mode));
        g_config.modulePositions[i].mode = StringToPositionMode(modeStr);

        std::string grpStr = readStr(sec.c_str(), "group", HudGroupToString(def.group));
        g_config.modulePositions[i].group = StringToHudGroup(grpStr);

        g_config.modulePositions[i].groupIndex = readInt(sec.c_str(), "group_index", def.groupIndex);

        std::string posAncStr = readStr(sec.c_str(), "pos_anchor", AnchorPointToString(def.posAnchor));
        g_config.modulePositions[i].posAnchor = StringToAnchorPoint(posAncStr);

        std::string elemAncStr = readStr(sec.c_str(), "element_anchor", AnchorPointToString(def.elementAnchor));
        g_config.modulePositions[i].elementAnchor = StringToAnchorPoint(elemAncStr);

        g_config.modulePositions[i].offsetX = readFloat(sec.c_str(), "offset_x", def.offsetX);
        g_config.modulePositions[i].offsetY = readFloat(sec.c_str(), "offset_y", def.offsetY);
    }

    LogWrite("config: loaded successfully (radar=%d, gps=%d, cards=%d, search=%d, auras=%d, arcs=%d)",
             g_config.enabled ? 1 : 0, g_config.enableGps ? 1 : 0, g_config.showInfoCards ? 1 : 0,
             g_config.enableWantedSearchRadius ? 1 : 0, g_config.enableCopAuras ? 1 : 0, g_config.enableCopArcs ? 1 : 0);
    g_pendingConfig = g_config;
    g_config = activeConfig;
}

void ApplyPendingConfig() {
    g_config = g_pendingConfig;
}

void ResetConfig() {
    g_pendingConfig = Config();
    g_pendingConfig.maskModelKeys = ParseModelKeys(g_pendingConfig.maskModelNames);
    g_pendingConfig.disguiseModelKeys = ParseModelKeys(g_pendingConfig.disguiseModelNames);
    g_config = g_pendingConfig;
    SaveConfig();
    LogWrite("config: reset to defaults");
}

void SaveConfig() {
    Config activeConfig = g_config;
    g_config = g_pendingConfig;
    char path[MAX_PATH] = {};
    GetIniPath(path, sizeof(path));
    char colorsPath[MAX_PATH] = {};
    GetColorsIniPath(colorsPath, sizeof(colorsPath));

    auto writeStr = [&](const char* section, const char* key, const char* val) {
        WritePrivateProfileStringA(section, key, val, path);
    };

    auto writeInt = [&](const char* section, const char* key, int val) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", val);
        writeStr(section, key, buf);
    };

    auto writeFloat = [&](const char* section, const char* key, float val, const char* fmt = "%.2f") {
        char buf[32];
        snprintf(buf, sizeof(buf), fmt, val);
        writeStr(section, key, buf);
    };

    auto writeScalePct = [&](const char* section, const char* key, float val) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.0f", val * 100.0f);
        writeStr(section, key, buf);
    };

    auto writeColor = [&](const char* section, const char* key, CRGBA col) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d, %d, %d, %d", col.r, col.g, col.b, col.a);
        // Colors are stored strictly in 6hud/colors.ini, never polluting 6hud.ini
        WritePrivateProfileStringA(section, key, buf, colorsPath);
    };

    // [palette]
    writeColor("palette", "primary_color", g_config.palettePrimary);
    writeColor("palette", "backdrop_color", g_config.paletteBackdrop);
    writeColor("palette", "border_color", g_config.paletteBorder);
    writeColor("palette", "text_primary", g_config.paletteTextPrimary);
    writeColor("palette", "text_accent", g_config.paletteTextAccent);

    // [Global]
    char hudKeyBuf[2] = { (char)g_config.showHudKey, '\0' };
    writeStr("Global", "SHOW_HUD_KEY", hudKeyBuf);
    writeStr("Global", "GAME_UNIT", g_config.gameUnit == 1 ? "IMPERIAL" : "METRIC");
    writeFloat("Global", "UI_IDLE_TIMEOUT", g_config.uiIdleTimeout, "%.2f");
    writeFloat("Global", "BACKDROP_BLUR_INTENSITY", g_config.blurIntensity, "%.2f");
    writeInt("Global", "BACKDROP_BLUR_QUALITY", g_config.blurQuality);

    // [weapon]
    writeInt("weapon", "USE_WEAPON_UI", g_config.useWeaponUi ? 1 : 0);
    writeStr("weapon", "WEAPON_UI_TOTAL_AMMO_TYPE", g_config.weaponTotalAmmoType == 1 ? "TOTAL_CLIP" : "TOTAL_AMMO");

    // [money]
    writeInt("money", "USE_MONEY_UI", g_config.useMoneyUi ? 1 : 0);

    // [minimap.marker]
    writeColor("minimap.marker", "0", g_config.markerColors[0]);
    writeColor("minimap.marker", "1", g_config.markerColors[1]);
    writeColor("minimap.marker", "2", g_config.markerColors[2]);
    writeColor("minimap.marker", "3", g_config.markerColors[3]);
    writeColor("minimap.marker", "4", g_config.markerColors[4]);

    // [radar] / [Minimap.Radar]
    writeInt("Minimap.Radar", "USE_RADAR", g_config.enabled ? 1 : 0);
    writeFloat("Minimap.Radar", "RADAR_SIZE_SCALE", g_config.radarSizeScale, "%.2f");
    writeInt("Minimap.Radar", "SHOW_ON_FOOT", g_config.showOnFoot ? 1 : 0);
    writeStr("Minimap.Radar", "GAME_CLOCK_FORMAT", g_config.gameClockFormat == 0 ? "12H_FORMAT" : "24H_FORMAT");
    char zoomKeyBuf[2] = { (char)g_config.peekKey, '\0' };
    writeStr("Minimap.Radar", "RADAR_ZOOM_OUT_KEY", zoomKeyBuf);
    writeFloat("Minimap.Radar", "RADAR_ZOOM_MULTIPLIER", g_config.peekZoomMult, "%.2f");
    writeFloat("Minimap.Radar", "CLOSED_SPACE_ZOOM_MULTIPLIER", g_config.closedSpaceZoomMultiplier, "%.2f");
    writeInt("Minimap.Radar", "USE_ALTITUDE_ZOOM", g_config.enableAltitudeZoom ? 1 : 0);
    writeFloat("Minimap.Radar", "ALTITUDE_ZOOM_MULTIPLIER", g_config.altitudeMaxZoomMult, "%.2f");
    writeFloat("Minimap.Radar", "MIN_ALTITUDE", g_config.altitudeMinHeight, "%.1f");
    writeFloat("Minimap.Radar", "MAX_ALTITUDE", g_config.altitudeMaxHeight, "%.1f");
    writeInt("Minimap.Radar", "LOCK_PLAYER_ELEVATION", g_config.lockPlayerElevation ? 1 : 0);
    writeFloat("Minimap.Radar", "RADAR_OPACITY", g_config.radarOpacity * 100.0f, "%.0f");
    writeColor("minimap.radar", "sea", g_config.radarSeaColor);
    writeColor("minimap.radar", "sea_night", g_config.radarSeaColorNight);
    writeInt("Minimap.Radar", "EXPERIMENTAL_SHOW_3D_OBJECTS", g_config.experimentalShow3dObjects ? 1 : 0);
    writeColor("minimap.radar", "3d_objects", g_config.color3dObjects);
    writeInt("Minimap.Radar", "USE_CUSTOM_MAP_TILE", g_config.useCustomMapTile ? 1 : 0);
    writeInt("Minimap.Radar", "USE_CUSTOM_PAUSE_MAP", g_config.useCustomPauseMap ? 1 : 0);

    // [gps] / [Minimap.GPSNavigation]
    writeInt("Minimap.GPSNavigation", "USE_RADAR_GPS", g_config.enableGps ? 1 : 0);
    writeInt("Minimap.GPSNavigation", "USE_MENU_MAP_GPS", g_config.enableMenuMapGps ? 1 : 0);
    writeInt("Minimap.GPSNavigation", "USE_NAV_INDICATOR", g_config.enableGpsNavigator ? 1 : 0);
    writeInt("Minimap.GPSNavigation", "USE_WALKING_NODE", g_config.useWalkingNode ? 1 : 0);
    writeFloat("Minimap.GPSNavigation", "GPS_LINE_WIDTH", g_config.gpsLineWidth, "%.1f");
    writeFloat("Minimap.GPSNavigation", "GPS_LINE_STROKE_WIDTH", g_config.gpsCasingWidth, "%.1f");

    // [info_cards] / Minimap Information Cards
    writeInt("Minimap.PlaceInformation", "USE_PLACE_INFO", g_config.usePlaceInfo ? 1 : 0);
    writeInt("Minimap.VehicleInformation", "USE_VEHICLE_INFO", g_config.useVehicleInfo ? 1 : 0);
    writeInt("Minimap.Extension.VehicleFuel", "USE_VEHICLE_FUEL", g_config.useVehicleFuel ? 1 : 0);
    writeInt("Minimap.Extension.VehicleFuel", "USE_VEHICLE_FUEL_ON_MISSION", g_config.useVehicleFuelOnMission ? 1 : 0);
    writeFloat("Minimap.Extension.VehicleFuel", "VEHICLE_FUEL_CONSUMPTION", g_config.vehicleFuelConsumption, "%.2f");
    writeFloat("Minimap.Extension.VehicleFuel", "VEHICLE_FUEL_RNG_MIN", g_config.vehicleFuelRngMin, "%.2f");
    writeFloat("Minimap.Extension.VehicleFuel", "VEHICLE_FUEL_RNG_MAX", g_config.vehicleFuelRngMax, "%.2f");

    // [Wanted.Visual]
    writeInt("Wanted.Visual", "USE_WANTED_STAR", g_config.useWantedStar ? 1 : 0);

    // [Wanted.Extension.SearchArea]
    writeInt("Wanted.Extension.SearchArea", "USE_WANTED_SEARCH_AREA", g_config.useWantedSearchArea ? 1 : 0);
    writeInt("Wanted.Extension.SearchArea", "USE_WANTED_SEARCH_AREA_ON_MISSION", g_config.useWantedSearchAreaOnMission ? 1 : 0);
    writeFloat("Wanted.Extension.SearchArea", "EXT_WANTED_SEARCH_AREA_ZOOM", g_config.extWantedSearchAreaZoom, "%.2f");
    writeFloat("Wanted.Extension.SearchArea", "EXT_WANTED_SEARCH_RADIUS_BASE", g_config.extWantedSearchRadiusBase, "%.1f");
    writeFloat("Wanted.Extension.SearchArea", "EXT_WANTED_SEARCH_RADIUS_MULTIPLIER_VALUE", g_config.extWantedSearchRadiusMultiplier, "%.1f");
    writeFloat("Wanted.Extension.SearchArea", "EXT_WANTED_ESCAPE_BASE_DURATION", g_config.extWantedEscapeBaseDuration, "%.1f");
    writeFloat("Wanted.Extension.SearchArea", "EXT_WANTED_ESCAPE_MULTIPLIER_VALUE", g_config.extWantedEscapeMultiplier, "%.1f");

    // [Wanted.Extension.HeatArea]
    writeInt("Wanted.Extension.HeatArea", "USE_WANTED_HEAT_AREA", g_config.useWantedHeatArea ? 1 : 0);
    writeInt("Wanted.Extension.HeatArea", "USE_WANTED_HEAT_AREA_ON_MISSION", g_config.useWantedHeatAreaOnMission ? 1 : 0);
    writeFloat("Wanted.Extension.HeatArea", "EXT_WANTED_HEAT_AREA_BASE_DURATION", g_config.extWantedHeatAreaBaseDuration, "%.1f");
    writeFloat("Wanted.Extension.HeatArea", "EXT_WANTED_HEAT_AREA_DURATION_MULTIPLIER", g_config.extWantedHeatAreaDurationMultiplier, "%.1f");
    writeFloat("Wanted.Extension.HeatArea", "EXT_WANTED_HEAT_AREA_ZONE_MULTIPLIER", g_config.extWantedHeatAreaZoneMultiplier, "%.2f");

    // [Wanted.Extension.PoliceSirenIndicator]
    writeInt("Wanted.Extension.PoliceSirenIndicator", "USE_POLICE_SIREN_INDICATOR", g_config.usePoliceSirenIndicator ? 1 : 0);
    writeInt("Wanted.Extension.PoliceSirenIndicator", "USE_POLICE_SIREN_INDICATOR_ON_MISSION", g_config.usePoliceSirenIndicatorOnMission ? 1 : 0);
    writeInt("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_ON_FOOT", g_config.extWantedPoliceSirenOnFoot ? 1 : 0);
    writeFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_ON_FOOT_DISTANCE", g_config.extWantedPoliceSirenOnFootDist, "%.1f");
    writeFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_MIN_INTENSITY", g_config.extWantedPoliceSirenMinIntensity, "%.2f");
    writeFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_MAX_INTENSITY", g_config.extWantedPoliceSirenMaxIntensity, "%.2f");
    writeFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_MIN_SIZE", g_config.extWantedPoliceSirenMinSize, "%.2f");
    writeFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_MAX_SIZE", g_config.extWantedPoliceSirenMaxSize, "%.2f");
    writeFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_FAR_SPEED_MULTIPLIER", g_config.extWantedPoliceSirenFarSpeedMult, "%.2f");
    writeFloat("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIRE_INDICATOR_NO_DRIVER_OPACITY", g_config.extWantedPoliceSirenNoDriverOpacity, "%.2f");
    writeInt("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_ROADBLOCK", g_config.extWantedPoliceSirenRoadblock ? 1 : 0);
    writeInt("Wanted.Extension.PoliceSirenIndicator", "EXT_WANTED_POLICE_SIREN_INDICATOR_HELICOPTER", g_config.extWantedPoliceSirenHelicopter ? 1 : 0);

    // [Wanted.Extension.PoliceArcIndicator]
    writeInt("Wanted.Extension.PoliceArcIndicator", "USE_POLICE_ARC_INDICATOR", g_config.usePoliceArcIndicator ? 1 : 0);
    writeInt("Wanted.Extension.PoliceArcIndicator", "USE_POLICE_ARC_INDICATOR_ON_MISSION", g_config.usePoliceArcIndicatorOnMission ? 1 : 0);
    writeFloat("Wanted.Extension.PoliceArcIndicator", "EXT_WANTED_POLICE_ARC_INDICATOR_DISTANCE", g_config.extWantedPoliceArcDistance, "%.1f");
    writeFloat("Wanted.Extension.PoliceArcIndicator", "EXT_WANTED_POLICE_ARC_INDICATOR_MAX_DISTANCE", g_config.extWantedPoliceArcMaxDist, "%.1f");
    writeFloat("Wanted.Extension.PoliceArcIndicator", "EXT_WANTED_POLICE_INDICATOR_MIN_DISTANCE", g_config.extWantedPoliceArcMinDist, "%.1f");

    // [Wanted.Extension.CriminalProfile]
    writeInt("Wanted.Extension.CriminalProfile", "USE_CRIMINAL_PROFILE", g_config.useCriminalProfile ? 1 : 0);
    writeInt("Wanted.Extension.CriminalProfile", "USE_CRIMINAL_PROFILE_ON_MISSION", g_config.useCriminalProfileOnMission ? 1 : 0);
    writeInt("Wanted.Extension.CriminalProfile", "EXT_WANTED_CRIMINAL_PROFILE_TUNING_CHANGE_PLATE", g_config.extWantedChangePlateTransfender ? 1 : 0);
    writeInt("Wanted.Extension.CriminalProfile", "EXT_WANTED_CRIMINAL_PROFILE_GUN_HEAT_DECAY", g_config.extWantedGunHeatDecay ? 1 : 0);
    writeStr("Wanted.Extension.CriminalProfile", "MASK_MODEL_NAMES", g_config.maskModelNames.c_str());
    writeStr("Wanted.Extension.CriminalProfile", "DISGUISE_MODEL_NAMES", g_config.disguiseModelNames.c_str());

    // [Wanted.Extension.SnitchSystem]
    writeInt("Wanted.Extension.SnitchSystem", "USE_SNITCH_SYSTEM", g_config.useSnitchSystem ? 1 : 0);
    writeInt("Wanted.Extension.SnitchSystem", "USE_SNITCH_SYSTEM_ON_MISSION", g_config.useSnitchSystemOnMission ? 1 : 0);
    writeFloat("Wanted.Extension.SnitchSystem", "EXT_WANTED_SNITCH_MIN_POLICE_DISPATCH_TIMER", g_config.extWantedSnitchMinDispatchTime, "%.1f");
    writeFloat("Wanted.Extension.SnitchSystem", "EXT_WANTED_SNITCH_MAX_POLICE_DISPATCH_TIMER", g_config.extWantedSnitchMaxDispatchTime, "%.1f");
    writeFloat("Wanted.Extension.SnitchSystem", "EXT_WANTED_SNITCH_RADIUS", g_config.extWantedSnitchRadius, "%.1f");
    writeFloat("Wanted.Extension.SnitchSystem", "EXT_WANTED_SNITCH_MIN_PANIC_RUN", g_config.extWantedSnitchMinPanicRun, "%.1f");
    writeFloat("Wanted.Extension.SnitchSystem", "EXT_WANTED_SNITCH_MAX_PANIC_RUN", g_config.extWantedSnitchMaxPanicRun, "%.1f");

    writeColor("wanted.area", "blue", g_config.wantedColorBlue);
    writeColor("wanted.area", "red", g_config.wantedColorRed);
    writeColor("wanted.area", "heat", g_config.wantedAreaHeatColor);
    writeColor("wanted.arcs", "color", g_config.wantedArcColor);
    writeColor("minimap.card", "background", g_config.cardBackdropColor);
    writeColor("minimap.card", "darker", g_config.cardDarkerColor);
    writeColor("minimap.card", "foreground", g_config.cardTextColor);
    writeColor("minimap.card", "border", g_config.cardBorderColor);

    // =========================================================================
    // [layout.<module>]
    // =========================================================================
    for (int i = 0; i < (int)HudModuleId::Count; ++i) {
        HudModuleId modId = (HudModuleId)i;
        const char* modName = GetModuleName(modId);
        std::string sec = std::string("layout.") + modName;
        const auto& pos = g_config.modulePositions[i];

        writeStr(sec.c_str(), "position_mode", PositionModeToString(pos.mode));
        writeStr(sec.c_str(), "group", HudGroupToString(pos.group));
        writeInt(sec.c_str(), "group_index", pos.groupIndex);
        writeStr(sec.c_str(), "pos_anchor", AnchorPointToString(pos.posAnchor));
        writeStr(sec.c_str(), "element_anchor", AnchorPointToString(pos.elementAnchor));
        writeFloat(sec.c_str(), "offset_x", pos.offsetX, "%.1f");
        writeFloat(sec.c_str(), "offset_y", pos.offsetY, "%.1f");
    }

    // Strip noise sections if previously present in INI
    WritePrivateProfileStringA("debug", NULL, NULL, path);
    WritePrivateProfileStringA("menu", NULL, NULL, path);

    // Update timestamp to avoid immediate hot-reload loop
    WIN32_FILE_ATTRIBUTE_DATA data = {};
    if (GetFileAttributesExA(path, GetFileExInfoStandard, &data)) {
        s_lastWriteTime = data.ftLastWriteTime;
    }

    LogWrite("config: saved settings to %s", path);
    g_config = activeConfig;
}

void RequestConfigReload() {
    s_reloadRequested = true;
}

void CheckHotReload() {
    if (s_reloadRequested) {
        s_reloadRequested = false;
        LoadConfig();
        LogWrite("config: menu reload applied at process boundary");
    }

    static int s_frameTicker = 0;
    if (++s_frameTicker < 30) return;
    s_frameTicker = 0;

    char path[MAX_PATH] = {};
    GetIniPath(path, sizeof(path));

    WIN32_FILE_ATTRIBUTE_DATA data = {};
    if (GetFileAttributesExA(path, GetFileExInfoStandard, &data)) {
        if (s_lastWriteTime.dwLowDateTime == 0 && s_lastWriteTime.dwHighDateTime == 0) {
            s_lastWriteTime = data.ftLastWriteTime;
            return;
        }
        if (CompareFileTime(&s_lastWriteTime, &data.ftLastWriteTime) != 0) {
            s_lastWriteTime = data.ftLastWriteTime;
            LogWrite("config: hot-reload triggered from 6hud.ini");
            LoadConfig();
        }
    }
}

} // namespace hud6
