static bool ShouldShowRadar() {
    if (!g_config.enabled || !g_playerOk || g_radarDead || !GameplayRunning()) return false;

    CPlayerPed* player = FindPlayerPed(0);
    if (!player) return false;

    CVehicle* veh = GetStablePlayerVehicle(player);
    bool inCar = (veh != nullptr);

    // Simplified radar visibility:
    // 1. Show when in vehicle.
    // 2. Hide on foot unless showOnFoot = true or holding/pressed SHOW_HUD_KEY.
    if (inCar) return true;
    if (g_config.showOnFoot) return true;
    if (g_hudKeyHeld || s_hudKeyHoldTimer > 0.0f) return true;

    return false;
}

void Radar::Process() {
    UpdateRadarRect();
    CheckHotReload();

    if (g_config.enabled != s_lastRadarEnabled ||
        g_config.enableGps != s_lastGpsEnabled ||
        g_config.enableWantedSearchRadius != s_lastSearchEnabled ||
        g_config.showInfoCards != s_lastCardsEnabled ||
        g_config.enableCopAuras != s_lastAurasEnabled ||
        g_config.enableCopArcs != s_lastArcsEnabled) {
        if (!g_config.enableGps) {
            g_pathCount = 0;
            g_pathFrameTicker = 0;
            g_gpsDistance = 0.0f;
        }
        if (!g_config.enableWantedSearchRadius) {
            g_wantedActive = false;
            g_copHasVisual = false;
            g_escapeTimer = 0.0f;
        }
        if (!g_config.showInfoCards) {
            s_locAlpha = 0.0f;
            s_vehCardAlpha = 0.0f;
            s_vehNameAlpha = 0.0f;
            s_vehNameTimer = 0.0f;
            s_pLastVehicle = nullptr;
        }
        if (!g_config.enabled) {
            s_radarFadeProgress = 0.0f;
            g_radarAlphaFade = 0.0f;
        }
        s_lastRadarEnabled = g_config.enabled;
        s_lastGpsEnabled = g_config.enableGps;
        s_lastSearchEnabled = g_config.enableWantedSearchRadius;
        s_lastCardsEnabled = g_config.showInfoCards;
        s_lastAurasEnabled = g_config.enableCopAuras;
        s_lastArcsEnabled = g_config.enableCopArcs;
    }

    UpdateInput();
    GuardedRun(UpdatePlayerData, "process.player");
    if (!g_playerOk) return;

    if (g_config.enabled)                  GuardedRun(UpdateCamera, "process.camera");
    if (g_config.enableGps)                GuardedRun(UpdateGps, "process.gps");
    if (g_config.enableWantedSearchRadius) GuardedRun(UpdateWantedSearch, "process.wanted");
    if (g_config.showInfoCards)            GuardedRun(UpdateInfoCards, "process.cards");
    GuardedRun(WriteDebugLog, "process.debug");

    if (g_config.enabled) {
        float dt = CTimer::ms_fTimeStep * 0.02f;
        if (dt <= 0.0f || dt > 0.1f) dt = 0.0166f;
        float duration = 0.300f; // 300ms default transition
        float rate = dt / duration;

        // Update Game Clock & HUD key state
        bool hudKeyHeld = false;
        if (g_config.showHudKey > 0 && !FrontEndMenuManager.m_bMenuActive) {
            hudKeyHeld = ((GetAsyncKeyState(g_config.showHudKey) & 0x8000) != 0)
                      || (CPad::NewKeyState.standardKeys[g_config.showHudKey] != 0);
        }
        g_hudKeyHeld = hudKeyHeld;
        if (hudKeyHeld) {
            s_hudKeyHoldTimer = g_config.uiIdleTimeout;
        } else if (s_hudKeyHoldTimer > 0.0f) {
            s_hudKeyHoldTimer -= dt;
            if (s_hudKeyHoldTimer < 0.0f) s_hudKeyHoldTimer = 0.0f;
        }

        bool showAll = hudKeyHeld || (s_hudKeyHoldTimer > 0.0f);
        g_hudShowAll = showAll;
        if (showAll) {
            s_clockFadeProgress = std::min(1.0f, s_clockFadeProgress + rate);
        } else {
            s_clockFadeProgress = std::max(0.0f, s_clockFadeProgress - rate);
        }

        bool visible = ShouldShowRadar();
        if (visible) {
            s_radarFadeProgress = std::min(1.0f, s_radarFadeProgress + rate);
        } else {
            s_radarFadeProgress = std::max(0.0f, s_radarFadeProgress - rate);
        }

        // Smoothstep ease-in-out curve
        g_radarAlphaFade = s_radarFadeProgress * s_radarFadeProgress * (3.0f - 2.0f * s_radarFadeProgress);

        // Update destination cross-fade (0 = iFruit icon, 1 = Distance Remaining)
        CVector destPos;
        bool hasDest = HasActiveDestination(destPos);
        if (hasDest) {
            s_destFade = std::min(1.0f, s_destFade + rate);
        } else {
            s_destFade = std::max(0.0f, s_destFade - rate);
        }

        // Update player puck wanted / pursuit color inversion fade
        // Invert to wanted player blip ONLY during active pursuit or line-of-sight loss (flashing stars).
        // Revert back to default black when in heat area, hollow star state, criminal profile, or peaceful.
        CPlayerPed* player = FindPlayerPed(0);
        bool wantedActive = (player && player->m_pPlayerData && player->m_pPlayerData->m_pWanted
                             && player->m_pPlayerData->m_pWanted->m_nWantedLevel > 0);
        bool isPursuitOrLkp = wantedActive && (g_hollowStarsCount == 0) &&
                              (g_wantedStage == WANTED_STAGE_PURSUIT || g_wantedStage == WANTED_STAGE_LKP ||
                               (g_wantedStage == WANTED_STAGE_INACTIVE && wantedActive));
        float targetWantedFade = (isPursuitOrLkp && g_wantedStage != WANTED_STAGE_HEAT) ? 1.0f : 0.0f;
        if (s_playerWantedFade < targetWantedFade) {
            s_playerWantedFade = std::min(targetWantedFade, s_playerWantedFade + rate);
        } else if (s_playerWantedFade > targetWantedFade) {
            s_playerWantedFade = std::max(targetWantedFade, s_playerWantedFade - rate);
        }
    } else {
        s_radarFadeProgress = 0.0f;
        g_radarAlphaFade = 0.0f;
        s_destFade = 0.0f;
        s_clockFadeProgress = 0.0f;
        s_hudKeyHoldTimer = 0.0f;
        s_playerWantedFade = 0.0f;
    }
}

static RwTexture* GetFirstTexture(RwTexDictionary* dict) {
    if (!dict) return nullptr;
    RwTexture* result = nullptr;
    RwTexDictionaryForAllTextures(dict, [](RwTexture* tex, void* data) -> RwTexture* {
        *(RwTexture**)data = tex;
        return nullptr;
    }, &result);
    return result;
}

