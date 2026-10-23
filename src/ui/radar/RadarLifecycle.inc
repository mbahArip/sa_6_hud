static IDirect3DTexture9* g_pBlurRT = nullptr;
static int s_lastBlurWidth = 0;
static int s_lastBlurHeight = 0;
static int s_lastBlurQuality = 0;

void Radar::OnD3dLost() {
    if (g_pBlurRT) {
        g_pBlurRT->Release();
        g_pBlurRT = nullptr;
    }
    s_lastBlurWidth = 0;
    s_lastBlurHeight = 0;
    s_lastBlurQuality = 0;
    DestroyShaders();
}

void Radar::OnD3dReset() {
    CreateShaders();
}

void Radar::DrawBackdrop(const CRect& rect, float alpha) {
    if (g_config.enableBackdropBlur && alpha > 0.01f) {
        IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
        if (dev) {
            int sw = RsGlobal.maximumWidth;
            int sh = RsGlobal.maximumHeight;
            if (sw > 100 && sh > 100) {
                if (g_pBlurRT && (sw != s_lastBlurWidth || sh != s_lastBlurHeight || g_config.blurQuality != s_lastBlurQuality)) {
                    g_pBlurRT->Release();
                    g_pBlurRT = nullptr;
                }

                IDirect3DSurface9* pBackBuf = nullptr;
                if (SUCCEEDED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuf))) {
                    D3DSURFACE_DESC desc;
                    if (SUCCEEDED(pBackBuf->GetDesc(&desc))) {
                        // Downscale quality divisor (1 = 8x, 2 = 4x, 3 = 2x, 4 = 1x native)
                        int div = (g_config.blurQuality == 1) ? 8 : (g_config.blurQuality == 3) ? 2 : (g_config.blurQuality == 4) ? 1 : 4;
                        int tw = std::max(sw / div, 32);
                        int th = std::max(sh / div, 32);

                        if (!g_pBlurRT) {
                            if (SUCCEEDED(dev->CreateTexture(tw, th, 1, D3DUSAGE_RENDERTARGET, desc.Format, D3DPOOL_DEFAULT, &g_pBlurRT, NULL))) {
                                s_lastBlurWidth = sw;
                                s_lastBlurHeight = sh;
                                s_lastBlurQuality = g_config.blurQuality;
                            }
                        }

                        if (g_pBlurRT) {
                            IDirect3DSurface9* pRTSurf = nullptr;
                            if (SUCCEEDED(g_pBlurRT->GetSurfaceLevel(0, &pRTSurf))) {
                                if (SUCCEEDED(dev->StretchRect(pBackBuf, NULL, pRTSurf, NULL, D3DTEXF_LINEAR))) {
                                    DrawD3DTexturedRoundedFill(dev, g_pBlurRT, rect, Radius(), (float)sw, (float)sh, CRGBA(255, 255, 255, (unsigned char)(255 * alpha)));
                                }
                                pRTSurf->Release();
                            }
                        }
                    }
                    pBackBuf->Release();
                }
            }
        }
    }

    float unifiedOpacity = g_config.radarOpacity * alpha;
    CRGBA col = ModColor(g_config.backdropColor, unifiedOpacity);
    DrawRoundedFill(rect, Radius(), col);
}

static bool GameplayRunning() {
    if (CCutsceneMgr::ms_running || CCutsceneMgr::ms_cutsceneProcessing) return false;
    if (FrontEndMenuManager.m_bMenuActive) return false;
    return true;
}

typedef void(*StepFn)();
static void GuardedRun(StepFn step, const char* tag) {
    __try {
        step();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        g_radarDead = true;
        LogWrite("radar error caught in %s — fallback active", tag);
    }
}

static void* g_pSearchRadiusPS = nullptr;
static void* g_pPoliceAuraPS   = nullptr;

static void ForgetShaders();

void Radar::CreateShaders() {
    IDirect3DDevice9* currentDevice = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
    if (currentDevice && currentDevice != g_textureDevice) {
        // Device changed under us (save load recreates it without firing the
        // lost/reset events). Every cached texture belongs to the old device, so
        // drop them all before rebuilding. Pointers are non-null but dead.
        if (g_textureDevice) {
            ForgetShaders();
        }
        g_textureDevice = currentDevice;
    }

    if (!g_pSearchRadiusPS) {
        RwBool ok = RwD3D9CreatePixelShader((const RwUInt32*)g_psSearchRadius, &g_pSearchRadiusPS);
        LogWrite("shader: SearchRadiusPS created %s (%p)", ok ? "OK" : "FAILED", g_pSearchRadiusPS);
    }
    if (!g_pPoliceAuraPS) {
        RwBool ok = RwD3D9CreatePixelShader((const RwUInt32*)g_psPoliceAura, &g_pPoliceAuraPS);
        LogWrite("shader: PoliceAuraPS created %s (%p)", ok ? "OK" : "FAILED", g_pPoliceAuraPS);
    }
    if (!g_pBackdropBlurPS) {
        RwBool ok = RwD3D9CreatePixelShader((const RwUInt32*)g_psBackdropBlur, &g_pBackdropBlurPS);
        LogWrite("shader: BackdropBlurPS created %s (%p)", ok ? "OK" : "FAILED", g_pBackdropBlurPS);
    }

    if (!g_pTexStarBase) {
        g_pTexStarBase = LoadTexture("icons\\wantedStar.dds");
    }
    if (!g_pTexStarBorder) {
        g_pTexStarBorder = LoadTexture("icons\\wantedStarBorder.dds");
    }
    if (!g_pTexPersonnelArc) {
        g_pTexPersonnelArc = LoadTexture("ui\\cops_arrow.dds");
    }
    if (!g_pTexProfileIdentity) {
        g_pTexProfileIdentity = LoadTexture("icons\\profile\\identity.dds");
    }
    if (!g_pTexProfileVehicle) {
        g_pTexProfileVehicle = LoadTexture("icons\\profile\\vehicle.dds");
    }
    if (!g_pTexProfileOutfit) {
        g_pTexProfileOutfit = LoadTexture("icons\\profile\\outfit.dds");
    }
    if (!g_pTexProfileWeapon) {
        g_pTexProfileWeapon = LoadTexture("icons\\profile\\weapon.dds");
    }
    if (!g_pTexIfruit)      g_pTexIfruit      = LoadTexture("icons\\ifruit.dds");
    if (!g_pTexMission)     g_pTexMission     = LoadTexture("blips\\mission.dds");
    if (!g_pTexZAbove)      g_pTexZAbove      = LoadTexture("blips\\z_above.dds");
    if (!g_pTexZBelow)      g_pTexZBelow      = LoadTexture("blips\\z_below.dds");
    if (!g_pTexPlayer)      g_pTexPlayer      = LoadTexture("blips\\player.dds");
    if (!g_pTexPlayerWanted)g_pTexPlayerWanted= LoadTexture("blips\\player_wanted.dds");
    if (!g_pTexNorth)       g_pTexNorth       = LoadTexture("blips\\north.dds");
    if (!g_pTexEngine)      g_pTexEngine      = LoadTexture("icons\\engine.dds");
    if (!g_pTexFuel)        g_pTexFuel        = LoadTexture("icons\\fuel.dds");
    if (!g_pTexNitro)       g_pTexNitro       = LoadTexture("icons\\nitro.dds");
    if (!g_pTexWallet)      g_pTexWallet      = LoadTexture("icons\\wallet.dds");
}

// Drops every cached pointer WITHOUT releasing. Used when the D3D device was
// recreated behind our back: the old resources died with the old device, so
// calling Release() on them would touch freed memory.
static void ForgetShaders() {
    g_pSearchRadiusPS = nullptr;
    g_pPoliceAuraPS   = nullptr;
    g_pBackdropBlurPS = nullptr;

    g_pTexStarBase = nullptr;
    g_pTexStarBorder = nullptr;
    g_pTexIfruit = nullptr;
    g_pTexMission = nullptr;
    g_pTexZAbove = nullptr;
    g_pTexZBelow = nullptr;
    g_pTexPlayer = nullptr;
    g_pTexPlayerWanted = nullptr;
    g_pTexNorth = nullptr;
    g_pTexEngine = nullptr;
    g_pTexFuel = nullptr;
    g_pTexNitro = nullptr;
    g_pTexWallet = nullptr;
    g_pTexPersonnelArc = nullptr;
    g_pTexProfileIdentity = nullptr;
    g_pTexProfileVehicle = nullptr;
    g_pTexProfileOutfit = nullptr;
    g_pTexProfileWeapon = nullptr;
    g_pTexSoftGlow = nullptr;

    for (int i = 0; i < 60; ++i) {
        s_customWepIcons[i] = nullptr;
        s_customWepIconChecked[i] = false;
    }

    for (int i = 0; i < 144; ++i) {
        s_mapTiles[i].pDayTex = nullptr;
        s_mapTiles[i].pNightTex = nullptr;
        s_mapTiles[i].searchedDay = false;
        s_mapTiles[i].searchedNight = false;
    }
}

void Radar::DestroyShaders() {
    if (g_pSearchRadiusPS) {
        ((IDirect3DPixelShader9*)g_pSearchRadiusPS)->Release();
        g_pSearchRadiusPS = nullptr;
    }
    if (g_pPoliceAuraPS) {
        ((IDirect3DPixelShader9*)g_pPoliceAuraPS)->Release();
        g_pPoliceAuraPS = nullptr;
    }
    if (g_pBackdropBlurPS) {
        ((IDirect3DPixelShader9*)g_pBackdropBlurPS)->Release();
        g_pBackdropBlurPS = nullptr;
    }

    if (g_pTexStarBase)    { g_pTexStarBase->Release();    g_pTexStarBase = nullptr; }
    if (g_pTexStarBorder)  { g_pTexStarBorder->Release();  g_pTexStarBorder = nullptr; }
    if (g_pTexIfruit)      { g_pTexIfruit->Release();      g_pTexIfruit = nullptr; }
    if (g_pTexMission)     { g_pTexMission->Release();     g_pTexMission = nullptr; }
    if (g_pTexZAbove)      { g_pTexZAbove->Release();      g_pTexZAbove = nullptr; }
    if (g_pTexZBelow)      { g_pTexZBelow->Release();      g_pTexZBelow = nullptr; }
    if (g_pTexPlayer)      { g_pTexPlayer->Release();      g_pTexPlayer = nullptr; }
    if (g_pTexPlayerWanted){ g_pTexPlayerWanted->Release();g_pTexPlayerWanted = nullptr; }
    if (g_pTexNorth)       { g_pTexNorth->Release();       g_pTexNorth = nullptr; }
    if (g_pTexEngine)      { g_pTexEngine->Release();      g_pTexEngine = nullptr; }
    if (g_pTexFuel)        { g_pTexFuel->Release();        g_pTexFuel = nullptr; }
    if (g_pTexNitro)       { g_pTexNitro->Release();       g_pTexNitro = nullptr; }
    if (g_pTexWallet)      { g_pTexWallet->Release();      g_pTexWallet = nullptr; }
    if (g_pTexPersonnelArc){ g_pTexPersonnelArc->Release();g_pTexPersonnelArc = nullptr; }
    if (g_pTexProfileIdentity){ g_pTexProfileIdentity->Release(); g_pTexProfileIdentity = nullptr; }
    if (g_pTexProfileVehicle) { g_pTexProfileVehicle->Release();  g_pTexProfileVehicle = nullptr; }
    if (g_pTexProfileOutfit)  { g_pTexProfileOutfit->Release();   g_pTexProfileOutfit = nullptr; }
    if (g_pTexProfileWeapon)  { g_pTexProfileWeapon->Release();   g_pTexProfileWeapon = nullptr; }
    if (g_pTexSoftGlow)       { g_pTexSoftGlow->Release();        g_pTexSoftGlow = nullptr; }
    ReleaseCustomWeaponIcons();

    for (int i = 0; i < 144; ++i) {
        if (s_mapTiles[i].pDayTex)   { s_mapTiles[i].pDayTex->Release();   s_mapTiles[i].pDayTex = nullptr; }
        if (s_mapTiles[i].pNightTex) { s_mapTiles[i].pNightTex->Release(); s_mapTiles[i].pNightTex = nullptr; }
        s_mapTiles[i].searchedDay = false;
        s_mapTiles[i].searchedNight = false;
    }
}

void Radar::Init() {
    // Installs only the hooks whose module is enabled in the config. Modules the
    // user disabled are never patched, so they cost nothing at runtime.
    SyncModulePatches();
    g_radarDead = false;
    g_warmupFrames = 0;
    for (int i = 0; i < (int)HudModuleId::Count; ++i) {
        g_moduleAlpha[i] = 1.0f;
    }
    g_curDist = g_config.radarZoom;
    float elevDeg = g_config.radarTiltDeg;
    float pitchDeg = 90.0f - elevDeg;
    if (pitchDeg < 0.0f) pitchDeg = 0.0f;
    g_curTiltRad = pitchDeg * kPi / 180.0f;
    g_curVCenter = 0.70f;
    CreateShaders();
    UpdateRadarRect();
    for (int i = 0; i < GAUGE_COUNT; ++i) {
        s_gaugeSlots[i] = GaugeAnimSlot();
    }
    LogWrite("radar: initialized");
}

void Radar::Shutdown() {
    DestroyShaders();
    g_pathCount = 0;
    g_pathFrameTicker = 0;
    WipeAllHeatAndSearch();
}

void Radar::Restart() {
    Shutdown();
    ResetMoneyAnimation();
    // Shutdown() just released every texture and shader. The device is live again
    // by the time restartGameEvent fires, so rebuild here instead of waiting for a
    // d3dReset event that a save load never sends.
    CreateShaders();
    g_warmupFrames = 0;
    g_radarAlphaFade = 0.0f;
    g_radarBlinkTimer = 1.0f;
    UpdateRadarRect();
    for (int i = 0; i < GAUGE_COUNT; ++i) {
        s_gaugeSlots[i] = GaugeAnimSlot();
    }
}

void Radar::SetPin(float x, float y, float z) {
    g_pinPos = CVector(x, y, z);
    g_pinActive = true;
    g_pathFrameTicker = 99;
}

void Radar::ClearPin() {
    g_pinActive = false;
    g_pathCount = 0;
}

