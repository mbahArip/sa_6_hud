void Radar::DrawPoliceBlips(const CRect& rect, float alpha) {
    if (!g_config.showPoliceFoot && !g_config.showPoliceVehicle) return;

    CPlayerPed* player = FindPlayerPed(0);
    if (!player || !player->m_pPlayerData || !player->m_pPlayerData->m_pWanted) return;
    if (player->m_pPlayerData->m_pWanted->m_nWantedLevel == 0) return;

    float rw = rect.right - rect.left;
    float rh = rect.bottom - rect.top;
    float sH = ScreenH();
    float margin = sH * 0.012f;
    CRect innerRect(rect.left + margin, rect.top + margin, rect.right - margin, rect.bottom - margin);
    float innerRad = Radius() - margin;
    if (innerRad < 2.0f) innerRad = 2.0f;

    CVehicle* playerVeh = GetStablePlayerVehicle(player);
    float baseRad = sH * 0.0068f;

    // 1. Police vehicles
    if (g_config.showPoliceVehicle && CPools::ms_pVehiclePool) {
        int vehCount = CPools::ms_pVehiclePool->m_nSize;

        for (int i = 0; i < vehCount; ++i) {
            CVehicle* veh = CPools::ms_pVehiclePool->GetAt(i);
            if (!veh) continue;
            if (!veh->IsLawEnforcementVehicle()) continue;
            if (veh->m_fHealth <= 0.0f) continue;
            if (veh == playerVeh) continue;

            int model = veh->m_nModelIndex;
            float typeScale = g_config.policeCarScale;
            if (model == 432 || model == 520) {
                typeScale = g_config.policeTankHydraScale;
            } else if (model == 523 || veh->m_nVehicleClass == VEHICLE_BIKE || veh->m_nVehicleClass == VEHICLE_QUAD || veh->m_nVehicleClass == VEHICLE_BMX) {
                typeScale = g_config.policeBikeScale;
            } else if (model == 596 || model == 597 || model == 598 || model == 599) {
                typeScale = g_config.policeCarScale;
            } else {
                typeScale = g_config.policeHeliTruckScale;
            }

            float rad = baseRad * typeScale;

            bool hasDriver = (veh->m_pDriver != nullptr);
            float sirenFreq = hasDriver ? 0.012f : 0.004f;
            float sirenT = sinf((float)CTimer::m_snTimeInMillisecondsNonClipped * sirenFreq);

            unsigned char fillA = (unsigned char)((hasDriver ? 255 : 140) * alpha);
            unsigned char rimA  = (unsigned char)((hasDriver ? 180 : 100) * alpha);
            CRGBA sirenCol = (sirenT > 0.0f) ? CRGBA(30, 130, 255, fillA) : CRGBA(255, 45, 55, fillA);

            CVector worldPos = veh->GetPosition();
            float u, v, x, y;
            ProjectToUV(worldPos, u, v);
            float sx = rect.left + rw * u;
            float sy = rect.top + rh * v;

            if (IsInside(sx, sy, innerRect, innerRad)) {
                x = sx;
                y = sy;
            } else {
                CVector rel = worldPos - g_playerPos;
                float rx = rel.Dot(g_camRight);
                float ry = rel.Dot(g_camUp);
                ProjectToRim(rect, rx, -ry, x, y);
            }

            DrawCircle(x, y, rad + 1.2f, CRGBA(0, 0, 0, rimA), 20);
            DrawCircle(x, y, rad, sirenCol, 20);
        }
    }

    // 2. Police on foot
    if (g_config.showPoliceFoot && CPools::ms_pPedPool) {
        int pedCount = CPools::ms_pPedPool->m_nSize;
        float footRad = baseRad * g_config.policePedScale;
        CRGBA footCopCol(30, 130, 255, (unsigned char)(255 * alpha));

        for (int i = 0; i < pedCount; ++i) {
            CPed* ped = CPools::ms_pPedPool->GetAt(i);
            if (!ped) continue;
            if (ped->m_nPedType != PED_TYPE_COP) continue;
            if (ped->m_fHealth <= 0.0f || ped->m_ePedState == PEDSTATE_DEAD || ped->m_ePedState == PEDSTATE_DIE) continue;

            bool inVehicle = ped->bInVehicle || ped->m_ePedState == PEDSTATE_DRIVING || ped->m_ePedState == PEDSTATE_PASSENGER;
            if (inVehicle) continue;

            CVector worldPos = ped->GetPosition();
            float u, v, x, y;
            ProjectToUV(worldPos, u, v);
            float sx = rect.left + rw * u;
            float sy = rect.top + rh * v;

            if (IsInside(sx, sy, innerRect, innerRad)) {
                x = sx;
                y = sy;
            } else {
                CVector rel = worldPos - g_playerPos;
                float rx = rel.Dot(g_camRight);
                float ry = rel.Dot(g_camUp);
                ProjectToRim(rect, rx, -ry, x, y);
            }

            DrawCircle(x, y, footRad + 1.0f, CRGBA(0, 0, 0, (unsigned char)(160 * alpha)), 16);
            DrawCircle(x, y, footRad, footCopCol, 16);
        }
    }
}

