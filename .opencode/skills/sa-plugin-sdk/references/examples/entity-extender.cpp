/*
    GTA SA Plugin SDK — Entity Extender (per-entity plugin data)
    Demonstrates: VehicleExtendedData<T>, PedExtendedData<T>
    Source: examples/VehicleRemap/Main.cpp, shared/extender/

    Extenders attach plugin-owned data to game entities without modifying
    game class layouts. Constructor receives entity pointer.
    Lifecycle hooks (initPools, ctor, dtor) are injected automatically.
*/
#include <plugin.h>
#include "CGeneral.h"
#include "CModelInfo.h"

using namespace plugin;

// --- Example: track per-vehicle state ---

struct VehicleData {
    bool hasBeenMod;
    int spawnTime;

    // Constructor receives CVehicle* (available when vehicle is created)
    VehicleData(CVehicle* vehicle) {
        hasBeenMod = false;
        spawnTime = 0;
    }
};

struct PedData {
    int killCount;
    float damageTaken;

    PedData(CPed* ped) : killCount(0), damageTaken(0.0f) {}
};

class ExtenderDemo {
public:
    ExtenderDemo() {
        // Declare extenders as static — one instance per plugin, never stack
        static VehicleExtendedData<VehicleData> vehicleData;
        static PedExtendedData<PedData> pedData;

        // Access extended data in vehicle events
        Events::vehicleRenderEvent += [](CVehicle* vehicle) {
            VehicleData& data = vehicleData.Get(vehicle);

            if (!data.hasBeenMod && vehicle->m_nModelIndex > 399 && vehicle->m_nModelIndex < 612) {
                // Apply remap on first render
                auto* mi = reinterpret_cast<CVehicleModelInfo*>(
                    CModelInfo::ms_modelInfoPtrs[vehicle->m_nModelIndex]);
                unsigned int count = mi->GetNumRemaps();
                if (count > 0) {
                    unsigned int remap = CGeneral::GetRandomNumberInRange(0, count);
                    vehicle->SetRemap(remap);
                }
                data.hasBeenMod = true;
            }
        };

        // Access extended data in ped events
        Events::pedCtorEvent += [](CPed* ped) {
            PedData& data = pedData.Get(ped);
            data.killCount = 0;
            data.damageTaken = 0.0f;
        };

        Events::gameProcessEvent += [] {
            // Iterate all peds and access their data
            for (auto* ped : CPools::ms_pPedPool) {
                PedData& data = pedData.Get(ped);
                (void)data.killCount;  // use it
            }
        };
    }
} extenderDemo;

/*
  NOTES:
  - T must be constructible from entity pointer: T(CVehicle*), T(CPed*), T(CObject*)
  - extender.Get(ptr) returns T& — valid only while entity is alive
  - Extender auto-hooks: initPoolsEvent (allocate), ctorEvent (new T), dtorEvent (delete T)
  - ObjectExtendedData<T> works identically for CObject*
*/
