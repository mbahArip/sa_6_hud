/*
    GTA SA Plugin SDK — Memory Patching Examples
    Demonstrates: patch::Nop, RedirectCall, RedirectJump, Set, StaticHook,
                  ADDRESS_BY_VERSION, MakeInline register hooks
*/
#include <plugin.h>

using namespace plugin;

class PatchExamples {
public:
    PatchExamples() {
        // --- NOP out 6 bytes at address ---
        patch::Nop(0x6B2114, 6);

        // --- Redirect call at address to our function ---
        patch::RedirectCall(0x6A3EA0, &MyNitrousFunc);

        // --- Redirect jump ---
        patch::RedirectJump(0x6D076C, &MyAccMultiplier);

        // --- Write typed value ---
        patch::SetFloat(0x8D36C0, 999.0f);  // e.g. max wanted radius
        patch::SetInt(0x8A5A90, 0);          // e.g. disable something
        patch::SetUChar(0x6D6065, 0x75);     // patch conditional jump byte

        // --- Read value ---
        float val = patch::GetFloat(0x8D36C0);

        // --- Generic typed write ---
        patch::Set<uint32_t>(0x8D36C0, 0xDEADBEEF);

        // --- Batch patch multiple sites ---
        patch::Nop({0x6B2114, 0x6B2220, 0x6B2330}, 6);
        patch::SetFloat({0x8D36C0, 0x8D36C4}, 0.0f);

        // --- Mid-function register hook ---
        // Hooks at single 5-byte site, saves/restores all registers
        patch::StaticHook(0x6D076C, [](patch::RegPack& regs) {
            // Read/write x86 registers
            float acc = *reinterpret_cast<float*>(regs.eax + 0x1C);
            acc *= 2.0f;
            *reinterpret_cast<float*>(regs.eax + 0x1C) = acc;
        });

        // --- Ranged hook (start..end, must be >= 5 bytes) ---
        patch::StaticHook(0x6D076C, 0x6D0776, [](patch::RegPack& regs) {
            regs.eax = 42;
        });

        // --- Multi-version address ---
        // ADDRESS_BY_VERSION(Compact, Hoodlum, 1.0EU, 1.1US, 1.1EU, Steam)
        uintptr_t addr = ADDRESS_BY_VERSION(0x6A3EA0, 0x6A3EA0, 0x6A3F10, 0x6A4050, 0x6A40B0, 0x6A3F80);
        patch::RedirectJump(addr, &MyNitrousFunc);

        // --- Translate existing call/jump target ---
        auto* existingFunc = patch::TranslateCallOffset<void(*)()>(0x53E981);

        // --- Put RETN ---
        patch::PutRetn(0x6A3EA0);         // just RET
        patch::PutRetn(0x6A3EA0, 4);      // RETN 4 (pops 4 bytes)
        patch::PutRetn1(0x6A3EA0);        // MOV EAX,1 + RET (stub returning true)
        patch::PutRetn0(0x6A3EA0);        // MOV EAX,0 + RET
    }

    static void MyNitrousFunc() {}
    static void MyAccMultiplier() {}
} patchExamples;

/*
  CALLING GAME FUNCTIONS:

  // __cdecl
  plugin::Call<0x53E981>();
  float result = plugin::CallAndReturn<float, 0x56E0A0>(arg1, arg2);

  // __thiscall (member)
  plugin::CallMethod<0x5E5560, CPed>(playerPed, weaponType, ammo, true);
  bool ok = plugin::CallMethodAndReturn<bool, 0x6D5B40, CVehicle>(vehicle);

  // vtable
  plugin::CallVirtualMethod<0, CEntity>(entity);  // first vtable slot

  // Dynamic address
  plugin::CallDyn(runtimeAddr, arg1);
  plugin::CallMethodDyn(runtimeAddr, thisPtr, arg1);

  // via meta (preferred — symbolic)
  plugin::Call<addrof(CStreaming::RequestModel)>(modelId, flags);
  plugin::CallMethod<addrof(CPed::GiveWeapon), CPed>(ped, weaponType, ammo, true);
*/
