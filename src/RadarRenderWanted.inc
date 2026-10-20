void Radar::DrawWantedSearchRadius(const CRect& rect, float alpha) {
    bool onMission = CTheScripts::IsPlayerOnAMission();
    bool isSearchActive = g_config.useWantedSearchArea ? (!onMission || g_config.useWantedSearchAreaOnMission) : g_config.enableWantedSearchRadius;
    if (!g_wantedActive || !isSearchActive) return;

    CPlayerPed* player = FindPlayerPed(0);
    if (!player || !player->m_pPlayerData || !player->m_pPlayerData->m_pWanted) return;
    if (player->m_pPlayerData->m_pWanted->m_nWantedLevel == 0) return;

    if (!g_pSearchRadiusPS) {
        CreateShaders();
        if (!g_pSearchRadiusPS) return;
    }

    float rw = rect.right - rect.left;
    float rh = rect.bottom - rect.top;
    if (rw <= 0.0f || rh <= 0.0f) return;

    float cu, cv;
    ProjectToUV(g_searchCenter, cu, cv);

    float hh = rh * 0.5f;
    float hw = rw * 0.5f;
    float radarRange = (g_curDist < 10.0f) ? 150.0f : g_curDist;
    float cosP = cosf(g_curTiltRad);
    if (cosP < 0.2f) cosP = 0.2f;

    float scale = hh / radarRange;
    float radiusU = (g_searchRadius * scale) / rw;
    float radiusV = (g_searchRadius * scale * cosP) / rh;

    float pulse = 0.5f + 0.5f * sinf(g_searchPulseAngle);

    // Heat perimeter scale and opacity
    float heatScale = g_config.heatRadiusScale;
    if (heatScale < 1.05f) heatScale = 2.50f;
    float heatOpa = g_config.heatRadiusOpacity;
    if (heatOpa > 1.0f) heatOpa /= 100.0f;
    if (heatOpa <= 0.001f) heatOpa = 0.33f;

    float searchBlur = g_config.searchRadiusBlur;
    if (searchBlur < 0.005f) searchBlur = 0.005f;
    float heatBlur = g_config.heatRadiusBlur;
    if (heatBlur < 0.005f) heatBlur = 0.005f;

    float innerAlpha = g_innerSearchAlpha;
    bool isHeatActive = g_config.useWantedHeatArea ? (!onMission || g_config.useWantedHeatAreaOnMission) : g_config.enableHeatArea;
    float heatAlpha = isHeatActive ? 1.0f : 0.0f;

    float constants[20] = {
        // c0: (centerX, centerY, radiusX, radiusY) in UV [0..1]
        cu, cv, radiusU, radiusV,
        // c1: (heatScale, heatOpacity, copVisual, pulse)
        heatScale, heatOpa, g_copHasVisual ? 1.0f : 0.0f, pulse,
        // c2: (strobeRed: r, g, b, a)
        (float)g_config.wantedColorRed.r / 255.0f, (float)g_config.wantedColorRed.g / 255.0f, (float)g_config.wantedColorRed.b / 255.0f, ((float)g_config.wantedColorRed.a / 255.0f) * alpha,
        // c3: (strobeBlue: r, g, b, a)
        (float)g_config.wantedColorBlue.r / 255.0f, (float)g_config.wantedColorBlue.g / 255.0f, (float)g_config.wantedColorBlue.b / 255.0f, ((float)g_config.wantedColorBlue.a / 255.0f) * alpha,
        // c4: (searchBlur, heatBlur, innerSearchAlpha, heatAlpha)
        searchBlur, heatBlur, innerAlpha, heatAlpha
    };

    IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
    if (dev) {
        dev->SetPixelShaderConstantF(0, constants, 5);
    }

    RwIm2DVertex quad[4];
    // Top-left
    RwIm2DVertexSetScreenX(&quad[0], rect.left);
    RwIm2DVertexSetScreenY(&quad[0], rect.top);
    RwIm2DVertexSetScreenZ(&quad[0], 0.0f);
    RwIm2DVertexSetRecipCameraZ(&quad[0], 1.0f);
    RwIm2DVertexSetIntRGBA(&quad[0], 255, 255, 255, (unsigned char)(255 * alpha));
    RwIm2DVertexSetU(&quad[0], 0.0f, 1.0f);
    RwIm2DVertexSetV(&quad[0], 0.0f, 1.0f);

    // Top-right
    RwIm2DVertexSetScreenX(&quad[1], rect.right);
    RwIm2DVertexSetScreenY(&quad[1], rect.top);
    RwIm2DVertexSetScreenZ(&quad[1], 0.0f);
    RwIm2DVertexSetRecipCameraZ(&quad[1], 1.0f);
    RwIm2DVertexSetIntRGBA(&quad[1], 255, 255, 255, (unsigned char)(255 * alpha));
    RwIm2DVertexSetU(&quad[1], 1.0f, 1.0f);
    RwIm2DVertexSetV(&quad[1], 0.0f, 1.0f);

    // Bottom-left
    RwIm2DVertexSetScreenX(&quad[2], rect.left);
    RwIm2DVertexSetScreenY(&quad[2], rect.bottom);
    RwIm2DVertexSetScreenZ(&quad[2], 0.0f);
    RwIm2DVertexSetRecipCameraZ(&quad[2], 1.0f);
    RwIm2DVertexSetIntRGBA(&quad[2], 255, 255, 255, (unsigned char)(255 * alpha));
    RwIm2DVertexSetU(&quad[2], 0.0f, 1.0f);
    RwIm2DVertexSetV(&quad[2], 1.0f, 1.0f);

    // Bottom-right
    RwIm2DVertexSetScreenX(&quad[3], rect.right);
    RwIm2DVertexSetScreenY(&quad[3], rect.bottom);
    RwIm2DVertexSetScreenZ(&quad[3], 0.0f);
    RwIm2DVertexSetRecipCameraZ(&quad[3], 1.0f);
    RwIm2DVertexSetIntRGBA(&quad[3], 255, 255, 255, (unsigned char)(255 * alpha));
    RwIm2DVertexSetU(&quad[3], 1.0f, 1.0f);
    RwIm2DVertexSetV(&quad[3], 1.0f, 1.0f);

    RwD3D9SetIm2DPixelShader(g_pSearchRadiusPS);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, quad, 4);

    RwD3D9SetIm2DPixelShader(nullptr);
}

void Radar::DrawPoliceTint(const CRect& rect, float alpha) {
    static float s_wantedFade = 0.0f;
    CPlayerPed* player = FindPlayerPed(0);
    unsigned int wanted = (player && player->m_pPlayerData && player->m_pPlayerData->m_pWanted)
                              ? player->m_pPlayerData->m_pWanted->m_nWantedLevel : 0;
    if (wanted > 0) {
        s_wantedFade = Lerpf(s_wantedFade, 1.0f, 0.05f);
    } else {
        s_wantedFade = Lerpf(s_wantedFade, 0.0f, 0.03f);
    }

    if (s_wantedFade < 0.005f) return;

    float dur = g_config.wantedDuration;
    if (dur < 0.1f) dur = 1.2f;
    float timeSec = (float)CTimer::m_snTimeInMillisecondsNonClipped * 0.001f;
    float phase = (float)fmod((double)timeSec, (double)dur) / dur;
    float t = 0.5f + 0.5f * sinf(phase * 2.0f * kPi);

    const CRGBA& colB = g_config.wantedColorBlue;
    const CRGBA& colR = g_config.wantedColorRed;

    unsigned char r = (unsigned char)Lerpf((float)colB.r, (float)colR.r, t);
    unsigned char g = (unsigned char)Lerpf((float)colB.g, (float)colR.g, t);
    unsigned char b = (unsigned char)Lerpf((float)colB.b, (float)colR.b, t);
    unsigned char a = (unsigned char)(Lerpf((float)colB.a, (float)colR.a, t) * s_wantedFade * alpha);

    DrawRoundedFill(rect, Radius(), CRGBA(r, g, b, a));
}

