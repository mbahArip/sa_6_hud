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

static bool IsValidPoliceUnit(CVehicle* vehicle) {
    if (!vehicle || vehicle->m_fHealth <= 0.0f || !IsPoliceVehicleModel(vehicle->m_nModelIndex)) return false;
    return HasValidPoliceDriver(vehicle);
}

static int CountActivePoliceVehicles(const CVector& playerPosition, float radius) {
    if (!CPools::ms_pVehiclePool) return 0;
    int count = 0;
    int vehicleCount = CPools::ms_pVehiclePool->m_nSize;
    float radiusSq = radius * radius;
    for (int i = 0; i < vehicleCount; ++i) {
        CVehicle* vehicle = CPools::ms_pVehiclePool->GetAt(i);
        if (!IsValidPoliceUnit(vehicle)) continue;
        if ((vehicle->GetPosition() - playerPosition).MagnitudeSqr() <= radiusSq) ++count;
    }
    return count;
}

static bool IsDispatchPositionAllowed(const CVector& position, const CVector& playerPosition,
    const CVector& playerForward, float frontExclusionDistance, bool preferBehind) {
    CVector offset = position - playerPosition;
    float distance = offset.Magnitude2D();
    if (distance <= 0.001f) return false;

    float forwardLength = playerForward.Magnitude2D();
    CVector forward = forwardLength > 0.001f
        ? CVector(playerForward.x / forwardLength, playerForward.y / forwardLength, 0.0f)
        : CVector(0.0f, 1.0f, 0.0f);
    float forwardDot = offset.Dot(forward);
    if (forwardDot > 0.0f && distance <= frontExclusionDistance) return false;
    if (preferBehind && forwardDot > -0.15f * distance) return false;
    return true;
}

static bool IsDispatchPositionAroundCorner(const CVector& playerPosition, const CVector& position,
    float pathDistance, bool requireCorner) {
    if (!g_config.copAiHeatAreaSpawnAroundCorner || !requireCorner) return true;

    CVector lineStart = playerPosition + CVector(0.0f, 0.0f, 1.0f);
    CVector lineEnd = position + CVector(0.0f, 0.0f, 1.0f);
    bool hasVisualCover = !CWorld::GetIsLineOfSightClear(lineStart, lineEnd,
        true, false, false, true, false, false, false);
    float directDistance = (position - playerPosition).Magnitude2D();
    bool hasRouteDetour = pathDistance >= directDistance + g_config.copAiHeatAreaCornerPathExtraDistance;
    return hasVisualCover || hasRouteDetour;
}

static bool FindDispatchPosition(const CVector& playerPosition, const CVector& playerForward,
    float minDistance, float maxDistance, bool preferBehind, bool requireCorner, int attempt, CVector& position) {
    float forwardLength = playerForward.Magnitude2D();
    CVector forward = forwardLength > 0.001f
        ? CVector(playerForward.x / forwardLength, playerForward.y / forwardLength, 0.0f)
        : CVector(0.0f, 1.0f, 0.0f);
    float forwardAngle = atan2f(forward.y, forward.x);
    float angle = forwardAngle + (float)attempt * 2.399963f;
    if (preferBehind) {
        float rearSpread = ((float)((attempt * 37) % 100) / 100.0f - 0.5f) * 2.0f * 1.10f;
        angle = forwardAngle + kPi + rearSpread;
    }
    float distance = minDistance + (float)((attempt * 37) % 100) / 100.0f * (maxDistance - minDistance);
    CVector candidate(playerPosition.x + cosf(angle) * distance, playerPosition.y + sinf(angle) * distance, playerPosition.z);
    if (!IsDispatchPositionAllowed(candidate, playerPosition, forward, g_config.copAiHeatAreaFrontExclusionDistance, preferBehind)) {
        return false;
    }

    CNodeAddress nodes[4] = {};
    short nodeCount = 0;
    float pathDistance = 0.0f;
    ThePaths.DoPathSearch(0, playerPosition, CNodeAddress(), candidate, nodes, &nodeCount, 4, &pathDistance,
        maxDistance * 2.0f, nullptr, 999999.0f, false, CNodeAddress(), false, false);
    if (nodeCount > 0) {
        CPathNode* node = ThePaths.GetPathNode(nodes[nodeCount - 1]);
        if (node) {
            position = node->GetNodeCoors();
            if ((position - playerPosition).Magnitude2D() < minDistance) return false;
            return IsDispatchPositionAllowed(position, playerPosition, forward,
                g_config.copAiHeatAreaFrontExclusionDistance, preferBehind) &&
                IsDispatchPositionAroundCorner(playerPosition, position, pathDistance, requireCorner);
        }
    }
    return false;
}

static CVehicle* CreatePoliceGroundVehicle(int model, const CVector& position, const CVector& target) {
    if (model != MODEL_COPBIKE && !CModelInfo::IsCarModel(model)) return nullptr;
    if (!CPools::ms_pVehiclePool) return nullptr;

    unsigned char oldFlags = CStreaming::ms_aInfoForModel[model].m_nFlags;
    CStreaming::RequestModel(model, GAME_REQUIRED | PRIORITY_REQUEST);
    CStreaming::LoadAllRequestedModels(false);
    if (CStreaming::ms_aInfoForModel[model].m_nLoadState != LOADSTATE_LOADED) {
        if (g_config.debugMode >= 1) LogWrite("cop_ai: ground unit model %d was not loaded", model);
        return nullptr;
    }

    CVehicle* vehicle = model == MODEL_COPBIKE
        ? static_cast<CVehicle*>(new CBike(model, RANDOM_VEHICLE))
        : static_cast<CVehicle*>(new CAutomobile(model, RANDOM_VEHICLE, true));
    if (!vehicle) return nullptr;

    CVector spawnPosition = position;
    spawnPosition.z += vehicle->GetHeightAboveRoad();
    float heading = atan2f(target.y - position.y, target.x - position.x);
    vehicle->SetOrientation(0.0f, 0.0f, heading);
    vehicle->Teleport(spawnPosition);
    vehicle->m_nStatus = STATUS_PHYSICS;
    vehicle->bEngineOn = true;
    vehicle->m_eDoorLock = DOORLOCK_LOCKED_PLAYER_INSIDE;

    if (model == MODEL_COPBIKE) {
        CBike* bike = static_cast<CBike*>(vehicle);
        bike->bOnSideStand = false;
        bike->PlaceOnRoadProperly();
    } else {
        static_cast<CAutomobile*>(vehicle)->PlaceOnRoadProperly();
    }
    vehicle->UpdateRwMatrix();
    CTheScripts::ClearSpaceForMissionEntity(vehicle->GetPosition(), vehicle);

    CPed* driver = vehicle->SetUpDriver(PED_TYPE_COP, false, false);
    if (!driver || driver->m_fHealth <= 0.0f || driver->m_nPedType != PED_TYPE_COP) {
        CCarCtrl::PossiblyRemoveVehicle(vehicle);
        return nullptr;
    }
    vehicle->ProcessControl();

    if (!(oldFlags & GAME_REQUIRED)) {
        CStreaming::SetModelIsDeletable(model);
        CStreaming::SetModelTxdIsDeletable(model);
    }
    return vehicle;
}

static bool RegisterPoliceVehicleForPursuit(CVehicle* vehicle, CWanted* wanted) {
    if (!IsValidPoliceUnit(vehicle) || !wanted) return false;
    CCopPed* cop = static_cast<CCopPed*>(vehicle->m_pDriver);
    cop->bWantedByPolice = true;
    cop->bBeingChasedByPolice = true;
    return wanted->SetPursuitCop(cop);
}

static CVehicle* SpawnEmergencyVehicleAt(CPlayerPed* player, int model, float minDistance, float maxDistance) {
    if (!g_config.copAiHeatAreaSpawnEnabled || !CPools::ms_pVehiclePool) return nullptr;
    if (!player || !player->m_pPlayerData || !player->m_pPlayerData->m_pWanted) return nullptr;
    CVector playerPosition = player->GetPosition();
    CVector playerForward = player->m_pVehicle && player->m_pVehicle->m_matrix
        ? player->m_pVehicle->GetForward()
        : (player->m_matrix ? player->GetForward() : CVector(0.0f, 1.0f, 0.0f));
    bool preferBehind = CGeneral::GetRandomNumberInRange(0.0f, 1.0f) < g_config.copAiHeatAreaBehindSpawnChance;
    for (int attempt = 0; attempt < 16; ++attempt) {
        CVector position;
        bool requireCorner = attempt < 8;
        int placementAttempt = attempt % 8;
        if (!FindDispatchPosition(playerPosition, playerForward, minDistance, maxDistance,
            preferBehind, requireCorner, placementAttempt, position)) continue;
        if ((position - playerPosition).Magnitude2D() < minDistance) continue;
        CVehicle* vehicle = CreatePoliceGroundVehicle(model, position, playerPosition);
        if (!vehicle) continue;
        if (RegisterPoliceVehicleForPursuit(vehicle, player->m_pPlayerData->m_pWanted)) return vehicle;
        CCarCtrl::PossiblyRemoveVehicle(vehicle);
        if (g_config.debugMode >= 1) LogWrite("cop_ai: rejected model %d because native pursuit registration failed", model);
        return nullptr;
    }
    return nullptr;
}

static void ReleasePluginPoliceUnits() {
    if (CPools::ms_pVehiclePool) {
        for (int vehicleRef : g_pluginPoliceUnitRefs) {
            CVehicle* vehicle = CPools::ms_pVehiclePool->GetAtRef(vehicleRef);
            if (vehicle && CPools::ms_pVehiclePool->IsObjectValid(vehicle)) {
                vehicle->bStreamingDontDelete = false;
            }
        }
    }
    g_pluginPoliceUnitRefs.clear();
    g_dispatchTimer = 0.0f;
}

static void UpdateNativeWantedComposition(CPlayerPed* player, unsigned int wanted) {
    if (!player || !player->m_pPlayerData || !player->m_pPlayerData->m_pWanted) return;
    if (wanted == g_lastCopCompositionWanted) return;

    CWanted* wantedData = player->m_pPlayerData->m_pWanted;
    wantedData->m_bSwatRequired = wanted >= 4;
    wantedData->m_bFbiRequired = wanted >= 5;
    wantedData->m_bArmyRequired = wanted >= 6;
    g_lastCopCompositionWanted = wanted;
}

static void TryDispatchSupplementalPolice(CPlayerPed* player, unsigned int wanted, float dt) {
    if (!g_config.copAiEnabled || !g_config.copAiHeatAreaSpawnEnabled || !player || wanted == 0) return;

    g_dispatchTimer += dt;
    if (g_dispatchTimer < std::max(1.0f, g_config.copAiSupplementalDispatchCooldown)) return;
    g_dispatchTimer = 0.0f;

    if ((int)g_pluginPoliceUnitRefs.size() >= std::max(0, g_config.copAiMaxSupplementalUnits)) return;

    float radius = g_searchRadius * std::max(1.0f, g_config.copAiHeatAreaSpawnRadiusScale) + 50.0f;
    if (CountActivePoliceVehicles(player->GetPosition(), radius) > 0) return;

    bool useBike = player->m_pVehicle && wanted <= 3 &&
        CGeneral::GetRandomNumberInRange(0.0f, 1.0f) < 0.25f;
    int model = useBike ? MODEL_COPBIKE : MODEL_COPCARLA;
    CVehicle* vehicle = SpawnEmergencyVehicleAt(player, model,
        g_config.copAiHeatAreaSpawnMinDistance, g_config.copAiHeatAreaSpawnMaxDistance);
    if (vehicle) {
        g_pluginPoliceUnitRefs.push_back(CPools::ms_pVehiclePool->GetRef(vehicle));
    }
}

static void UpdateCopAi(CPlayerPed* player, unsigned int wanted, float dt) {
    if (!player) return;

    UpdateNativeWantedComposition(player, wanted);
    if (wanted == 0) {
        ReleasePluginPoliceUnits();
        return;
    }

    if (!g_config.copAiEnabled) return;
    TryDispatchSupplementalPolice(player, wanted, dt);
}

