void Radar::DrawNorthBadge(const CRect& rect, float alpha) {
    CVector north(0.0f, 1.0f, 0.0f);
    float nx = north.Dot(g_camRight);
    float ny = north.Dot(g_camUp);
    float bx, by;
    ProjectToRim(rect, nx, -ny, bx, by);

    const float sH = ScreenH();
    float badgeRad = sH * 0.0105f * g_config.northBadgeScale * g_config.radarSizeScale;

    if (g_pTexNorth) {
        float size = badgeRad * 2.0f;
        DrawD3DTexturedQuad(g_pTexNorth, bx - size * 0.5f, by - size * 0.5f, size, size,
                            CRGBA(255, 255, 255, (unsigned char)(255 * alpha)));
        return;
    }

    CSprite2d* northSprite = (CRadar::RadarBlipSprites) ? &CRadar::RadarBlipSprites[RADAR_SPRITE_NORTH] : nullptr;
    if (northSprite && northSprite->m_pTexture && northSprite->m_pTexture->raster) {
        CRect spriteRect(bx - badgeRad, by - badgeRad, bx + badgeRad, by + badgeRad);
        northSprite->Draw(spriteRect, CRGBA(255, 255, 255, (unsigned char)(255 * alpha)));
        return;
    }

    DrawCircle(bx, by, badgeRad + 1.2f, CRGBA(0, 0, 0, (unsigned char)(160 * alpha)), 24);
    if (g_config.showNorthBadgeStroke && g_config.northBadgeStroke > 0.05f) {
        DrawCircle(bx, by, badgeRad + g_config.northBadgeStroke, CRGBA(255, 255, 255, (unsigned char)(220 * alpha)), 24);
    }
    DrawCircle(bx, by, badgeRad, CRGBA(10, 12, 16, (unsigned char)(255 * alpha)), 24);

    float nH = badgeRad * 1.05f;
    float nW = nH * 0.70f;
    float stemW = badgeRad * 0.22f;
    float xL = bx - nW * 0.5f;
    float xR = bx + nW * 0.5f;
    float yT = by - nH * 0.5f;
    float yB = by + nH * 0.5f;
    CRGBA nCol(255, 255, 255, (unsigned char)(255 * alpha));

    CSprite2d::Draw2DPolygon(xL, yT, xL + stemW, yT, xL + stemW, yB, xL, yB, nCol);
    CSprite2d::Draw2DPolygon(xR - stemW, yT, xR, yT, xR, yB, xR - stemW, yB, nCol);
    CSprite2d::Draw2DPolygon(xL, yT, xL + stemW, yT, xR, yB, xR - stemW, yB, nCol);
}

