# 25 - Criminal Profile Mask Config & Safe Vehicle Handles

## Objective
Enhance Criminal Profile (`Wanted.Extension.CriminalProfile`) with:
1. **Configurable Mask & Disguise Model Names**: Move hardcoded balaclava/mask detection to INI-driven list (`MASK_MODEL_NAMES` and `DISGUISE_MODEL_NAMES`) parsed via `CKeyGen::GetUppercaseKey`.
2. **Safe Vehicle Pool Handles**: Store `int vehicleHandle` via `CPools::GetVehicleRef` in `HotVehicleSignature` and resolve via `CPools::GetVehicle` to eliminate dangling pointer crashes when vehicles despawn.
3. **Cop Scanning Spatial Optimization**: Early Manhattan/AABB distance rejection in `ProcessCopScanning` before squared-magnitude distance and line-of-sight checks.

---

## 1. INI Schema & Config
Under `[Wanted.Extension.CriminalProfile]`:
```ini
MASK_MODEL_NAMES = bandmask, hockeymask, helmet, moto, bikerhelmet, zorromask, balaclava
DISGUISE_MODEL_NAMES = grouchos, glasses01, glasses03, glasses04, glasses05
```

Data structures in `Config.h`:
```cpp
std::string maskModelNames     = "bandmask, hockeymask, helmet, moto, bikerhelmet, zorromask, balaclava";
std::string disguiseModelNames = "grouchos, glasses01, glasses03, glasses04, glasses05";
std::vector<unsigned int> maskModelKeys;
std::vector<unsigned int> disguiseModelKeys;
```

---

## 2. Mask & Disguise Detection
In `RadarHeatCore.inc`:
- `IsPlayerWearingMask(player)`:
  - Check `desc->GetIsWearingBalaclava()`.
  - Check `desc->m_anModelKeys[CLOTHES_MODEL_GLASSES]`, `CLOTHES_MODEL_HATS`, and `CLOTHES_MODEL_SPECIAL` against `g_config.maskModelKeys`.
- `IsPlayerDisguised(player)`:
  - Check `desc->m_anModelKeys[CLOTHES_MODEL_GLASSES]` and `CLOTHES_MODEL_HATS` against `g_config.disguiseModelKeys`.
  - Fallback to generic hat/glasses texture check.

---

## 3. Vehicle Pool Handle Safety
In `HotVehicleSignature`:
```cpp
struct HotVehicleSignature {
    bool          active = false;
    int           vehicleHandle = 0; // SCM handle from CPools::GetVehicleRef
    int           modelIndex = 0;
    unsigned char color1 = 0;
    unsigned char color2 = 0;
    RwTexture*    customPlateTex = nullptr;
};
```
- When adding hot vehicle: `vehicleHandle = CPools::GetVehicleRef(veh)`.
- When checking vehicle:
  ```cpp
  CVehicle* poolVeh = CPools::GetVehicle(g_hotVehicles[i].vehicleHandle);
  bool sameVehicle = (poolVeh && poolVeh == veh);
  ```

---

## 4. Cop Scanning Optimization
In `ProcessCopScanning`:
- Bounding box check before distance calculation:
  ```cpp
  float dx = fabsf(copPos.x - pPos.x);
  float dy = fabsf(copPos.y - pPos.y);
  if (dx > scanDist || dy > scanDist) continue;
  ```
- Same early rejection for vehicle scan pass.
