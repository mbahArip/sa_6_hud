void Radar::DrawMenuMapOverlay() {
    if (!FrontEndMenuManager.m_bDrawRadarOrMap) return;

    float sW = ScreenW();
    float sH = ScreenH();

    // 0. Custom PNG tiles on the pause map. Vanilla CRadar::DrawRadarMap is patched to
    // RET while the 6HUD radar is active, so without this the pause map has no tiles at
    // all. Sea color is painted underneath and shows through transparent water pixels.
    if (g_config.enabled && g_config.useCustomMapTile && g_config.useCustomPauseMap) {
        // Raw D3D9 tile blits below; snapshot and restore so blips, GPS ribbon and the
        // rest of the frontend draw with the state they expect.
        MapDrawState savedState;
        savedState.Capture((IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice());

        auto worldToScreen = [&](float wx, float wy, float& sx, float& sy) {
            CVector2D radarSpace;
            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(wx, wy));
            CVector2D scr;
            CRadar::TransformRadarPointToScreenSpace(scr, radarSpace);
            sx = scr.x * sW / 640.0f;
            sy = scr.y * sH / 448.0f;
        };

        float nightBlend = GetDayNightBlend();
        CRGBA seaCol(
            (unsigned char)Lerpf((float)g_config.radarSeaColor.r, (float)g_config.radarSeaColorNight.r, nightBlend),
            (unsigned char)Lerpf((float)g_config.radarSeaColor.g, (float)g_config.radarSeaColorNight.g, nightBlend),
            (unsigned char)Lerpf((float)g_config.radarSeaColor.b, (float)g_config.radarSeaColorNight.b, nightBlend),
            255
        );

        // Sea base: the whole 6000x6000 world square, which is exactly the map extent.
        float seaX0, seaY0, seaX1, seaY1, seaX2, seaY2, seaX3, seaY3;
        worldToScreen(-3000.0f,  3000.0f, seaX0, seaY0); // top-left
        worldToScreen( 3000.0f,  3000.0f, seaX1, seaY1); // top-right
        worldToScreen(-3000.0f, -3000.0f, seaX3, seaY3); // bottom-left
        worldToScreen( 3000.0f, -3000.0f, seaX2, seaY2); // bottom-right

        if (g_config.fillEmptySea) {
            IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
            if (dev) {
                DWORD dwCol = D3DCOLOR_ARGB(seaCol.a, seaCol.r, seaCol.g, seaCol.b);
                D3D2DVert quad[4] = {
                    { seaX0, seaY0, 0.0f, 1.0f, dwCol, 0.0f, 0.0f },
                    { seaX1, seaY1, 0.0f, 1.0f, dwCol, 0.0f, 0.0f },
                    { seaX3, seaY3, 0.0f, 1.0f, dwCol, 0.0f, 0.0f },
                    { seaX2, seaY2, 0.0f, 1.0f, dwCol, 0.0f, 0.0f },
                };
                BeginRawD3DQuad(dev);
                dev->SetTexture(0, NULL);
                dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(D3D2DVert));
            }
        }

        for (int ty = 0; ty < 12; ++ty) {
            for (int tx = 0; tx < 12; ++tx) {
                float wx0 = -3000.0f + (float)tx * 500.0f;
                float wx1 = wx0 + 500.0f;
                float wy0 =  3000.0f - (float)(ty + 1) * 500.0f;
                float wy1 =  3000.0f - (float)ty * 500.0f;

                float sx0, sy0, sx1, sy1, sx2, sy2, sx3, sy3;
                worldToScreen(wx0, wy0, sx0, sy0);
                worldToScreen(wx1, wy0, sx1, sy1);
                worldToScreen(wx1, wy1, sx2, sy2);
                worldToScreen(wx0, wy1, sx3, sy3);

                if (std::max(std::max(sx0, sx1), std::max(sx2, sx3)) < 0.0f ||
                    std::min(std::min(sx0, sx1), std::min(sx2, sx3)) > sW ||
                    std::max(std::max(sy0, sy1), std::max(sy2, sy3)) < 0.0f ||
                    std::min(std::min(sy0, sy1), std::min(sy2, sy3)) > sH) {
                    continue;
                }

                BlitMapTileBlended(tx + 12 * ty, sx0, sy0, sx1, sy1, sx2, sy2, sx3, sy3, 1.0f);
            }
        }

        savedState.Restore();
    }

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

    // Pump icons on the pause map, drawn before the GPS ribbon so the route sits on top.
    DrawPumpIconsOnMenuMap();

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

