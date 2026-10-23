static bool IsPoliceVehicleModel(int model) {
    return model == MODEL_COPCARLA || model == MODEL_COPCARSF ||
        model == MODEL_COPCARVG || model == MODEL_COPCARRU ||
        model == MODEL_COPBIKE || model == MODEL_FBIRANCH ||
        model == MODEL_RANCHER || model == MODEL_SWATVAN ||
        model == MODEL_ENFORCER || model == MODEL_POLMAV ||
        model == MODEL_HUNTER || model == MODEL_RHINO ||
        model == MODEL_BARRACKS || model == MODEL_PREDATOR;
}

static bool HasValidPoliceDriver(CVehicle* vehicle) {
    if (!vehicle || !vehicle->m_pDriver || vehicle->m_pDriver->m_fHealth <= 0.0f) return false;
    return vehicle->m_pDriver->m_nPedType == PED_TYPE_COP;
}

static bool IsPoliceHelicopter(CVehicle* vehicle) {
    if (!vehicle) return false;
    return vehicle->m_nModelIndex == MODEL_POLMAV || vehicle->m_nModelIndex == MODEL_HUNTER ||
        vehicle->m_nVehicleClass == VEHICLE_HELI;
}

void Radar::UpdateWantedSearch() {
    CPlayerPed* player = FindPlayerPed(0);
    if (!player) return;

    float dt = CTimer::ms_fTimeStep * 0.02f;
    if (dt > 0.1f) dt = 0.1f;

    CWanted* pWanted = (player && player->m_pPlayerData) ? player->m_pPlayerData->m_pWanted : nullptr;
    unsigned int wanted = pWanted ? pWanted->m_nWantedLevel : 0;
    bool onParole = pWanted ? ((pWanted->m_nTimeOfParole > 0) || pWanted->m_bPoliceBackOffGarage) : false;

    // Check cheats (AEZAKMI / TURNDOWNTHEHEAT)
    bool cheatClearActive = CCheat::m_aCheatsActive[CHEAT_I_DO_AS_I_PLEASE] || CCheat::m_aCheatsActive[CHEAT_CLEAR_WANTED_LEVEL];
    if (cheatClearActive) {
        WipeAllHeatAndSearch();
        CCheat::m_aCheatsActive[CHEAT_CLEAR_WANTED_LEVEL] = false;
    }

    if (onParole) {
        if (player->m_pVehicle && IsCurrentVehicleHot(player)) {
            s_resprayCarWasHot = true;
        }
        g_wantedActive = false;
        g_wantedStage = WANTED_STAGE_INACTIVE;
        g_copHasVisual = false;
        g_escapeTimer = 0.0f;
        g_heatEscapeTimer = 0.0f;
    }

    UnlockTransFenderGarages();

    bool onMission = CTheScripts::IsPlayerOnAMission();
    bool isSearchAreaActive = g_config.useWantedSearchArea ? (!onMission || g_config.useWantedSearchAreaOnMission) : g_config.enableWantedSearchRadius;
    bool isHeatAreaActive = g_config.useWantedHeatArea ? (!onMission || g_config.useWantedHeatAreaOnMission) : g_config.enableHeatArea;
    bool isCriminalProfileActive = g_config.useCriminalProfile && (!onMission || g_config.useCriminalProfileOnMission);
    bool isSnitchActive = g_config.useSnitchSystem && (!onMission || g_config.useSnitchSystemOnMission);

    if (isCriminalProfileActive) {
        TrackGunHeat(player, wanted, dt);
        ProcessPayNSpray(player);
        ProcessTransfenderPlateMenu(player);
    }

    UpdateSnitchSystem(player, wanted, dt);

    if (wanted == 0 || !isSearchAreaActive) {
        g_wantedActive = false;
        g_wantedStage = WANTED_STAGE_INACTIVE;
        g_copHasVisual = false;
        g_escapeTimer = 0.0f;
        g_heatEscapeTimer = 0.0f;
        g_innerSearchAlpha = 1.0f;

        if (isCriminalProfileActive) {
            TrackClothesAndVehicleHeat(player, 0, false, dt);
            ProcessCopScanning(player, dt);
        }
        return;
    }

    // Strobe frequency: 600ms cycle time during visual pursuit; 900ms transition time when visual lost
    float strobeSpeed = g_copHasVisual ? (2.0f * kPi / 0.600f) : (2.0f * kPi / 0.900f);
    g_searchPulseAngle += dt * strobeSpeed;

    // Scale search radius dynamically based on wanted level (base 100m at 1 star, up to 1250m at 6 stars)
    float targetRadius = g_config.extWantedSearchRadiusBase + (float)(wanted > 0 ? (wanted - 1) : 0) * g_config.extWantedSearchRadiusMultiplier;
    g_searchRadius = Lerpf(g_searchRadius, targetRadius, dt * 4.0f);

    // Dynamic escape durations
    float lkpTargetEscapeTime = g_config.extWantedEscapeBaseDuration + (float)(wanted > 0 ? (wanted - 1) : 0) * g_config.extWantedEscapeMultiplier;
    float heatTargetEscapeTime = g_config.extWantedHeatAreaBaseDuration + (float)wanted * g_config.extWantedHeatAreaDurationMultiplier;
    if (isCriminalProfileActive) {
        heatTargetEscapeTime *= g_config.extWantedHeatAreaZoneMultiplier;
    }

    bool hasVisual = false;
    CVector pPos = player->GetPosition();
    CVector playerEyePos = pPos + CVector(0.0f, 0.0f, 0.70f);

    // 1. Check cops on foot (must be alive, cop ped type, not player)
    if (CPools::ms_pPedPool) {
        int pedCount = CPools::ms_pPedPool->m_nSize;
        for (int i = 0; i < pedCount && !hasVisual; ++i) {
            CPed* ped = CPools::ms_pPedPool->GetAt(i);
            if (!ped || ped == player || ped->m_fHealth <= 0.0f) continue;
            if (ped->m_nPedType != PED_TYPE_COP) continue;

            CVector copPos = ped->GetPosition();
            float distSq = (copPos - pPos).MagnitudeSqr();
            if (distSq > (85.0f * 85.0f)) continue;

            float dist = sqrtf(distSq);
            CVector copEyePos = copPos + CVector(0.0f, 0.0f, 0.75f);

            if (dist < 8.0f) {
                // Within 8m, 360 degree sound/awareness
                if (CWorld::GetIsLineOfSightClear(copEyePos, playerEyePos, true, false, false, true, false, false, false)) {
                    hasVisual = true;
                    break;
                }
            } else {
                // Beyond 8m, check 160 degree forward vision cone
                CVector toPlayer = (pPos - copPos).Normalized();
                CVector copFwd = ped->m_matrix ? ped->GetForward() : CVector(0.0f, 1.0f, 0.0f);
                if (copFwd.Dot(toPlayer) > 0.15f) {
                    if (CWorld::GetIsLineOfSightClear(copEyePos, playerEyePos, true, false, false, true, false, false, false)) {
                        hasVisual = true;
                        break;
                    }
                }
            }
        }
    }

    // 2. Check active police vehicles (must have alive cop driver, or be a police helicopter)
    CVehicle* playerVeh = GetStablePlayerVehicle(player);
    if (!hasVisual && CPools::ms_pVehiclePool) {
        int vehCount = CPools::ms_pVehiclePool->m_nSize;
        for (int i = 0; i < vehCount && !hasVisual; ++i) {
            CVehicle* veh = CPools::ms_pVehiclePool->GetAt(i);
            if (!veh || veh->m_fHealth <= 0.0f) continue;
            if (veh == playerVeh) continue;

            int model = veh->m_nModelIndex;
            bool isPoliceModel = IsPoliceVehicleModel(model) || veh->IsLawEnforcementVehicle();
            if (!isPoliceModel) continue;

            bool isHeli = IsPoliceHelicopter(veh);
            if (veh->m_pDriver == player || !HasValidPoliceDriver(veh)) continue;

            CVector vehPos = veh->GetPosition();
            float maxRange = isHeli ? 250.0f : 150.0f;
            float distSq = (vehPos - pPos).MagnitudeSqr();
            if (distSq > (maxRange * maxRange)) continue;

            if (!isHeli && distSq > (12.0f * 12.0f)) {
                CVector toPlayer = (pPos - vehPos).Normalized();
                CVector vehFwd = veh->m_matrix ? veh->GetForward() : CVector(0.0f, 1.0f, 0.0f);
                if (vehFwd.Dot(toPlayer) <= 0.15f) continue;
            }

            CVector vehEyePos = vehPos + CVector(0.0f, 0.0f, isHeli ? -0.5f : 0.8f);
            if (CWorld::GetIsLineOfSightClear(vehEyePos, playerEyePos, true, false, false, true, false, false, false)) {
                hasVisual = true;
                break;
            }
        }
    }

    bool observedVisual = hasVisual;
    if (hasVisual) {
        g_visualLossTimer = 0.0f;
    } else {
        g_visualLossTimer += dt;
    }

    g_copHasVisual = hasVisual;

    if (g_config.enableHeatIdentification) {
        TrackClothesAndVehicleHeat(player, wanted, hasVisual, dt);
    }

    // 3-Stage Evasion State Machine
    if (hasVisual) {
        // Stage 1: Active Pursuit (solid stars, search circle follows player)
        if (!g_wantedActive) g_searchCenter = pPos;
        else g_searchCenter = LerpVec(g_searchCenter, pPos, dt * 8.0f);

        g_wantedStage = WANTED_STAGE_PURSUIT;
        g_wantedActive = true;
        g_escapeTimer = 0.0f;
        g_heatEscapeTimer = 0.0f;
        g_innerSearchAlpha = Lerpf(g_innerSearchAlpha, 1.0f, dt * 6.0f);
    } else {
        // Visual lost: locked at LKP
        if (g_wantedStage == WANTED_STAGE_PURSUIT || g_wantedStage == WANTED_STAGE_INACTIVE) {
            g_wantedStage = WANTED_STAGE_LKP;
            g_escapeTimer = 0.0f;
        }

        if (g_wantedStage == WANTED_STAGE_LKP) {
            // Stage 2: LKP Search Area (100% blinking stars)
            g_innerSearchAlpha = Lerpf(g_innerSearchAlpha, 1.0f, dt * 6.0f);
            float distToLkp = (pPos - g_searchCenter).Magnitude2D();
            bool isOutsideSearch = distToLkp > g_searchRadius;
            bool isHiding = g_config.allowHideInSearchArea && player->bIsDucking;
            bool isDucking = player->bIsDucking;

            if (isOutsideSearch || isHiding) {
                float timerRate = isDucking ? 1.5f : 1.0f;
                g_escapeTimer += dt * timerRate;
                if (g_escapeTimer >= lkpTargetEscapeTime) {
                    if (isHeatAreaActive && g_config.enableHeatSearchStage) {
                        g_wantedStage = WANTED_STAGE_HEAT;
                        g_heatEscapeTimer = 0.0f;
                        LogWrite("wanted: LKP search cleared, entering stage 3 heat area evasion");
                    } else {
                        pWanted->SetWantedLevel(0);
                        g_wantedStage = WANTED_STAGE_INACTIVE;
                        g_wantedActive = false;
                        g_escapeTimer = 0.0f;
                        LogWrite("wanted: player escaped search radius, wanted level cleared");
                    }
                }
            } else {
                g_escapeTimer = 0.0f;
            }
        } else if (g_wantedStage == WANTED_STAGE_HEAT) {
            // Stage 3: Heat Area Evasion (inner circle removed, ONLY wide heat area, 50% opacity solid stars)
            g_innerSearchAlpha = Lerpf(g_innerSearchAlpha, 0.0f, dt * 4.0f);
            float heatRadiusMeters = g_searchRadius * g_config.heatRadiusScale;
            float distToLkp = (pPos - g_searchCenter).Magnitude2D();
            bool isOutsideHeat = distToLkp > heatRadiusMeters;
            bool isNearHeat = !isOutsideHeat || (distToLkp <= heatRadiusMeters * 1.5f);
            bool isDucking = player->bIsDucking;

            float timerRate = 1.0f;
            if (!isOutsideHeat) {
                // Pauses completely while player remains inside zone
                timerRate = 0.0f;
            } else if (isNearHeat) {
                // Decay rate slows when player is near
                timerRate = 0.5f;
            } else {
                timerRate = 1.0f;
            }
            if (isDucking && isOutsideHeat) timerRate *= 1.5f;

            if (timerRate > 0.0f) {
                g_heatEscapeTimer += dt * timerRate;
                if (g_heatEscapeTimer >= heatTargetEscapeTime) {
                    pWanted->SetWantedLevel(0);
                    g_wantedStage = WANTED_STAGE_INACTIVE;
                    g_wantedActive = false;
                    g_escapeTimer = 0.0f;
                    g_heatEscapeTimer = 0.0f;
                    LogWrite("wanted: player escaped heat area, wanted level cleared");
                }
            }
        }
    }
}

static const char* GetVehicleClassString(CVehicle* veh, CVehicleModelInfo* minfo) {
    if (!veh || !minfo) return "VEHICLE";

    int modelId = veh->m_nModelIndex;

    // 1. Subclass / Type check
    switch (minfo->m_nVehicleType) {
        case VEHICLE_HELI:  return "HELICOPTER";
        case VEHICLE_PLANE: return "PLANE";
        case VEHICLE_BOAT:  return "BOAT";
        case VEHICLE_BMX:   return "CYCLE";
        case VEHICLE_BIKE:  return "MOTORCYCLE";
        case VEHICLE_QUAD:
        case VEHICLE_MTRUCK: return "OFF-ROAD";
        case VEHICLE_TRAIN: return "TRAIN";
        default: break;
    }

    // 2. Specific emergency / service / racing / military model overrides (handles CLASS_IGNORE)
    switch (modelId) {
        case 416: // Ambulance
        case 407: // Firetruck
        case 544: // Firetruck ladder
        case 427: // Enforcer
        case 490: // FBI Rancher
        case 528: // FBI Truck
        case 596: // Police LS
        case 597: // Police SF
        case 598: // Police LV
        case 599: // Police Ranger
        case 523: // HPV1000
            return "EMERGENCY";

        case 432: // Rhino
        case 433: // Barracks
        case 470: // Patriot
            return "MILITARY";

        case 494: // Hotring
        case 502: // Hotrina
        case 503: // Hotrinb
        case 504: // Bloodring
            return "SPORTS";

        case 441: // RC Bandit
        case 464: // RC Baron
        case 465: // RC Raider
        case 501: // RC Goblin
        case 564: // RC Tiger
        case 594: // RC Cam
            return "RC VEHICLE";

        case 539: // Vortex
            return "HOVERCRAFT";

        case 486: // Dozer
        case 530: // Forklift
        case 532: // Combine
        case 574: // Sweeper
            return "INDUSTRIAL";

        default: break;
    }

    // 3. Standard IDE Class mapping (eVehicleClass)
    switch (minfo->m_nVehicleClass) {
        case CLASS_EXECUTIVE:   return "SPORTS";
        case CLASS_RICHFAMILY:  return "SEDAN";
        case CLASS_NORMAL:      return "SEDAN";
        case CLASS_POORFAMILY:  return "COMPACT";
        case CLASS_WORKER:      return "COMMERCIAL";
        case CLASS_BIG:         return "INDUSTRIAL";
        case CLASS_TAXI:        return "SERVICE";
        case CLASS_MOPED:
        case CLASS_MOTORBIKE:   return "MOTORCYCLE";
        case CLASS_BICYCLE:     return "CYCLE";
        case CLASS_LEISUREBOAT:
        case CLASS_WORKERBOAT:  return "BOAT";
        default: break;
    }

    return "AUTOMOBILE";
}

