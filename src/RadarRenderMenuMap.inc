void Radar::DrawMenuMapOverlay() {
    if (!FrontEndMenuManager.m_bDrawRadarOrMap) return;

    float sW = ScreenW();
    float sH = ScreenH();

    // 1. Draw Wanted Search Area & Heat Area on Pause ESC Map
    if (g_wantedActive && g_config.enableWantedSearchRadius && g_config.showHeatOnMap) {
        CVector2D centerRadarSpace;
        CRadar::TransformRealWorldPointToRadarSpace(centerRadarSpace, CVector2D(g_searchCenter.x, g_searchCenter.y));
        CRadar::LimitRadarPoint(centerRadarSpace);
        CVector2D centerScr;
        CRadar::TransformRadarPointToScreenSpace(centerScr, centerRadarSpace);
        centerScr.x *= sW / 640.0f;
        centerScr.y *= sH / 448.0f;

        CVector2D edgeRadarSpace;
        CRadar::TransformRealWorldPointToRadarSpace(edgeRadarSpace, CVector2D(g_searchCenter.x + g_searchRadius, g_searchCenter.y));
        CRadar::LimitRadarPoint(edgeRadarSpace);
        CVector2D edgeScr;
        CRadar::TransformRadarPointToScreenSpace(edgeScr, edgeRadarSpace);
        edgeScr.x *= sW / 640.0f;
        edgeScr.y *= sH / 448.0f;

        float searchScreenRadius = fabsf(edgeScr.x - centerScr.x);
        if (searchScreenRadius < 5.0f) searchScreenRadius = 5.0f;

        // Draw Outer Heat Area Ring on Map
        if (g_config.enableHeatArea) {
            float heatScreenRadius = searchScreenRadius * g_config.heatRadiusScale;
            float heatOpa = g_config.heatRadiusOpacity;
            if (heatOpa > 1.0f) heatOpa /= 100.0f;
            CRGBA heatMapFill(225, 30, 45, (unsigned char)(heatOpa * 140.0f));
            CRGBA heatMapBorder(225, 45, 55, (unsigned char)(heatOpa * 230.0f));

            DrawCircle(centerScr.x, centerScr.y, heatScreenRadius, heatMapFill, 48);
            DrawRing(centerScr.x, centerScr.y, heatScreenRadius - 1.5f, heatScreenRadius + 1.5f, heatMapBorder, 48);
        }

        // Draw Inner Search Radius on Map (unless in Stage 3 where inner circle is removed)
        if (g_wantedStage != WANTED_STAGE_HEAT || g_innerSearchAlpha > 0.05f) {
            float innerAlpha = g_innerSearchAlpha;
            CRGBA searchMapFill = g_copHasVisual ? CRGBA(225, 30, 45, (unsigned char)(70.0f * innerAlpha))
                                                 : CRGBA(25, 95, 225, (unsigned char)(65.0f * innerAlpha));
            CRGBA searchMapBorder = g_copHasVisual ? CRGBA(255, 60, 70, (unsigned char)(180.0f * innerAlpha))
                                                   : CRGBA(60, 140, 255, (unsigned char)(180.0f * innerAlpha));

            DrawCircle(centerScr.x, centerScr.y, searchScreenRadius, searchMapFill, 48);
            DrawRing(centerScr.x, centerScr.y, searchScreenRadius - 1.5f, searchScreenRadius + 1.5f, searchMapBorder, 48);
        }
    }

    if (!g_config.enableMenuMapGps) return;

    UpdateGps();

    int targetIdx = GetTargetBlipIndex();
    bool active = (targetIdx >= 0 || g_pinActive);
    if (!active || g_pathCount < 2) return;

    CVector destPos = (targetIdx >= 0) ? CRadar::ms_RadarTrace[targetIdx].m_vecPos : g_pinPos;

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

    static RibbonPoint mapPts[4096];
    int mapCount = 0;

    for (int i = 0; i < smoothCount && mapCount < 4080; ++i) {
        CVector2D tmpPoint;
        CRadar::TransformRealWorldPointToRadarSpace(tmpPoint, CVector2D(smoothPts[i].x, smoothPts[i].y));
        CRadar::LimitRadarPoint(tmpPoint);
        CVector2D scrPoint;
        CRadar::TransformRadarPointToScreenSpace(scrPoint, tmpPoint);
        scrPoint.x *= sW / 640.0f;
        scrPoint.y *= sH / 448.0f;
        CRadar::LimitToMap(&scrPoint.x, &scrPoint.y);

        mapPts[mapCount].x = scrPoint.x;
        mapPts[mapCount].y = scrPoint.y;
        mapPts[mapCount].alpha = 1.0f;
        mapPts[mapCount].valid = true;
        mapCount++;
    }

    if (mapCount < 2) return;

    float mp = FrontEndMenuManager.m_fMapZoom - 140.0f;
    if (mp < 140.0f) mp = 140.0f;
    else if (mp > 960.0f) mp = 960.0f;
    mp = mp / 960.0f + 0.4f;

    float mapCoreW = g_config.mapGpsLineWidth * mp * (sH / 720.0f);
    float mapCasingW = g_config.mapGpsCasingWidth * mp * (sH / 720.0f);
    float outerHalfW = (mapCoreW + mapCasingW * 2.0f) * 0.5f;
    float innerHalfW = mapCoreW * 0.5f;

    const CRGBA& colCore = g_config.gpsColor;
    const CRGBA& colCasing = g_config.gpsCasingColor;
    bool dotted = (g_config.useWalkingNode && g_isWalkingPath);

    if (colCasing.a > 0) {
        RenderTriangleStripRibbon(mapPts, mapCount, outerHalfW, colCasing, dotted);
        if (!dotted) {
            DrawCircle(mapPts[0].x, mapPts[0].y, outerHalfW, colCasing, 16);
            DrawCircle(mapPts[mapCount - 1].x, mapPts[mapCount - 1].y, outerHalfW, colCasing, 16);
        }
    }

    if (colCore.a > 0) {
        RenderTriangleStripRibbon(mapPts, mapCount, innerHalfW, colCore, dotted);
        if (!dotted) {
            DrawCircle(mapPts[0].x, mapPts[0].y, innerHalfW, colCore, 16);
            DrawCircle(mapPts[mapCount - 1].x, mapPts[mapCount - 1].y, innerHalfW, colCore, 16);
        }
    }
}

