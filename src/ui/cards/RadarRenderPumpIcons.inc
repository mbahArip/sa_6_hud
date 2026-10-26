// Radar icons for petrol pumps. Each cluster of pumps within kPumpClusterRadius
// shows one icon, so a forecourt with four pumps reads as a single marker. Reuses
// the telemetry fuel texture rather than adding an asset.

// Pause-map variant. The ESC map uses CRadar's own world->radar-space transform
// rather than the in-game projection, so it needs its own pass. Draws every cluster
// regardless of player distance.
void Radar::DrawPumpIconsOnMenuMap() {
    if (!g_config.useVehicleFuel) return;
    if (s_pumpClusterCount <= 0) return;

    const float sW = ScreenW();
    const float sH = ScreenH();
    const float size = sH * 0.020f;

    IDirect3DTexture9* tex = g_pTexFuel;

    for (int i = 0; i < s_pumpClusterCount; ++i) {
        const CVector& pos = s_pumpClusters[i].pos;

        CVector2D radarSpace;
        CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(pos.x, pos.y));
        CRadar::LimitRadarPoint(radarSpace);

        CVector2D scr;
        CRadar::TransformRadarPointToScreenSpace(scr, radarSpace);
        const float sx = scr.x * sW / 640.0f;
        const float sy = scr.y * sH / 448.0f;

        if (tex) {
            DrawD3DTexturedQuad(tex, sx - size * 0.5f, sy - size * 0.5f, size, size,
                                CRGBA(255, 255, 255, 235));
        } else {
            DrawCircle(sx, sy, size * 0.35f, g_config.palettePrimary, 16);
        }
    }
}


void Radar::DrawPumpIcons(const CRect& rect, float alpha, bool drawAll) {
    if (!g_config.useVehicleFuel) return;
    if (s_pumpClusterCount <= 0) return;

    const float sH = ScreenH();
    const float rw = rect.right - rect.left;
    const float rh = rect.bottom - rect.top;
    const float size = sH * 0.020f * g_config.radarSizeScale;

    // In gameplay only show pumps near the player so the map stays readable.
    const float nearSq = 100.0f * 100.0f;

    for (int i = 0; i < s_pumpClusterCount; ++i) {
        const CVector& pos = s_pumpClusters[i].pos;

        if (!drawAll) {
            const float dx = pos.x - g_playerPos.x;
            const float dy = pos.y - g_playerPos.y;
            if (dx * dx + dy * dy > nearSq) continue;
        }

        float u = 0.0f, v = 0.0f;
        if (!ProjectToUV(pos, u, v)) continue;

        const float sx = rect.left + rw * u;
        const float sy = rect.top + rh * v;

        // Skip anything that lands outside the radar disc.
        if (sx < rect.left || sx > rect.right || sy < rect.top || sy > rect.bottom) continue;

        IDirect3DTexture9* tex = g_pTexFuel;
        if (tex) {
            DrawD3DTexturedQuad(tex, sx - size * 0.5f, sy - size * 0.5f, size, size,
                                CRGBA(255, 255, 255, (unsigned char)(235.0f * alpha)));
        } else {
            DrawCircle(sx, sy, size * 0.35f, ModColor(g_config.palettePrimary, alpha), 16);
        }
    }
}
