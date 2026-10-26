#pragma once

static float s_weaponIdleTimer = 4.0f;
static float s_weaponFadeAlpha = 1.0f;
static int   s_lastWepSlot     = -1;
static int   s_lastWepType     = -1;
static int   s_lastTotalAmmo   = -1;
static int   s_lastAmmoInClip  = -1;

static float s_moneyIdleTimer   = 4.0f;
static float s_moneyFadeAlpha   = 1.0f;
static float s_renderMoney      = -1.0f;
static int   s_lastMoney        = 0;
static bool  s_moneyInitialized = false;

// Money readout rolls from s_moneyAnimFrom to s_moneyAnimTo over kMoneyAnimDuration.
// A plain speed clamp could not bound the animation: a large balance gap took tens
// of seconds, so the counter was still rolling long after the cash changed.
static const float kMoneyAnimDuration = 3.0f;
static float s_moneyAnimFrom = 0.0f;
static float s_moneyAnimTo   = 0.0f;
static float s_moneyAnimT    = 1.0f;

// Called on new game and on save load. Restarting the counter at 0 makes the readout
// roll up to the session's starting balance instead of continuing from whatever the
// previous session left behind (load a rich save, then start a new game, and the
// counter used to count *down* from the old max).
static void ResetMoneyAnimation() {
    s_moneyInitialized = false;
    s_moneyAnimFrom = 0.0f;
    s_moneyAnimTo   = 0.0f;
    s_moneyAnimT    = 1.0f;
    s_renderMoney   = 0.0f;
    s_lastMoney     = 0;
    s_moneyIdleTimer = 4.0f;
    s_moneyFadeAlpha = 1.0f;
}

static float s_wepSlotProgress  = 0.0f;

static void DrawTopRightHUD(float masterAlpha) {
    CPlayerPed* player = FindPlayerPed(0);
    bool isPreview = Menu::IsLayoutPreview();
    if (!player && !isPreview) return;

    float sW = ScreenW();
    float sH = ScreenH();

    float dt = CTimer::ms_fTimeStep * 0.02f;
    if (dt <= 0.0f || dt > 0.1f) dt = 0.0166f;
    float rate = dt / 0.25f; // 250ms slide transition rate

    // =========================================================================
    // 0. Update Module Transition States
    // =========================================================================
    CWanted* wanted = (player->m_pPlayerData) ? player->m_pPlayerData->m_pWanted : nullptr;
    unsigned int wantedLevel = wanted ? wanted->m_nWantedLevel : 0;
    bool hasWantedStars = (wantedLevel > 0 || g_hollowStarsCount > 0);

    bool isVehActive = IsCurrentVehicleHot(player);
    bool isGunActive = g_hasGunHeat || IsPlayerOpenlyCarrying(player);
    bool hasAnyHeat = (g_hasIdentityHeat || g_hasClothingHeat || isVehActive || isGunActive);
    bool onMission = CTheScripts::IsPlayerOnAMission();

    bool hasBadges = false;
    if (g_config.useCriminalProfile && (!onMission || g_config.useCriminalProfileOnMission)) {
        hasBadges = hasAnyHeat;
    }

    bool isWantedActive = hasWantedStars;
    if (!isWantedActive && g_config.useCriminalProfile && !onMission && hasAnyHeat) {
        // Investigation star gauge is active
        isWantedActive = true;
    }

    if (isWantedActive) {
        g_wantedAnimProgress = std::min(1.0f, g_wantedAnimProgress + rate);
    } else {
        g_wantedAnimProgress = std::max(0.0f, g_wantedAnimProgress - rate);
    }

    if (hasBadges) {
        g_heatBadgeAnimProgress = std::min(1.0f, g_heatBadgeAnimProgress + rate);
    } else {
        g_heatBadgeAnimProgress = std::max(0.0f, g_heatBadgeAnimProgress - rate);
    }

    // =========================================================================
    // 1. Weapon HUD State & Activity Tracking
    // =========================================================================
    int slot = player->m_nSelectedWepSlot;
    CWeapon& wep = player->m_aWeapons[slot];
    eWeaponType wepType = wep.m_eWeaponType;
    int totalAmmo = (int)wep.m_nAmmoTotal;
    int ammoInClip = (int)wep.m_nAmmoInClip;
    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(wepType, player->GetWeaponSkill());
    int clipSize = info ? info->m_nAmmoClip : 0;

    int wepSlot = (info) ? (int)info->m_nSlot : slot;
    bool hasAmmo = (wepSlot >= 2 && wepSlot <= 9 && totalAmmo > 0);
    bool showWep = false;

    if (g_config.useWeaponUi) {
        bool wepChanged = (slot != s_lastWepSlot) || (wepType != s_lastWepType);
        bool ammoChanged = (totalAmmo != s_lastTotalAmmo) || (ammoInClip != s_lastAmmoInClip);
        s_lastWepSlot = slot;
        s_lastWepType = wepType;
        s_lastTotalAmmo = totalAmmo;
        s_lastAmmoInClip = ammoInClip;

        CPad* pad = CPad::GetPad(0);
        bool isAiming = (pad && pad->GetTarget()) || CPad::NewMouseControllerState.rmb;
        bool isFiring = (wep.m_nState == WEAPONSTATE_FIRING) || (wep.m_nState == WEAPONSTATE_RELOADING)
                     || (pad && (pad->NewState.ButtonCircle != 0 || pad->GetMeleeAttack() != 0))
                     || CPad::NewMouseControllerState.lmb;

        if (wepChanged || ammoChanged || isAiming || isFiring) {
            s_weaponIdleTimer = g_config.uiIdleTimeout;
        } else if (s_weaponIdleTimer > 0.0f) {
            s_weaponIdleTimer -= dt;
            if (s_weaponIdleTimer < 0.0f) s_weaponIdleTimer = 0.0f;
        }

        showWep = g_config.alwaysShowWeapon || (s_weaponIdleTimer > 0.0f) || isAiming || isFiring || g_hudShowAll;
        if (showWep) {
            s_weaponFadeAlpha = std::min(1.0f, s_weaponFadeAlpha + rate);
        } else {
            s_weaponFadeAlpha = std::max(0.0f, s_weaponFadeAlpha - rate);
        }
    } else {
        s_weaponFadeAlpha = std::max(0.0f, s_weaponFadeAlpha - rate);
    }

    // =========================================================================
    // 2. Accordion Slide Layout Calculation
    // =========================================================================
    auto Smoothstep = [](float t) -> float {
        float c = std::clamp(t, 0.0f, 1.0f);
        return c * c * (3.0f - 2.0f * c);
    };

    // When Show HUD key ('J') activates while elements are hidden, pre-expand slots while opacity is 0
    if (g_hudShowAll && s_weaponFadeAlpha <= 0.05f) {
        s_wepSlotProgress = 1.0f;
        if (hasAmmo) g_ammoSlideProgress = 1.0f;
    }

    if (g_config.useWeaponUi && showWep) {
        s_wepSlotProgress = std::min(1.0f, s_wepSlotProgress + rate);
    } else if (s_weaponFadeAlpha <= 0.01f) {
        // Only collapse slot after weapon has fully faded out
        s_wepSlotProgress = std::max(0.0f, s_wepSlotProgress - rate);
    }

    bool ammoActive = g_config.useWeaponUi && hasAmmo && showWep;
    if (ammoActive) {
        g_ammoSlideProgress = std::min(1.0f, g_ammoSlideProgress + rate);
    } else if (showWep && !hasAmmo) {
        // Weapon switched to melee/fist while HUD visible -> smooth upward glide
        g_ammoSlideProgress = std::max(0.0f, g_ammoSlideProgress - rate);
    } else if (s_weaponFadeAlpha <= 0.01f) {
        // Full weapon HUD hidden -> collapse only after fade out is complete
        g_ammoSlideProgress = std::max(0.0f, g_ammoSlideProgress - rate);
    }

    float wantedEase = Smoothstep(g_wantedAnimProgress);
    float badgeEase  = Smoothstep(g_heatBadgeAnimProgress);
    float ammoEase   = Smoothstep(g_ammoSlideProgress);
    float iconEase   = Smoothstep(s_wepSlotProgress);

    float ammoSlotH   = 26.0f * (sH / 720.0f);
    float wepIconSize = 44.0f * (sH / 720.0f);

    // Publish live visibility so the group stack solver can collapse hidden slots.
    g_moduleAlpha[(int)HudModuleId::Wanted]    = std::max(wantedEase, badgeEase);
    g_moduleAlpha[(int)HudModuleId::WeaponHud] = std::max(ammoEase, iconEase);
    g_moduleAlpha[(int)HudModuleId::MoneyHud]  = s_moneyFadeAlpha;

    // Preview shows the resting layout; gameplay uses the alpha-collapsed group stack.
    const CRect* rects = isPreview ? g_moduleRects : g_animatedModuleRects;

    CRect wepRc  = rects[(int)HudModuleId::WeaponHud];
    CRect monRc  = rects[(int)HudModuleId::MoneyHud];
    float wepRightEdge   = wepRc.right;
    float moneyRightEdge = monRc.right;
    float ammoY = wepRc.top;
    float iconY = ammoY + ammoSlotH;
    float moneyY = monRc.top;

    // =========================================================================
    // 3. Render Weapon Ammo & Weapon Icon
    // =========================================================================
    float currentAmmoAlpha = isPreview ? 1.0f : (s_weaponFadeAlpha * g_ammoSlideProgress * masterAlpha);
    if ((g_config.useWeaponUi || isPreview) && currentAmmoAlpha > 0.005f) {
        static char s_lastCurAmmoStr[32] = "";
        static char s_lastTotAmmoStr[32] = "";

        if (isPreview && s_lastCurAmmoStr[0] == '\0') {
            snprintf(s_lastCurAmmoStr, sizeof(s_lastCurAmmoStr), "30");
            snprintf(s_lastTotAmmoStr, sizeof(s_lastTotAmmoStr), "300");
        } else if (hasAmmo) {
            if (clipSize <= 1) {
                snprintf(s_lastCurAmmoStr, sizeof(s_lastCurAmmoStr), "%d", totalAmmo);
                s_lastTotAmmoStr[0] = '\0';
            } else {
                snprintf(s_lastCurAmmoStr, sizeof(s_lastCurAmmoStr), "%d", ammoInClip);
                if (g_config.weaponTotalAmmoType == 1) {
                    // TOTAL_CLIP mode: (totalAmmo - ammoInClip) / clipSize
                    int reserveClips = (totalAmmo > ammoInClip) ? ((totalAmmo - ammoInClip) / clipSize) : 0;
                    snprintf(s_lastTotAmmoStr, sizeof(s_lastTotAmmoStr), "%d", reserveClips);
                } else {
                    // TOTAL_AMMO mode
                    snprintf(s_lastTotAmmoStr, sizeof(s_lastTotAmmoStr), "%d", totalAmmo);
                }
            }
        }

        if (s_lastCurAmmoStr[0] != '\0') {
            // 20px font scale: 16px was 0.00065f / 0.00130f, 20px = 1.25x
            float fontScaleX = sH * 0.0008125f;
            float fontScaleY = sH * 0.001625f;

            CFont::SetFontStyle(FONT_PRICEDOWN);
            CFont::SetScale(fontScaleX, fontScaleY);
            CFont::SetProportional(true);
            CFont::SetJustify(false);
            CFont::SetOrientation(ALIGN_LEFT);
            CFont::SetEdge(1);
            CFont::SetDropColor(ModColor(g_config.weaponAmmoStrokeColor, currentAmmoAlpha));

            float gap = 12.0f * (sH / 720.0f);
            float totW = (s_lastTotAmmoStr[0] != '\0') ? CFont::GetStringWidth(s_lastTotAmmoStr, true) : 0.0f;
            float curW = CFont::GetStringWidth(s_lastCurAmmoStr, true);

            float ammoX = wepRightEdge - totW;
            if (s_lastTotAmmoStr[0] != '\0') {
                CFont::SetColor(ModColor(g_config.weaponAmmoTotalColor, currentAmmoAlpha));
                CFont::PrintString(ammoX, ammoY, s_lastTotAmmoStr);
            }

            float curX = (s_lastTotAmmoStr[0] != '\0') ? (ammoX - gap - curW) : (wepRightEdge - curW);
            CFont::SetColor(ModColor(g_config.weaponAmmoCurrentColor, currentAmmoAlpha));
            CFont::PrintString(curX, ammoY, s_lastCurAmmoStr);
        }
    }

    float currentWepAlpha = isPreview ? 1.0f : (s_weaponFadeAlpha * masterAlpha);
    if ((g_config.useWeaponUi || isPreview) && currentWepAlpha > 0.005f) {
        float iconX = wepRightEdge - wepIconSize;

        IDirect3DTexture9* pCustomIcon = GetCustomWeaponIcon(wepType);
        if (pCustomIcon) {
            DrawD3DTexturedQuad(pCustomIcon, iconX, iconY, wepIconSize, wepIconSize, CRGBA(255, 255, 255, (unsigned char)(255.0f * currentWepAlpha)));
        } else {
            RwTexture* iconTex = nullptr;
            if (info && info->m_nModelId > 0) {
                CBaseModelInfo* binfo = CModelInfo::GetModelInfo(info->m_nModelId);
                if (binfo && binfo->m_nTxdIndex >= 0) {
                    CPool<TxdDef>* txdPool = CTxdStore::ms_pTxdPool;
                    if (txdPool) {
                        TxdDef* def = txdPool->GetAt(binfo->m_nTxdIndex);
                        if (def && def->m_pRwDictionary) {
                            unsigned int hash = CKeyGen::AppendStringToKey(binfo->m_nKey, "ICON");
                            iconTex = RwTexDictionaryFindHashNamedTexture(def->m_pRwDictionary, hash);
                            if (!iconTex) {
                                iconTex = RwTexDictionaryFindNamedTexture(def->m_pRwDictionary, "icon");
                            }
                        }
                    }
                }
            }
            if (!iconTex) {
                if (CHud::Sprites && CHud::Sprites[HUDSPRITE_FIST].m_pTexture) {
                    iconTex = CHud::Sprites[HUDSPRITE_FIST].m_pTexture;
                }
            }

            if (iconTex) {
                CSprite2d spr;
                spr.m_pTexture = iconTex;
                spr.Draw(CRect(iconX, iconY, iconX + wepIconSize, iconY + wepIconSize), CRGBA(255, 255, 255, (unsigned char)(255.0f * currentWepAlpha)));
            }
        }
    }

    // =========================================================================
    // 1.3 Money Counter & Telemetry Gauge
    // =========================================================================
    if (g_config.useMoneyUi) {
        int curMoney = CWorld::Players[0].m_nMoney;
        if (!s_moneyInitialized) {
            s_lastMoney = curMoney;
            s_moneyAnimFrom = 0.0f;
            s_moneyAnimTo = (float)curMoney;
            s_moneyAnimT = (curMoney == 0) ? 1.0f : 0.0f;
            s_renderMoney = 0.0f;
            s_moneyInitialized = true;
        }

        bool moneyChanged = (curMoney != s_lastMoney);
        if (moneyChanged) {
            s_moneyIdleTimer = g_config.uiIdleTimeout;
            s_moneyAnimFrom = s_renderMoney;
            s_moneyAnimTo = (float)curMoney;
            s_moneyAnimT = 0.0f;
            s_lastMoney = curMoney;
        } else if (s_moneyIdleTimer > 0.0f) {
            s_moneyIdleTimer -= dt;
            if (s_moneyIdleTimer < 0.0f) s_moneyIdleTimer = 0.0f;
        }

        bool isInterior = (CGame::currArea > 0) || (player->m_nAreaCode > 0);
        // The services menu spends money, so keep the counter on screen while it is open.
        bool showMoney = g_config.alwaysShowMoney || s_serviceOpen || isInterior || g_hudShowAll || (s_moneyIdleTimer > 0.0f);

        if (showMoney) {
            s_moneyFadeAlpha = std::min(1.0f, s_moneyFadeAlpha + dt / 0.25f);
        } else {
            s_moneyFadeAlpha = std::max(0.0f, s_moneyFadeAlpha - dt / 0.25f);
        }

        // Ease-out roll, hard-capped at kMoneyAnimDuration so a huge balance gap
        // cannot leave the counter spinning for tens of seconds.
        if (s_moneyAnimT < 1.0f) {
            s_moneyAnimT = std::min(1.0f, s_moneyAnimT + dt / kMoneyAnimDuration);
            float inv = 1.0f - s_moneyAnimT;
            float ease = 1.0f - inv * inv * inv;
            s_renderMoney = s_moneyAnimFrom + (s_moneyAnimTo - s_moneyAnimFrom) * ease;
            if (s_moneyAnimT >= 1.0f) s_renderMoney = s_moneyAnimTo;
        }

        float currentMoneyAlpha = isPreview ? 1.0f : (s_moneyFadeAlpha * masterAlpha);
        if (currentMoneyAlpha > 0.001f) {
            int displayMoney = isPreview ? 350000 : (int)std::round(s_renderMoney);
            char rawMoney[32];
            snprintf(rawMoney, sizeof(rawMoney), "%d", std::abs(displayMoney));
            int len = (int)strlen(rawMoney);

            char moneyBuf[64];
            int outIdx = 0;
            if (displayMoney < 0) {
                moneyBuf[outIdx++] = '-';
            }
            moneyBuf[outIdx++] = '$';
            for (int i = 0; i < len; ++i) {
                if (i > 0 && (len - i) % 3 == 0) {
                    moneyBuf[outIdx++] = ',';
                }
                moneyBuf[outIdx++] = rawMoney[i];
            }
            moneyBuf[outIdx] = '\0';

            float gaugeRad = 15.0f * (sH / 720.0f);
            float gaugeTrackW = 2.5f * (sH / 720.0f);
            float gaugeGap = gaugeTrackW * 0.7f;
            float iconSize = (gaugeRad - gaugeTrackW) * 2.0f * 0.70f;

            float gx = moneyRightEdge - gaugeRad;
            float gy = moneyY + gaugeRad;

            // Background circle
            CRGBA gaugeBg = ModColor(g_config.cardBackdropColor, currentMoneyAlpha);
            DrawCircle(gx, gy, gaugeRad + 1.0f, gaugeBg, 48);

            // Track ring
            CRGBA trackCol(40, 40, 45, (unsigned char)(180.0f * currentMoneyAlpha));
            DrawArcStrip(gx, gy, gaugeRad - gaugeTrackW - gaugeGap, gaugeRad - gaugeGap, 0.0f, 2.0f * kPi, trackCol, 48);

            // Value arc
            float ratio = s_renderMoney / 999999999.0f;
            if (ratio < 0.0f) ratio = 0.0f;
            if (ratio > 1.0f) ratio = 1.0f;

            if (ratio > 0.001f) {
                float startAngle = -kPi * 0.5f;
                float endAngle = startAngle + ratio * 2.0f * kPi;
                CRGBA gaugeValCol = ModColor(g_config.moneyGaugeColor, currentMoneyAlpha);
                DrawArcStrip(gx, gy, gaugeRad - gaugeTrackW - gaugeGap, gaugeRad - gaugeGap, startAngle, endAngle, gaugeValCol, 48);
            }

            // Wallet icon in gauge center
            if (g_pTexWallet) {
                static int s_walletLogBudget = 3;
                if (s_walletLogBudget > 0) {
                    --s_walletLogBudget;
                    IDirect3DDevice9* probeDev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
                    IDirect3DVertexShader9* probeVS = nullptr;
                    IDirect3DVertexDeclaration9* probeDecl = nullptr;
                    if (probeDev) {
                        probeDev->GetVertexShader(&probeVS);
                        probeDev->GetVertexDeclaration(&probeDecl);
                        if (probeVS) probeVS->Release();
                        if (probeDecl) probeDecl->Release();
                    }
                    LogWrite("wallet: draw tex=%p rect=(%.1f,%.1f,%.1f,%.1f) alpha=%.2f vs=%p decl=%p",
                             (void*)g_pTexWallet, gx - iconSize * 0.5f, gy - iconSize * 0.5f,
                             iconSize, iconSize, currentMoneyAlpha, (void*)probeVS, (void*)probeDecl);
                }
                DrawD3DTexturedQuad(g_pTexWallet, gx - iconSize * 0.5f, gy - iconSize * 0.5f, iconSize, iconSize, CRGBA(255, 255, 255, (unsigned char)(255.0f * currentMoneyAlpha)));
            }

            // Money text (20px font scale, baseline aligned with gauge center)
            float fontScaleX = sH * 0.0008125f;
            float fontScaleY = sH * 0.001625f;

            CFont::SetFontStyle(FONT_PRICEDOWN);
            CFont::SetScale(fontScaleX, fontScaleY);
            CFont::SetProportional(true);
            CFont::SetJustify(false);
            CFont::SetOrientation(ALIGN_LEFT);
            CFont::SetEdge(1);
            CFont::SetDropColor(ModColor(g_config.moneyStrokeColor, currentMoneyAlpha));
            CFont::SetColor(ModColor(g_config.moneyTextColor, currentMoneyAlpha));

            float textW = CFont::GetStringWidth(moneyBuf, true);
            float textGap = 10.0f * (sH / 720.0f);
            float textX = (gx - gaugeRad) - textGap - textW;
            float textY = gy - 13.0f * (sH / 720.0f);

            CFont::PrintString(textX, textY, moneyBuf);
        }
    }
}

// =============================================================================
// 2. Temporary Top-Left Health & Armor Bar
// =============================================================================
static void DrawTopLeftHealthBar(float alpha) {
    CPlayerPed* player = FindPlayerPed(0);
    bool isPreview = Menu::IsLayoutPreview();
    if (!player && !isPreview) return;

    float sW = ScreenW();
    float sH = ScreenH();

    CRect barRect = g_animatedModuleRects[(int)HudModuleId::HealthArmor];
    float barX = barRect.left;
    float barY = barRect.top;
    float barW = barRect.right - barRect.left;
    float barH = 7.0f * (sH / 720.0f);
    float strokeW = 1.0f;

    float hpRatio = 1.0f;
    float armour = 100.0f;
    float armourRatio = 1.0f;

    if (player) {
        float hp = player->m_fHealth;
        float maxHp = player->m_fMaxHealth;
        if (maxHp <= 0.0f) maxHp = 100.0f;
        hpRatio = std::clamp(hp / maxHp, 0.0f, 1.0f);
        armour = player->m_fArmour;
        armourRatio = std::clamp(armour / 100.0f, 0.0f, 1.0f);
    }
    if (isPreview) {
        hpRatio = 1.0f;
        armour = 100.0f;
        armourRatio = 1.0f;
    }

    if (armour > 0.5f) {
        // Armor background
        CSprite2d::DrawRect(CRect(barX - strokeW, barY - strokeW, barX + barW + strokeW, barY + barH + strokeW), CRGBA(0, 0, 0, 200));
        CSprite2d::DrawRect(CRect(barX, barY, barX + barW, barY + barH), CRGBA(30, 30, 35, 180));
        // Armor fill
        if (armourRatio > 0.0f) {
            CSprite2d::DrawRect(CRect(barX, barY, barX + barW * armourRatio, barY + barH), CRGBA(225, 240, 255, 240));
        }
        barY += barH + 4.0f * (sH / 720.0f);
    }

    // Health background
    CSprite2d::DrawRect(CRect(barX - strokeW, barY - strokeW, barX + barW + strokeW, barY + barH + strokeW), CRGBA(0, 0, 0, 200));
    CSprite2d::DrawRect(CRect(barX, barY, barX + barW, barY + barH), CRGBA(40, 15, 20, 180));
    // Health fill
    if (hpRatio > 0.0f) {
        CRGBA hpCol(225, 30, 45, 240);
        CSprite2d::DrawRect(CRect(barX, barY, barX + barW * hpRatio, barY + barH), hpCol);
    }
}
