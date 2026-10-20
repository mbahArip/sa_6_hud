void Radar::DrawGpsPath(const CRect& rect, float alpha) {
    if (!g_config.enableGps) return;
    int targetIdx = GetTargetBlipIndex();
    bool active = (targetIdx >= 0 || g_pinActive);
    if (!active || g_pathCount < 2) return;

    CVector destPos = (targetIdx >= 0) ? CRadar::ms_RadarTrace[targetIdx].m_vecPos : g_pinPos;

    float rw = rect.right - rect.left;
    float rh = rect.bottom - rect.top;
    float sH = ScreenH();

    CVector rawNodes[kMaxPathNodes + 2];
    int rawCount = 0;
    rawNodes[rawCount++] = g_playerPos;

    for (int i = 0; i < g_pathCount && rawCount < kMaxPathNodes + 1; ++i) {
        rawNodes[rawCount++] = g_pathNodes[i];
    }
    if ((destPos - rawNodes[rawCount - 1]).Magnitude2D() > 1.0f) {
        rawNodes[rawCount++] = destPos;
    }

    if (rawCount < 2) return;

    static CVector smoothPts[4096];
    int smoothCount = 0;
    GenerateSmoothPath(rawNodes, rawCount, smoothPts, smoothCount, 4000);

    if (smoothCount < 2) return;

    static RibbonPoint ribbonPts[4096];
    int ribbonCount = 0;

    float fadeStartMeters = g_config.gpsMaskInner;
    float fadeEndMeters   = g_config.gpsMaskOuter;
    if (fadeEndMeters <= fadeStartMeters) fadeEndMeters = fadeStartMeters + 1.0f;

    for (int i = 0; i < smoothCount && ribbonCount < 4080; ++i) {
        float r = (smoothPts[i] - g_playerPos).Magnitude2D();
        float fade = Clampf((r - fadeStartMeters) / (fadeEndMeters - fadeStartMeters), 0.0f, 1.0f);

        float u, v;
        bool ok = ProjectToUV(smoothPts[i], u, v);
        float sx = rect.left + rw * u;
        float sy = rect.top + rh * v;

        ribbonPts[ribbonCount].x = sx;
        ribbonPts[ribbonCount].y = sy;
        ribbonPts[ribbonCount].alpha = fade;
        ribbonPts[ribbonCount].valid = ok;
        ribbonCount++;
    }

    if (ribbonCount < 2) return;

    float baseCoreW = g_config.gpsLineWidth * (sH / 720.0f) * g_config.radarSizeScale;
    if (baseCoreW < 2.0f) baseCoreW = 2.0f;
    float baseCasingW = g_config.gpsCasingWidth * (sH / 720.0f) * g_config.radarSizeScale;
    if (baseCasingW < 0.5f) baseCasingW = 0.5f;

    float outerHalfW = (baseCoreW + baseCasingW * 2.0f) * 0.5f;
    float innerHalfW = baseCoreW * 0.5f;

    CRGBA colCore = ModColor(g_config.gpsColor, alpha);
    CRGBA colCasing = ModColor(g_config.gpsCasingColor, alpha);
    bool dotted = (g_config.useWalkingNode && g_isWalkingPath);

    if (colCasing.a > 0) {
        RenderTriangleStripRibbon(ribbonPts, ribbonCount, outerHalfW, colCasing, dotted);
        if (!dotted) {
            if (ribbonPts[0].alpha > 0.85f) DrawCircle(ribbonPts[0].x, ribbonPts[0].y, outerHalfW, CRGBA(colCasing.r, colCasing.g, colCasing.b, (unsigned char)(colCasing.a * ribbonPts[0].alpha)), 16);
            DrawCircle(ribbonPts[ribbonCount - 1].x, ribbonPts[ribbonCount - 1].y, outerHalfW, CRGBA(colCasing.r, colCasing.g, colCasing.b, (unsigned char)(colCasing.a * ribbonPts[ribbonCount - 1].alpha)), 16);
        }
    }

    if (colCore.a > 0) {
        RenderTriangleStripRibbon(ribbonPts, ribbonCount, innerHalfW, colCore, dotted);
        if (!dotted) {
            if (ribbonPts[0].alpha > 0.85f) DrawCircle(ribbonPts[0].x, ribbonPts[0].y, innerHalfW, CRGBA(colCore.r, colCore.g, colCore.b, (unsigned char)(colCore.a * ribbonPts[0].alpha)), 16);
            DrawCircle(ribbonPts[ribbonCount - 1].x, ribbonPts[ribbonCount - 1].y, innerHalfW, CRGBA(colCore.r, colCore.g, colCore.b, (unsigned char)(colCore.a * ribbonPts[ribbonCount - 1].alpha)), 16);
        }
    }
}

