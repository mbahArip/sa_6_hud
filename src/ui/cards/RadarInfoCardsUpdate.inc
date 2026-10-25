void Radar::UpdateInfoCards() {
    if (!g_config.showInfoCards) return;

    float dt = CTimer::ms_fTimeStep * 0.02f;
    if (dt > 0.1f) dt = 0.1f;

    CPlayerPed* player = FindPlayerPed(0);
    if (!player) return;

    float trSpd = g_config.cardTransitionSpeed;
    if (trSpd < 0.01f) trSpd = 0.08f;

    // -------------------------------------------------------------------------
    // 1. GPS Turn-by-Turn Navigator
    // -------------------------------------------------------------------------
    CVehicle* pVeh = GetStablePlayerVehicle(player);
    bool isAir = pVeh && (pVeh->m_nVehicleSubClass == VEHICLE_PLANE ||
                          pVeh->m_nVehicleSubClass == VEHICLE_HELI ||
                          pVeh->m_nVehicleSubClass == VEHICLE_FPLANE ||
                          pVeh->m_nVehicleSubClass == VEHICLE_FHELI);
    bool isBoat = pVeh && (pVeh->m_nVehicleSubClass == VEHICLE_BOAT);
    bool isTrain = pVeh && (pVeh->m_nVehicleSubClass == VEHICLE_TRAIN);
    bool isLandVehicle = (pVeh != nullptr && !isAir && !isBoat && !isTrain);

    int targetIdx = GetTargetBlipIndex();
    bool gpsActive = (targetIdx >= 0 || g_pinActive);
    bool shouldShowNav = false;

    if (isLandVehicle && g_config.enableGps && g_config.enableGpsNavigator) {
        if (gpsActive && g_pathCount >= 2) {
            // Project player onto nearest path segment to eliminate discrete node jitter
            int bestSeg = 0;
            float bestSegT = 0.0f;
            float minProjDist = 1e9f;

            int maxCheckNodes = std::min(g_pathCount - 1, 12);
            for (int i = 0; i < maxCheckNodes; ++i) {
                CVector pA = g_pathNodes[i];
                CVector pB = g_pathNodes[i + 1];
                CVector ab = pB - pA;
                float segLen2 = ab.x * ab.x + ab.y * ab.y;
                if (segLen2 < 0.25f) continue;

                CVector ap = g_playerPos - pA;
                float t = (ap.x * ab.x + ap.y * ab.y) / segLen2;
                float tClamped = Clampf(t, 0.0f, 1.0f);
                CVector proj = pA + ab * tClamped;
                float d = (g_playerPos - proj).Magnitude2D();

                if (d < minProjDist) {
                    minProjDist = d;
                    bestSeg = i;
                    bestSegT = tClamped;
                }
            }

            // Calculate exact remaining distance along current segment
            CVector segAB = g_pathNodes[bestSeg + 1] - g_pathNodes[bestSeg];
            float curSegRemaining = (1.0f - bestSegT) * segAB.Magnitude2D();
            float accDist = curSegRemaining;

            GpsManeuver detectedManeuver = GpsManeuver::Straight;
            float turnDist = 0.0f;
            bool foundTurn = false;

            for (int i = bestSeg + 1; i < g_pathCount - 1 && accDist < 350.0f; ++i) {
                CVector seg1 = g_pathNodes[i] - g_pathNodes[i - 1];
                CVector seg2 = g_pathNodes[i + 1] - g_pathNodes[i];
                float segLen = seg2.Magnitude2D();
                if (segLen < 1.0f) continue;

                float cross = seg1.x * seg2.y - seg1.y * seg2.x;
                float dot   = seg1.x * seg2.x + seg1.y * seg2.y;
                float angleDeg = atan2f(cross, dot) * (180.0f / kPi);

                if (fabsf(angleDeg) >= 20.0f) {
                    foundTurn = true;
                    turnDist = accDist;

                    bool isUTurn = (fabsf(angleDeg) >= 110.0f);
                    float cumAngle = angleDeg;
                    if (!isUTurn && (i + 1 < g_pathCount - 1)) {
                        CVector nextSeg1 = g_pathNodes[i + 1] - g_pathNodes[i];
                        CVector nextSeg2 = g_pathNodes[i + 2] - g_pathNodes[i + 1];
                        float nextLen = nextSeg2.Magnitude2D();
                        if (nextLen >= 1.0f && (seg2.Magnitude2D() < 40.0f)) {
                            float nCross = nextSeg1.x * nextSeg2.y - nextSeg1.y * nextSeg2.x;
                            float nDot   = nextSeg1.x * nextSeg2.x + nextSeg1.y * nextSeg2.y;
                            float nextAngle = atan2f(nCross, nDot) * (180.0f / kPi);
                            if ((angleDeg * nextAngle > 0.0f) && (fabsf(angleDeg + nextAngle) >= 115.0f)) {
                                isUTurn = true;
                                cumAngle = angleDeg + nextAngle;
                            }
                        }
                    }

                    if (isUTurn) {
                        detectedManeuver = (cumAngle > 0.0f) ? GpsManeuver::UTurnLeft : GpsManeuver::UTurnRight;
                    } else if (angleDeg < -50.0f) { // Negative cross product = Right in GTA SA
                        detectedManeuver = GpsManeuver::TurnRight;
                    } else if (angleDeg > 50.0f) {  // Positive cross product = Left in GTA SA
                        detectedManeuver = GpsManeuver::TurnLeft;
                    } else if (angleDeg < -20.0f) {
                        detectedManeuver = GpsManeuver::SlightRight;
                    } else {
                        detectedManeuver = GpsManeuver::SlightLeft;
                    }
                    break;
                }
                accDist += segLen;
            }

            // Only show turn navigator when within 65m of an upcoming maneuver (Left, Right, Bear, U-Turn)
            if (foundTurn && turnDist <= 65.0f) {
                shouldShowNav = true;
                s_gpsNavState.maneuver = detectedManeuver;
                s_gpsNavState.turnDistance = turnDist;

                switch (detectedManeuver) {
                    case GpsManeuver::UTurnLeft:   snprintf(s_gpsNavState.maneuverText, sizeof(s_gpsNavState.maneuverText), "%s", I18n(I18N_GPS_UTURN_LEFT)); break;
                    case GpsManeuver::UTurnRight:  snprintf(s_gpsNavState.maneuverText, sizeof(s_gpsNavState.maneuverText), "%s", I18n(I18N_GPS_UTURN_RIGHT)); break;
                    case GpsManeuver::TurnRight:   snprintf(s_gpsNavState.maneuverText, sizeof(s_gpsNavState.maneuverText), "%s", I18n(I18N_GPS_TURN_RIGHT)); break;
                    case GpsManeuver::TurnLeft:    snprintf(s_gpsNavState.maneuverText, sizeof(s_gpsNavState.maneuverText), "%s", I18n(I18N_GPS_TURN_LEFT)); break;
                    case GpsManeuver::SlightRight: snprintf(s_gpsNavState.maneuverText, sizeof(s_gpsNavState.maneuverText), "%s", I18n(I18N_GPS_BEAR_RIGHT)); break;
                    case GpsManeuver::SlightLeft:  snprintf(s_gpsNavState.maneuverText, sizeof(s_gpsNavState.maneuverText), "%s", I18n(I18N_GPS_BEAR_LEFT)); break;
                    default:                       snprintf(s_gpsNavState.maneuverText, sizeof(s_gpsNavState.maneuverText), "%s", I18n(I18N_GPS_CONTINUE)); break;
                }

                if (turnDist <= 12.0f) {
                    snprintf(s_gpsNavState.distanceText, sizeof(s_gpsNavState.distanceText), "%s",
                             I18n(g_config.distanceUnit == 0 ? I18N_GPS_NOW_M : I18N_GPS_NOW_FT));
                } else if (g_config.distanceUnit == 0) {
                    if (turnDist >= 1000.0f) {
                        snprintf(s_gpsNavState.distanceText, sizeof(s_gpsNavState.distanceText), I18n(I18N_GPS_IN_KM), turnDist / 1000.0f);
                    } else {
                        snprintf(s_gpsNavState.distanceText, sizeof(s_gpsNavState.distanceText), I18n(I18N_GPS_IN_M), turnDist);
                    }
                } else {
                    float miles = (turnDist * 3.28084f) / 5280.0f;
                    if (miles >= 0.10f) {
                        snprintf(s_gpsNavState.distanceText, sizeof(s_gpsNavState.distanceText), I18n(I18N_GPS_IN_MI), miles);
                    } else {
                        float feet = turnDist * 3.28084f;
                        snprintf(s_gpsNavState.distanceText, sizeof(s_gpsNavState.distanceText), I18n(I18N_GPS_IN_FT), feet);
                    }
                }
            } else {
                // Beyond trigger range or straightaway: fade out
                shouldShowNav = false;
            }
        }
    }

    float targetNavAlpha = shouldShowNav ? 1.0f : 0.0f;
    s_navAlpha = Lerpf(s_navAlpha, targetNavAlpha, trSpd);

    // -------------------------------------------------------------------------
    // 2. Location / Zone Info
    // -------------------------------------------------------------------------
    int curArea = (int)player->m_nAreaCode;
    bool areaChanged = (curArea != s_lastAreaCode);
    s_lastAreaCode = curArea;
    s_isInterior = (player->m_nAreaCode != 0 || CGame::currArea != 0);

    CZone* zone = CTheZones::FindSmallestZoneForPosition(g_playerPos, false);
    const char* zoneKey = zone ? zone->m_szTextKey : "";
    if (zoneKey[0] == '\0') zoneKey = zone ? zone->m_szLabel : "SAN_ANDREAS";

    if (strcmp(zoneKey, s_lastZoneKey) != 0 || (areaChanged && s_isInterior)) {
        strncpy(s_lastZoneKey, zoneKey, sizeof(s_lastZoneKey) - 1);
        char newName[64] = {};
        const char* gxtText = TheText.Get(zoneKey);
        if (gxtText && gxtText[0] != '\0' && gxtText[0] != '?') {
            strncpy(newName, gxtText, sizeof(newName) - 1);
        } else if (zone && zone->m_szLabel[0] != '\0') {
            strncpy(newName, zone->m_szLabel, sizeof(newName) - 1);
        } else {
            strncpy(newName, "San Andreas", sizeof(newName) - 1);
        }

        if (s_curZoneName[0] != '\0' && strcmp(s_curZoneName, newName) != 0) {
            strncpy(s_prevZoneName, s_curZoneName, sizeof(s_prevZoneName) - 1);
            s_zoneFadeAlpha = 1.0f; // Begin cross-fade
        }
        strncpy(s_curZoneName, newName, sizeof(s_curZoneName) - 1);
        s_locShowTimer = g_config.locationDuration;
    }

    if (s_zoneFadeAlpha > 0.0f) {
        s_zoneFadeAlpha -= dt * 4.0f; // ~250ms cross-fade duration
        if (s_zoneFadeAlpha < 0.0f) s_zoneFadeAlpha = 0.0f;
    }

    if (g_hudKeyHeld || s_hudKeyHoldTimer > 0.0f) {
        s_locShowTimer = g_config.locationDuration;
    } else if (s_locShowTimer > 0.0f) {
        s_locShowTimer -= dt;
        if (s_locShowTimer < 0.0f) s_locShowTimer = 0.0f;
    }

    float targetLocAlpha = (g_config.usePlaceInfo && (g_config.alwaysShowPlaceInfo || s_locShowTimer > 0.0f)) ? 1.0f : 0.0f;
    s_locAlpha = Lerpf(s_locAlpha, targetLocAlpha, trSpd);

    // -------------------------------------------------------------------------
    // 3. Vehicle Info
    // -------------------------------------------------------------------------
    CVehicle* veh = GetStablePlayerVehicle(player);
    s_inVehicle = (veh != nullptr);

    if (s_inVehicle && veh != s_pLastVehicle) {
        s_pLastVehicle = veh;
        s_vehNameTimer = g_config.vehicleNameDuration;

        CVehicleModelInfo* minfo = nullptr;
        if (veh->m_nModelIndex >= 400 && veh->m_nModelIndex <= 611) {
            CBaseModelInfo* baseInfo = CModelInfo::GetModelInfo(veh->m_nModelIndex);
            if (baseInfo && baseInfo->GetModelType() == MODEL_INFO_VEHICLE) {
                minfo = static_cast<CVehicleModelInfo*>(baseInfo);
            }
        }

        if (minfo && minfo->m_szGameName[0] != '\0') {
            char gxtKey[9] = {};
            memcpy(gxtKey, minfo->m_szGameName, sizeof(minfo->m_szGameName));
            gxtKey[8] = '\0';

            const char* gxt = TheText.Get(gxtKey);
            if (gxt && gxt[0] != '\0' && gxt[0] != '?') {
                strncpy_s(s_vehName, sizeof(s_vehName), gxt, _TRUNCATE);
            } else {
                strncpy_s(s_vehName, sizeof(s_vehName), gxtKey, _TRUNCATE);
            }
        } else {
            strncpy_s(s_vehName, sizeof(s_vehName), I18n(I18N_VEHICLE_FALLBACK), _TRUNCATE);
        }

        const char* clsName = GetVehicleClassString(veh, minfo);
        strncpy_s(s_vehClass, sizeof(s_vehClass), clsName, _TRUNCATE);
    } else if (!s_inVehicle) {
        s_pLastVehicle = nullptr;
    }

    if (s_inVehicle && veh) {
        if (g_hudKeyHeld || s_hudKeyHoldTimer > 0.0f) {
            s_vehNameTimer = g_config.vehicleNameDuration;
        } else if (s_vehNameTimer > 0.0f) {
            s_vehNameTimer -= dt;
            if (s_vehNameTimer < 0.0f) s_vehNameTimer = 0.0f;
        }
    } else {
        s_vehNameTimer = 0.0f;
    }

    float targetVehCardAlpha = (g_config.useVehicleInfo && s_inVehicle) ? 1.0f : 0.0f;
    float targetVehNameAlpha = (g_config.useVehicleInfo && s_inVehicle && (g_config.alwaysShowVehInfo || s_vehNameTimer > 0.0f)) ? 1.0f : 0.0f;

    s_vehCardAlpha = Lerpf(s_vehCardAlpha, targetVehCardAlpha, trSpd);
    s_vehNameAlpha = Lerpf(s_vehNameAlpha, targetVehNameAlpha, trSpd);

    // -------------------------------------------------------------------------
    // 4. Dynamic 4-Gauge Telemetry Engine
    // -------------------------------------------------------------------------
    // G0: Waypoint (Holds for 5 seconds at 0.0 upon reaching destination)
    static bool s_wasGpsActive = false;
    bool waypointActive = false;
    if (g_config.showGaugeWaypoint) {
        if (gpsActive) {
            CVector destPos = (targetIdx >= 0) ? CRadar::ms_RadarTrace[targetIdx].m_vecPos : g_pinPos;
            float meters = (g_gpsDistance > 0.0f) ? g_gpsDistance : (g_playerPos - destPos).Magnitude2D();
            if (s_initialWaypointDist < 1.0f) s_initialWaypointDist = std::max(meters, 50.0f);

            if (meters <= g_config.waypointReachRadius || meters < 10.0f) {
                s_gaugeSlots[GAUGE_WAYPOINT].targetValue = 0.0f;
                if (s_waypointHoldTimer <= 0.0f) s_waypointHoldTimer = 5.0f;
            } else {
                s_gaugeSlots[GAUGE_WAYPOINT].targetValue = Clampf(meters / s_initialWaypointDist, 0.0f, 1.0f);
            }
            waypointActive = true;
        } else if (s_wasGpsActive && s_waypointHoldTimer <= 0.0f) {
            s_waypointHoldTimer = 5.0f;
            s_gaugeSlots[GAUGE_WAYPOINT].targetValue = 0.0f;
            waypointActive = true;
        } else if (s_waypointHoldTimer > 0.0f) {
            s_waypointHoldTimer -= dt;
            s_gaugeSlots[GAUGE_WAYPOINT].targetValue = 0.0f;
            waypointActive = (s_waypointHoldTimer > 0.0f);
        } else {
            s_initialWaypointDist = 0.0f;
        }
    }
    s_wasGpsActive = gpsActive;
    s_gaugeSlots[GAUGE_WAYPOINT].active = waypointActive;
    s_gaugeSlots[GAUGE_WAYPOINT].color = g_config.gaugeWaypointColor;

    // G1: Engine Health (100% pristine condition; 0% engine catching fire <= 250.0f)
    bool healthActive = g_config.showGaugeHealth && s_inVehicle;
    if (healthActive && veh) {
        float rawHp = veh->m_fHealth;
        float normHp = Clampf((rawHp - 250.0f) / 750.0f, 0.0f, 1.0f);
        s_gaugeSlots[GAUGE_HEALTH].targetValue = normHp;

        // Color gradient: Green -> Yellow -> Red
        if (normHp > 0.50f) {
            float t = (normHp - 0.50f) / 0.50f;
            s_gaugeSlots[GAUGE_HEALTH].color = CRGBA((unsigned char)(245 + (65 - 245) * t), (unsigned char)(205 + (225 - 205) * t), (unsigned char)(45 + (130 - 45) * t), 255);
        } else {
            float t = normHp / 0.50f;
            s_gaugeSlots[GAUGE_HEALTH].color = CRGBA(245, (unsigned char)(55 + (205 - 55) * t), (unsigned char)(65 + (45 - 65) * t), 255);
        }
    }
    s_gaugeSlots[GAUGE_HEALTH].active = healthActive;

    // G2: Fuel Gauge
    // Cosmetic at 100% when USE_VEHICLE_FUEL = 0. When 1, randomizes upon entry
    // within [VEHICLE_FUEL_RNG_MIN, VEHICLE_FUEL_RNG_MAX] and drains while driving.
    bool needsFuel = IsFuelVehicle(veh);

    bool fuelActive = g_config.showGaugeFuel && s_inVehicle && needsFuel;
    if (fuelActive && veh) {
        VehicleFuelExt& ext = s_vehicleFuelExt.Get(veh);
        if (!ext.initialized) {
            ext.initialized = true;
            if (g_config.useVehicleFuel) {
                float r = (float)rand() / (float)RAND_MAX;
                ext.fuelLevel = g_config.vehicleFuelRngMin + r * (g_config.vehicleFuelRngMax - g_config.vehicleFuelRngMin);
            } else {
                ext.fuelLevel = 1.0f;
            }
        }

        if (veh != s_pTrackedFuelVehicle) {
            s_pTrackedFuelVehicle = veh;
            s_lastVehPos = g_playerPos;
            s_vehFuel = ext.fuelLevel;
        } else {
            float dist = (g_playerPos - s_lastVehPos).Magnitude2D();
            s_lastVehPos = g_playerPos;

            if (g_config.useVehicleFuel) {
                bool onMission = CTheScripts::IsPlayerOnAMission();
                if (!onMission || g_config.useVehicleFuelOnMission) {
                    s_vehFuel = std::max(0.0f, s_vehFuel - (dist / 150.0f) * 0.01f * g_config.vehicleFuelConsumption);
                }
            } else {
                s_vehFuel = 1.0f;
            }
            ext.fuelLevel = s_vehFuel;
        }

        // Engine control when out of fuel: cut throttle, keep brakes/steering intact
        if (g_config.useVehicleFuel && s_vehFuel <= 0.001f) {
            veh->bEngineOn = false;
            veh->m_fGasPedal = 0.0f;
        }

        s_gaugeSlots[GAUGE_FUEL].targetValue = s_vehFuel;
        // Color gradient: Green -> Yellow -> Red
        if (s_vehFuel > 0.50f) {
            float t = (s_vehFuel - 0.50f) / 0.50f;
            s_gaugeSlots[GAUGE_FUEL].color = CRGBA((unsigned char)(245 + (65 - 245) * t), (unsigned char)(205 + (225 - 205) * t), (unsigned char)(45 + (130 - 45) * t), 255);
        } else {
            float t = s_vehFuel / 0.50f;
            s_gaugeSlots[GAUGE_FUEL].color = CRGBA(245, (unsigned char)(55 + (205 - 55) * t), (unsigned char)(65 + (45 - 65) * t), 255);
        }
    } else if (!s_inVehicle) {
        s_pTrackedFuelVehicle = nullptr;
    }
    s_gaugeSlots[GAUGE_FUEL].active = fuelActive;

    // G3: Nitro (Only when nitro upgrade is installed!)
    bool hasNitro = false;
    float nitroVal = 0.0f;
    s_curNitroBoosts = 0;
    if (s_inVehicle && veh) {
        int nitroUpgrade = veh->GetUpgrade(UPGRADE_NITRO);
        if (nitroUpgrade > 0 || veh->m_nNitroBoosts > 0) {
            hasNitro = true;
            s_curNitroBoosts = veh->m_nNitroBoosts;
            CAutomobile* aVeh = (veh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE) ? (CAutomobile*)veh : nullptr;
            float currentNitro = aVeh ? aVeh->m_fNitroValue : 0.0f;

            if (g_config.nitroGaugeType == 1 || g_config.enableControllableNitro) {
                // Controlled continuous mode (0.0 -> 1.0)
                nitroVal = Clampf(currentNitro, 0.0f, 1.0f);
            } else {
                // Vanilla mode:
                if (currentNitro > 0.001f) {
                    // Actively burning bottle
                    nitroVal = Clampf(currentNitro, 0.0f, 1.0f);
                } else if (veh->m_nNitroBoosts > 0) {
                    // Ready full bottle
                    nitroVal = 1.0f;
                } else {
                    // 0 bottles remaining
                    nitroVal = 0.0f;
                }
            }
        }
    }
    bool nitroActive = g_config.showGaugeNitro && s_inVehicle && hasNitro;
    if (nitroActive) {
        s_gaugeSlots[GAUGE_NITRO].targetValue = Clampf(nitroVal, 0.0f, 1.0f);
        if (nitroVal <= 0.001f) {
            s_gaugeSlots[GAUGE_NITRO].color = CRGBA(110, 120, 135, 255);
        } else {
            s_gaugeSlots[GAUGE_NITRO].color = g_config.gaugeNitroColor;
        }
    }
    s_gaugeSlots[GAUGE_NITRO].active = nitroActive;

    // Layout Dynamic Centering & Horizontal Slide & Smooth Value Lerp
    bool isPreview = Menu::IsLayoutPreview();
    if (isPreview) {
        s_gaugeSlots[GAUGE_HEALTH].active = true;
        s_gaugeSlots[GAUGE_HEALTH].targetValue = 1.0f;
        s_gaugeSlots[GAUGE_HEALTH].color = CRGBA(65, 225, 130, 255);
        s_gaugeSlots[GAUGE_FUEL].active = true;
        s_gaugeSlots[GAUGE_FUEL].targetValue = 0.85f;
        s_gaugeSlots[GAUGE_FUEL].color = g_config.gaugeFuelColor;
        s_gaugeSlots[GAUGE_NITRO].active = true;
        s_gaugeSlots[GAUGE_NITRO].targetValue = 1.0f;
        s_gaugeSlots[GAUGE_NITRO].color = g_config.gaugeNitroColor;
    }

    int activeCount = 0;
    for (int i = 0; i < GAUGE_COUNT; ++i) {
        if (s_gaugeSlots[i].active) activeCount++;
    }

    CRect vehRc = g_moduleRects[(int)HudModuleId::VehicleInfo];
    float cardCenterX = (vehRc.right + vehRc.left) * 0.5f;

    float sH = ScreenH();
    float spacing = (26.0f * (sH / 720.0f)) * g_config.gaugeSpacing * g_config.radarSizeScale;

    int slotIdx = 0;
    for (int i = 0; i < GAUGE_COUNT; ++i) {
        if (s_gaugeSlots[i].active) {
            float slotOffset = (-(float)(activeCount - 1) * 0.5f + (float)slotIdx) * spacing;
            s_gaugeSlots[i].targetX = cardCenterX + slotOffset;
            s_gaugeSlots[i].targetAlpha = 1.0f;
            slotIdx++;
        } else {
            s_gaugeSlots[i].targetAlpha = 0.0f;
        }

        if (!s_gaugeSlots[i].active || s_gaugeSlots[i].curAlpha < 0.01f || s_gaugeSlots[i].curX < 1.0f) {
            s_gaugeSlots[i].curX = s_gaugeSlots[i].targetX;
            s_gaugeSlots[i].curValue = s_gaugeSlots[i].targetValue;
        } else {
            s_gaugeSlots[i].curX = Lerpf(s_gaugeSlots[i].curX, s_gaugeSlots[i].targetX, 0.18f);
            s_gaugeSlots[i].curValue = Lerpf(s_gaugeSlots[i].curValue, s_gaugeSlots[i].targetValue, 0.12f);
        }
        s_gaugeSlots[i].curAlpha = Lerpf(s_gaugeSlots[i].curAlpha, s_gaugeSlots[i].targetAlpha, 0.15f);
    }
}

// -----------------------------------------------------------------------------
// Gas pump services
// -----------------------------------------------------------------------------
// Replaces the old free auto-refill. Standing still next to a pump shows a
// world-anchored "(KEY) Services" prompt; holding the key opens a two-row list
// that refuels or repairs for money. Prices are never shown to the player.

// HESOYAM also tops up the tank when CHEAT_REFILL_FUEL is on. Tailing the vanilla
// cheat keeps money/armour/health behaving exactly as the game ships it.
static void __cdecl Hooked_MoneyArmourHealthCheat() {
    CCheat::MoneyArmourHealthCheat();
    if (!g_config.cheatRefillFuel) return;

    CPlayerPed* player = FindPlayerPed(0);
    if (!player || !player->bInVehicle || !player->m_pVehicle) return;
    CVehicle* veh = player->m_pVehicle;
    if (!IsFuelVehicle(veh)) return;

    s_vehicleFuelExt.Get(veh).fuelLevel = 1.0f;
}

// Renders a key code as the short name shown inside the "(KEY)" badge.
static void KeyBadgeName(int vk, int pad, char* out, size_t outSize) {
    if (pad != PADBTN_NONE) {
        switch (pad) {
            case PADBTN_A:          snprintf(out, outSize, "A");      return;
            case PADBTN_B:          snprintf(out, outSize, "B");      return;
            case PADBTN_X:          snprintf(out, outSize, "X");      return;
            case PADBTN_Y:          snprintf(out, outSize, "Y");      return;
            case PADBTN_LB:         snprintf(out, outSize, "LB");     return;
            case PADBTN_LT:         snprintf(out, outSize, "LT");     return;
            case PADBTN_RB:         snprintf(out, outSize, "RB");     return;
            case PADBTN_RT:         snprintf(out, outSize, "RT");     return;
            case PADBTN_START:      snprintf(out, outSize, "START");  return;
            case PADBTN_BACK:       snprintf(out, outSize, "BACK");   return;
            case PADBTN_DPAD_UP:    snprintf(out, outSize, "DUP");    return;
            case PADBTN_DPAD_DOWN:  snprintf(out, outSize, "DDOWN");  return;
            case PADBTN_DPAD_LEFT:  snprintf(out, outSize, "DLEFT");  return;
            case PADBTN_DPAD_RIGHT: snprintf(out, outSize, "DRIGHT"); return;
            default:                snprintf(out, outSize, "-");      return;
        }
    }
    switch (vk) {
        case VK_SPACE:  snprintf(out, outSize, "SPACE");  return;
        case VK_RETURN: snprintf(out, outSize, "ENTER");  return;
        case VK_UP:     snprintf(out, outSize, "UP");     return;
        case VK_DOWN:   snprintf(out, outSize, "DOWN");   return;
        case VK_ESCAPE: snprintf(out, outSize, "ESC");    return;
        case VK_TAB:    snprintf(out, outSize, "TAB");    return;
        default: break;
    }
    if (vk > 0 && vk < 0x80) snprintf(out, outSize, "%c", (char)vk);
    else snprintf(out, outSize, "-");
}

// Resolves the price of a full refill. Tiers are 10% of tank, so a nearly empty
// car pays ten units while a 95% full one pays one.
static int RefuelPrice(const CVector& pumpPos, float currentFuel) {
    float needed = (1.0f - currentFuel) * 100.0f;
    if (needed <= 0.0f) return 0;
    int units = (int)ceilf(needed / 10.0f);
    bool isCountryside = (CTheZones::GetLevelFromPosition(&pumpPos) == LEVEL_NAME_COUNTRY_SIDE);
    float price = (float)units * g_config.fuelBasePrice;
    if (isCountryside) price *= g_config.fuelCsPriceMultiplier;
    return (int)ceilf(price);
}

// Damage-scaled repair price, clamped to the configured range.
static int RepairPrice(const CVehicle* veh) {
    float t = Clampf((1000.0f - veh->m_fHealth) / 750.0f, 0.0f, 1.0f);
    return (int)ceilf(Lerpf(g_config.repairPriceMin, g_config.repairPriceMax, t));
}

static bool s_servicePadDisabled = false;

void Radar::UpdateFuelServices() {
    CPlayerPed* player = FindPlayerPed(0);
    if (!player) { s_serviceOpen = false; s_servicePromptAlpha = 0.0f; return; }

    CVehicle* veh = GetStablePlayerVehicle(player);
    bool eligible = g_config.useVehicleFuel && IsFuelVehicle(veh) && veh->m_pDriver == player;    bool wanted = player->m_pPlayerData && player->m_pPlayerData->m_pWanted
                  && player->m_pPlayerData->m_pWanted->m_nWantedLevel > 0;

    // Nearest pump within reach, and whether the car is actually parked there.
    const GasStationPos* pump = nullptr;
    if (eligible) {
        float bestSq = 12.0f * 12.0f;
        CVector pos = veh->GetPosition();
        for (const auto& gs : kGasStations) {
            float dx = pos.x - gs.x;
            float dy = pos.y - gs.y;
            float sq = dx * dx + dy * dy;
            if (sq < bestSq) { bestSq = sq; pump = &gs; }
        }
    }

    bool stopped = eligible && veh->m_vecMoveSpeed.Magnitude() < 0.05f;
    bool canOpen = pump && stopped && !wanted;

    // Pursuit gained mid-service, or the car left the pump: drop out and restore
    // control immediately.
    if (s_serviceOpen && (wanted || !eligible || !pump)) s_serviceOpen = false;

    // Controls are only held while the menu is actually up. Doing this before the
    // open/closed split guarantees the pad is released on every exit path.
    if (!s_serviceOpen && s_servicePadDisabled) {
        CPad::GetPad(0)->DisablePlayerControls = 0;
        s_servicePadDisabled = false;
    }

    float dt = CTimer::ms_fTimeStep * 0.02f;
    if (dt <= 0.0f || dt > 0.1f) dt = 0.0166f;

    bool selectDown = (g_config.serviceSelectKey > 0 && (GetAsyncKeyState(g_config.serviceSelectKey) & 0x8000) != 0)
                   || IsPadButtonDown(g_config.serviceSelectPad);
    bool upDown     = (g_config.serviceUpKey > 0 && (GetAsyncKeyState(g_config.serviceUpKey) & 0x8000) != 0)
                   || IsPadButtonDown(g_config.serviceUpPad);
    bool downDown   = (g_config.serviceDownKey > 0 && (GetAsyncKeyState(g_config.serviceDownKey) & 0x8000) != 0)
                   || IsPadButtonDown(g_config.serviceDownPad);
    bool backDown   = (g_config.serviceBackKey > 0 && (GetAsyncKeyState(g_config.serviceBackKey) & 0x8000) != 0)
                   || IsPadButtonDown(g_config.serviceBackPad);

    if (!s_serviceOpen) {
        if (canOpen) {
            bool holdDown = (g_config.serviceKey > 0 && (GetAsyncKeyState(g_config.serviceKey) & 0x8000) != 0)
                         || IsPadButtonDown(g_config.servicePad);
            s_serviceAnchor = CVector(pump->x, pump->y, pump->z + 0.9f);
            s_servicePromptAlpha = std::min(1.0f, s_servicePromptAlpha + dt * 6.0f);
            if (holdDown) {
                s_serviceHoldTime += dt;
                if (s_serviceHoldTime >= 1.0f) {
                    s_serviceOpen = true;
                    s_serviceRow = SERVICE_ROW_REFUEL;
                    s_serviceHoldTime = 0.0f;
                    if (!s_servicePadDisabled) {
                        CPad::GetPad(0)->DisablePlayerControls = 1;
                        s_servicePadDisabled = true;
                    }
                }
            } else {
                s_serviceHoldTime = 0.0f;
            }
        } else {
            s_servicePromptAlpha = std::max(0.0f, s_servicePromptAlpha - dt * 6.0f);
            s_serviceHoldTime = 0.0f;
        }
        return;
    }

    // Menu is open: keep the prompt fully opaque and drive the list.
    s_servicePromptAlpha = 1.0f;
    if (upDown && !s_serviceLastUp)     s_serviceRow = (s_serviceRow + SERVICE_ROW_COUNT - 1) % SERVICE_ROW_COUNT;
    if (downDown && !s_serviceLastDown) s_serviceRow = (s_serviceRow + 1) % SERVICE_ROW_COUNT;
    s_serviceLastUp = upDown;
    s_serviceLastDown = downDown;

    if (backDown && !s_serviceLastBack) {
        s_serviceOpen = false;
    } else if (selectDown && !s_serviceLastSelect) {
        VehicleFuelExt& ext = s_vehicleFuelExt.Get(veh);
        if (s_serviceRow == SERVICE_ROW_REFUEL) {
            int price = RefuelPrice(CVector(pump->x, pump->y, pump->z), ext.fuelLevel);
            if (price > 0 && CWorld::Players[0].m_nMoney >= price) {
                CWorld::Players[0].m_nMoney -= price;
                s_vehFuel = 1.0f;
                ext.fuelLevel = 1.0f;
                LogWrite("fuel: refuelled for $%d", price);
            }
        } else {
            int price = RepairPrice(veh);
            bool damaged = veh->m_fHealth < 995.0f;
            if (damaged && CWorld::Players[0].m_nMoney >= price) {
                CWorld::Players[0].m_nMoney -= price;
                veh->Fix();
                LogWrite("fuel: repaired for $%d", price);
            }
        }
    }
    s_serviceLastSelect = selectDown;
    s_serviceLastBack = backDown;

    if (!s_serviceOpen && s_servicePadDisabled) {
        CPad::GetPad(0)->DisablePlayerControls = 0;
        s_servicePadDisabled = false;
    }
}