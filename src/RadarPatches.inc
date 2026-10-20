static const float kPi = 3.141592653589793f;
static const int   kMaxPathNodes = 2000;

#define MAX_ENGINE_PATH_NODES 50000
static int s_enginePathNodes[MAX_ENGINE_PATH_NODES] = {};
static bool s_enginePatchesApplied = false;

static void ApplyGpsEnginePatches() {
    if (s_enginePatchesApplied) return;
    s_enginePatchesApplied = true;

    for (int i = 0; i < MAX_ENGINE_PATH_NODES - 1; ++i) {
        s_enginePathNodes[i] = -1;
    }

    plugin::patch::SetFloat(0x450ACD + 1, 6000.0f);
    plugin::patch::SetPointer(0x451782, s_enginePathNodes);
    plugin::patch::SetPointer(0x451904, s_enginePathNodes);
    plugin::patch::SetPointer(0x451AC3, s_enginePathNodes);
    plugin::patch::SetPointer(0x451B33, s_enginePathNodes);
    plugin::patch::SetUInt(0x4518F8, MAX_ENGINE_PATH_NODES);
    plugin::patch::SetUInt(0x4519B0, MAX_ENGINE_PATH_NODES - 5);
    LogWrite("gps: engine pathfinder buffers expanded to 50000 nodes, search radius set to 6000.0f");
}

static bool s_nitroPatchesApplied = false;

static void __fastcall Hooked_NitrousControl(CAutomobile* automobile, int, char numBoostsToSet) {
    if (!automobile) return;

    if (numBoostsToSet > 0) {
        automobile->m_nHandlingFlags.bNosInst = true;
        automobile->m_fNitroValue = 1.0f;
        automobile->m_nNitroBoosts = (unsigned char)numBoostsToSet;
        automobile->StopNitroEffect();
        return;
    }
    if (numBoostsToSet < 0) {
        automobile->m_nHandlingFlags.bNosInst = false;
        automobile->m_fNitroValue = 1.0f;
        automobile->m_nNitroBoosts = 0;
        automobile->StopNitroEffect();
        return;
    }

    if (automobile->m_nNitroBoosts <= 0 && !automobile->m_nHandlingFlags.bNosInst) {
        automobile->StopNitroEffect();
        return;
    }

    if (!automobile->m_pDriver) {
        automobile->StopNitroEffect();
        return;
    }

    bool nitroPressed = false;
    if (automobile->m_pDriver->IsPlayer()) {
        CPlayerPed* pPed = static_cast<CPlayerPed*>(automobile->m_pDriver);
        CPad* pad = pPed->GetPadFromPlayer();
        if (pad && pad->GetCarGunFired() && !pad->GetLookLeft() && !pad->GetLookRight() && !pad->GetLookBehindForCar()) {
            nitroPressed = true;
        }
    } else {
        nitroPressed = (automobile->m_fNitroValue > 0.0f);
    }

    if (!g_config.enableControllableNitro) {
        // Vanilla Nitro Mode:
        if (nitroPressed && automobile->m_fNitroValue <= 0.0f && automobile->m_nNitroBoosts > 0) {
            if (automobile->m_nNitroBoosts != 101 && automobile->m_nNitroBoosts != 255) {
                automobile->m_nNitroBoosts--;
            }
            automobile->m_fNitroValue = 1.0f;
        }

        if (automobile->m_fNitroValue > 0.0f) {
            // Vanilla drain speed: 1.0f over ~1000 frames @ 50fps = ~20 seconds
            automobile->m_fNitroValue -= CTimer::ms_fTimeStep * 0.001f;
            if (automobile->m_fNitroValue <= 0.0f) {
                automobile->m_fNitroValue = 0.0f;
                automobile->StopNitroEffect();
            } else {
                if (automobile->m_fGasPedal > 0.0f)
                    automobile->DoNitroEffect(fabsf(automobile->m_fGasPedal) * 0.5f + 0.5f);
                else
                    automobile->DoNitroEffect(0.5f);
            }
        } else {
            automobile->StopNitroEffect();
        }
        return;
    }

    // Controllable Nitro Mode (Hold-to-Boost + Tier-scaled Drain + Constant Recharge):
    unsigned char boosts = automobile->m_nNitroBoosts;
    if (boosts > 10) boosts = 10;
    if (boosts < 1) boosts = 1;

    // 2x (~4s), 5x (~7s), 10x (~16s)
    float drainRate    = (11.0f - (float)boosts) * 0.0005f;
    // Constant recharge speed across all bottle types: ~8-10 seconds to full tank
    float rechargeRate = 0.0020f;

    if (nitroPressed && automobile->m_fNitroValue > 0.005f) {
        automobile->m_fNitroValue -= CTimer::ms_fTimeStep * drainRate;
        if (automobile->m_fNitroValue < 0.0f) automobile->m_fNitroValue = 0.0f;

        if (automobile->m_fGasPedal > 0.0f)
            automobile->DoNitroEffect(fabsf(automobile->m_fGasPedal) * 0.5f + 0.5f);
        else
            automobile->DoNitroEffect(0.5f);
    } else {
        automobile->StopNitroEffect();
        if (automobile->m_fNitroValue < 1.0f) {
            float gasPedalFactor = 1.0f - fabsf(automobile->m_fGasPedal);
            if (gasPedalFactor < 0.5f) gasPedalFactor = 0.5f;

            automobile->m_fNitroValue += gasPedalFactor * CTimer::ms_fTimeStep * rechargeRate;
            if (automobile->m_fNitroValue > 1.0f) automobile->m_fNitroValue = 1.0f;
        }
    }
}

static void ApplyNitroPatches() {
    if (s_nitroPatchesApplied) return;
    s_nitroPatchesApplied = true;
    plugin::patch::RedirectJump(0x6A3EA0, Hooked_NitrousControl);
    LogWrite("nitro: nitrous control hook installed at 0x6A3EA0");
}

