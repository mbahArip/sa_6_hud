void Radar::DrawGpsDistance(const CRect& rect, float alpha) {
    if (alpha <= 0.005f) return;

    float sH = ScreenH();
    float sW = ScreenW();
    const float inset = 8.0f;

    // =========================================================================
    // 1. Distance Remaining / iFruit Icon (Top-Left, 8px inset)
    // =========================================================================
    float destEased = s_destFade * s_destFade * (3.0f - 2.0f * s_destFade);

    // 1.1 iFruit Icon when no destination is active (cross-fade out when destination appears)
    float iconFade = 1.0f - destEased;
    if (iconFade > 0.005f && g_pTexIfruit) {
        float iconH = 28.0f * (sH / 1080.0f) * g_config.radarSizeScale;
        if (iconH < 20.0f) iconH = 20.0f;
        float iconW = iconH;

        float iconX = rect.left + inset;
        float iconY = rect.top + inset;
        float iconAlpha = alpha * iconFade;

        DrawD3DTexturedQuad(g_pTexIfruit, iconX, iconY, iconW, iconH,
                            CRGBA(255, 255, 255, (unsigned char)(255 * iconAlpha)));
    }

    // 1.2 Distance Remaining text when destination exists (cross-fade in)
    if (destEased > 0.005f) {
        CVector destPos(0.0f, 0.0f, 0.0f);
        HasActiveDestination(destPos);
        float meters = (g_gpsDistance > 0.0f) ? g_gpsDistance : (g_playerPos - destPos).Magnitude2D();

        char distStr[32] = {};
        if (g_config.gameUnit == 0) {
            snprintf(distStr, sizeof(distStr), "%.2fkm", meters / 1000.0f);
        } else {
            float miles = (meters * 3.28084f) / 5280.0f;
            snprintf(distStr, sizeof(distStr), "%.2fmi", miles);
        }

        float textX = rect.left + inset;
        float textY = rect.top + inset;

        CFont::SetFontStyle(FONT_SUBTITLES);
        CFont::SetScale(sH * 0.00048f * g_config.radarSizeScale, sH * 0.00148f * g_config.radarSizeScale);
        CFont::SetProportional(true);
        CFont::SetJustify(false);
        CFont::SetOrientation(ALIGN_LEFT);
        CFont::SetWrapx(sW);
        CFont::SetEdge(0);
        CFont::SetDropShadowPosition(0);

        float textAlpha = alpha * destEased;

        // 1px stroke
        CRGBA strokeCol(13, 13, 13, (unsigned char)(255 * textAlpha));
        CFont::SetColor(strokeCol);
        CFont::PrintString(textX - 1.0f, textY, distStr);
        CFont::PrintString(textX + 1.0f, textY, distStr);
        CFont::PrintString(textX, textY - 1.0f, distStr);
        CFont::PrintString(textX, textY + 1.0f, distStr);

        // Foreground text
        CRGBA fgCol(255, 255, 255, (unsigned char)(255 * textAlpha));
        CFont::SetColor(fgCol);
        CFont::PrintString(textX, textY, distStr);
    }

    // =========================================================================
    // 2. Game Clock (Top-Right, 8px inset, visible while holding SHOW_HUD_KEY)
    // =========================================================================
    float clockEased = s_clockFadeProgress * s_clockFadeProgress * (3.0f - 2.0f * s_clockFadeProgress);
    if (clockEased > 0.005f) {
        char clockStr[32] = {};
        unsigned char hours = CClock::ms_nGameClockHours;
        unsigned char mins  = CClock::ms_nGameClockMinutes;

        if (g_config.gameClockFormat == 0) { // 12H_FORMAT ("7:29 am")
            int h12 = hours % 12;
            if (h12 == 0) h12 = 12;
            const char* ampm = (hours < 12) ? "am" : "pm";
            snprintf(clockStr, sizeof(clockStr), "%d:%02d %s", h12, mins, ampm);
        } else { // 24H_FORMAT ("20:00")
            snprintf(clockStr, sizeof(clockStr), "%02d:%02d", hours, mins);
        }

        float clockRight = rect.right - inset;
        float clockY = rect.top + inset;

        CFont::SetFontStyle(FONT_SUBTITLES);
        CFont::SetScale(sH * 0.00048f * g_config.radarSizeScale, sH * 0.00148f * g_config.radarSizeScale);
        CFont::SetProportional(true);
        CFont::SetJustify(false);
        CFont::SetOrientation(ALIGN_RIGHT);
        CFont::SetRightJustifyWrap(rect.left);
        CFont::SetEdge(0);
        CFont::SetDropShadowPosition(0);

        float clockAlpha = alpha * clockEased;

        // 1px stroke
        CRGBA clockStrokeCol(13, 13, 13, (unsigned char)(255 * clockAlpha));
        CFont::SetColor(clockStrokeCol);
        CFont::PrintString(clockRight - 1.0f, clockY, clockStr);
        CFont::PrintString(clockRight + 1.0f, clockY, clockStr);
        CFont::PrintString(clockRight, clockY - 1.0f, clockStr);
        CFont::PrintString(clockRight, clockY + 1.0f, clockStr);

        // Foreground text
        CRGBA clockTextCol(255, 255, 255, (unsigned char)(255 * clockAlpha));
        CFont::SetColor(clockTextCol);
        CFont::PrintString(clockRight, clockY, clockStr);

        // Restore default left alignment to prevent state pollution
        CFont::SetOrientation(ALIGN_LEFT);
        CFont::SetWrapx(sW);
    }
}
