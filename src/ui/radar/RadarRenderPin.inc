void Radar::DrawPinMarker(const CRect& rect, float alpha) {
    int targetIdx = GetTargetBlipIndex();
    bool active = (targetIdx >= 0 || g_pinActive);
    if (!active) return;

    CVector destPos = (targetIdx >= 0) ? CRadar::ms_RadarTrace[targetIdx].m_vecPos : g_pinPos;

    float rw = rect.right - rect.left;
    float rh = rect.bottom - rect.top;
    float sH = ScreenH();

    float u, v;
    ProjectToUV(destPos, u, v);
    float sx = rect.left + rw * u;
    float sy = rect.top + rh * v;

    float margin = sH * 0.012f;
    CRect innerRect(rect.left + margin, rect.top + margin, rect.right - margin, rect.bottom - margin);
    float innerRad = Radius() - margin;
    if (innerRad < 2.0f) innerRad = 2.0f;

    float bx, by;
    if (IsInside(sx, sy, innerRect, innerRad)) {
        bx = sx;
        by = sy;
    } else {
        CVector rel = destPos - g_playerPos;
        float rx = rel.Dot(g_camRight);
        float ry = rel.Dot(g_camUp);
        ProjectToRim(rect, rx, -ry, bx, by);
    }

    CSprite2d* wpSprite = (CRadar::RadarBlipSprites) ? &CRadar::RadarBlipSprites[RADAR_SPRITE_WAYPOINT] : nullptr;
    if (wpSprite && wpSprite->m_pTexture && wpSprite->m_pTexture->raster) {
        float size = sH * 0.018f * g_config.targetBlipScale;
        CRect sRect(bx - size * 0.5f, by - size * 0.5f, bx + size * 0.5f, by + size * 0.5f);
        wpSprite->Draw(sRect, CRGBA(255, 255, 255, (unsigned char)(255 * alpha)));
        return;
    }

    float crossRad = sH * 0.0090f * g_config.targetBlipScale;
    float armLen   = crossRad * 0.70f;
    float strokeW  = crossRad * 0.35f;

    if (g_config.showTargetBlipStroke && g_config.targetBlipStroke > 0.05f) {
        float st = g_config.targetBlipStroke;
        DrawCircle(bx, by, crossRad + st, CRGBA(0, 0, 0, (unsigned char)(200 * alpha)), 20);
    }
    DrawCircle(bx, by, crossRad, ModColor(g_config.targetBlipColor, alpha), 20);

    CRGBA crossCol(255, 255, 255, (unsigned char)(255 * alpha));
    CSprite2d::Draw2DPolygon(bx - armLen, by - strokeW * 0.5f, bx + armLen, by - strokeW * 0.5f, bx + armLen, by + strokeW * 0.5f, bx - armLen, by + strokeW * 0.5f, crossCol);
    CSprite2d::Draw2DPolygon(bx - strokeW * 0.5f, by - armLen, bx + strokeW * 0.5f, by - armLen, bx + strokeW * 0.5f, by + armLen, bx - strokeW * 0.5f, by + armLen, crossCol);
}

