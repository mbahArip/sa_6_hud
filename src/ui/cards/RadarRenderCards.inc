void Radar::DrawInfoCards(const CRect& rect, float alpha) {
    bool isPreview = Menu::IsLayoutPreview();
    if (!g_config.showInfoCards && !isPreview) return;

    if (isPreview) alpha = 1.0f;

    float sH = ScreenH();
    float cardScale = g_config.radarSizeScale;
    float cardCornerRad = Radius() * 0.5f;

    // Publish live visibility so the group stack solver can collapse hidden slots.
    g_moduleAlpha[(int)HudModuleId::GpsNav]      = s_navAlpha;
    g_moduleAlpha[(int)HudModuleId::PlaceInfo]   = s_locAlpha;
    g_moduleAlpha[(int)HudModuleId::VehicleInfo] = s_vehCardAlpha;

    // Preview shows the resting layout; gameplay uses the alpha-collapsed stack.
    const CRect* rects = isPreview ? g_moduleRects : g_animatedModuleRects;

    // =========================================================================
    // LAYER 3 (Bottom of stack, directly above radar frame): GPS Turn Navigator
    // =========================================================================
    float navH = sH * 0.064f * cardScale;
    float navAlphaMod = isPreview ? 1.0f : (alpha * s_navAlpha);

    CRect navRect = rects[(int)HudModuleId::GpsNav];
    float navCardX = navRect.left;
    float navCardY = navRect.top;
    float navCardW = navRect.right - navRect.left;

    if (navAlphaMod > 0.005f) {
        // Backdrop
        CRGBA navBg = ModColor(g_config.cardBackdropColor, navAlphaMod * g_config.cardBackdropOpacity);
        DrawRoundedFill(navRect, cardCornerRad, navBg);

        // Right Accent Badge (1:1 square pink block on right edge of card) (1.1)
        float badgeWidth = navH;
        float badgeX = navCardX + navCardW - badgeWidth;
        CRect badgeRect(badgeX, navCardY, navCardX + navCardW, navCardY + navH);

        CRGBA badgePink = ModColor(g_config.palettePrimary, navAlphaMod);
        DrawRightRoundedFill(badgeRect, cardCornerRad, badgePink);

        GpsManeuver maneuverType = s_gpsNavState.maneuver;
        if (isPreview && maneuverType == GpsManeuver::None) maneuverType = GpsManeuver::TurnLeft;

        CRGBA arrowCol = ModColor(g_config.palettePrimaryForeground, navAlphaMod);
        DrawManeuverIcon(maneuverType, badgeX + badgeWidth * 0.50f, navCardY + navH * 0.50f, navH * 0.52f, arrowCol);

        // Border
        if (g_config.cardBorderThickness > 0.05f) {
            CRGBA navBrd = ModColor(g_config.cardBorderColor, navAlphaMod);
            DrawRoundedBorder(navRect, cardCornerRad, g_config.cardBorderThickness, navBrd);
        }

        // Left Maneuver Text & Subtitle
        float textLeft = navCardX + sH * 0.014f * cardScale;
        float topTextY = navCardY + sH * 0.0065f * cardScale;
        float botTextY = navCardY + sH * 0.0330f * cardScale;

        const char* mText = s_gpsNavState.maneuverText;
        const char* dText = s_gpsNavState.distanceText;
        if (isPreview && (!mText || mText[0] == '\0')) {
            mText = "Turn Left";
            dText = "150m";
        }

        CRGBA shadowCol(0, 0, 0, (unsigned char)(200 * navAlphaMod));
        CRGBA txtCol = ModColor(g_config.cardTextColor, navAlphaMod);

        // Line 1: Maneuver instruction (font pricedown, minimap.card.foreground)
        CFont::SetFontStyle(FONT_PRICEDOWN);
        CFont::SetScale(sH * 0.00062f * cardScale, sH * 0.00138f * cardScale);
        CFont::SetProportional(true);
        CFont::SetJustify(false);
        CFont::SetOrientation(ALIGN_LEFT);
        CFont::SetEdge(0);
        CFont::SetDropShadowPosition(0);
        CFont::SetColor(txtCol);
        CFont::PrintString(textLeft, topTextY, const_cast<char*>(mText));

        // Line 2: Distance to turn (font subtitles, minimap.card.foreground)
        CFont::SetFontStyle(FONT_SUBTITLES);
        CFont::SetScale(sH * 0.00055f * cardScale, sH * 0.00120f * cardScale);
        CFont::SetProportional(true);
        CFont::SetJustify(false);
        CFont::SetOrientation(ALIGN_LEFT);
        CFont::SetEdge(0);
        CFont::SetDropShadowPosition(0);
        CFont::SetColor(txtCol);
        CFont::PrintString(textLeft, botTextY, const_cast<char*>(dText));
    }

    // =========================================================================
    // LAYER 2 (Middle of stack): Location / Zone Info Card (USE_PLACE_INFO)
    // =========================================================================
    float locH = sH * 0.038f * cardScale;
    float locAlphaMod = isPreview ? 1.0f : ((alpha > 0.005f) ? (alpha * s_locAlpha) : s_locAlpha);

    CRect locRect = rects[(int)HudModuleId::PlaceInfo];
    float locCardX = locRect.left;
    float locCardY = locRect.top;
    float locCardW = locRect.right - locRect.left;

    if (locAlphaMod > 0.005f) {
        // Backdrop: minimap.card.background
        CRGBA locBg = ModColor(g_config.cardBackdropColor, locAlphaMod * g_config.cardBackdropOpacity);
        DrawRoundedFill(locRect, cardCornerRad, locBg);

        // Border: minimap.card.border (1px solid)
        if (g_config.cardBorderThickness > 0.05f) {
            CRGBA locBrd = ModColor(g_config.cardBorderColor, locAlphaMod);
            DrawRoundedBorder(locRect, cardCornerRad, g_config.cardBorderThickness, locBrd);
        }

        // Zone Name Text (Font: FONT_PRICEDOWN, minimap.card.foreground)
        CFont::SetFontStyle(FONT_PRICEDOWN);
        CFont::SetScale(sH * 0.00065f * cardScale, sH * 0.00130f * cardScale);
        CFont::SetProportional(true);
        CFont::SetJustify(false);
        CFont::SetEdge(0);
        CFont::SetDropShadowPosition(0);

        auto drawZoneString = [&](const char* str, float aMod) {
            if (!str || str[0] == '\0' || aMod <= 0.005f) return;
            CRGBA col = ModColor(g_config.cardTextColor, aMod);
            CFont::SetColor(col);

            float textW = CFont::GetStringWidth(const_cast<char*>(str), true);
            float padX = 4.0f * (sH / 720.0f) * cardScale;
            float maxW = locCardW - padX * 2.0f;
            float textY = locCardY + (locH - sH * 0.015f * cardScale) * 0.5f - locH * 0.15f;

            if (textW > maxW) {
                float overflow = textW - maxW;
                float cycle = overflow + 30.0f;
                float scroll = fmodf(CTimer::m_snTimeInMilliseconds * 0.035f, cycle * 2.0f);
                if (scroll > cycle) scroll = cycle * 2.0f - scroll;

                IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
                RECT oldScissor;
                DWORD scissorEnabled = 0;
                dev->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissorEnabled);
                dev->GetScissorRect(&oldScissor);

                RECT cardScissor = { (LONG)(locCardX + padX), (LONG)locRect.top, (LONG)(locCardX + locCardW - padX), (LONG)locRect.bottom };
                dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
                dev->SetScissorRect(&cardScissor);

                CFont::SetOrientation(ALIGN_LEFT);
                float printX = (locCardX + padX) - scroll;
                CFont::PrintString(printX, textY, const_cast<char*>(str));

                dev->SetScissorRect(&oldScissor);
                dev->SetRenderState(D3DRS_SCISSORTESTENABLE, scissorEnabled);
            } else {
                CFont::SetOrientation(ALIGN_CENTER);
                CFont::SetCentreSize(locCardW);
                float printX = locCardX + locCardW * 0.5f;
                CFont::PrintString(printX, textY, const_cast<char*>(str));
            }
        };

        if (isPreview && s_curZoneName[0] == '\0') {
            drawZoneString("Ganton", 1.0f);
        } else if (s_zoneFadeAlpha > 0.01f && s_prevZoneName[0] != '\0') {
            drawZoneString(s_prevZoneName, locAlphaMod * s_zoneFadeAlpha);
            drawZoneString(s_curZoneName, locAlphaMod * (1.0f - s_zoneFadeAlpha));
        } else {
            drawZoneString(s_curZoneName, locAlphaMod);
        }
    }

    // =========================================================================
    // LAYER 1 (Top of stack): Split Vehicle Info Card + Telemetry (USE_VEHICLE_INFO)
    // =========================================================================
    float vehAlphaMod = isPreview ? 1.0f : ((alpha > 0.005f) ? (alpha * s_vehCardAlpha) : s_vehCardAlpha);
    if (vehAlphaMod > 0.005f) {
        CRect vehCardRect = rects[(int)HudModuleId::VehicleInfo];
        float vehCardX = vehCardRect.left;
        float vehCardW = vehCardRect.right - vehCardRect.left;
        float cardTop = vehCardRect.top;
        float cardBottom = vehCardRect.bottom;

        float splitY = cardTop + sH * 0.063f * cardScale;
        CRect top2_3Rect(vehCardX, cardTop, vehCardX + vehCardW, splitY);
        CRect bot1_3Rect(vehCardX, splitY, vehCardX + vehCardW, cardBottom);

        float nameAlphaMod = isPreview ? 1.0f : (vehAlphaMod * s_vehNameAlpha);
        if (nameAlphaMod > 0.005f) {
            // Top 2/3 background: gradient from minimap.card.background to minimap.card.darker
            CRGBA gradLeft = ModColor(g_config.cardBackdropColor, nameAlphaMod * g_config.cardBackdropOpacity);
            CRGBA gradRight = ModColor(g_config.cardDarkerColor, nameAlphaMod * g_config.cardBackdropOpacity);
            DrawTopRoundedHorizontalGradientFill(top2_3Rect, cardCornerRad, gradLeft, gradRight);

            // Bottom 1/3 background: minimap.card.background
            CRGBA rightBg = ModColor(g_config.cardBackdropColor, nameAlphaMod * g_config.cardBackdropOpacity);
            DrawBottomRoundedFill(bot1_3Rect, cardCornerRad, rightBg);

            // Grouped Container Border
            if (g_config.cardBorderThickness > 0.05f) {
                CRGBA vehBrd = ModColor(g_config.cardBorderColor, nameAlphaMod);
                DrawRoundedBorder(vehCardRect, cardCornerRad, g_config.cardBorderThickness, vehBrd);
            }

            // Top 2/3 Vehicle Info section (centered, padded text)
            float cardCenterX = vehCardX + vehCardW * 0.5f;
            float topTextY = cardTop + sH * 0.007f * cardScale;
            float botTextY = cardTop + sH * 0.026f * cardScale;

            const char* vClass = s_vehClass;
            const char* vName  = s_vehName;
            if (isPreview && vName[0] == '\0') {
                vClass = "SPORTS";
                vName  = "Infernus";
            }

            // 1. Top: Vehicle class (FONT_MENU, centered, minimap.card.foreground)
            if (vClass[0] != '\0') {
                CFont::SetFontStyle(FONT_MENU);
                CFont::SetScale(sH * 0.00053f * g_config.vehicleClassScale * cardScale, sH * 0.00110f * g_config.vehicleClassScale * cardScale);
                CFont::SetProportional(true);
                CFont::SetJustify(false);
                CFont::SetOrientation(ALIGN_CENTER);
                CFont::SetCentreSize(vehCardW);
                CFont::SetEdge(0);
                CFont::SetDropShadowPosition(0);

                CRGBA classCol = ModColor(g_config.cardTextColor, nameAlphaMod);
                CFont::SetColor(classCol);
                CFont::PrintString(cardCenterX, topTextY, const_cast<char*>(vClass));
            }

            // 2. Bottom: Vehicle name (FONT_PRICEDOWN, centered, minimap.card.foreground at 80% opacity)
            CFont::SetFontStyle(FONT_PRICEDOWN);
            CFont::SetScale(sH * 0.00072f * g_config.vehicleNameScale * cardScale, sH * 0.00144f * g_config.vehicleNameScale * cardScale);
            CFont::SetProportional(true);
            CFont::SetJustify(false);
            CFont::SetOrientation(ALIGN_CENTER);
            CFont::SetCentreSize(vehCardW);
            CFont::SetEdge(0);
            CFont::SetDropShadowPosition(0);

            CRGBA nameCol = ModColor(g_config.cardTextColor, nameAlphaMod * 0.80f);
            CFont::SetColor(nameCol);
            CFont::PrintString(cardCenterX, botTextY, const_cast<char*>(vName));
        }

        // Bottom 1/3: Vehicle Telemetry section (circular gauges)
        float statsCenterY = splitY + (cardBottom - splitY) * 0.5f;
        float gaugeRad = 11.75f * (sH / 720.0f) * g_config.gaugeScale * cardScale;
        float gaugeTrackW = 1.6f * (sH / 720.0f) * g_config.gaugeScale * cardScale;
        float iconSize = (gaugeRad - gaugeTrackW) * 2.0f * 0.675f;
        float midRad = gaugeRad - gaugeTrackW * 0.5f;
        float capRad = gaugeTrackW * 0.5f;

        for (int i = 0; i < GAUGE_COUNT; ++i) {
            float gAlpha = s_gaugeSlots[i].curAlpha * vehAlphaMod;
            if (gAlpha <= 0.005f) continue;

            float gx = s_gaugeSlots[i].curX;
            float gy = statsCenterY;

            // Gauge background: 9px radius, minimap.card.background (with 2px padding)
            CRGBA gaugeBg = ModColor(g_config.cardBackdropColor, gAlpha);
            DrawCircle(gx, gy, gaugeRad + 1.0f, gaugeBg, 48);

            // Background track ring
            CRGBA trackCol(60, 68, 80, (unsigned char)(140 * gAlpha));
            DrawArcStrip(gx, gy, gaugeRad - gaugeTrackW, gaugeRad, 0.0f, 2.0f * kPi, trackCol, 48);

            // Active progress arc with circular end-caps
            float val = s_gaugeSlots[i].curValue;
            if (val > 0.01f) {
                float startAngle = -kPi * 0.5f;
                float endAngle = startAngle + (2.0f * kPi * Clampf(val, 0.0f, 1.0f));
                CRGBA arcCol = ModColor(s_gaugeSlots[i].color, gAlpha);
                DrawArcStrip(gx, gy, gaugeRad - gaugeTrackW, gaugeRad, startAngle, endAngle, arcCol, 48);

                // Circular start cap at 12 o'clock
                float startCapX = gx + midRad * cosf(startAngle);
                float startCapY = gy + midRad * sinf(startAngle);
                DrawCircle(startCapX, startCapY, capRad, arcCol, 32);

                // Circular end cap at current progress end
                float endCapX = gx + midRad * cosf(endAngle);
                float endCapY = gy + midRad * sinf(endAngle);
                DrawCircle(endCapX, endCapY, capRad, arcCol, 32);
            }

            // 14px centered icon (texture or vector fallback)
            IDirect3DTexture9* pIconTex = nullptr;
            if (i == GAUGE_HEALTH) pIconTex = g_pTexEngine;
            else if (i == GAUGE_FUEL) pIconTex = g_pTexFuel;
            else if (i == GAUGE_NITRO) pIconTex = g_pTexNitro;

            CRGBA iconCol;
            if (val <= 0.01f) {
                // Grayscale/muted when empty (0 value)
                iconCol = CRGBA(120, 125, 135, (unsigned char)(160 * gAlpha));
            } else {
                iconCol = ModColor(s_gaugeSlots[i].color, gAlpha);
            }

            if (pIconTex) {
                DrawD3DTexturedQuad(pIconTex, gx - iconSize * 0.5f, gy - iconSize * 0.5f, iconSize, iconSize, iconCol);
            } else {
                DrawCircle(gx, gy, iconSize * 0.35f, iconCol, 14);
            }

            // Bottle count label for Vanilla Nitro
            if (i == GAUGE_NITRO && g_config.nitroGaugeType == 0 && !g_config.enableControllableNitro && s_curNitroBoosts > 0) {
                char badgeBuf[16];
                if (s_curNitroBoosts == 101 || s_curNitroBoosts == 255) {
                    strcpy(badgeBuf, "inf");
                } else {
                    sprintf(badgeBuf, "%u", s_curNitroBoosts);
                }

                float textX = gx + gaugeRad * 0.48f;
                float textY = gy + gaugeRad * 0.22f;

                CFont::SetFontStyle(FONT_SUBTITLES);
                CFont::SetScale(sH * 0.00028f * g_config.gaugeScale * cardScale, sH * 0.00085f * g_config.gaugeScale * cardScale);
                CFont::SetProportional(true);
                CFont::SetJustify(false);
                CFont::SetOrientation(ALIGN_LEFT);
                CFont::SetEdge(0);
                CFont::SetDropShadowPosition(0);

                // Subtle shadow for legibility
                CRGBA shadowCol(0, 0, 0, (unsigned char)(220 * gAlpha));
                CFont::SetColor(shadowCol);
                CFont::PrintString(textX + 1.0f, textY + 1.0f, badgeBuf);

                CRGBA numCol = ModColor(g_config.gaugeNitroColor, gAlpha);
                CFont::SetColor(numCol);
                CFont::PrintString(textX, textY, badgeBuf);
            }
        }
    }
}

