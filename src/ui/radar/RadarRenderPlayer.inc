void Radar::DrawPlayerPuck(const CRect& rect, float alpha) {
    float cx = (rect.left + rect.right) * 0.5f;
    float cy = rect.top + (rect.bottom - rect.top) * g_curVCenter;
    float sH = ScreenH();

    CPlayerPed* player = FindPlayerPed(0);
    CVector pFwd = g_playerFwd;
    if (player) {
        if (player->m_matrix) {
            pFwd = player->m_matrix->GetForward();
        } else {
            float heading = player->GetHeading();
            pFwd = CVector(-sinf(heading), cosf(heading), 0.0f);
        }
    }
    CVehicle* veh = GetStablePlayerVehicle(player);
    if (veh) {
        if (veh->m_matrix) {
            pFwd = veh->m_matrix->GetForward();
        } else {
            float heading = veh->GetHeading();
            pFwd = CVector(-sinf(heading), cosf(heading), 0.0f);
        }
    }

    float sx = pFwd.Dot(g_camRight);
    float sy = pFwd.Dot(g_camUp);
    float blipAngle = atan2f(sx, sy);

    float discRad = sH * 0.0112f * g_config.playerPuckScale * g_config.radarSizeScale;

    float baseCos = cosf(g_curTiltRad);
    if (baseCos < 0.35f) baseCos = 0.35f;

    float cosP = Lerpf(1.0f, baseCos, g_curPuckTilt);
    discRad *= Lerpf(1.0f, 1.10f, g_curPuckTilt);
    float raiseY = g_curPuckTilt * (1.0f - baseCos) * (sH * 0.0035f);

    if (g_pTexPlayer) {
        float puckSize = discRad * 2.2f;
        float puckW = puckSize;
        float puckH = puckSize * cosP;

        if (g_curPuckTilt > 0.01f) {
            float shadowOffsetY = raiseY * 1.5f + 1.2f;
            unsigned char shadowA = (unsigned char)(140.0f * g_curPuckTilt * alpha);
            DrawD3DRotatedQuad(g_pTexPlayer, cx, cy + shadowOffsetY, puckW + 1.2f, (puckW + 1.2f) * cosP, blipAngle, CRGBA(0, 0, 0, shadowA));
        }

        float wantedEased = s_playerWantedFade * s_playerWantedFade * (3.0f - 2.0f * s_playerWantedFade);
        if (wantedEased <= 0.005f) {
            DrawD3DRotatedQuad(g_pTexPlayer, cx, cy - raiseY, puckW, puckH, blipAngle, CRGBA(255, 255, 255, (unsigned char)(255 * alpha)));
        } else if (wantedEased >= 0.995f) {
            DrawD3DRotatedQuad(g_pTexPlayerWanted ? g_pTexPlayerWanted : g_pTexPlayer, cx, cy - raiseY, puckW, puckH, blipAngle, CRGBA(255, 255, 255, (unsigned char)(255 * alpha)));
        } else {
            float normalA = alpha * (1.0f - wantedEased);
            float wantedA = alpha * wantedEased;
            DrawD3DRotatedQuad(g_pTexPlayer, cx, cy - raiseY, puckW, puckH, blipAngle, CRGBA(255, 255, 255, (unsigned char)(255 * normalA)));
            if (g_pTexPlayerWanted) {
                DrawD3DRotatedQuad(g_pTexPlayerWanted, cx, cy - raiseY, puckW, puckH, blipAngle, CRGBA(255, 255, 255, (unsigned char)(255 * wantedA)));
            }
        }
        return;
    }

    if (g_curPuckTilt > 0.01f) {
        float shadowOffsetY = raiseY * 1.5f + 1.2f;
        unsigned char shadowA = (unsigned char)(140.0f * g_curPuckTilt * alpha);
        DrawEllipse(cx, cy + shadowOffsetY, discRad + 1.2f, (discRad + 1.2f) * cosP, CRGBA(0, 0, 0, shadowA), 24);
    }

    // 1. Transparent 25% black disc background
    CRGBA discCol = ModColor(g_config.playerPuckDiscColor, alpha);
    DrawEllipse(cx, cy - raiseY, discRad, discRad * cosP, discCol, 24);

    // 2. Hollow black stroke ring around disc rim
    if (g_config.showPlayerPuckStroke && g_config.playerPuckStroke > 0.05f) {
        float st = g_config.playerPuckStroke;
        CRGBA strokeCol = ModColor(g_config.playerPuckStrokeColor, alpha);
        DrawArcStrip(cx, cy - raiseY, discRad, discRad + st, 0.0f, 2.0f * kPi, strokeCol, 24);
    }

    // 3. Player white arrow pointer
    float ca = cosf(blipAngle), sa = sinf(blipAngle);
    auto rot = [&](float lx, float ly, float& ox, float& oy) {
        float rx = lx * ca + ly * sa;
        float ry = (ly * ca - lx * sa) * cosP;
        ox = cx + rx;
        oy = (cy - raiseY) - ry;
    };

    float h = discRad * 0.760f;
    float w = discRad * 0.462f;
    float notch = discRad * 0.18f;
    float xTip, yTip, xL, yL, xR, yR, xNotch, yNotch;

    rot(0.0f, h - 0.7f, xTip, yTip);
    rot(-w, -h * 0.55f - 0.7f, xL, yL);
    rot(w, -h * 0.55f - 0.7f, xR, yR);
    rot(0.0f, -notch - 0.7f, xNotch, yNotch);
    CSprite2d::Draw2DPolygon(xTip, yTip, xL, yL, xNotch, yNotch, xNotch, yNotch, CRGBA(0, 0, 0, (unsigned char)(180 * alpha)));
    CSprite2d::Draw2DPolygon(xTip, yTip, xNotch, yNotch, xR, yR, xR, yR, CRGBA(0, 0, 0, (unsigned char)(180 * alpha)));

    CRGBA arrowCol = ModColor(g_config.playerPuckArrowColor, alpha);
    rot(0.0f, h, xTip, yTip);
    rot(-w, -h * 0.55f, xL, yL);
    rot(w, -h * 0.55f, xR, yR);
    rot(0.0f, -notch, xNotch, yNotch);
    CSprite2d::Draw2DPolygon(xTip, yTip, xL, yL, xNotch, yNotch, xNotch, yNotch, arrowCol);
    CSprite2d::Draw2DPolygon(xTip, yTip, xNotch, yNotch, xR, yR, xR, yR, arrowCol);
}

