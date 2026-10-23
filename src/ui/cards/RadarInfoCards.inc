// Stacked Info Cards & GPS Turn Navigator State
enum class GpsManeuver {
    None,
    Straight,
    TurnLeft,
    TurnRight,
    SlightLeft,
    SlightRight,
    UTurnLeft,
    UTurnRight,
    Arrival
};

struct GpsNavState {
    GpsManeuver maneuver = GpsManeuver::None;
    float turnDistance = 0.0f;
    char maneuverText[64] = {};
    char distanceText[32] = {};
};
static GpsNavState s_gpsNavState;
static float s_navAlpha = 0.0f;

enum GaugeId {
    GAUGE_WAYPOINT = 0,
    GAUGE_HEALTH   = 1,
    GAUGE_FUEL     = 2,
    GAUGE_NITRO    = 3,
    GAUGE_COUNT    = 4
};

struct GaugeAnimSlot {
    bool  active      = false;
    float curAlpha    = 0.0f;
    float targetAlpha = 0.0f;
    float curX        = 0.0f;
    float targetX     = 0.0f;
    float curValue    = 0.0f;
    float targetValue = 1.0f;
    CRGBA color       = { 255, 255, 255, 255 };
};
static GaugeAnimSlot s_gaugeSlots[GAUGE_COUNT];
static float s_waypointHoldTimer = 0.0f;
static float s_initialWaypointDist = 0.0f;
static unsigned char s_curNitroBoosts = 0;

static char  s_curZoneName[64]   = {};
static char  s_prevZoneName[64]  = {};
static float s_zoneFadeAlpha     = 0.0f;
static char  s_lastZoneKey[16]   = {};
static float s_locShowTimer      = 0.0f;
static float s_locAlpha          = 0.0f;
static bool  s_isInterior        = false;
static int   s_lastAreaCode      = 0;

static bool  s_inVehicle         = false;
static CVehicle* s_pLastVehicle  = nullptr;
static char  s_vehName[64]       = {};
static char  s_vehClass[64]      = {};
static float s_vehNameTimer      = 0.0f;
static float s_vehCardAlpha      = 0.0f;
static float s_vehNameAlpha      = 0.0f;
static float s_vehHealth         = 1.0f;
static float s_vehStateLerp      = 0.0f;

static float s_vehFuel           = 1.0f;
static CVector s_lastVehPos(0.0f, 0.0f, 0.0f);
static CVehicle* s_pTrackedFuelVehicle = nullptr;

struct VehicleFuelExt {
    float fuelLevel = -1.0f;
    bool initialized = false;
    VehicleFuelExt(CVehicle*) {}
};
static plugin::VehicleExtendedData<VehicleFuelExt> s_vehicleFuelExt;

struct GasStationPos { float x, y, z; };
static const GasStationPos kGasStations[] = {
    { 1938.0f, -1773.0f, 13.5f }, // Idlewood
    { 1004.0f, -939.0f,  42.0f }, // Mulholland
    { -93.0f,  -1175.0f, 2.4f },  // Flint County
    { 658.0f,  -565.0f,  16.3f }, // Dillimore
    { 1383.0f, 461.0f,   20.2f }, // Montgomery
    { -2249.0f,-2559.0f, 31.9f }, // Whetstone
    { -1606.0f,-2714.0f, 48.5f }, // Angel Pine
    { -2413.0f, 976.0f,  45.3f }, // Juniper Hill
    { -1676.0f, 413.0f,  7.2f  }, // Easter Basin
    { -1328.0f, 2677.0f, 50.1f }, // El Quebrados
    { -732.0f,  2742.0f, 47.2f }, // Tierra Robada
    { 69.0f,    1219.0f, 18.8f }, // Fort Carson
    { 612.0f,   1694.0f, 7.0f  }, // Bone County
    { 2115.0f,  920.0f,  10.8f }, // Las Venturas South
    { 2639.0f,  1106.0f, 10.8f }, // Las Venturas East
    { 2147.0f,  2748.0f, 10.8f }, // Las Venturas North
    { 1596.0f,  2198.0f, 10.8f }, // Redsands East
};
static bool s_lastRadarEnabled   = false;
static bool s_lastGpsEnabled     = false;
static bool s_lastSearchEnabled  = false;
static bool s_lastCardsEnabled   = false;
static bool s_lastAurasEnabled   = false;
static bool s_lastArcsEnabled    = false;

static CVehicle* GetStablePlayerVehicle(CPlayerPed* player) {
    if (!player || !player->bInVehicle || !player->m_pVehicle) return nullptr;
    if (!CPools::ms_pVehiclePool || !CPools::ms_pVehiclePool->IsObjectValid(player->m_pVehicle)) return nullptr;
    if (player->m_pVehicle->m_fHealth <= 0.0f) return nullptr;
    return player->m_pVehicle;
}

static CRGBA s_radarSeaColor(56, 73, 80, 255);

static float Clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float Lerpf(float a, float b, float t)    { return a + (b - a) * t; }
static inline CVector LerpVec(const CVector& a, const CVector& b, float t) { return a + (b - a) * t; }

static inline unsigned char ModAlpha(unsigned char origAlpha, float mult) {
    float result = (float)origAlpha * mult;
    if (result < 0.0f) result = 0.0f;
    if (result > 255.0f) result = 255.0f;
    return (unsigned char)result;
}

static inline CRGBA ModColor(CRGBA col, float mult) {
    return CRGBA(col.r, col.g, col.b, ModAlpha(col.a, mult));
}

static void GenerateSmoothPath(const CVector* inNodes, int inCount, CVector* outPts, int& outCount, int maxOut) {
    outCount = 0;
    if (inCount <= 0 || !inNodes) return;
    if (inCount == 1) {
        if (outCount < maxOut) outPts[outCount++] = inNodes[0];
        return;
    }

    static CVector pts[kMaxPathNodes + 2];
    int count = 0;
    for (int i = 0; i < inCount && count < kMaxPathNodes + 2; ++i) {
        if (count == 0 || (inNodes[i] - pts[count - 1]).Magnitude2D() > 1.2f) {
            pts[count++] = inNodes[i];
        }
    }
    if (count < 2) {
        if (count == 1 && outCount < maxOut) outPts[outCount++] = pts[0];
        return;
    }

    // Two-iteration Chaikin subdivision to smooth sharp corner turns into continuous curves
    static CVector chaikin1[2048];
    int c1Count = 0;
    if (count == 2) {
        chaikin1[c1Count++] = pts[0];
        chaikin1[c1Count++] = pts[1];
    } else {
        chaikin1[c1Count++] = pts[0];
        for (int i = 0; i < count - 1 && c1Count < 2040; ++i) {
            CVector p0 = pts[i];
            CVector p1 = pts[i + 1];
            CVector d = p1 - p0;
            if (i == 0) {
                chaikin1[c1Count++] = p0 + d * 0.75f;
            } else if (i == count - 2) {
                chaikin1[c1Count++] = p0 + d * 0.25f;
                chaikin1[c1Count++] = p1;
            } else {
                chaikin1[c1Count++] = p0 + d * 0.25f;
                chaikin1[c1Count++] = p0 + d * 0.75f;
            }
        }
    }

    static CVector chaikinPts[4096];
    int chaikinCount = 0;
    if (c1Count == 2) {
        chaikinPts[chaikinCount++] = chaikin1[0];
        chaikinPts[chaikinCount++] = chaikin1[1];
    } else {
        chaikinPts[chaikinCount++] = chaikin1[0];
        for (int i = 0; i < c1Count - 1 && chaikinCount < 4090; ++i) {
            CVector p0 = chaikin1[i];
            CVector p1 = chaikin1[i + 1];
            CVector d = p1 - p0;
            if (i == 0) {
                chaikinPts[chaikinCount++] = p0 + d * 0.75f;
            } else if (i == c1Count - 2) {
                chaikinPts[chaikinCount++] = p0 + d * 0.25f;
                chaikinPts[chaikinCount++] = p1;
            } else {
                chaikinPts[chaikinCount++] = p0 + d * 0.25f;
                chaikinPts[chaikinCount++] = p0 + d * 0.75f;
            }
        }
    }

    float totalLen = 0.0f;
    for (int i = 0; i < chaikinCount - 1; ++i) {
        totalLen += (chaikinPts[i + 1] - chaikinPts[i]).Magnitude2D();
    }

    float maxStep = 2.0f;
    if (totalLen > 0.0f) {
        float dynamicStep = totalLen / (float)(maxOut - 30);
        if (dynamicStep > maxStep) maxStep = dynamicStep;
    }

    for (int i = 0; i < chaikinCount - 1 && outCount < maxOut - 2; ++i) {
        CVector p0 = chaikinPts[i];
        CVector p1 = chaikinPts[i + 1];
        float segLen = (p1 - p0).Magnitude2D();
        int subSteps = (int)ceilf(segLen / maxStep);
        if (subSteps < 1) subSteps = 1;
        if (subSteps > 32) subSteps = 32;

        for (int s = 0; s < subSteps && outCount < maxOut - 1; ++s) {
            float t = (float)s / (float)subSteps;
            outPts[outCount++] = p0 * (1.0f - t) + p1 * t;
        }
    }
    if (chaikinCount > 0 && outCount < maxOut) {
        outPts[outCount++] = chaikinPts[chaikinCount - 1];
    }
}

