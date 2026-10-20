static void TrackGunHeat(CPlayerPed* player, unsigned int wanted, float dt) {
    if (!player) return;
    CWeapon& wep = player->m_aWeapons[player->m_nSelectedWepSlot];
    eWeaponType type = wep.m_eWeaponType;

    if (g_hasGunHeat && type != g_lastWeaponType && type != WEAPONTYPE_UNARMED) {
        if (g_config.weaponSwitchCleanupMode == 2) {
            g_hasGunHeat = false;
            g_hotWeaponType = WEAPONTYPE_UNARMED;
            g_gunShotCount = 0;
            g_gunCooldownTimer = 0.0f;
        } else if (g_config.weaponSwitchCleanupMode == 1) {
            g_weaponCleanupScore += g_config.weaponSwitchCleanupWeight;
            g_hotWeaponType = type;
            if (g_config.debugMode >= 2) LogWrite("heat: weapon signature changed, cleanup score %d", g_weaponCleanupScore);
        }
    }

    bool isGun = (type >= WEAPONTYPE_PISTOL && type <= WEAPONTYPE_MINIGUN);
    if (!isGun) {
        g_gunContinuousFireTimer = 0.0f;
    } else {
        if (wep.m_nState == WEAPONSTATE_FIRING) {
            g_gunContinuousFireTimer += dt;
            if (wanted > 0) {
                g_gunShotCount++;
                if (g_gunShotCount >= 5 || g_gunContinuousFireTimer >= 2.0f) {
                    g_hasGunHeat = true;
                    g_hotWeaponType = type;
                    if (g_config.heatCleanupMode != 0 && g_config.weaponWeightingEnabled) {
                        int weight = g_config.weaponWeightPistol;
                        if (type >= WEAPONTYPE_SHOTGUN && type <= WEAPONTYPE_SPAS12) weight = g_config.weaponWeightShotgun;
                        else if (type >= WEAPONTYPE_MICRO_UZI && type <= WEAPONTYPE_TEC9) weight = g_config.weaponWeightSmg;
                        else if (type >= WEAPONTYPE_AK47 && type <= WEAPONTYPE_COUNTRYRIFLE) weight = g_config.weaponWeightRifle;
                        else if (type == WEAPONTYPE_SNIPERRIFLE) weight = g_config.weaponWeightSniper;
                        else if (type >= WEAPONTYPE_RLAUNCHER && type <= WEAPONTYPE_MINIGUN) weight = g_config.weaponWeightHeavy;
                        else if (type >= WEAPONTYPE_GRENADE && type <= WEAPONTYPE_MOLOTOV) weight = g_config.weaponWeightExplosive;
                        g_weaponCleanupScore = std::min(g_config.weaponCleanupThreshold, g_weaponCleanupScore + weight);
                    }
                    g_gunCooldownTimer = g_config.gunBaseCooldown + g_config.gunCooldownPerStar * (float)std::max(1, (int)wanted);
                }
            }
        } else {
            g_gunContinuousFireTimer = 0.0f;
        }

        if (type == g_lastWeaponType && wep.m_nAmmoTotal < g_lastWeaponAmmo) {
            if (wanted > 0) {
                g_gunShotCount += (int)(g_lastWeaponAmmo - wep.m_nAmmoTotal);
                if (g_gunShotCount >= 5) {
                    g_hasGunHeat = true;
                    g_hotWeaponType = type;
                    if (g_config.heatCleanupMode != 0 && g_config.weaponWeightingEnabled) {
                        g_weaponCleanupScore = std::min(g_config.weaponCleanupThreshold, g_weaponCleanupScore + g_config.weaponWeightPerShot * (int)(g_lastWeaponAmmo - wep.m_nAmmoTotal));
                    }
                    g_gunCooldownTimer = g_config.gunBaseCooldown + g_config.gunCooldownPerStar * (float)std::max(1, (int)wanted);
                }
            }
        }
    }
    g_lastWeaponAmmo = wep.m_nAmmoTotal;
    g_lastWeaponType = type;

    if (wanted == 0 && g_hasGunHeat) {
        if (g_config.extWantedGunHeatDecay) {
            g_gunCooldownTimer -= dt * std::max(1.0f, g_config.weaponHeatDecayPerSecond);
            if (g_gunCooldownTimer <= 0.0f) {
                g_hasGunHeat = false;
                g_hotWeaponType = WEAPONTYPE_UNARMED;
                g_gunShotCount = 0;
                g_gunCooldownTimer = 0.0f;
            }
        }
    }
}

static void TrackClothesAndVehicleHeat(CPlayerPed* player, unsigned int wanted, bool hasVisual, float dt) {
    if (!player) return;

    if (!player->m_pVehicle) {
        // A vehicle left before a later foot crime must not remain the active VEH identifier.
        g_currentVehicleHeat = nullptr;
    }

    if (wanted > 0) {
        g_lastKnownWantedStars = (int)wanted;

        if (hasVisual) {
            bool isMasked = IsPlayerWearingMask(player);

            if (player->m_pVehicle) {
                g_carVisualTimer += dt;
                g_footVisualTimer = 0.0f;
                if (g_carVisualTimer >= 15.0f) {
                    if (player->m_pPlayerData && player->m_pPlayerData->m_pPedClothesDesc) {
                        memcpy(g_hotClothesKeys, player->m_pPlayerData->m_pPedClothesDesc->m_anTextureKeys, sizeof(g_hotClothesKeys));
                        g_hasClothingHeat = true;
                    }
                }
            } else {
                g_footVisualTimer += dt;
                g_carVisualTimer = 0.0f;
                if (g_footVisualTimer >= 2.5f) {
                    if (player->m_pPlayerData && player->m_pPlayerData->m_pPedClothesDesc) {
                        memcpy(g_hotClothesKeys, player->m_pPlayerData->m_pPedClothesDesc->m_anTextureKeys, sizeof(g_hotClothesKeys));
                        g_hasClothingHeat = true;
                    }
                }
            }

            // Facial / Identity profile logging
            if (isMasked) {
                g_wasMaskedWhenSpotted = true;
            } else {
                if (g_footVisualTimer >= 1.5f || g_carVisualTimer >= 4.0f) {
                    g_hasIdentityHeat = true;
                    if (player->m_pPlayerData && player->m_pPlayerData->m_pPedClothesDesc) {
                        g_hotHairKey = player->m_pPlayerData->m_pPedClothesDesc->m_anTextureKeys[CLOTHES_TEXTURE_HEAD];
                    }
                }
            }
        } else {
            g_footVisualTimer = 0.0f;
            g_carVisualTimer = 0.0f;
        }

        if (player->m_pVehicle && (!g_config.vehicleHeatRequireCurrentVisual || hasVisual)) {
            AddHotVehicle(player->m_pVehicle);
        }
    } else {
        g_footVisualTimer = 0.0f;
        g_carVisualTimer = 0.0f;

        // Changing hair/beard at barber shop completely clears facial profile
        if (g_hasIdentityHeat && player->m_pPlayerData && player->m_pPlayerData->m_pPedClothesDesc) {
            if (player->m_pPlayerData->m_pPedClothesDesc->m_anTextureKeys[CLOTHES_TEXTURE_HEAD] != g_hotHairKey) {
                g_hasIdentityHeat = false;
                g_hotHairKey = 0;
            }
        }

        // Removing mask out of sight clears masked-suspect profile
        if (g_wasMaskedWhenSpotted && !hasVisual) {
            if (!IsPlayerWearingMask(player)) {
                g_wasMaskedWhenSpotted = false;
            }
        }

        if (g_hasClothingHeat && HasClothesChanged(player)) {
            float reduction = GetOutfitHeatReduction(player);
            if (reduction >= 0.99f || g_config.heatCleanupMode == 0 || !g_config.clothingWeightingEnabled) {
                g_hasClothingHeat = false;
            } else {
                int changedSlots = CountChangedClothingSlots(player);
                g_clothingCleanupScore = std::min(g_config.clothingCleanupThreshold,
                    g_clothingCleanupScore + changedSlots * g_config.clothingWeightPerSlot);
                memcpy(g_hotClothesKeys, player->m_pPlayerData->m_pPedClothesDesc->m_anTextureKeys, sizeof(g_hotClothesKeys));
                if (g_clothingCleanupScore >= g_config.clothingCleanupThreshold) {
                    g_hasClothingHeat = false;
                    g_clothingCleanupScore = 0;
                }
            }
        }
    }
}

static bool s_resprayCarWasHot = false;

static bool RefreshVehicleLicensePlate(CVehicle* vehicle, const char* plateText, unsigned char region) {
    if (!vehicle || !vehicle->m_pRwClump || !plateText || plateText[0] == '\0') return false;
    vehicle->CustomCarPlate_TextureDestroy();
    CCustomCarPlateMgr::SetupClump(vehicle->m_pRwClump, const_cast<char*>(plateText), region);
    vehicle->UpdateRwMatrix();
    return vehicle->m_pCustomCarPlate != nullptr;
}

static void ProcessPayNSpray(CPlayerPed* player) {
    if (!player || !player->m_pVehicle) return;
    CVehicle* pVeh = player->m_pVehicle;

    static CVehicle* s_resprayVehicle = nullptr;
    if (s_resprayVehicle != pVeh) {
        s_resprayVehicle = pVeh;
        s_resprayCarWasHot = false;
    }

    // Track vehicle heat during respray
    if (IsCurrentVehicleHot(player)) {
        s_resprayCarWasHot = true;
    }

    if (pVeh->bHasBeenResprayed) {
        bool wasHot = IsCurrentVehicleHot(player) || s_resprayCarWasHot;

        // Option 2: License plate randomization only occurs if the vehicle currently has active heat
        if (wasHot && g_config.payNSprayRandomizePlate) {
            char newPlate[16] = {};
            CCustomCarPlateMgr::GeneratePlateText(newPlate, 8);
            unsigned char region = CCustomCarPlateMgr::GetMapRegionPlateDesign();
            if (RefreshVehicleLicensePlate(pVeh, newPlate, region)) {
                ApplyVehicleCleanupWeight(pVeh, g_config.vehicleWeightPlate, "plate");
            }
        }

        if (g_config.payNSprayCost > 100) {
            int extraCost = g_config.payNSprayCost - 100;
            if (CWorld::Players[0].m_nMoney >= extraCost) {
                CWorld::Players[0].m_nMoney -= extraCost;
            } else {
                CWorld::Players[0].m_nMoney = 0;
            }
        }

        RemoveHotVehicle(pVeh);
        s_resprayCarWasHot = false;
        pVeh->bHasBeenResprayed = false;

        // Wipe pursuit and search state upon respray
        g_wantedActive = false;
        g_wantedStage = WANTED_STAGE_INACTIVE;
        g_copHasVisual = false;
        g_escapeTimer = 0.0f;
        g_heatEscapeTimer = 0.0f;
    }
}

struct ModShopLocation {
    CVector pos;
    float radius;
};
static const ModShopLocation kModShops[] = {
    { CVector(1041.5f, -1025.5f, 32.1f), 25.0f },  // TransFender Los Santos
    { CVector(-1936.0f, 245.0f, 34.5f),  25.0f },  // TransFender San Fierro
    { CVector(2386.5f, 1050.0f, 10.8f),  25.0f },  // TransFender Las Venturas
    { CVector(2645.0f, -2043.5f, 13.5f), 25.0f },  // Loco Low Co (Willowfield)
    { CVector(-2711.0f, 217.0f, 4.3f),   25.0f }   // Wheel Arch Angels (Ocean Flats)
};

static void ProcessTransfenderPlateMenu(CPlayerPed* player) {
    if (!g_config.transfenderPlateMenu || !player || !player->m_pVehicle) return;

    CVehicle* pVeh = player->m_pVehicle;
    CVector vehPos = pVeh->GetPosition();

    // Check if player vehicle is actually inside or near a mod shop
    bool nearModShop = false;
    for (const auto& shop : kModShops) {
        if ((vehPos - shop.pos).Magnitude2D() <= shop.radius) {
            nearModShop = true;
            break;
        }
    }
    if (!nearModShop) return;

    // Detect vehicle color modification inside mod shop / tuning menu to remove hot vehicle state
    static unsigned char s_lastCol1 = 0;
    static unsigned char s_lastCol2 = 0;
    static CVehicle* s_lastModVeh = nullptr;
    if (s_lastModVeh == pVeh) {
        if (pVeh->m_nPrimaryColor != s_lastCol1 || pVeh->m_nSecondaryColor != s_lastCol2) {
            ApplyVehicleCleanupWeight(pVeh, g_config.vehicleWeightRespray, "respray");
        }
    }
    s_lastModVeh = pVeh;
    s_lastCol1 = pVeh->m_nPrimaryColor;
    s_lastCol2 = pVeh->m_nSecondaryColor;

    // Check hotkey 'P' to customize license plate safely inside mod shops without corrupting third-party menus
    static bool s_lastPKeyState = false;
    bool pKeyDown = (GetAsyncKeyState('P') & 0x8000) != 0;
    bool pKeyPressed = pKeyDown && !s_lastPKeyState;
    s_lastPKeyState = pKeyDown;

    if (pKeyPressed) {
        int cost = g_config.plateModCost;
        if (CWorld::Players[0].m_nMoney >= cost) {
            CWorld::Players[0].m_nMoney -= cost;

            static unsigned char s_plateRegion = 0;
            s_plateRegion = (s_plateRegion + 1) % 3; // cycle through LS, SF, LV styles

            char newPlate[16] = {};
            CCustomCarPlateMgr::GeneratePlateText(newPlate, 8);
            if (RefreshVehicleLicensePlate(pVeh, newPlate, s_plateRegion)) {
                if (g_config.extWantedChangePlateTransfender) {
                    RemoveHotVehicle(pVeh);
                    if (g_currentVehicleHeat == pVeh) g_currentVehicleHeat = nullptr;
                }
                ApplyVehicleCleanupWeight(pVeh, g_config.vehicleWeightPlate, "plate");
                AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_BUY_CAR_MOD, 0.0f, 1.0f);
            }
        } else {
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_NO_CASH, 0.0f, 1.0f);
        }
    }
}

static void ProcessCopScanning(CPlayerPed* player, float dt) {
    if (!g_config.enableHeatIdentification || !player) return;

    CWanted* pWanted = player->m_pPlayerData ? player->m_pPlayerData->m_pWanted : nullptr;
    unsigned int wanted = pWanted ? pWanted->m_nWantedLevel : 0;
    if (wanted > 0) {
        g_copScanningPlayer = false;
        g_copScanProgress = 0.0f;
        return;
    }

    bool hotGunActive = (g_hasGunHeat && IsPlayerHoldingHotGun(player));
    bool hotVehActive = IsCurrentVehicleHot(player);
    bool openCarryActive = IsPlayerOpenlyCarrying(player);

    int matchCount = 0;
    if (g_hasIdentityHeat) matchCount++;
    if (g_hasClothingHeat) matchCount++;
    if (hotVehActive) matchCount++;
    if (hotGunActive || openCarryActive) matchCount++;

    if (matchCount == 0) {
        g_copScanningPlayer = false;
        g_copScanProgress = std::max(0.0f, g_copScanProgress - dt * 0.4f);
        return;
    }

    float speedMult = 1.0f + (float)(matchCount - 1) * 0.75f;
    if (g_hasIdentityHeat) {
        if (IsPlayerDisguised(player)) {
            speedMult *= 0.5f; // Hats/sunglasses slow recognition rate
        } else {
            speedMult *= 2.5f; // Full facial profile recognized rapidly
        }
    }
    if (g_hasClothingHeat) {
        float outfitRed = GetOutfitHeatReduction(player);
        speedMult *= (1.0f - outfitRed * 0.7f);
    }

    CVector pPos = player->GetPosition();
    CVector pEyePos = pPos + CVector(0.0f, 0.0f, 0.7f);

    static bool s_cachedScanned = false;
    static float s_cachedScanRate = 0.0f;
    static unsigned int s_lastScanTick = 0;

    unsigned int curTick = CTimer::m_snTimeInMillisecondsNonClipped;
    if (curTick - s_lastScanTick >= 80) { // Evaluate LOS checks every 80ms
        s_lastScanTick = curTick;
        s_cachedScanned = false;
        s_cachedScanRate = 0.0f;

        if (CPools::ms_pPedPool) {
            int pedCount = CPools::ms_pPedPool->m_nSize;
            for (int i = 0; i < pedCount; ++i) {
                CPed* cop = CPools::ms_pPedPool->GetAt(i);
                if (!cop || cop == player || cop->m_fHealth <= 0.0f) continue;
                if (cop->m_nPedType != PED_TYPE_COP) continue;

                CVector copPos = cop->GetPosition();
                float dx = fabsf(copPos.x - pPos.x);
                float dy = fabsf(copPos.y - pPos.y);
                if (dx > g_config.footCopScanDist || dy > g_config.footCopScanDist) continue;

                float distSq = (copPos - pPos).MagnitudeSqr();
                if (distSq > (g_config.footCopScanDist * g_config.footCopScanDist)) continue;

                float dist = sqrtf(distSq);
                if (dist > 8.0f) {
                    CVector toPlayer = (pPos - copPos).Normalized();
                    CVector copFwd = cop->m_matrix ? cop->GetForward() : CVector(0.0f, 1.0f, 0.0f);
                    if (copFwd.Dot(toPlayer) <= 0.15f) continue;
                }

                CVector copEyePos = copPos + CVector(0.0f, 0.0f, 0.7f);
                if (CWorld::GetIsLineOfSightClear(copEyePos, pEyePos, true, false, false, true, false, false, false)) {
                    s_cachedScanned = true;
                    s_cachedScanRate = std::max(s_cachedScanRate, (1.0f / std::max(0.5f, g_config.footCopScanTime)) * speedMult);
                    break;
                }
            }
        }

        if (!s_cachedScanned && CPools::ms_pVehiclePool) {
            int vehCount = CPools::ms_pVehiclePool->m_nSize;
            for (int i = 0; i < vehCount; ++i) {
                CVehicle* copVeh = CPools::ms_pVehiclePool->GetAt(i);
                if (!copVeh || copVeh->m_fHealth <= 0.0f) continue;
                if (copVeh == player->m_pVehicle) continue;

                int model = copVeh->m_nModelIndex;
                bool isPoliceModel = (model == 596 || model == 597 || model == 598 || model == 599 ||
                                      model == 523 || model == 427 || model == 490 || model == 528 ||
                                      model == 497 || copVeh->IsLawEnforcementVehicle());
                if (!isPoliceModel) continue;

                bool hasCopDriver = (copVeh->m_pDriver && copVeh->m_pDriver != player && copVeh->m_pDriver->m_fHealth > 0.0f);
                if (!hasCopDriver && model != 497) continue;

                CVector vehPos = copVeh->GetPosition();
                float dx = fabsf(vehPos.x - pPos.x);
                float dy = fabsf(vehPos.y - pPos.y);
                if (dx > g_config.vehCopScanDist || dy > g_config.vehCopScanDist) continue;

                float distSq = (vehPos - pPos).MagnitudeSqr();
                if (distSq > (g_config.vehCopScanDist * g_config.vehCopScanDist)) continue;

                if (model != 497 && distSq > (12.0f * 12.0f)) {
                    CVector toPlayer = (pPos - vehPos).Normalized();
                    CVector vehFwd = copVeh->m_matrix ? copVeh->GetForward() : CVector(0.0f, 1.0f, 0.0f);
                    if (vehFwd.Dot(toPlayer) <= 0.15f) continue;
                }

                CVector vehEyePos = vehPos + CVector(0.0f, 0.0f, (model == 497) ? -0.5f : 0.8f);
                if (CWorld::GetIsLineOfSightClear(vehEyePos, pEyePos, true, false, false, true, false, false, false)) {
                    s_cachedScanned = true;
                    s_cachedScanRate = std::max(s_cachedScanRate, (1.0f / std::max(0.5f, g_config.vehCopScanTime)) * speedMult);
                    break;
                }
            }
        }
    }

    g_copScanningPlayer = s_cachedScanned;

    if (s_cachedScanned) {
        g_copScanProgress += dt * s_cachedScanRate;
        if (g_copScanProgress >= 1.0f) {
            if (pWanted) {
                int resumeStars = std::clamp(g_lastKnownWantedStars, 1, 6);
                if (hotGunActive) {
                    resumeStars = std::max(2, resumeStars);
                } else if (openCarryActive && !g_hasIdentityHeat && !g_hasClothingHeat && !hotVehActive) {
                    resumeStars = 1;
                }
                pWanted->SetWantedLevel(resumeStars);
                g_wantedStage = WANTED_STAGE_PURSUIT;
                g_wantedActive = true;
                g_escapeTimer = 0.0f;
                g_heatEscapeTimer = 0.0f;
                LogWrite("heat: suspect identified by police, wanted level %d resumed", resumeStars);
            }
            g_copScanProgress = 0.0f;
            g_copScanningPlayer = false;
            s_cachedScanned = false;
        }
    } else {
        g_copScanProgress = std::max(0.0f, g_copScanProgress - dt * 0.4f);
    }
}

static void UpdateSnitchSystem(CPlayerPed* player, unsigned int wanted, float dt) {
    if (!player) return;
    bool onMission = CTheScripts::IsPlayerOnAMission();
    bool isSnitchActive = g_config.useSnitchSystem && (!onMission || g_config.useSnitchSystemOnMission);
    if (!isSnitchActive) {
        g_hollowStarsCount = 0;
        g_hollowStarsTimer = 0.0f;
        return;
    }

    CWanted* pWanted = player->m_pPlayerData ? player->m_pPlayerData->m_pWanted : nullptr;

    // 1. Process hollow stars dispatch timer
    if (g_hollowStarsCount > 0) {
        g_hollowStarsTimer -= dt;
        if (g_hollowStarsTimer <= 0.0f) {
            if (pWanted && wanted == 0) {
                pWanted->SetWantedLevel(g_hollowStarsCount);
                g_wantedStage = WANTED_STAGE_PURSUIT;
                g_wantedActive = true;
                g_escapeTimer = 0.0f;
                g_heatEscapeTimer = 0.0f;
                LogWrite("snitch: 911 police dispatch arrived at crime scene, wanted level %d set", g_hollowStarsCount);
            }
            g_hollowStarsCount = 0;
            g_hollowStarsTimer = 0.0f;
        }
    }

    // 2. Update active witnesses
    CVector pPos = player->GetPosition();
    for (int i = 0; i < kMaxSnitches; ++i) {
        if (!g_snitches[i].active) continue;
        CPed* witness = g_snitches[i].ped;
        if (!witness || witness->m_fHealth <= 0.0f) {
            // Elimination: killing witness prevents report!
            g_snitches[i].active = false;
            g_snitches[i].ped = nullptr;
            continue;
        }

        // Intimidation: aiming weapon at witness prevents 911 call
        CVector wPos = witness->GetPosition();
        float distToWitness = (wPos - pPos).Magnitude2D();
        bool isAiming = false;
        if ((CEntity*)player->m_pTargetedObject == (CEntity*)witness) {
            isAiming = true;
        }
        if (!isAiming && distToWitness < 15.0f && IsPlayerOpenlyCarrying(player)) {
            CVector pFwd = player->m_matrix ? player->GetForward() : CVector(0.0f, 1.0f, 0.0f);
            CVector toWitness = (wPos - pPos).Normalized();
            if (pFwd.Dot(toWitness) > 0.85f) isAiming = true;
        }
        if (isAiming) {
            // Intimidated! Witness cowers, abort report
            g_snitches[i].active = false;
            g_snitches[i].ped = nullptr;
            continue;
        }

        float runDist = (wPos - g_snitches[i].startPos).Magnitude2D();
        if (runDist >= g_snitches[i].panicRunDist && !g_snitches[i].hasCalled) {
            // Witness reached safe distance, called 911!
            g_snitches[i].hasCalled = true;
            g_hollowStarsCount = std::max(g_hollowStarsCount, g_snitches[i].severityStars);
            g_hollowStarsTimer = g_snitches[i].dispatchTimer;

            // Log reported traits:
            if (g_snitches[i].reportedVehicle && player->m_pVehicle) {
                AddHotVehicle(player->m_pVehicle);
            }
            if (g_snitches[i].reportedGun) {
                g_hasGunHeat = true;
            }
            if (!player->m_pVehicle) {
                g_hasClothingHeat = true;
                if (!IsPlayerWearingMask(player)) {
                    g_hasIdentityHeat = true;
                    if (player->m_pPlayerData && player->m_pPlayerData->m_pPedClothesDesc) {
                        g_hotHairKey = player->m_pPlayerData->m_pPedClothesDesc->m_anTextureKeys[CLOTHES_TEXTURE_HEAD];
                    }
                }
            }
            g_snitches[i].active = false;
            g_snitches[i].ped = nullptr;
        }
    }

    // 3. Detect new crimes when wanted == 0
    if (wanted > 0) return;

    CWeapon& wep = player->m_aWeapons[player->m_nSelectedWepSlot];
    bool isFiring = (wep.m_nState == WEAPONSTATE_FIRING && wep.m_eWeaponType >= WEAPONTYPE_PISTOL && wep.m_eWeaponType <= WEAPONTYPE_MINIGUN);
    bool isTheft = (player->m_pVehicle && player->m_pVehicle->m_pDriver == player && player->m_pVehicle->m_nStatus == STATUS_PLAYER && player->m_pVehicle->m_fHealth > 0.0f && !player->m_pVehicle->bIsHandbrakeOn);

    if (!isFiring && !isTheft) return;

    // Find nearest civilian eyewitness
    if (CPools::ms_pPedPool) {
        int pedCount = CPools::ms_pPedPool->m_nSize;
        float radiusSq = g_config.extWantedSnitchRadius * g_config.extWantedSnitchRadius;
        for (int i = 0; i < pedCount; ++i) {
            CPed* ped = CPools::ms_pPedPool->GetAt(i);
            if (!ped || ped == player || ped->m_fHealth <= 0.0f) continue;
            // Gang members retaliate directly rather than reporting
            if (ped->m_nPedType >= PED_TYPE_GANG1 && ped->m_nPedType <= PED_TYPE_DEALER) continue;
            if (ped->m_nPedType == PED_TYPE_COP) continue;

            CVector pedPos = ped->GetPosition();
            float distSq = (pedPos - pPos).MagnitudeSqr();
            if (distSq > radiusSq) continue;

            // Check if ped already a snitch
            bool already = false;
            for (int s = 0; s < kMaxSnitches; ++s) {
                if (g_snitches[s].active && g_snitches[s].ped == ped) { already = true; break; }
            }
            if (already) continue;

            // Find free slot
            for (int s = 0; s < kMaxSnitches; ++s) {
                if (!g_snitches[s].active) {
                    g_snitches[s].active = true;
                    g_snitches[s].ped = ped;
                    g_snitches[s].startPos = pedPos;
                    float minRun = g_config.extWantedSnitchMinPanicRun;
                    float maxRun = g_config.extWantedSnitchMaxPanicRun;
                    float runT = (float)(rand() % 100) / 100.0f;
                    g_snitches[s].panicRunDist = Lerpf(minRun, maxRun, runT);

                    float minDisp = g_config.extWantedSnitchMinDispatchTime;
                    float maxDisp = g_config.extWantedSnitchMaxDispatchTime;
                    float dispT = (float)(rand() % 100) / 100.0f;
                    g_snitches[s].dispatchTimer = Lerpf(minDisp, maxDisp, dispT);

                    g_snitches[s].severityStars = isFiring ? 2 : 1;
                    g_snitches[s].hasCalled = false;
                    g_snitches[s].reportedVehicle = (player->m_pVehicle != nullptr);
                    g_snitches[s].reportedGun = isFiring;
                    break;
                }
            }
            break; // One witness triggered per crime event
        }
    }
}

