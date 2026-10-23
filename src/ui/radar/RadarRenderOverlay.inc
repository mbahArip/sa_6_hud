void Radar::DrawOverlay() {
    bool isPreview = Menu::IsLayoutPreview();
    float alpha = isPreview ? 1.0f : g_radarAlphaFade;

    // HUD overlays (top-right ammo/icon/money, top-left health/armor)
    if ((CTheScripts::bDisplayHud && CReplay::Mode != 1) || isPreview) {
        DrawTopRightHUD(1.0f);
        DrawTopLeftHealthBar(1.0f);
    }

    // Wanted gauge normally renders from the game's HUD pass (hook on 0x58D9A0), which the
    // open menu suppresses — draw it here so the Layout tab preview is complete.
    if (isPreview) Hooked_DrawWanted();

    // Wanted overlays are gameplay-critical — draw them regardless of radar fade state
    CPlayerPed* player = FindPlayerPed(0);
    bool wantedActive = (player && player->m_pPlayerData && player->m_pPlayerData->m_pWanted
                         && player->m_pPlayerData->m_pWanted->m_nWantedLevel > 0);
    if (wantedActive) {
        if (g_config.enableCopArcs) DrawPoliceThreatArcs(1.0f);
    }

    if (alpha <= 0.005f) {
        // Radar hidden, but police threat auras must still be visible on screen
        if (wantedActive && g_config.enableCopAuras) {
            DrawPoliceThreatAuras(1.0f);
        }
        if (g_config.showInfoCards) {
            DrawInfoCards(g_radarRect, 0.0f);
        }
        return;
    }

    if (g_config.showInfoCards) {
        DrawInfoCards(g_radarRect, alpha);
    }

    IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
    RECT sc = { (LONG)g_radarRect.left, (LONG)g_radarRect.top, (LONG)g_radarRect.right, (LONG)g_radarRect.bottom };
    D3DRECT d3dRect = { (LONG)g_radarRect.left, (LONG)g_radarRect.top, (LONG)g_radarRect.right, (LONG)g_radarRect.bottom };

    // Always set up stencil + scissor when wanted — search radius needs the rounded clip mask
    bool needRadar = (g_config.enabled && alpha > 0.005f);
    bool needStencil = (needRadar || (wantedActive && g_wantedActive && g_config.enableWantedSearchRadius));
    if (needStencil && dev) {
        dev->SetScissorRect(&sc);
        dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
        dev->Clear(1, &d3dRect, D3DCLEAR_STENCIL, 0, 1.0f, 0);

        dev->SetRenderState(D3DRS_STENCILENABLE, TRUE);
        dev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
        dev->SetRenderState(D3DRS_STENCILREF, 1);
        dev->SetRenderState(D3DRS_STENCILMASK, 0xFF);
        dev->SetRenderState(D3DRS_STENCILWRITEMASK, 0xFF);
        dev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
        dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0);

        DrawRoundedFill(g_radarRect, Radius(), CRGBA(255, 255, 255, 255));

        dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
        dev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
        dev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
    }

    if (needRadar) DrawBackdrop(g_radarRect, alpha);
    if (needRadar) DrawMap2D(g_radarRect, alpha);
    if (needRadar && g_config.experimentalShow3dObjects) Draw3dObjects(g_radarRect, alpha);
    if (g_config.enableWantedSearchRadius) {
        DrawWantedSearchRadius(g_radarRect, 1.0f); // always full opacity when active
    }
    if (needRadar && g_config.enableGps) {
        DrawGpsPath(g_radarRect, alpha);
    }

    if (needStencil && dev) {
        dev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    }

    if (needRadar) {
        DrawBorder(g_radarRect, alpha);
        // Vanilla shows an empty dark disc inside interiors: no blips, no compass.
        if (CGame::currArea == 0) {
            DrawBlips(g_radarRect, alpha);
            DrawPoliceBlips(g_radarRect, alpha);
            DrawNorthBadge(g_radarRect, alpha);
        }
        DrawPlayerPuck(g_radarRect, alpha);
        DrawPinMarker(g_radarRect, alpha);
        DrawGpsDistance(g_radarRect, alpha);
    }

    // Top-most HUD layer: GTA 6 police threat auras wash light directly OVER the radar & map
    if (wantedActive && g_config.enableCopAuras) {
        DrawPoliceThreatAuras(1.0f);
    }
}

