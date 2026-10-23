void Radar::DrawPoliceThreatArcs(float alpha) {
    bool onMission = CTheScripts::IsPlayerOnAMission();
    bool active = g_config.usePoliceArcIndicator ? (!onMission || g_config.usePoliceArcIndicatorOnMission) : g_config.enableCopArcs;
    if (!active || !CPools::ms_pPedPool) return;

    CPlayerPed* player = FindPlayerPed(0);
    if (!player) return;

    // Personnel arcs only track while player is ON FOOT
    if (player->m_pVehicle || player->bInVehicle) return;

    CVector camPos = g_playerPos;
    CVector camFwd = g_camUp;
    CVector camRgt = g_camRight;

    float sW = ScreenW();
    float sH = ScreenH();
    float cx = sW * 0.5f;
    float cy = sH * 0.5f;

    float arcRadius = (g_config.extWantedPoliceArcDistance * (sH / 720.0f)) * g_config.copArcScale;
    float strokeThickness = (sH / 720.0f) * 4.0f * g_config.copArcScale;

    int pedCount = CPools::ms_pPedPool->m_nSize;

    float minDist = g_config.extWantedPoliceArcMinDist;
    float maxDist = g_config.extWantedPoliceArcMaxDist;
    if (maxDist <= minDist) maxDist = minDist + 1.0f;

    for (int i = 0; i < pedCount; ++i) {
        CPed* ped = CPools::ms_pPedPool->GetAt(i);
        if (!ped || ped == player || ped->m_fHealth <= 0.0f) continue;
        if (ped->m_nPedType != PED_TYPE_COP) continue;

        // Skip cops inside vehicles (handled by vehicle siren indicator)
        bool inVeh = (ped->bInVehicle || ped->m_pVehicle != nullptr || 
                      ped->m_ePedState == PEDSTATE_DRIVING || ped->m_ePedState == PEDSTATE_PASSENGER);
        if (inVeh) continue;

        CVector copPos = ped->GetPosition();
        CVector diff = copPos - camPos;
        float dist = diff.Magnitude2D();
        if (dist > maxDist || dist < 0.5f) continue;

        // Opacity scales with distance: 100% at minDist, down to 0% at maxDist
        float prox = 1.0f - Clampf((dist - minDist) / (maxDist - minDist), 0.0f, 1.0f);
        if (prox <= 0.005f) continue;

        float relX = diff.Dot(camRgt);
        float relY = diff.Dot(camFwd);
        float dirAngle = atan2f(-relY, relX);

        // Arc length spans 60 deg (30 deg each side)
        float halfSpan = 30.0f * (kPi / 180.0f);
        float inRad  = arcRadius - strokeThickness * 0.5f;
        float outRad = arcRadius + strokeThickness * 0.5f;

        float coreAlphaF = prox * alpha * 255.0f;
        CRGBA arcCol = ModColor(g_config.wantedArcColor, (coreAlphaF / 255.0f));
        CRGBA shadowCol(10, 10, 15, (unsigned char)(coreAlphaF * 0.6f));

        if (g_pTexPersonnelArc) {
            float arcTexSize = arcRadius * 0.70f;
            DrawD3DRotatedQuad(g_pTexPersonnelArc, cx + arcRadius * cosf(dirAngle), cy + arcRadius * sinf(dirAngle), arcTexSize, arcTexSize, dirAngle, arcCol);
        } else {
            // Draw procedural 60-degree arc
            DrawArcStrip(cx, cy, inRad - 1.5f, outRad + 1.5f, dirAngle - halfSpan, dirAngle + halfSpan, shadowCol, 16);
            DrawArcStrip(cx, cy, inRad, outRad, dirAngle - halfSpan, dirAngle + halfSpan, arcCol, 16);
        }

        // Height difference indicator: vertical offset > 2.5m
        float zDiff = copPos.z - camPos.z;
        if (fabsf(zDiff) > 2.5f) {
            float tipDist = (zDiff > 0.0f) ? (outRad + strokeThickness * 2.2f) : (inRad - strokeThickness * 2.2f);
            float baseAngle1 = dirAngle - 0.06f;
            float baseAngle2 = dirAngle + 0.06f;
            float baseDist   = (zDiff > 0.0f) ? outRad : inRad;

            static RwIm2DVertex chevVerts[4];
            Set2DVert(chevVerts[0], cx + tipDist * cosf(dirAngle), cy + tipDist * sinf(dirAngle), arcCol);
            Set2DVert(chevVerts[1], cx + baseDist * cosf(baseAngle1), cy + baseDist * sinf(baseAngle1), arcCol);
            Set2DVert(chevVerts[2], cx + baseDist * cosf(baseAngle2), cy + baseDist * sinf(baseAngle2), arcCol);

            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
            RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, chevVerts, 3);
        }
    }
}

