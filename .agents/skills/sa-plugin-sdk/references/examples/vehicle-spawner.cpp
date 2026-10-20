/*
    GTA SA Plugin SDK — Vehicle Spawning
    Demonstrates: CStreaming, CVehicleModelInfo, all vehicle subclass ctors,
                  CWorld::Add, Teleport, SetOrientation, PlaceOnRoadProperly
    Source: examples/CreateCar/Main.cpp
*/
#include <plugin.h>
#include "CBmx.h"
#include "CBoat.h"
#include "CBike.h"
#include "CAutomobile.h"
#include "CHeli.h"
#include "CModelInfo.h"
#include "CMonsterTruck.h"
#include "CPlane.h"
#include "CQuadBike.h"
#include "CStreaming.h"
#include "CTheScripts.h"
#include "CTimer.h"
#include "CTrailer.h"
#include "CTrain.h"
#include "CWorld.h"

using namespace plugin;

// Returns nullptr on failure. Caller does NOT own the pointer (game manages lifetime).
CVehicle* SpawnVehicle(unsigned int modelIndex, CVector position, float heading) {
    unsigned char oldFlags = CStreaming::ms_aInfoForModel[modelIndex].m_nFlags;

    CStreaming::RequestModel(modelIndex, GAME_REQUIRED);
    CStreaming::LoadAllRequestedModels(false);
    if (CStreaming::ms_aInfoForModel[modelIndex].m_nLoadState != LOADSTATE_LOADED)
        return nullptr;

    if (!(oldFlags & GAME_REQUIRED)) {
        CStreaming::SetModelIsDeletable(modelIndex);
        CStreaming::SetModelTxdIsDeletable(modelIndex);
    }

    auto modelInfo = CModelInfo::GetModelInfo(modelIndex);
    if (!modelInfo || modelInfo->GetModelType() != ModelInfoType::MODEL_INFO_VEHICLE)
        return nullptr;

    auto vehModelInfo = reinterpret_cast<CVehicleModelInfo*>(modelInfo);
    CVehicle* vehicle = nullptr;

    switch (vehModelInfo->m_nVehicleType) {
        case VEHICLE_BMX:       vehicle = new CBmx(modelIndex, eVehicleCreatedBy::RANDOM_VEHICLE); break;
        case VEHICLE_BIKE:      vehicle = new CBike(modelIndex, eVehicleCreatedBy::RANDOM_VEHICLE); break;
        case VEHICLE_QUAD:      vehicle = new CQuadBike(modelIndex, eVehicleCreatedBy::RANDOM_VEHICLE); break;
        case VEHICLE_AUTOMOBILE:vehicle = new CAutomobile(modelIndex, eVehicleCreatedBy::RANDOM_VEHICLE, true); break;
        case VEHICLE_MTRUCK:    vehicle = new CMonsterTruck(modelIndex, eVehicleCreatedBy::RANDOM_VEHICLE); break;
        case VEHICLE_TRAILER:   vehicle = new CTrailer(modelIndex, eVehicleCreatedBy::RANDOM_VEHICLE); break;
        case VEHICLE_BOAT:      vehicle = new CBoat(modelIndex, eVehicleCreatedBy::RANDOM_VEHICLE); break;
        case VEHICLE_HELI:      vehicle = new CHeli(modelIndex, eVehicleCreatedBy::RANDOM_VEHICLE); break;
        case VEHICLE_PLANE:     vehicle = new CPlane(modelIndex, eVehicleCreatedBy::RANDOM_VEHICLE); break;
        case VEHICLE_TRAIN:
            vehicle = new CTrain(modelIndex, eVehicleCreatedBy::RANDOM_VEHICLE);
            reinterpret_cast<CTrain*>(vehicle)->m_nTrainFlags.bNotOnARailRoad = true;
            break;
        default: break;
    }

    if (!vehicle) return nullptr;

    position.z += vehicle->GetHeightAboveRoad();
    vehicle->SetOrientation(0.0f, 0.0f, heading);
    vehicle->Teleport(position);  // calls CWorld::Add internally

    vehicle->m_nStatus = eEntityStatus::STATUS_ABANDONED;
    vehicle->bEngineOn = false;
    vehicle->m_eDoorLock = DOORLOCK_UNLOCKED;

    // Place on road properly for ground vehicles
    switch (vehicle->m_nVehicleClass) {
        case VEHICLE_BIKE:
            reinterpret_cast<CBike*>(vehicle)->bOnSideStand = true;
            reinterpret_cast<CBike*>(vehicle)->PlaceOnRoadProperly();
            break;
        case VEHICLE_AUTOMOBILE:
            reinterpret_cast<CAutomobile*>(vehicle)->PlaceOnRoadProperly();
            break;
    }

    vehicle->UpdateRwMatrix();
    CTheScripts::ClearSpaceForMissionEntity(vehicle->GetPosition(), vehicle);
    vehicle->ProcessControl();
    return vehicle;
}

class SpawnOnKeyPress {
public:
    SpawnOnKeyPress() {
        Events::gameProcessEvent += [] {
            static bool wasPressed = false;
            bool pressed = KeyPressed(VK_F6);
            if (pressed && !wasPressed) {
                CPed* player = FindPlayerPed();
                if (player && player->m_nAreaCode == 0) {
                    CVector pos = player->TransformFromObjectSpace({0.0f, 8.0f, -1.0f});
                    float rot = player->m_fCurrentRotation;
                    SpawnVehicle(411 /* Infernus */, pos, rot);
                }
            }
            wasPressed = pressed;
        };
    }
} spawnOnKeyPress;
