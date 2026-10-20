/*
    GTA SA Plugin SDK — Ped Spawning
    Demonstrates: CCivilianPed ctor, CStreaming, CWorld::Add,
                  SetPosition, SetHeading, RpAnimBlendClumpUpdateAnimations
    Source: examples/PedSpawner/Main.cpp
*/
#include <plugin.h>
#include "CCivilianPed.h"
#include "CMessages.h"
#include "CModelInfo.h"
#include "CStreaming.h"
#include "CTheScripts.h"
#include "CTimer.h"
#include "CWorld.h"

using namespace plugin;

class PedSpawner {
    static char msgBuf[128];
public:
    PedSpawner() {
        Events::gameProcessEvent += [] {
            static bool wasPressed = false;
            bool pressed = KeyPressed(VK_F7);
            if (!pressed || wasPressed) { wasPressed = pressed; return; }
            wasPressed = pressed;

            CPed* player = FindPlayerPed();
            if (!player) return;

            // Find a valid ped model
            int modelId = CModelInfo::FindNextModel(MODEL_INFO_PED, 0, true);
            if (modelId == MODEL_INVALID) return;

            CStreaming::RequestModel(modelId, 0);
            CStreaming::LoadAllRequestedModels(false);
            if (!CStreaming::HasModelLoaded(modelId)) {
                sprintf_s(msgBuf, "~r~~h~Failed to load ped model: %d", modelId);
                CMessages::AddMessageJumpQ(msgBuf, 3000, 0);
                return;
            }

            CVector pos = player->TransformFromObjectSpace({0.0f, 4.0f, -1.0f});
            float heading = player->GetHeading() + DegToRad(180.0f);

            CWorld::ClearExcitingStuffFromArea(pos, 2.0f, false);

            auto ped = new CCivilianPed(PED_TYPE_CIVMALE, modelId);
            if (!ped) return;

            pos.z += 1.0f;
            ped->SetPosition(pos.x, pos.y, pos.z);
            ped->SetHeading(heading);
            ped->m_fHeadingCurrent = ped->m_fHeadingGoal = heading;
            ped->GetMatrix().UpdateRW();

            if (ped->m_pRwClump)
                RpAnimBlendClumpUpdateAnimations(ped->m_pRwClump, CTimer::ms_fTimeStep);

            ped->m_fContactSurfaceBrightness = 0.5f;

            CTheScripts::ClearSpaceForMissionEntity(ped->GetPosition(), ped);
            CWorld::Add(ped);

            sprintf_s(msgBuf, "Spawned ped model: %d", modelId);
            CMessages::AddMessageJumpQ(msgBuf, 3000, 0);
        };
    }
} pedSpawner;

char PedSpawner::msgBuf[128];
