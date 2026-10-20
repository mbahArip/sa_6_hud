void Radar::Render() {
    if (g_radarDead || !g_config.enabled) {
        CHud::bScriptDontDisplayRadar = false;
        CHud::bDrawClock = true;
        return;
    }

    CHud::bScriptDontDisplayRadar = true;
    CHud::bDrawClock = false;

    if (!GameplayRunning() || g_warmupFrames < 10) return;

    CPlayerPed* player = FindPlayerPed(0);
    if (!player || !Scene.m_pWorld) return;

    UpdateRadarRect();

    GuardedRun(DrawOverlay, "render.overlay");
}

} // namespace hud6
