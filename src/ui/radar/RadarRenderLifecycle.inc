void Radar::Render() {
    if (g_radarDead || !g_config.enabled) {
        CHud::bScriptDontDisplayRadar = false;
        CHud::bDrawClock = true;
        return;
    }

    CHud::bScriptDontDisplayRadar = true;
    CHud::bDrawClock = false;

    if ((!GameplayRunning() && !Menu::IsLayoutPreview()) || g_warmupFrames < 10) return;

    CPlayerPed* player = FindPlayerPed(0);
    if (!player || !Scene.m_pWorld) return;

    // Cheap while warm: every loader early-outs on a non-null pointer, and the
    // device comparison catches the save-load case where the device was swapped
    // without a lost/reset event and the cached textures went stale.
    CreateShaders();

    UpdateRadarRect();

    GuardedRun(DrawOverlay, "render.overlay");
}

} // namespace hud6
