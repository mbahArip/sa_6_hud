/*
    GTA SA Plugin SDK — Task System
    Demonstrates: assigning tasks to peds via CTaskManager,
                  CTaskComplexPlayHandSignalAnim, CTaskComplexWanderStandard,
                  CDecisionMaker, CPedIntelligence
    Source: examples/HandSignals/Main.cpp, examples/DecisionMaker/Main.cpp
*/
#include <plugin.h>
#include "CCivilianPed.h"
#include "CDecisionMakerTypes.h"
#include "CStreaming.h"
#include "CTaskComplexPlayHandSignalAnim.h"
#include "CTaskComplexWanderStandard.h"
#include "CTimer.h"
#include "CWorld.h"

using namespace plugin;

class TaskDemo {
public:
    TaskDemo() {
        // --- Assign secondary task: hand signal animation ---
        Events::gameProcessEvent += [] {
            static int lastTime = 0;
            CPed* player = FindPlayerPed();
            if (!player || !player->IsAlive()) return;
            if (CTimer::m_snTimeInMilliseconds - lastTime < 500) return;

            if (KeyPressed(0x31)) { // 1
                lastTime = CTimer::m_snTimeInMilliseconds;
                // SetTaskSecondary(task, slot)
                // slot 4 = TASK_SECONDARY_PARTIAL_ANIM
                player->m_pIntelligence->m_TaskMgr.SetTaskSecondary(
                    new CTaskComplexPlayHandSignalAnim(320, 4.0f),
                    4
                );
            }
        };

        // --- Assign primary task: make ped wander ---
        Events::pedCtorEvent += [](CPed* ped) {
            if (!ped->m_pIntelligence) return;
            // TASK_PRIMARY_DEFAULT = slot 4
            ped->m_pIntelligence->m_TaskMgr.SetTask(
                new CTaskComplexWanderStandard(PEDMOVE_WALK, 0, true),
                TASK_PRIMARY_DEFAULT
            );
        };
    }
} taskDemo;

/*
  TASK MANAGER SLOTS:
  Primary tasks (ePrimaryTasks):
    TASK_PRIMARY_PHYSICAL_RESPONSE    = 0  (hit reactions, ragdoll)
    TASK_PRIMARY_EVENT_RESPONSE_TEMP  = 1  (immediate reactions)
    TASK_PRIMARY_EVENT_RESPONSE_NONTEMP = 2 (sustained reactions)
    TASK_PRIMARY_PRIMARY              = 3  (mission action)
    TASK_PRIMARY_DEFAULT              = 4  (idle / wander)

  Secondary tasks (eSecondaryTasks):
    TASK_SECONDARY_ATTACK      = 0
    TASK_SECONDARY_DUCK        = 1
    TASK_SECONDARY_SAY         = 2
    TASK_SECONDARY_FACIAL_COMPLEX = 3
    TASK_SECONDARY_PARTIAL_ANIM = 4
    TASK_SECONDARY_IK          = 5

  API:
    ped->m_pIntelligence->m_TaskMgr.SetTask(CTask*, int primarySlot)
    ped->m_pIntelligence->m_TaskMgr.SetTaskSecondary(CTask*, int secondarySlot)
    ped->m_pIntelligence->m_TaskMgr.GetSimplestActiveTask() -> CTask*
    ped->m_pIntelligence->SetPedDecisionMakerType(eDecisionMakerType)

  DECISION MAKER:
    CDecisionMakerTypes* dm = CDecisionMakerTypes::GetInstance();
    eDecisionMakerType handle = dm->AddDecisionMaker(&templateDm);  // max 10 custom DMs
    dm->AddEventResponse(handle, eEventType::EVENT_GUN_AIMED_AT,
        eTaskType::TASK_SIMPLE_HANDS_UP,
        DecisionChances(4, 4, 4, 4),     // weight / 5 chance per ped variation
        DecisionContext(true, false)       // (inCar, armed)
    );
    ped->m_pIntelligence->SetPedDecisionMakerType(handle);

  COMMON TASK TYPES:
    CTaskComplexWanderStandard(eMoveState, heading, clockwise)
    CTaskComplexKillPedOnFoot(CPed* target)
    CTaskComplexDie(eWeaponType, animGroup, animId)
    CTaskComplexEnterCarAsDriver(CVehicle*)
    CTaskComplexLeaveCar(CVehicle*, int exitSide, int, bool, bool)
    CTaskSimpleFight(CPed* target, int animId, unsigned int duration)
    CTaskSimpleUseGun(CPed* target, CVector, eWeaponType, int, ...)
    CTaskSimpleHandsUp(unsigned int duration)
    CTaskSimpleDuck(unsigned int duration)
    CTaskSimpleJetPack()
    CTaskComplexJump(bool isRunning)
    CTaskComplexClimb(CEntity*, CVector, float, int, bool)
    CTaskSimpleRunNamedAnim(const char* animGroup, const char* animName, float blendIn, float blendOut, int, bool, bool, bool, float, bool)
*/
