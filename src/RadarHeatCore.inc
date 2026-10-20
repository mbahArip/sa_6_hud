// GTA 6 3-Tier Heat Identification System State
// =============================================================================
static bool          g_hasIdentityHeat        = false;
static unsigned int  g_hotHairKey             = 0;
static bool          g_wasMaskedWhenSpotted   = false;
static bool          g_hasClothingHeat        = false;
static unsigned int  g_hotClothesKeys[18]     = {};
static float         g_footVisualTimer        = 0.0f;
static float         g_carVisualTimer         = 0.0f;
static int            g_vehicleCleanupScore    = 0;
static int            g_clothingCleanupScore   = 0;
static int            g_weaponCleanupScore     = 0;
static CVehicle*      g_clearedVehicle        = nullptr;
static int            g_clearedVehicleModel   = 0;
static unsigned char  g_clearedVehicleColor1 = 0;
static unsigned char  g_clearedVehicleColor2 = 0;
static RwTexture*     g_clearedVehiclePlate  = nullptr;
static CVehicle*      g_currentVehicleHeat   = nullptr;

struct HotVehicleSignature {
    bool          active = false;
    int           vehicleHandle = 0;
    int           modelIndex = 0;
    unsigned char color1 = 0;
    unsigned char color2 = 0;
    RwTexture*    customPlateTex = nullptr;
};

static const int           kMaxHotVehicles = 5;
static HotVehicleSignature g_hotVehicles[kMaxHotVehicles] = {};
static int                 g_hotVehicleHead = 0;

static bool          g_hasGunHeat             = false;
static eWeaponType   g_hotWeaponType          = WEAPONTYPE_UNARMED;
static int           g_gunShotCount           = 0;
static float         g_gunContinuousFireTimer = 0.0f;
static float         g_gunCooldownTimer       = 0.0f;
static unsigned int  g_lastWeaponAmmo         = 0;
static eWeaponType   g_lastWeaponType         = WEAPONTYPE_UNARMED;

static int           g_lastKnownWantedStars   = 1;
static bool          g_copScanningPlayer      = false;
static float         g_copScanProgress        = 0.0f;

static bool HasClothesChanged(CPlayerPed* player) {
    if (!player || !player->m_pPlayerData || !player->m_pPlayerData->m_pPedClothesDesc) return false;
    CPedClothesDesc* desc = player->m_pPlayerData->m_pPedClothesDesc;
    for (int i = 0; i < 18; ++i) {
        if (g_hotClothesKeys[i] != desc->m_anTextureKeys[i]) {
            return true;
        }
    }
    return false;
}

static bool IsPlayerWearingMask(CPlayerPed* player) {
    if (!player || !player->m_pPlayerData || !player->m_pPlayerData->m_pPedClothesDesc) return false;
    CPedClothesDesc* desc = player->m_pPlayerData->m_pPedClothesDesc;
    if (desc->GetIsWearingBalaclava()) return true;

    auto matchesList = [](unsigned int key, const std::vector<unsigned int>& list) {
        if (key == 0) return false;
        for (unsigned int k : list) {
            if (key == k) return true;
        }
        return false;
    };

    if (!g_config.maskModelKeys.empty()) {
        if (matchesList(desc->m_anModelKeys[CLOTHES_MODEL_GLASSES], g_config.maskModelKeys)) return true;
        if (matchesList(desc->m_anModelKeys[CLOTHES_MODEL_HATS], g_config.maskModelKeys)) return true;
        if (matchesList(desc->m_anModelKeys[CLOTHES_MODEL_SPECIAL], g_config.maskModelKeys)) return true;
    } else {
        const char* defs[] = { "BANDMASK", "HOCKEYMASK", "HELMET", "MOTO", "BIKERHELMET", "ZORROMASK", "BALACLAVA" };
        for (const char* name : defs) {
            unsigned int k = HashUppercaseKey(name);
            if (desc->m_anModelKeys[CLOTHES_MODEL_GLASSES] == k ||
                desc->m_anModelKeys[CLOTHES_MODEL_HATS] == k ||
                desc->m_anModelKeys[CLOTHES_MODEL_SPECIAL] == k) {
                return true;
            }
        }
    }

    return false;
}

static bool IsPlayerDisguised(CPlayerPed* player) {
    if (!player || !player->m_pPlayerData || !player->m_pPlayerData->m_pPedClothesDesc) return false;
    CPedClothesDesc* desc = player->m_pPlayerData->m_pPedClothesDesc;

    auto matchesList = [](unsigned int key, const std::vector<unsigned int>& list) {
        if (key == 0) return false;
        for (unsigned int k : list) {
            if (key == k) return true;
        }
        return false;
    };

    if (matchesList(desc->m_anModelKeys[CLOTHES_MODEL_GLASSES], g_config.disguiseModelKeys)) return true;
    if (matchesList(desc->m_anModelKeys[CLOTHES_MODEL_HATS], g_config.disguiseModelKeys)) return true;

    bool hasHat = (desc->m_anTextureKeys[CLOTHES_TEXTURE_HATS] != 0);
    bool hasGlasses = (desc->m_anTextureKeys[CLOTHES_TEXTURE_GLASSES] != 0);
    return hasHat || hasGlasses;
}

static float GetOutfitHeatReduction(CPlayerPed* player) {
    if (!player || !player->m_pPlayerData || !player->m_pPlayerData->m_pPedClothesDesc) return 0.0f;
    CPedClothesDesc* desc = player->m_pPlayerData->m_pPedClothesDesc;
    float reduction = 0.0f;
    if (desc->m_anTextureKeys[CLOTHES_TEXTURE_TORSO] != g_hotClothesKeys[CLOTHES_TEXTURE_TORSO]) reduction += 0.50f;
    if (desc->m_anTextureKeys[CLOTHES_TEXTURE_LEGS]  != g_hotClothesKeys[CLOTHES_TEXTURE_LEGS])  reduction += 0.40f;
    if (desc->m_anTextureKeys[CLOTHES_TEXTURE_SHOES] != g_hotClothesKeys[CLOTHES_TEXTURE_SHOES]) reduction += 0.10f;
    return reduction;
}

static bool IsPlayerOpenlyCarrying(CPlayerPed* player) {
    if (!player) return false;
    CWeapon& wep = player->m_aWeapons[player->m_nSelectedWepSlot];
    return wep.m_eWeaponType >= WEAPONTYPE_PISTOL && wep.m_eWeaponType <= WEAPONTYPE_MINIGUN && wep.m_nAmmoTotal > 0;
}

static int CountChangedClothingSlots(CPlayerPed* player) {
    if (!player || !player->m_pPlayerData || !player->m_pPlayerData->m_pPedClothesDesc) return 0;
    CPedClothesDesc* desc = player->m_pPlayerData->m_pPedClothesDesc;
    int changedSlots = 0;
    for (int i = 0; i < 18; ++i) {
        if (g_hotClothesKeys[i] != desc->m_anTextureKeys[i]) ++changedSlots;
    }
    return changedSlots;
}

static bool IsVehicleInHotList(CVehicle* veh) {
    if (!veh) return false;
    for (int i = 0; i < kMaxHotVehicles; ++i) {
        if (!g_hotVehicles[i].active) continue;
        CVehicle* poolVeh = (g_hotVehicles[i].vehicleHandle != 0) ? CPools::GetVehicle(g_hotVehicles[i].vehicleHandle) : nullptr;
        bool sameVehicle = (poolVeh && poolVeh == veh);
        bool sameFallback = g_config.vehicleHeatAllowHistoryFallback &&
            g_hotVehicles[i].modelIndex == veh->m_nModelIndex;
        if ((sameVehicle || sameFallback) &&
            g_hotVehicles[i].color1 == veh->m_nPrimaryColor &&
            g_hotVehicles[i].color2 == veh->m_nSecondaryColor &&
            g_hotVehicles[i].customPlateTex == veh->m_pCustomCarPlate) {
            return true;
        }
    }
    return false;
}

static void AddHotVehicle(CVehicle* veh) {
    if (!veh) return;
    int vehHandle = CPools::GetVehicleRef(veh);
    if (g_clearedVehicle == veh &&
        g_clearedVehicleModel == veh->m_nModelIndex &&
        g_clearedVehicleColor1 == veh->m_nPrimaryColor &&
        g_clearedVehicleColor2 == veh->m_nSecondaryColor &&
        g_clearedVehiclePlate == veh->m_pCustomCarPlate) {
        return;
    }
    for (int i = 0; i < kMaxHotVehicles; ++i) {
        if (!g_hotVehicles[i].active) continue;
        CVehicle* poolVeh = (g_hotVehicles[i].vehicleHandle != 0) ? CPools::GetVehicle(g_hotVehicles[i].vehicleHandle) : nullptr;
        bool sameVehicle = (poolVeh && poolVeh == veh);
        bool sameFallback = g_config.vehicleHeatAllowHistoryFallback && (g_hotVehicles[i].modelIndex == veh->m_nModelIndex);
        if ((sameVehicle || sameFallback) &&
            g_hotVehicles[i].color1 == veh->m_nPrimaryColor &&
            g_hotVehicles[i].color2 == veh->m_nSecondaryColor &&
            g_hotVehicles[i].customPlateTex == veh->m_pCustomCarPlate) {
            g_hotVehicles[i].vehicleHandle = vehHandle;
            g_currentVehicleHeat = veh;
            return;
        }
    }
    g_hotVehicles[g_hotVehicleHead].active = true;
    g_hotVehicles[g_hotVehicleHead].vehicleHandle = vehHandle;
    g_hotVehicles[g_hotVehicleHead].modelIndex = veh->m_nModelIndex;
    g_hotVehicles[g_hotVehicleHead].color1 = veh->m_nPrimaryColor;
    g_hotVehicles[g_hotVehicleHead].color2 = veh->m_nSecondaryColor;
    g_hotVehicles[g_hotVehicleHead].customPlateTex = veh->m_pCustomCarPlate;
    g_currentVehicleHeat = veh;
    g_hotVehicleHead = (g_hotVehicleHead + 1) % kMaxHotVehicles;
}

static void RemoveHotVehicle(CVehicle* veh) {
    if (!veh) return;
    for (int i = 0; i < kMaxHotVehicles; ++i) {
        if (!g_hotVehicles[i].active) continue;
        CVehicle* poolVeh = (g_hotVehicles[i].vehicleHandle != 0) ? CPools::GetVehicle(g_hotVehicles[i].vehicleHandle) : nullptr;
        bool sameVehicle = (poolVeh && poolVeh == veh);
        bool sameFallback = g_config.vehicleHeatAllowHistoryFallback &&
            (g_hotVehicles[i].modelIndex == veh->m_nModelIndex);
        if (sameVehicle || sameFallback) {
            g_hotVehicles[i].active = false;
            g_hotVehicles[i].vehicleHandle = 0;
        }
    }
    g_clearedVehicle = veh;
    g_clearedVehicleModel = veh->m_nModelIndex;
    g_clearedVehicleColor1 = veh->m_nPrimaryColor;
    g_clearedVehicleColor2 = veh->m_nSecondaryColor;
    g_clearedVehiclePlate = veh->m_pCustomCarPlate;
    if (g_currentVehicleHeat == veh) g_currentVehicleHeat = nullptr;
    if (g_config.debugMode >= 2) LogWrite("heat: vehicle marked clean model=%d", veh->m_nModelIndex);
}

static void ApplyVehicleCleanupWeight(CVehicle* veh, int weight, const char* reason) {
    if (!veh || weight <= 0) return;
    if (g_config.heatCleanupMode == 0 || !g_config.vehicleWeightingEnabled) {
        RemoveHotVehicle(veh);
        return;
    }

    g_vehicleCleanupScore = std::min(g_config.vehicleCleanupThreshold, g_vehicleCleanupScore + weight);
    if (g_config.debugMode >= 2) {
        LogWrite("heat: vehicle cleanup +%d (%s), score %d/%d", weight, reason,
                 g_vehicleCleanupScore, g_config.vehicleCleanupThreshold);
    }
    if (g_vehicleCleanupScore >= g_config.vehicleCleanupThreshold) {
        RemoveHotVehicle(veh);
        g_vehicleCleanupScore = 0;
    }
}

static bool HasAnyHotVehicle() {
    for (int i = 0; i < kMaxHotVehicles; ++i) {
        if (g_hotVehicles[i].active) return true;
    }
    return false;
}

static void ClearAllHotVehicles() {
    for (int i = 0; i < kMaxHotVehicles; ++i) {
        g_hotVehicles[i] = HotVehicleSignature();
    }
    g_hotVehicleHead = 0;
    g_currentVehicleHeat = nullptr;
}

static bool IsCurrentVehicleHot(CPlayerPed* player) {
    return player && player->m_pVehicle && g_currentVehicleHeat == player->m_pVehicle &&
        IsVehicleInHotList(player->m_pVehicle);
}

static bool IsPlayerHoldingHotGun(CPlayerPed* player) {
    if (!player || !g_hasGunHeat || g_hotWeaponType == WEAPONTYPE_UNARMED) return false;
    eWeaponType currentWep = player->m_aWeapons[player->m_nSelectedWepSlot].m_eWeaponType;
    return (currentWep == g_hotWeaponType);
}

