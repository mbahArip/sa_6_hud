void Radar::DrawBorder(const CRect& rect, float alpha) {
    CRGBA col = ModColor(g_config.borderColor, alpha);
    DrawRoundedBorder(rect, Radius(), g_config.borderThickness, col);
}

void Radar::DrawBlips(const CRect& rect, float alpha) {
    if (!CRadar::ms_RadarTrace) return;
    float rw = rect.right - rect.left;
    float rh = rect.bottom - rect.top;
    float sH = ScreenH();

    int targetIdx = GetTargetBlipIndex();

    for (unsigned int i = 0; i < MAX_RADAR_TRACES; ++i) {
        if ((int)i == targetIdx) continue;

        tRadarTrace& trace = CRadar::ms_RadarTrace[i];
        if (!trace.m_bInUse || trace.m_nBlipDisplay == 0) continue;
        if (trace.m_nRadarSprite == RADAR_SPRITE_CENTRE || trace.m_nRadarSprite == RADAR_SPRITE_MAP_HERE) continue;

        CVector worldPos = trace.m_vecPos;
        if (trace.m_nBlipType == BLIP_CAR) {
            CVehicle* veh = CPools::GetVehicle(trace.m_nEntityHandle);
            if (!veh) continue;
            worldPos = veh->GetPosition();
        } else if (trace.m_nBlipType == BLIP_CHAR) {
            CPed* ped = CPools::GetPed(trace.m_nEntityHandle);
            if (!ped) continue;
            worldPos = ped->GetPosition();
        } else if (trace.m_nBlipType == BLIP_OBJECT) {
            CObject* obj = CPools::GetObject(trace.m_nEntityHandle);
            if (!obj) continue;
            worldPos = obj->GetPosition();
        }

        float u, v;
        ProjectToUV(worldPos, u, v);
        float sx = rect.left + rw * u;
        float sy = rect.top + rh * v;

        float margin = sH * 0.012f * g_config.radarSizeScale;
        CRect innerRect(rect.left + margin, rect.top + margin, rect.right - margin, rect.bottom - margin);\
        float innerRad = Radius() - margin;
        if (innerRad < 2.0f) innerRad = 2.0f;

        float bx, by;
        if (IsInside(sx, sy, innerRect, innerRad)) {
            bx = sx;
            by = sy;
        } else {
            bool isShortRange = trace.m_bShortRange != 0;
            bool isMissionBlip = (trace.m_nRadarSprite == RADAR_SPRITE_WAYPOINT ||
                                  trace.m_nRadarSprite == RADAR_SPRITE_NONE ||
                                  trace.m_nBlipType != BLIP_COORD);

            if (isShortRange || (trace.m_nRadarSprite > 0 && !isMissionBlip)) {
                continue;
            }

            CVector rel = worldPos - g_playerPos;
            float rx = rel.Dot(g_camRight);
            float ry = rel.Dot(g_camUp);
            ProjectToRim(rect, rx, -ry, bx, by);
        }

        int spriteId = (int)trace.m_nRadarSprite;
        if (spriteId == RADAR_SPRITE_NORTH && g_pTexNorth) {
            float size = sH * 0.021f * g_config.northBadgeScale * g_config.radarSizeScale;
            DrawD3DTexturedQuad(g_pTexNorth, bx - size * 0.5f, by - size * 0.5f, size, size,
                                CRGBA(255, 255, 255, (unsigned char)(255 * alpha)));
            continue;
        }

        if (spriteId > 0 && spriteId < 64) {
            CSprite2d* sprite = &CRadar::RadarBlipSprites[spriteId];
            if (sprite && sprite->m_pTexture) {
                float size = sH * 0.0165f * g_config.blipScale * g_config.radarSizeScale;
                CRect sRect(bx - size * 0.5f, by - size * 0.5f, bx + size * 0.5f, by + size * 0.5f);
                sprite->Draw(sRect, CRGBA(255, 255, 255, (unsigned char)(255 * alpha)));
                continue;
            }
        }

        // Mission Objective blip (RADAR_SPRITE_NONE)
        CRGBA markerCol;
        if (trace.m_nColour <= 4) {
            markerCol = g_config.markerColors[trace.m_nColour];
        } else {
            markerCol = CRGBA(CRadar::GetRadarTraceColour(trace.m_nColour, trace.m_bBright, trace.m_bFriendly));
        }
        markerCol = ModColor(markerCol, alpha);

        if (g_pTexMission) {
            float size = sH * 0.017f * g_config.missionBlipScale * g_config.radarSizeScale;
            float altDiff = worldPos.z - g_playerPos.z;
            bool differsAlt = fabsf(altDiff) > 2.0f;

            // Blink every 500ms by swapping with z_above.png or z_below.png without transition
            bool showAltTexture = differsAlt && (((CTimer::m_snTimeInMilliseconds / 500) % 2) == 1);

            IDirect3DTexture9* pActiveBlipTex = g_pTexMission;
            float blipW = size;
            float blipH = size;

            if (showAltTexture) {
                if (altDiff > 2.0f && g_pTexZAbove) {
                    pActiveBlipTex = g_pTexZAbove;
                    blipH = size * (167.0f / 256.0f);
                } else if (altDiff < -2.0f && g_pTexZBelow) {
                    pActiveBlipTex = g_pTexZBelow;
                    blipH = size * (167.0f / 256.0f);
                }
            }

            // Textures differ in aspect ratio; anchor to center (bx, by)
            DrawD3DTexturedQuad(pActiveBlipTex, bx - blipW * 0.5f, by - blipH * 0.5f, blipW, blipH, markerCol);
        } else {
            float dotRad = sH * 0.0055f * g_config.missionBlipScale;
            DrawCircle(bx, by, dotRad + 1.0f, CRGBA(0, 0, 0, (unsigned char)(180 * alpha)), 16);
            DrawCircle(bx, by, dotRad, markerCol, 16);
        }
    }
}

