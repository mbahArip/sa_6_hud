void Radar::DrawPoliceThreatAuras(float alpha) {
    bool onMission = CTheScripts::IsPlayerOnAMission();
    bool active = g_config.usePoliceSirenIndicator ? (!onMission || g_config.usePoliceSirenIndicatorOnMission) : g_config.enableCopAuras;
    if (!active || !CPools::ms_pVehiclePool) return;

    CPlayerPed* player = FindPlayerPed(0);
    if (!player) return;

    bool isFoot = (player->m_pVehicle == nullptr && !player->bInVehicle);
    if (isFoot && !g_config.extWantedPoliceSirenOnFoot) return;

    unsigned int wanted = (player->m_pPlayerData && player->m_pPlayerData->m_pWanted)
                              ? player->m_pPlayerData->m_pWanted->m_nWantedLevel : 1;
    float wantedRatio = Clampf((float)(wanted > 0 ? (wanted - 1) : 0) / 5.0f, 0.0f, 1.0f);
    float wantedIntensity = Lerpf(g_config.extWantedPoliceSirenMinIntensity, g_config.extWantedPoliceSirenMaxIntensity, wantedRatio);
    float wantedSizeMult  = Lerpf(g_config.extWantedPoliceSirenMinSize, g_config.extWantedPoliceSirenMaxSize, wantedRatio);

    CVector camPos = g_playerPos;
    CVector camFwd = g_camUp;
    CVector camRgt = g_camRight;

    float sW = ScreenW();
    float sH = ScreenH();
    float cx = sW * 0.5f;
    float cy = sH * 0.5f;
    float hw = cx;
    float hh = cy;

    unsigned int timeMs = CTimer::m_snTimeInMillisecondsNonClipped;
    int vehCount = CPools::ms_pVehiclePool->m_nSize;
    CVehicle* playerVeh = GetStablePlayerVehicle(player);

    for (int i = 0; i < vehCount; ++i) {
        CVehicle* veh = CPools::ms_pVehiclePool->GetAt(i);
        if (!veh || veh->m_fHealth <= 0.0f) continue;
        if (veh == playerVeh) continue;

        int model = veh->m_nModelIndex;
        bool isPoliceModel = (model == 596 || model == 597 || model == 598 || model == 599 || 
                              model == 523 || model == 427 || model == 490 || model == 528 || 
                              model == 497 || model == 432 || model == 520 || veh->IsLawEnforcementVehicle());
        if (!isPoliceModel) continue;

        bool isHeli = (model == 497 || veh->m_nVehicleClass == VEHICLE_HELI);
        if (isHeli && !g_config.extWantedPoliceSirenHelicopter) continue;

        bool hasDriver = (veh->m_pDriver && veh->m_pDriver != player && veh->m_pDriver->m_fHealth > 0.0f);
        float driverOpacity = 1.0f;
        if (!isHeli && !hasDriver) {
            if (!g_config.extWantedPoliceSirenRoadblock) continue;
            driverOpacity = g_config.extWantedPoliceSirenNoDriverOpacity;
        }

        CVector vehPos = veh->GetPosition();
        CVector diff = vehPos - camPos;
        float dist = diff.Magnitude2D();
        if (dist > g_config.copAuraMaxDist || dist < 1.0f) continue;

        float prox = 1.0f - (dist / g_config.copAuraMaxDist);
        if (prox <= 0.01f) continue;

        // Vehicle screen visibility culling: smoothly fade out aura when police vehicle is inside camera view
        float targetCullAlpha = 1.0f;
        if (g_config.copAuraVehCull) {
            RwV3d rwPos = { vehPos.x, vehPos.y, vehPos.z };
            RwV3d scrPos = {};
            float projW = 0.0f, projH = 0.0f;
            bool onScreen = CSprite::CalcScreenCoors(rwPos, &scrPos, &projW, &projH, true, true);

            if (onScreen && scrPos.z > 0.0f) {
                float margin = sH * 0.15f; // Generous 15% inner margin for smooth fade band
                float distLeft   = scrPos.x;
                float distRight  = sW - scrPos.x;
                float distTop    = scrPos.y;
                float distBottom = sH - scrPos.y;
                float minDistToEdge = fminf(fminf(distLeft, distRight), fminf(distTop, distBottom));

                if (minDistToEdge >= margin) {
                    targetCullAlpha = 0.0f; // Fully centered in view
                } else if (minDistToEdge > 0.0f) {
                    targetCullAlpha = 1.0f - (minDistToEdge / margin); // Smoothly ramp up as it nears the screen edge
                } else {
                    targetCullAlpha = 1.0f; // Outside screen boundary
                }
            } else {
                targetCullAlpha = 1.0f; // Off-screen / behind camera
            }
        }

        // Per-vehicle smooth temporal culling transition to eliminate snap pop-in/pop-out
        static float s_vehCullAlpha[128] = {};
        int cullSlot = i % 128;
        s_vehCullAlpha[cullSlot] = Lerpf(s_vehCullAlpha[cullSlot], targetCullAlpha, 0.12f);
        float cullAlpha = s_vehCullAlpha[cullSlot];
        if (cullAlpha <= 0.005f) continue;

        // Direction in screen-space
        float relX = diff.Dot(camRgt);
        float relY = diff.Dot(camFwd);
        float dirLen = sqrtf(relX * relX + relY * relY);
        if (dirLen < 0.001f) continue;

        float ndx = relX / dirLen;
        float ndy = -relY / dirLen; // Invert for screen Y (downwards)

        // Siren strobe cycle
        float strobePhase = ((float)(timeMs + (unsigned int)i * 180) * 0.012f);
        float strobeSin = sinf(strobePhase);

        float redAlphaVal  = (strobeSin > 0.0f ? 1.0f : 0.20f) * prox * wantedIntensity * alpha * cullAlpha * driverOpacity;
        float blueAlphaVal = (strobeSin <= 0.0f ? 1.0f : 0.20f) * prox * wantedIntensity * alpha * cullAlpha * driverOpacity;

        float tanX = -ndy;
        float tanY = ndx;
        float inwardX = cx + ndx * (hw * 0.70f);
        float inwardY = cy + ndy * (hh * 0.70f);

        if (g_pPoliceAuraPS) {
            float baseSize = sH * 0.28f * g_config.copAuraSize * wantedSizeMult;
            float curAspect = Lerpf(g_config.copAuraFootAspectRatio, g_config.copAuraAspectRatio, s_vehStateLerp);
            float dimForward = baseSize;
            float dimTangent = baseSize * curAspect;
            float hF = dimForward * 0.5f;
            float hT = dimTangent * 0.5f;

            // In-vehicle: screen edge ray cast with inward offset
            float scaleX = (fabsf(ndx) > 1e-4f) ? (hw / fabsf(ndx)) : 1e6f;
            float scaleY = (fabsf(ndy) > 1e-4f) ? (hh / fabsf(ndy)) : 1e6f;
            float R_edge = fminf(scaleX, scaleY);
            float inwardOffset = (0.50f - g_config.copAuraVehHiddenPercent) * dimForward;
            float R_veh = R_edge - inwardOffset;

            // On-foot: perimeter circle around player
            float footCircleRadius = (g_config.extWantedPoliceSirenOnFootDist * (sH / 720.0f));
            float R_foot = footCircleRadius;

            // Blended trajectory
            float superRadius = Lerpf(R_foot, R_veh, s_vehStateLerp);

            float centerX = cx + ndx * superRadius;
            float centerY = cy + ndy * superRadius;

            // Orientation
            float radRot = g_config.copAuraBaseRotation * (3.14159265f / 180.0f);
            float angle = atan2f(-ndy, -ndx) + radRot;
            float fwdX = cosf(angle);
            float fwdY = sinf(angle);
            float tX = -fwdY;
            float tY = fwdX;

            float fx = fwdX * hF;
            float fy = fwdY * hF;
            float tx = tX * hT;
            float ty = tY * hT;

            // Sequential 4-pod chase wave (frequency scales with distance)
            float speed = g_config.copAuraStrobeSpeed;
            if (speed < 0.1f) speed = 1.0f;
            float distSpeedMult = Lerpf(g_config.extWantedPoliceSirenFarSpeedMult, 1.0f, prox);
            float cyclePeriod = (0.25f / speed) / distSpeedMult;

            uintptr_t vehHash = ((uintptr_t)veh >> 4) * 2654435761u;
            float vehPhaseOffset = (float)(vehHash & 0xFFFF) / 65536.0f * 10.0f;
            float timeSec = ((float)timeMs * 0.001f) + vehPhaseOffset;
            float cycle = fmodf(timeSec, cyclePeriod) / cyclePeriod;

            float wavePos = (cycle < 0.50f)
                ? (cycle / 0.50f) * 3.0f
                : 3.0f - ((cycle - 0.50f) / 0.50f) * 3.0f;

            float threatAlpha = Lerpf(g_config.copAuraMinIntensity, g_config.copAuraMaxIntensity, prox);
            threatAlpha *= wantedIntensity * g_config.copAuraIntensity * alpha * cullAlpha * driverOpacity;
            threatAlpha = Clampf(threatAlpha, 0.0f, 1.0f);
            threatAlpha = Clampf(threatAlpha, 0.0f, 1.0f);

            float pAlpha[4];
            float pRad[4];
            float ambientFloor = 0.05f * g_config.copAuraChromaticDepth;

            for (int k = 0; k < 4; ++k) {
                float dist = fabsf(wavePos - (float)k);
                float pulse = expf(-dist * dist * 3.2f);
                pulse = powf(pulse, 1.4f);
                pulse = fmaxf(pulse, ambientFloor);

                pAlpha[k] = pulse * threatAlpha;
                pRad[k] = 0.075f * (0.70f + 0.65f * pulse);
            }

            float auraConsts[24] = {
                pAlpha[0], pAlpha[1], pAlpha[2], pAlpha[3],
                pRad[0], pRad[1], pRad[2], pRad[3],
                255.0f / 255.0f, 12.0f / 255.0f, 24.0f / 255.0f, 1.0f,
                5.0f / 255.0f, 120.0f / 255.0f, 255.0f / 255.0f, 1.0f,
                0.38f, 0.46f, 0.54f, 0.62f,
                0.22f, g_config.copAuraWhiteCoreStrength, 0.0f, 0.0f
            };

            IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
            if (dev) {
                dev->SetPixelShaderConstantF(0, auraConsts, 6);
            }

            RwIm2DVertex auraQuad[4];
            RwIm2DVertexSetScreenX(&auraQuad[0], centerX - tx + fx);
            RwIm2DVertexSetScreenY(&auraQuad[0], centerY - ty + fy);
            RwIm2DVertexSetScreenZ(&auraQuad[0], 0.0f);
            RwIm2DVertexSetRecipCameraZ(&auraQuad[0], 1.0f);
            RwIm2DVertexSetIntRGBA(&auraQuad[0], 255, 255, 255, 255);
            RwIm2DVertexSetU(&auraQuad[0], 0.0f, 1.0f);
            RwIm2DVertexSetV(&auraQuad[0], 1.0f, 1.0f);

            RwIm2DVertexSetScreenX(&auraQuad[1], centerX + tx + fx);
            RwIm2DVertexSetScreenY(&auraQuad[1], centerY + ty + fy);
            RwIm2DVertexSetScreenZ(&auraQuad[1], 0.0f);
            RwIm2DVertexSetRecipCameraZ(&auraQuad[1], 1.0f);
            RwIm2DVertexSetIntRGBA(&auraQuad[1], 255, 255, 255, 255);
            RwIm2DVertexSetU(&auraQuad[1], 1.0f, 1.0f);
            RwIm2DVertexSetV(&auraQuad[1], 1.0f, 1.0f);

            RwIm2DVertexSetScreenX(&auraQuad[2], centerX + tx - fx);
            RwIm2DVertexSetScreenY(&auraQuad[2], centerY + ty - fy);
            RwIm2DVertexSetScreenZ(&auraQuad[2], 0.0f);
            RwIm2DVertexSetRecipCameraZ(&auraQuad[2], 1.0f);
            RwIm2DVertexSetIntRGBA(&auraQuad[2], 255, 255, 255, 255);
            RwIm2DVertexSetU(&auraQuad[2], 1.0f, 1.0f);
            RwIm2DVertexSetV(&auraQuad[2], 0.0f, 1.0f);

            RwIm2DVertexSetScreenX(&auraQuad[3], centerX - tx - fx);
            RwIm2DVertexSetScreenY(&auraQuad[3], centerY - ty - fy);
            RwIm2DVertexSetScreenZ(&auraQuad[3], 0.0f);
            RwIm2DVertexSetRecipCameraZ(&auraQuad[3], 1.0f);
            RwIm2DVertexSetIntRGBA(&auraQuad[3], 255, 255, 255, 255);
            RwIm2DVertexSetU(&auraQuad[3], 0.0f, 1.0f);
            RwIm2DVertexSetV(&auraQuad[3], 0.0f, 1.0f);

            RwD3D9SetIm2DPixelShader(g_pPoliceAuraPS);
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
            RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

            RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, auraQuad, 4);

            RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

            RwD3D9SetIm2DPixelShader(nullptr);
        }
    }
}

