static void WipeAllHeatAndSearch() {
    g_wantedActive = false;
    g_wantedStage = WANTED_STAGE_INACTIVE;
    g_copHasVisual = false;
    g_escapeTimer = 0.0f;
    g_heatEscapeTimer = 0.0f;
    g_visualLossTimer = 0.0f;
    g_lastCopCompositionWanted = 0;
    ReleasePluginPoliceUnits();
    g_innerSearchAlpha = 1.0f;

    g_hasIdentityHeat = false;
    g_hotHairKey = 0;
    g_wasMaskedWhenSpotted = false;
    g_hollowStarsCount = 0;
    g_hollowStarsTimer = 0.0f;
    for (int i = 0; i < kMaxSnitches; ++i) g_snitches[i].active = false;

    g_hasClothingHeat = false;
    memset(g_hotClothesKeys, 0, sizeof(g_hotClothesKeys));
    g_footVisualTimer = 0.0f;
    g_carVisualTimer = 0.0f;

    ClearAllHotVehicles();

    g_hasGunHeat = false;
    g_hotWeaponType = WEAPONTYPE_UNARMED;
    g_gunShotCount = 0;
    g_gunContinuousFireTimer = 0.0f;
    g_gunCooldownTimer = 0.0f;

    g_copScanningPlayer = false;
    g_copScanProgress = 0.0f;
    g_vehicleCleanupScore = 0;
    g_clothingCleanupScore = 0;
    g_weaponCleanupScore = 0;
    g_clearedVehicle = nullptr;
    g_clearedVehicleModel = 0;
    g_clearedVehicleColor1 = 0;
    g_clearedVehicleColor2 = 0;
    g_clearedVehiclePlate = nullptr;
    g_currentVehicleHeat = nullptr;
}

// Persistent TransFender and mod shop garage unlocker using native SCM scripting opcodes
static void UnlockTransFenderGarages() {
    static unsigned int s_lastUnlockTick = 0;
    unsigned int curTick = CTimer::m_snTimeInMillisecondsNonClipped;
    if (curTick - s_lastUnlockTick < 2000) return;
    s_lastUnlockTick = curTick;

    plugin::scripting::CallCommandById(COMMAND_CHANGE_GARAGE_TYPE, (char*)"MODLAST", 35);
    plugin::scripting::CallCommandById(COMMAND_CHANGE_GARAGE_TYPE, (char*)"MODSF",   35);
    plugin::scripting::CallCommandById(COMMAND_CHANGE_GARAGE_TYPE, (char*)"MODLV",   35);
    plugin::scripting::CallCommandById(COMMAND_CHANGE_GARAGE_TYPE, (char*)"LOWRIDS", 33);
    plugin::scripting::CallCommandById(COMMAND_CHANGE_GARAGE_TYPE, (char*)"WHEELAR", 34);

    plugin::scripting::CallCommandById(COMMAND_ACTIVATE_GARAGE, (char*)"MODLAST");
    plugin::scripting::CallCommandById(COMMAND_ACTIVATE_GARAGE, (char*)"MODSF");
    plugin::scripting::CallCommandById(COMMAND_ACTIVATE_GARAGE, (char*)"MODLV");
    plugin::scripting::CallCommandById(COMMAND_ACTIVATE_GARAGE, (char*)"LOWRIDS");
    plugin::scripting::CallCommandById(COMMAND_ACTIVATE_GARAGE, (char*)"WHEELAR");
}

static void DrawHeatBadges(bool wantedGaugeDrawn) {
    if (!g_config.useCriminalProfile) return;
    bool onMission = CTheScripts::IsPlayerOnAMission();
    if (onMission && !g_config.useCriminalProfileOnMission) return;

    CPlayerPed* player = FindPlayerPed(0);
    if (!player) return;

    bool isVehActive = IsCurrentVehicleHot(player);
    bool isGunActive = g_hasGunHeat || IsPlayerOpenlyCarrying(player);
    bool hasAnyBadge = (g_hasIdentityHeat || isVehActive || g_hasClothingHeat || isGunActive);
    bool isPreview = Menu::IsLayoutPreview();
    if (!hasAnyBadge && g_heatBadgeAnimProgress <= 0.005f && !isPreview) return;

    float screenW = (float)RsGlobal.maximumWidth;
    float screenH = (float)RsGlobal.maximumHeight;
    float starSize = 22.0f * (screenH / 720.0f);

    float badgeW = 34.0f * (screenW / 1280.0f);
    float badgeH = 15.0f * (screenH / 720.0f);
    float badgeGap = 5.0f * (screenW / 1280.0f);

    CRect wantedRect = g_moduleRects[(int)HudModuleId::Wanted];
    float starSlotH = starSize + 6.0f * (screenH / 720.0f);
    float wantedEase = g_wantedAnimProgress * g_wantedAnimProgress * (3.0f - 2.0f * g_wantedAnimProgress);
    float badgeY = wantedRect.top + starSlotH * wantedEase;
    float startX = wantedRect.right;
    float badgeAlpha = isPreview ? 1.0f : std::clamp(g_heatBadgeAnimProgress, 0.0f, 1.0f);

    bool blinkFast = ((CTimer::m_snTimeInMilliseconds / 150) % 2) == 0;

    static bool s_lastBadgeActive[4] = { false, false, false, false };
    if (hasAnyBadge) {
        s_lastBadgeActive[0] = g_hasIdentityHeat;
        s_lastBadgeActive[1] = isVehActive;
        s_lastBadgeActive[2] = g_hasClothingHeat;
        s_lastBadgeActive[3] = isGunActive;
    }

    struct HeatBadge {
        const char* label;
        bool active;
        bool isScanned;
        IDirect3DTexture9* pIcon;
    };

    HeatBadge badges[4] = {
        { "ID",  hasAnyBadge ? g_hasIdentityHeat : s_lastBadgeActive[0], g_copScanningPlayer && g_hasIdentityHeat, g_pTexProfileIdentity },
        { "VEH", hasAnyBadge ? isVehActive       : s_lastBadgeActive[1], g_copScanningPlayer && isVehActive,       g_pTexProfileVehicle },
        { "CLO", hasAnyBadge ? g_hasClothingHeat : s_lastBadgeActive[2], g_copScanningPlayer && g_hasClothingHeat, g_pTexProfileOutfit },
        { "GUN", hasAnyBadge ? isGunActive       : s_lastBadgeActive[3], g_copScanningPlayer && isGunActive && (g_hasGunHeat || IsPlayerOpenlyCarrying(player)), g_pTexProfileWeapon }
    };

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

    int drawnIndex = 0;
    for (int b = 0; b < 4; ++b) {
        if (!badges[b].active) continue;

        float bx = startX - badgeW - (float)drawnIndex * (badgeW + badgeGap);
        float by = badgeY;

        CRGBA bgCol(180, 25, 35, (unsigned char)(230.0f * badgeAlpha));
        CRGBA textCol(255, 255, 255, (unsigned char)(255.0f * badgeAlpha));

        if (badges[b].isScanned) {
            if (blinkFast) {
                bgCol = CRGBA(255, 255, 255, (unsigned char)(240.0f * badgeAlpha));
                textCol = CRGBA(180, 25, 35, (unsigned char)(255.0f * badgeAlpha));
            } else {
                bgCol = CRGBA(235, 30, 45, (unsigned char)(255.0f * badgeAlpha));
                textCol = CRGBA(255, 255, 255, (unsigned char)(255.0f * badgeAlpha));
            }
        }

        CSprite2d::DrawRect(CRect(bx, by, bx + badgeW, by + badgeH), bgCol);

        if (badges[b].pIcon) {
            float iconSize = badgeH * 0.85f;
            DrawD3DTexturedQuad(badges[b].pIcon, bx + (badgeW - iconSize) * 0.5f, by + (badgeH - iconSize) * 0.5f, iconSize, iconSize, textCol);
        } else {
            CFont::SetFontStyle(FONT_SUBTITLES);
            CFont::SetScale(screenH * 0.00030f, screenH * 0.00095f);
            CFont::SetProportional(true);
            CFont::SetJustify(false);
            CFont::SetOrientation(ALIGN_CENTER);
            CFont::SetEdge(0);
            CFont::SetDropShadowPosition(0);
            CFont::SetColor(textCol);
            CFont::PrintString(bx + badgeW * 0.5f, by + 1.2f * (screenH / 720.0f), (char*)badges[b].label);
        }

        drawnIndex++;
    }
}

static void DrawInvestigationGauge() {
    if (!g_config.useCriminalProfile) return;
    bool onMission = CTheScripts::IsPlayerOnAMission();
    if (onMission && !g_config.useCriminalProfileOnMission) return;
    if (CReplay::Mode == 1) return;

    CPlayerPed* player = FindPlayerPed(0);
    if (!player) return;

    CWanted* pWanted = (player && player->m_pPlayerData) ? player->m_pPlayerData->m_pWanted : nullptr;
    unsigned int wanted = pWanted ? pWanted->m_nWantedLevel : 0;
    if (wanted > 0) return;

    bool isVehActive = IsCurrentVehicleHot(player);
    bool isGunActive = g_hasGunHeat || IsPlayerOpenlyCarrying(player);
    bool hasAnyHeat = (g_hasIdentityHeat || g_hasClothingHeat || isVehActive || isGunActive);
    if (!hasAnyHeat) {
        if (g_heatBadgeAnimProgress > 0.005f) {
            DrawHeatBadges(false);
        }
        return;
    }

    float screenW = (float)RsGlobal.maximumWidth;
    float screenH = (float)RsGlobal.maximumHeight;
    float starSize = 22.0f * (screenH / 720.0f);
    float starSpacing = starSize * 1.15f;

    CRect wantedRect = g_moduleRects[(int)HudModuleId::Wanted];
    float totalGroupW = 5.0f * starSpacing + starSize;
    float startX = wantedRect.right - totalGroupW;
    float topMargin = wantedRect.top;

    int starsCount = std::clamp(g_lastKnownWantedStars, 1, 6);
    float starAlpha = std::clamp(g_wantedAnimProgress, 0.0f, 1.0f);
    unsigned char activeA = (unsigned char)(255.0f * starAlpha);
    unsigned char inactiveA = (unsigned char)(128.0f * starAlpha);

    // Always draw 6 slots: fill sequentially from right (i=5) to left (i=0)
    for (int i = 0; i < 6; ++i) {
        float x = startX + (float)i * starSpacing;
        float y = topMargin;

        bool isActive = (i >= (6 - starsCount));
        if (isActive) {
            IDirect3DTexture9* pActiveTex = g_pTexStarLooking ? g_pTexStarLooking : g_pTexStarHeat;
            if (pActiveTex) {
                DrawD3DTexturedQuad(pActiveTex, x, y, starSize, starSize, CRGBA(255, 255, 255, activeA));
            } else {
                CHud::Sprites[2].Draw(CRect(x, y, x + starSize, y + starSize), CRGBA(100, 180, 255, activeA));
            }
        } else {
            if (g_pTexStarBase) {
                DrawD3DTexturedQuad(g_pTexStarBase, x, y, starSize, starSize, CRGBA(255, 255, 255, inactiveA));
            } else {
                CHud::Sprites[2].Draw(CRect(x, y, x + starSize, y + starSize), CRGBA(255, 255, 255, inactiveA));
            }
        }
    }

    DrawHeatBadges(true);
}

static bool  s_wantedPatchesApplied = false;
static float s_starSlide[6]         = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
static float s_starGlowProgress[6]  = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
static bool  s_wantedGaugeEverShown = false;
static int   s_prevWantedLevel      = 0;

static void Hooked_DrawWanted() {
    CPlayerPed* player = FindPlayerPed(0);
    if (!player || !player->m_pPlayerData || !player->m_pPlayerData->m_pWanted) return;
    if (CReplay::Mode == 1) return;

    CWanted* pWanted = player->m_pPlayerData->m_pWanted;
    unsigned int wanted = pWanted->m_nWantedLevel;
    bool onParole = (pWanted->m_nTimeOfParole > 0) || pWanted->m_bPoliceBackOffGarage;
    bool isPreview = Menu::IsLayoutPreview();
    if (wanted == 0 && g_hollowStarsCount == 0 && !isPreview) {
        s_wantedGaugeEverShown = false;
        s_prevWantedLevel = 0;
        for (int i = 0; i < 6; ++i) {
            s_starSlide[i] = 0.0f;
            s_starGlowProgress[i] = 0.0f;
        }
        if (!onParole) {
            DrawInvestigationGauge();
        } else {
            // Keep heat badges (CLO, VEH, GUN) visible during garage respray
            DrawHeatBadges(false);
        }
        return;
    }

    if (!g_config.useWantedStar) {
        float screenW = (float)RsGlobal.maximumWidth;
        float screenH = (float)RsGlobal.maximumHeight;
        float starSize = 16.0f * (screenH / 448.0f);
        float starSpacing = starSize * 1.15f;
        float rightMargin = 28.0f * (screenW / 640.0f);
        float topMargin = 28.0f * (screenH / 448.0f);

        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

        unsigned int effWanted = (wanted > 0) ? wanted : g_hollowStarsCount;
        for (unsigned int i = 0; i < effWanted && i < 6; ++i) {
            float x = screenW - rightMargin - (float)(5 - i) * starSpacing;
            float y = topMargin;
            CHud::Sprites[2].Draw(CRect(x, y, x + starSize, y + starSize), CRGBA(255, 255, 255, 255));
        }
        return;
    }

    // Modern 6-Star GTA 6 HUD Wanted Gauge (filled left-to-right, anchored top-right)
    float screenW = (float)RsGlobal.maximumWidth;
    float screenH = (float)RsGlobal.maximumHeight;
    float starSize = 22.0f * (screenH / 720.0f);
    float starSpacing = starSize * 1.15f;

    CRect wantedRect = g_moduleRects[(int)HudModuleId::Wanted];
    float totalGroupW = 5.0f * starSpacing + starSize;
    float startX = wantedRect.right - totalGroupW;
    float topMargin = wantedRect.top;

    bool blinkOn = true;
    if (g_wantedStage == WANTED_STAGE_LKP) {
        blinkOn = ((CTimer::m_snTimeInMilliseconds / 250) % 2) == 0;
    }

    bool isVehActive = IsCurrentVehicleHot(player);
    bool hasActiveProfile = (g_hasIdentityHeat || g_hasClothingHeat || isVehActive || g_hasGunHeat);
    bool isHollowState = (wanted == 0 && g_hollowStarsCount > 0);
    int effectiveWanted = isHollowState ? g_hollowStarsCount : (int)wanted;
    if (isPreview && effectiveWanted == 0) effectiveWanted = 3;
    float starAlpha = isPreview ? 1.0f : std::clamp(g_wantedAnimProgress, 0.0f, 1.0f);
    unsigned char activeA = (unsigned char)(255.0f * starAlpha);
    unsigned char inactiveA = (unsigned char)(128.0f * starAlpha);

    float dt = std::clamp(CTimer::ms_fTimeStep * 0.02f, 0.001f, 0.1f);

    // Detect wanted level raise while gauge already visible: new star appears instantly (no slide), ALL active stars glow!
    if (s_wantedGaugeEverShown && effectiveWanted > s_prevWantedLevel) {
        for (int i = 0; i < 6; ++i) {
            bool isSlotActive = (i >= (6 - effectiveWanted));
            if (isSlotActive) {
                s_starSlide[i] = 1.0f;           // Immediately in place, NO slide animation!
                s_starGlowProgress[i] = 0.0001f; // All active stars glow at same time!
            }
        }
    }
    s_prevWantedLevel = effectiveWanted;

    // Update star slide & micro-glow states from right (i=5, Star 1) to left (i=0, Star 6)
    for (int i = 5; i >= 0; --i) {
        bool isActive = (i >= (6 - effectiveWanted));
        if (isActive) {
            if (!s_wantedGaugeEverShown) {
                // First time showing: slide in sequentially from right to left (300ms)
                bool canAdvance = (i == 5) || (s_starSlide[i + 1] >= 0.40f);
                if (canAdvance && s_starSlide[i] < 1.0f) {
                    s_starSlide[i] += dt / 0.30f; // 300ms duration
                    if (s_starSlide[i] >= 1.0f) {
                        s_starSlide[i] = 1.0f;
                    }
                }
            }
        } else {
            s_starSlide[i] = 0.0f;
            s_starGlowProgress[i] = 0.0f;
        }

        // Advance 500ms glow (0% -> 100% -> 0% bell curve)
        if (s_starGlowProgress[i] > 0.0f) {
            s_starGlowProgress[i] += dt / 0.50f;
            if (s_starGlowProgress[i] >= 1.0f) {
                s_starGlowProgress[i] = 0.0f;
            }
        }
    }

    // Mark gauge as shown once the first-time slide reaches resting position
    if (!s_wantedGaugeEverShown && s_starSlide[6 - effectiveWanted] >= 1.0f) {
        s_wantedGaugeEverShown = true;
        // Trigger synchronized 500ms glow on ALL active stars at same time!
        for (int i = 0; i < 6; ++i) {
            if (i >= (6 - effectiveWanted)) {
                s_starGlowProgress[i] = 0.0001f;
            }
        }
    }

    // Always draw 6 slots: fill sequentially from right (i=5) to left (i=0)
    for (int i = 0; i < 6; ++i) {
        float x = startX + (float)i * starSpacing;
        float y = topMargin;

        bool isActive = (i >= (6 - effectiveWanted));

        // 1. Inactive background base star for slot
        if (g_pTexStarBase) {
            DrawD3DTexturedQuad(g_pTexStarBase, x, y, starSize, starSize, CRGBA(255, 255, 255, inactiveA));
        } else {
            CHud::Sprites[2].Draw(CRect(x, y, x + starSize, y + starSize), CRGBA(255, 255, 255, inactiveA));
        }

        if (isActive) {
            IDirect3DTexture9* pActiveTex = g_pTexStarBase;
            if (isHollowState) {
                pActiveTex = g_pTexStarNoInfo ? g_pTexStarNoInfo : g_pTexStarBase;
            } else if (g_wantedStage == WANTED_STAGE_HEAT) {
                if (hasActiveProfile && g_pTexStarLooking) {
                    pActiveTex = g_pTexStarLooking;
                } else if (g_pTexStarHeat) {
                    pActiveTex = g_pTexStarHeat;
                }
            }

            // 2. Micro-glow behind star slot: 0% -> 100% -> 0% bell pulse over 500ms
            if (s_starGlowProgress[i] > 0.0f) {
                float p = s_starGlowProgress[i];
                float glowPulse = sinf(p * 3.14159265f); // 0.0 -> 1.0 -> 0.0 bell curve
                if (glowPulse > 0.001f) {
                    EnsureSoftGlowTexture();
                    float cx = x + starSize * 0.5f;
                    float cy = y + starSize * 0.5f;

                    if (g_pTexSoftGlow) {
                        // A. Outer soft warm halo (smooth Gaussian alpha blend)
                        float haloRad = starSize * (1.15f + p * 0.35f);
                        CRGBA haloCol = isHollowState
                            ? CRGBA(180, 180, 180, (unsigned char)(140.0f * glowPulse * starAlpha))
                            : CRGBA(255, 205, 80,  (unsigned char)(160.0f * glowPulse * starAlpha));
                        DrawD3DTexturedQuad(g_pTexSoftGlow,
                                           cx - haloRad, cy - haloRad, haloRad * 2.0f, haloRad * 2.0f,
                                           haloCol);

                        // B. Core bright additive corona (smooth Gaussian additive blend)
                        float coreRad = starSize * (0.85f + p * 0.20f);
                        CRGBA coreCol(255, 255, 255, (unsigned char)(210.0f * glowPulse * starAlpha));
                        DrawD3DTexturedQuadAdditive(g_pTexSoftGlow,
                                                   cx - coreRad, cy - coreRad, coreRad * 2.0f, coreRad * 2.0f,
                                                   coreCol);
                    }

                    // C. In-place star core brightness flash (unscaled to eliminate jagged polygon edges)
                    DrawD3DTexturedQuadAdditive(pActiveTex ? pActiveTex : g_pTexStarBase,
                                               x, y, starSize, starSize,
                                               CRGBA(255, 255, 255, (unsigned char)(160.0f * glowPulse * starAlpha)));
                }
            }

            // 3. Active star with right-to-left slide animation
            float slideProgress = s_starSlide[i];
            if (slideProgress > 0.005f) {
                float inv = 1.0f - slideProgress;
                float ease = 1.0f - inv * inv * inv; // Ease-out cubic

                float slideDist = 16.0f * (screenH / 720.0f);
                float curX = x + (1.0f - ease) * slideDist;
                float curY = y;

                unsigned char drawA = (unsigned char)(activeA * ease);

                if (g_wantedStage == WANTED_STAGE_LKP && !blinkOn && !isHollowState) {
                    // When blinking OFF during LKP, base star remains visible
                } else {
                    if (pActiveTex) {
                        DrawD3DTexturedQuad(pActiveTex, curX, curY, starSize, starSize, CRGBA(255, 255, 255, drawA));
                    } else {
                        CHud::Sprites[2].Draw(CRect(curX, curY, curX + starSize, curY + starSize), CRGBA(255, 255, 255, drawA));
                    }
                }
            }
        }
    }

    bool onMission = CTheScripts::IsPlayerOnAMission();
    bool isProfileActive = g_config.useCriminalProfile && (!onMission || g_config.useCriminalProfileOnMission);
    if (isProfileActive) {
        DrawHeatBadges(true);
    }
}

static void ApplyWantedPatches() {
    if (s_wantedPatchesApplied) return;
    s_wantedPatchesApplied = true;
    plugin::patch::RedirectJump(0x58D9A0, Hooked_DrawWanted);
    LogWrite("wanted: wanted star renderer hook installed at 0x58D9A0");
}

