static CRect   g_radarRect;
static CRect   g_moduleRects[(int)HudModuleId::Count];
// Alpha-weighted group stacking: hidden modules collapse so the rest slide to close the gap.
static float   g_moduleAlpha[(int)HudModuleId::Count];
static CRect   g_animatedModuleRects[(int)HudModuleId::Count];
static CVector g_playerPos;
static CVector g_playerFwd;
static CVector g_camRight, g_camUp;

static float g_curTiltRad     = 35.0f * kPi / 180.0f;
static float g_curDist        = 250.0f;
static float g_curVCenter     = 0.55f;
static float g_curPuckTilt    = 0.0f;
static float g_radarAlphaFade = 0.0f;
static float s_radarFadeProgress = 0.0f;

static bool    g_pinActive   = false;
static CVector g_pinPos(0.0f, 0.0f, 0.0f);
static float   g_gpsDistance = 0.0f;

static CVector g_pathNodes[kMaxPathNodes];
static int     g_pathCount       = 0;
static int     g_pathFrameTicker = 0;
static CVector g_lastSearchPos;
static bool    g_isWalkingPath   = false;
static bool    g_hudKeyHeld      = false;
static bool    g_hudShowAll      = false;

static bool g_radarDead    = false;
static bool g_playerOk     = false;
static int  g_warmupFrames = 0;

// Wanted Search Area & Evasion State
enum eWantedSearchStage {
    WANTED_STAGE_INACTIVE = 0,
    WANTED_STAGE_PURSUIT  = 1, // Stage 1: Active pursuit, cops have visual, solid stars
    WANTED_STAGE_LKP      = 2, // Stage 2: Cops lost visual, locked at LKP, 100% blinking stars
    WANTED_STAGE_HEAT     = 3  // Stage 3: LKP search complete, wide heat area only, 50% opacity solid stars (NOT blinking)
};

static eWantedSearchStage g_wantedStage       = WANTED_STAGE_INACTIVE;
static bool               g_wantedActive      = false;
static CVector            g_searchCenter(0.0f, 0.0f, 0.0f);
static float              g_searchRadius      = 150.0f;
static bool               g_copHasVisual      = false;
static float              g_escapeTimer       = 0.0f;
static float              g_heatEscapeTimer   = 0.0f;
static float               g_visualLossTimer = 0.0f;

static inline CVector LerpVec(const CVector& a, const CVector& b, float t);
static float              g_innerSearchAlpha  = 1.0f;
static float              g_searchPulseAngle  = 0.0f;

// Top-Right HUD Accordion Slide Transition State
static float g_wantedAnimProgress    = 0.0f;
static float g_heatBadgeAnimProgress = 0.0f;
static float g_ammoSlideProgress     = 0.0f;

struct D3D2DVert {
    float x, y, z, rhw;
    DWORD color;
    float u, v;
};
#define D3DFVF_2DVERT (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

static IDirect3DTexture9* g_pTexStarBase    = nullptr;
static IDirect3DTexture9* g_pTexStarBorder  = nullptr;
static IDirect3DTexture9* g_pTexIfruit      = nullptr;
static IDirect3DTexture9* g_pTexMission     = nullptr;
static IDirect3DTexture9* g_pTexZAbove      = nullptr;
static IDirect3DTexture9* g_pTexZBelow      = nullptr;
static IDirect3DTexture9* g_pTexPlayer      = nullptr;
static IDirect3DTexture9* g_pTexPlayerWanted= nullptr;
static IDirect3DTexture9* g_pTexNorth       = nullptr;
static IDirect3DTexture9* g_pTexEngine      = nullptr;
static IDirect3DTexture9* g_pTexFuel        = nullptr;
static IDirect3DTexture9* g_pTexNitro       = nullptr;
static IDirect3DTexture9* g_pTexWallet      = nullptr;
static IDirect3DTexture9* g_pTexPersonnelArc= nullptr;
static IDirect3DTexture9* g_pTexProfileIdentity = nullptr;
static IDirect3DTexture9* g_pTexProfileVehicle  = nullptr;
static IDirect3DTexture9* g_pTexProfileOutfit   = nullptr;
static IDirect3DTexture9* g_pTexProfileWeapon   = nullptr;
static IDirect3DTexture9* g_pTexSoftGlow        = nullptr;

// Snitch / Witness System state
struct SnitchWitness {
    bool   active = false;
    CPed*  ped = nullptr;
    CVector startPos = { 0.0f, 0.0f, 0.0f };
    float  panicRunDist = 80.0f;
    float  dispatchTimer = 20.0f;
    bool   hasCalled = false;
    int    severityStars = 1;
    bool   reportedVehicle = false;
    bool   reportedGun = false;
};
static const int kMaxSnitches = 4;
static SnitchWitness g_snitches[kMaxSnitches] = {};
static int   g_hollowStarsCount = 0;
static float g_hollowStarsTimer = 0.0f;

static void*              g_pBackdropBlurPS = nullptr;
static float              s_destFade        = 0.0f;
static float              s_clockFadeProgress = 0.0f;
static float              s_hudKeyHoldTimer   = 0.0f;
static float              s_playerWantedFade  = 0.0f;

static IDirect3DTexture9* LoadTexture(const char* filename);

struct MapTileSlot {
    IDirect3DTexture9* pDayTex = nullptr;
    IDirect3DTexture9* pNightTex = nullptr;
    bool searchedDay = false;
    bool searchedNight = false;
};
static MapTileSlot s_mapTiles[144];

static float GetDayNightBlend() {
    float hour = (float)CClock::ms_nGameClockHours + (float)CClock::ms_nGameClockMinutes / 60.0f;
    if (hour >= 7.0f && hour <= 19.0f) {
        return 0.0f;
    } else if (hour > 19.0f && hour < 21.0f) {
        float t = (hour - 19.0f) / 2.0f;
        return t * t * (3.0f - 2.0f * t);
    } else if (hour >= 21.0f || hour <= 5.0f) {
        return 1.0f;
    } else {
        float t = (hour - 5.0f) / 2.0f;
        return 1.0f - (t * t * (3.0f - 2.0f * t));
    }
}

static IDirect3DTexture9* LoadTexture(const char* filename);

static void GetMapTileTextures(int tileIndex, IDirect3DTexture9*& outDay, IDirect3DTexture9*& outNight) {
    if (tileIndex < 0 || tileIndex >= 144) {
        outDay = nullptr;
        outNight = nullptr;
        return;
    }
    MapTileSlot& slot = s_mapTiles[tileIndex];
    if (!slot.searchedDay) {
        slot.searchedDay = true;
        char name[64];
        snprintf(name, sizeof(name), "maps\\day\\radar%02d.dds", tileIndex);
        slot.pDayTex = LoadTexture(name);
    }
    if (!slot.searchedNight) {
        slot.searchedNight = true;
        char name[64];
        snprintf(name, sizeof(name), "maps\\night\\radar%02d.dds", tileIndex);
        slot.pNightTex = LoadTexture(name);
    }
    outDay = slot.pDayTex;
    outNight = slot.pNightTex;
}

static IDirect3DTexture9* LoadTexture(const char* filename) {
    IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
    if (!dev) return nullptr;

    const char* prefixes[] = {
        "6hud\\assets\\",
        "assets\\",
        "plugins\\6hud\\assets\\",
        "modloader\\6hud\\assets\\",
        ""
    };

    char fullPath[MAX_PATH];
    IDirect3DTexture9* tex = nullptr;
    for (int i = 0; i < 5; ++i) {
        snprintf(fullPath, sizeof(fullPath), "%s%s", prefixes[i], filename);
        HRESULT hr = D3DXCreateTextureFromFileExA(
            dev,
            fullPath,
            D3DX_DEFAULT_NONPOW2,
            D3DX_DEFAULT_NONPOW2,
            D3DX_DEFAULT,
            0,
            D3DFMT_UNKNOWN,
            D3DPOOL_MANAGED,
            D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER,
            D3DX_FILTER_BOX,
            0,
            NULL,
            NULL,
            &tex
        );
        if (FAILED(hr)) {
            hr = D3DXCreateTextureFromFileExA(
                dev,
                fullPath,
                D3DX_DEFAULT_NONPOW2,
                D3DX_DEFAULT_NONPOW2,
                1,
                0,
                D3DFMT_UNKNOWN,
                D3DPOOL_MANAGED,
                D3DX_FILTER_NONE,
                D3DX_FILTER_NONE,
                0,
                NULL,
                NULL,
                &tex
            );
        }
        if (SUCCEEDED(hr) && tex) {
            LogWrite("texture: loaded %s from %s (%p)", filename, fullPath, tex);
            return tex;
        }
    }
    LogWrite("texture: FAILED to load %s", filename);
    return nullptr;
}

static void EnsureSoftGlowTexture() {
    if (g_pTexSoftGlow) return;
    IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
    if (!dev) return;
    HRESULT hr = dev->CreateTexture(64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &g_pTexSoftGlow, NULL);
    if (FAILED(hr) || !g_pTexSoftGlow) return;

    D3DLOCKED_RECT lr;
    if (SUCCEEDED(g_pTexSoftGlow->LockRect(0, &lr, NULL, 0))) {
        unsigned char* bits = (unsigned char*)lr.pBits;
        for (int y = 0; y < 64; ++y) {
            unsigned char* row = bits + y * lr.Pitch;
            for (int x = 0; x < 64; ++x) {
                float dx = (x + 0.5f - 32.0f) / 31.5f;
                float dy = (y + 0.5f - 32.0f) / 31.5f;
                float dist = sqrtf(dx * dx + dy * dy);
                float a = 0.0f;
                if (dist < 1.0f) {
                    float g = expf(-dist * dist * 3.2f);
                    float taper = 1.0f - dist;
                    taper = taper * taper * (3.0f - 2.0f * taper);
                    a = g * taper;
                }
                unsigned char val = (unsigned char)(255.0f * a);
                row[x * 4 + 0] = 255;
                row[x * 4 + 1] = 255;
                row[x * 4 + 2] = 255;
                row[x * 4 + 3] = val;
            }
        }
        g_pTexSoftGlow->UnlockRect(0);
    }
}

static void DrawSoftGlow(float cx, float cy, float radX, float radY, CRGBA centerCol, int segments, bool additive);

// Puts the device into a state where a raw D3D9 DrawPrimitiveUP quad is guaranteed
// to render. RenderWare's RwIm2D path binds a vertex shader and vertex declaration
// and leaves texture stages 1..3 enabled; SetFVF alone does not undo either, so a
// raw quad submitted afterwards gets discarded or shaded wrong. Every raw-D3D draw
// in this plugin goes through here first.
static void BeginRawD3DQuad(IDirect3DDevice9* dev) {
    if (!dev) return;
    dev->SetVertexShader(nullptr);
    dev->SetVertexDeclaration(nullptr);
    dev->SetFVF(D3DFVF_2DVERT);
    dev->SetPixelShader(nullptr);

    for (DWORD stage = 1; stage < 4; ++stage) {
        dev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_DISABLE);
        dev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    }

    dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_LIGHTING, FALSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
}

static void DrawD3DTexturedQuad(IDirect3DTexture9* pTex, float x, float y, float w, float h, CRGBA col) {
    if (!pTex || col.a == 0) return;
    IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
    if (!dev) return;

    DWORD dwCol = D3DCOLOR_ARGB(col.a, col.r, col.g, col.b);
    D3D2DVert quad[4] = {
        { x - 0.5f,     y - 0.5f,     0.0f, 1.0f, dwCol, 0.0f, 0.0f },
        { x + w - 0.5f, y - 0.5f,     0.0f, 1.0f, dwCol, 1.0f, 0.0f },
        { x - 0.5f,     y + h - 0.5f, 0.0f, 1.0f, dwCol, 0.0f, 1.0f },
        { x + w - 0.5f, y + h - 0.5f, 0.0f, 1.0f, dwCol, 1.0f, 1.0f },
    };

    BeginRawD3DQuad(dev);
    dev->SetTexture(0, pTex);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(D3D2DVert));
    dev->SetTexture(0, NULL);
}

static void DrawD3DTexturedQuadAdditive(IDirect3DTexture9* pTex, float x, float y, float w, float h, CRGBA col) {
    if (!pTex || col.a == 0) return;
    IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
    if (!dev) return;

    DWORD dwCol = D3DCOLOR_ARGB(col.a, col.r, col.g, col.b);
    D3D2DVert quad[4] = {
        { x - 0.5f,     y - 0.5f,     0.0f, 1.0f, dwCol, 0.0f, 0.0f },
        { x + w - 0.5f, y - 0.5f,     0.0f, 1.0f, dwCol, 1.0f, 0.0f },
        { x - 0.5f,     y + h - 0.5f, 0.0f, 1.0f, dwCol, 0.0f, 1.0f },
        { x + w - 0.5f, y + h - 0.5f, 0.0f, 1.0f, dwCol, 1.0f, 1.0f },
    };

    BeginRawD3DQuad(dev);
    dev->SetTexture(0, pTex);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

    dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(D3D2DVert));
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    dev->SetTexture(0, NULL);
}

static void DrawD3DRotatedQuad(IDirect3DTexture9* pTex, float cx, float cy, float w, float h, float angle, CRGBA col) {
    if (!pTex || col.a == 0) return;
    IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
    if (!dev) return;

    float cosA = cosf(angle);
    float sinA = sinf(angle);
    float hw = w * 0.5f;
    float hh = h * 0.5f;

    float dxW = hw * cosA;
    float dyW = hw * sinA;
    float dxH = hh * sinA;
    float dyH = hh * cosA;

    // Top-Left: -W, -H
    float x0 = cx - dxW + dxH - 0.5f;
    float y0 = cy - dyW - dyH - 0.5f;
    // Top-Right: +W, -H
    float x1 = cx + dxW + dxH - 0.5f;
    float y1 = cy + dyW - dyH - 0.5f;
    // Bottom-Left: -W, +H
    float x2 = cx - dxW - dxH - 0.5f;
    float y2 = cy - dyW + dyH - 0.5f;
    // Bottom-Right: +W, +H
    float x3 = cx + dxW - dxH - 0.5f;
    float y3 = cy + dyW + dyH - 0.5f;

    DWORD dwCol = D3DCOLOR_ARGB(col.a, col.r, col.g, col.b);
    D3D2DVert quad[4] = {
        { x0, y0, 0.0f, 1.0f, dwCol, 0.0f, 0.0f },
        { x1, y1, 0.0f, 1.0f, dwCol, 1.0f, 0.0f },
        { x2, y2, 0.0f, 1.0f, dwCol, 0.0f, 1.0f },
        { x3, y3, 0.0f, 1.0f, dwCol, 1.0f, 1.0f },
    };

    BeginRawD3DQuad(dev);
    dev->SetTexture(0, pTex);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(D3D2DVert));
    dev->SetTexture(0, NULL);
}

// =============================================================================
// Custom Weapon PNG Icon Caching
// =============================================================================
static IDirect3DTexture9* s_customWepIcons[60] = {};
static bool s_customWepIconChecked[60] = {};

static const char* GetWeaponNameString(eWeaponType type) {
    switch (type) {
        case WEAPONTYPE_UNARMED:          return "fist";
        case WEAPONTYPE_BRASSKNUCKLE:     return "brassknuckle";
        case WEAPONTYPE_GOLFCLUB:         return "golfclub";
        case WEAPONTYPE_NIGHTSTICK:       return "nitestick";
        case WEAPONTYPE_KNIFE:            return "knife";
        case WEAPONTYPE_BASEBALLBAT:      return "bat";
        case WEAPONTYPE_SHOVEL:           return "shovel";
        case WEAPONTYPE_POOLCUE:          return "poolcue";
        case WEAPONTYPE_KATANA:           return "katana";
        case WEAPONTYPE_CHAINSAW:         return "chainsaw";
        case WEAPONTYPE_DILDO1:           return "dildo1";
        case WEAPONTYPE_DILDO2:           return "dildo2";
        case WEAPONTYPE_VIBE1:            return "vibe1";
        case WEAPONTYPE_VIBE2:            return "vibe2";
        case WEAPONTYPE_FLOWERS:          return "flowers";
        case WEAPONTYPE_CANE:             return "cane";
        case WEAPONTYPE_GRENADE:          return "grenade";
        case WEAPONTYPE_TEARGAS:          return "teargas";
        case WEAPONTYPE_MOLOTOV:          return "molotov";
        case WEAPONTYPE_PISTOL:           return "colt45";
        case WEAPONTYPE_PISTOL_SILENCED:  return "silenced";
        case WEAPONTYPE_DESERT_EAGLE:     return "desert_eagle";
        case WEAPONTYPE_SHOTGUN:          return "chromegun";
        case WEAPONTYPE_SAWNOFF:          return "sawnoff";
        case WEAPONTYPE_SPAS12:           return "shotgspa";
        case WEAPONTYPE_MICRO_UZI:        return "micro_uzi";
        case WEAPONTYPE_MP5:              return "mp5";
        case WEAPONTYPE_AK47:             return "ak47";
        case WEAPONTYPE_M4:               return "m4";
        case WEAPONTYPE_TEC9:             return "tec9";
        case WEAPONTYPE_COUNTRYRIFLE:     return "cuntgun";
        case WEAPONTYPE_SNIPERRIFLE:      return "sniper";
        case WEAPONTYPE_RLAUNCHER:        return "rocketla";
        case WEAPONTYPE_RLAUNCHER_HS:     return "heatseek";
        case WEAPONTYPE_FTHROWER:         return "flame";
        case WEAPONTYPE_MINIGUN:          return "minigun";
        case WEAPONTYPE_SATCHEL_CHARGE:   return "satchel";
        case WEAPONTYPE_DETONATOR:        return "bomb";
        case WEAPONTYPE_SPRAYCAN:         return "spraycan";
        case WEAPONTYPE_EXTINGUISHER:     return "fire_ex";
        case WEAPONTYPE_CAMERA:           return "camera";
        case WEAPONTYPE_NIGHTVISION:      return "nvgoggles";
        case WEAPONTYPE_INFRARED:         return "irgoggles";
        case WEAPONTYPE_PARACHUTE:        return "parachute";
        default:                          return nullptr;
    }
}

static const char* GetWeaponAltNameString(eWeaponType type) {
    switch (type) {
        case WEAPONTYPE_UNARMED:          return "unarmed";
        case WEAPONTYPE_PISTOL:           return "pistol";
        case WEAPONTYPE_SHOTGUN:          return "shotgun";
        case WEAPONTYPE_SPAS12:           return "spas12";
        case WEAPONTYPE_COUNTRYRIFLE:     return "rifle";
        case WEAPONTYPE_RLAUNCHER:        return "rpg";
        case WEAPONTYPE_FTHROWER:         return "flamethrower";
        case WEAPONTYPE_EXTINGUISHER:     return "extinguisher";
        case WEAPONTYPE_NIGHTSTICK:       return "nightstick";
        case WEAPONTYPE_BASEBALLBAT:      return "baseballbat";
        case WEAPONTYPE_DETONATOR:        return "detonator";
        default:                          return nullptr;
    }
}

static IDirect3DTexture9* GetCustomWeaponIcon(eWeaponType type) {
    if ((int)type < 0 || (int)type >= 60) return nullptr;
    if (s_customWepIconChecked[type]) return s_customWepIcons[type];

    s_customWepIconChecked[type] = true;
    const char* name1 = GetWeaponNameString(type);
    if (name1) {
        char path[128];
        snprintf(path, sizeof(path), "weapon\\%s.dds", name1);
        s_customWepIcons[type] = LoadTexture(path);
    }
    if (!s_customWepIcons[type]) {
        const char* name2 = GetWeaponAltNameString(type);
        if (name2) {
            char path[128];
            snprintf(path, sizeof(path), "weapon\\%s.dds", name2);
            s_customWepIcons[type] = LoadTexture(path);
        }
    }
    return s_customWepIcons[type];
}

static void ReleaseCustomWeaponIcons() {
    for (int i = 0; i < 60; ++i) {
        if (s_customWepIcons[i]) {
            s_customWepIcons[i]->Release();
            s_customWepIcons[i] = nullptr;
        }
        s_customWepIconChecked[i] = false;
    }
}

// =============================================================================
