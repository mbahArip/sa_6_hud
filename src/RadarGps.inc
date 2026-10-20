void Radar::UpdatePlayerData() {
    CPlayerPed* player = FindPlayerPed(0);
    if (!player || !Scene.m_pWorld || !player->m_matrix) { g_playerOk = false; return; }
    g_playerOk = true;
    if (g_warmupFrames < 10) { ++g_warmupFrames; return; }

    g_playerPos = player->GetPosition();

    CVector forward;
    if (TheCamera.m_matrix) {
        forward = TheCamera.GetForward();
    } else {
        float ro = CRadar::m_fRadarOrientation;
        forward = CVector(-sinf(ro), cosf(ro), 0.0f);
    }
    forward.z = 0.0f;
    if (forward.Magnitude2D() < 0.01f) forward = CVector(0.0f, 1.0f, 0.0f);
    g_playerFwd = forward.Normalized();
}

static bool s_peekActive = false;
static bool s_lastInfoKeyDown = false;

static void UpdateInput() {
    if (g_config.peekKey > 0 && !FrontEndMenuManager.m_bMenuActive) {
        s_peekActive = (GetAsyncKeyState(g_config.peekKey) & 0x8000) != 0;
    } else {
        s_peekActive = false;
    }

    if (g_config.infoHotkey > 0 && !FrontEndMenuManager.m_bMenuActive) {
        bool infoDown = (GetAsyncKeyState(g_config.infoHotkey) & 0x8000) != 0;
        if (infoDown && !s_lastInfoKeyDown) {
            s_locShowTimer = g_config.locationDuration;
            if (s_inVehicle) {
                s_vehNameTimer = g_config.vehicleNameDuration;
            }
        }
        s_lastInfoKeyDown = infoDown;
    } else {
        s_lastInfoKeyDown = false;
    }
}

void Radar::UpdateCamera() {
    CPlayerPed* player = FindPlayerPed(0);
    CVehicle* veh = GetStablePlayerVehicle(player);
    bool inCar = (veh != nullptr);

    s_vehStateLerp = Lerpf(s_vehStateLerp, inCar ? 1.0f : 0.0f, 0.15f);

    float elevDeg = g_config.radarTiltDeg;
    float pitchDeg = 90.0f - elevDeg;
    if (pitchDeg < 0.0f) pitchDeg = 0.0f;
    float targetTilt = pitchDeg * kPi / 180.0f;

    float targetDist = g_config.radarZoom;
    if (inCar) {
        float speedKmh = veh->m_vecMoveSpeed.Magnitude() * 180.0f;
        float speedDelta = speedKmh - g_config.speedZoomThreshold;
        if (speedDelta > 0.0f) {
            float speedRange = 150.0f - g_config.speedZoomThreshold;
            if (speedRange < 10.0f) speedRange = 10.0f;
            float speedRatio = Clampf(speedDelta / speedRange, 0.0f, 1.0f);
            targetDist = Lerpf(g_config.radarZoom, g_config.radarMaxZoom, speedRatio);
        }
    }

    bool isClosedSpace = false;
    if (CWeather::InTunnelness > 0.05f) isClosedSpace = true;
    if (CGame::currArea > 0) isClosedSpace = true;
    if (player && player->m_nAreaCode > 0) isClosedSpace = true;
    if (g_playerPos.z < -2.0f) isClosedSpace = true;

    if (s_peekActive) {
        targetDist *= g_config.peekZoomMult;
    } else if (isClosedSpace) {
        targetDist *= g_config.closedSpaceZoomMultiplier;
    } else if (g_config.enableAltitudeZoom) {
        float minH = g_config.altitudeMinHeight;
        float maxH = g_config.altitudeMaxHeight;
        if (maxH - minH > 5.0f) {
            float altRatio = Clampf((g_playerPos.z - minH) / (maxH - minH), 0.0f, 1.0f);
            float altMult = Lerpf(1.0f, g_config.altitudeMaxZoomMult, altRatio);
            targetDist *= altMult;
        }
    }

    if (g_wantedActive && g_searchRadius > 1.0f) {
        float distToSearchCenter = (g_playerPos - g_searchCenter).Magnitude2D();
        if (distToSearchCenter <= g_searchRadius) {
            targetDist *= g_config.extWantedSearchAreaZoom;
        }
    }

    float targetVCenter = 0.70f;

    float spd = g_config.transitionSpeed;
    if (spd < 0.001f) spd = 0.025f;

    g_curTiltRad = Lerpf(g_curTiltRad, targetTilt, spd);
    g_curDist    = Lerpf(g_curDist,    targetDist, spd);
    g_curVCenter = Lerpf(g_curVCenter, targetVCenter, spd);

    float targetPuckTilt = (g_config.tiltPlayerBlip) ? 1.0f : 0.0f;
    g_curPuckTilt = Lerpf(g_curPuckTilt, targetPuckTilt, spd);

    float sinT = sinf(g_curTiltRad), cosT = cosf(g_curTiltRad);
    CVector viewDir = g_playerFwd * sinT - CVector(0.0f, 0.0f, 1.0f) * cosT;

    CVector camUp = g_playerFwd - viewDir * g_playerFwd.Dot(viewDir);
    if (camUp.Magnitude() < 0.0001f) camUp = CVector(0.0f, 1.0f, 0.0f);
    camUp = camUp.Normalized();
    CVector camRight = viewDir.Cross(camUp).Normalized();

    g_camRight = camRight;
    g_camUp = camUp;
}

static int GetTargetBlipIndex() {
    int handle = FrontEndMenuManager.m_nTargetBlipIndex;
    if (handle <= 0) return -1;
    int idx = handle & 0xFFFF;
    if (idx >= 0 && idx < MAX_RADAR_TRACES && CRadar::ms_RadarTrace[idx].m_bInUse) {
        return idx;
    }
    return -1;
}

static bool HasActiveDestination(CVector& outPos) {
    int targetIdx = GetTargetBlipIndex();
    if (targetIdx >= 0 && CRadar::ms_RadarTrace) {
        outPos = CRadar::ms_RadarTrace[targetIdx].m_vecPos;
        return true;
    }
    if (g_pinActive) {
        outPos = g_pinPos;
        return true;
    }
    if (CRadar::ms_RadarTrace) {
        for (unsigned int i = 0; i < MAX_RADAR_TRACES; ++i) {
            tRadarTrace& trace = CRadar::ms_RadarTrace[i];
            if (!trace.m_bInUse || trace.m_nBlipDisplay == 0) continue;
            if (trace.m_nRadarSprite == RADAR_SPRITE_CENTRE || trace.m_nRadarSprite == RADAR_SPRITE_MAP_HERE) continue;
            if (trace.m_nRadarSprite == RADAR_SPRITE_WAYPOINT ||
                (trace.m_nRadarSprite == RADAR_SPRITE_NONE && (trace.m_nColour == BLIP_COLOUR_YELLOW || trace.m_nColour == BLIP_COLOUR_RED))) {
                if (trace.m_nBlipType == BLIP_CAR) {
                    CVehicle* veh = CPools::GetVehicle(trace.m_nEntityHandle);
                    if (veh) { outPos = veh->GetPosition(); return true; }
                } else if (trace.m_nBlipType == BLIP_CHAR) {
                    CPed* ped = CPools::GetPed(trace.m_nEntityHandle);
                    if (ped) { outPos = ped->GetPosition(); return true; }
                } else if (trace.m_nBlipType == BLIP_OBJECT) {
                    CObject* obj = CPools::GetObject(trace.m_nEntityHandle);
                    if (obj) { outPos = obj->GetPosition(); return true; }
                } else {
                    outPos = trace.m_vecPos;
                    return true;
                }
            }
        }
    }
    return false;
}

void Radar::UpdateGps() {
    int targetIdx = GetTargetBlipIndex();
    bool active = (targetIdx >= 0 || g_pinActive);

    if (!active) {
        g_pathCount = 0;
        g_gpsDistance = 0.0f;
        return;
    }

    CVector targetPos = (targetIdx >= 0) ? CRadar::ms_RadarTrace[targetIdx].m_vecPos : g_pinPos;

    if (g_config.waypointReachRadius > 0.1f) {
        float distToTarget = (g_playerPos - targetPos).Magnitude2D();
        if (distToTarget <= g_config.waypointReachRadius) {
            if (targetIdx >= 0) {
                CRadar::ClearBlip(FrontEndMenuManager.m_nTargetBlipIndex);
                FrontEndMenuManager.m_nTargetBlipIndex = 0;
            }
            if (g_pinActive) {
                ClearPin();
            }
            g_pathCount = 0;
            g_gpsDistance = 0.0f;
            return;
        }
    }

    CPlayerPed* player = FindPlayerPed(0);
    CVehicle* veh = player ? GetStablePlayerVehicle(player) : nullptr;
    bool isAirVehicle = veh && (veh->m_nVehicleSubClass == VEHICLE_PLANE || 
                               veh->m_nVehicleSubClass == VEHICLE_HELI);
    bool isBoatVehicle = veh && (veh->m_nVehicleSubClass == VEHICLE_BOAT);

    if (isAirVehicle) {
        g_pathCount = 2;
        g_pathNodes[0] = g_playerPos;
        g_pathNodes[1] = targetPos;
        g_gpsDistance = (g_playerPos - targetPos).Magnitude2D();
        return;
    }

    static CVector s_prevTargetPos(0, 0, 0);
    static bool    s_prevHadTarget = false;
    bool targetChanged = (!s_prevHadTarget) || ((targetPos - s_prevTargetPos).MagnitudeSqr() > 2.0f * 2.0f);
    s_prevHadTarget = true;
    s_prevTargetPos = targetPos;

    float playerMoveSq = (g_playerPos - g_lastSearchPos).MagnitudeSqr2D();
    bool needsRecalc = targetChanged || (++g_pathFrameTicker >= 45) || (playerMoveSq > 15.0f * 15.0f);

    if (needsRecalc) {
        g_pathFrameTicker = 0;
        g_lastSearchPos = g_playerPos;

        short numNodes = 0;
        float totalDist = 0.0f;
        static CNodeAddress resultNodes[kMaxPathNodes];
        bool walking = false;

        // 1. Car Path (pathType = 0)
        ThePaths.DoPathSearch(
            0,
            g_playerPos,
            CNodeAddress(),
            targetPos,
            resultNodes,
            &numNodes,
            kMaxPathNodes,
            &totalDist,
            999999.0f,
            nullptr,
            999999.0f,
            false,
            CNodeAddress(),
            false,
            isBoatVehicle
        );

        // 2. Fallback: Pedestrian Path (pathType = 1) for alleys, off-road, parks
        if (numNodes == 0) {
            ThePaths.DoPathSearch(
                1,
                g_playerPos,
                CNodeAddress(),
                targetPos,
                resultNodes,
                &numNodes,
                kMaxPathNodes,
                &totalDist,
                999999.0f,
                nullptr,
                999999.0f,
                false,
                CNodeAddress(),
                false,
                false
            );
            if (numNodes > 0) {
                walking = true;
            }
        }

        if (numNodes > 0 && numNodes <= kMaxPathNodes) {
            g_pathCount = 0;
            if (isBoatVehicle) {
                g_pathNodes[g_pathCount++] = g_playerPos;
            }
            for (short i = 0; i < numNodes; ++i) {
                CPathNode* node = ThePaths.GetPathNode(resultNodes[i]);
                if (node) {
                    g_pathNodes[g_pathCount++] = node->GetNodeCoors();
                }
            }
            g_isWalkingPath = walking;
            g_gpsDistance = totalDist + (isBoatVehicle && g_pathCount > 1 ? (g_playerPos - g_pathNodes[1]).Magnitude2D() : 0.0f);
        } else {
            // 3. Fallback: Straight line [PlayerPos -> TargetPos]
            g_pathCount = 2;
            g_pathNodes[0] = g_playerPos;
            g_pathNodes[1] = targetPos;
            g_isWalkingPath = true;
            g_gpsDistance = (g_playerPos - targetPos).Magnitude2D();
        }
    }

    // Per-Frame Segment Projection & Dynamic Trimming at 60 FPS
    // Project player (X, Y) onto active segment (N_i -> N_{i+1}); drop node i once progress t >= 0.95
    while (g_pathCount >= 2) {
        CVector pA = g_pathNodes[0];
        CVector pB = g_pathNodes[1];
        CVector ab = pB - pA;
        float lenSq = ab.x * ab.x + ab.y * ab.y;
        if (lenSq < 0.25f) {
            for (int i = 0; i < g_pathCount - 1; ++i) g_pathNodes[i] = g_pathNodes[i + 1];
            g_pathCount--;
            continue;
        }

        CVector ap = g_playerPos - pA;
        float t = (ap.x * ab.x + ap.y * ab.y) / lenSq;
        if (t >= 0.95f) {
            for (int i = 0; i < g_pathCount - 1; ++i) g_pathNodes[i] = g_pathNodes[i + 1];
            g_pathCount--;
        } else {
            break;
        }
    }
}

