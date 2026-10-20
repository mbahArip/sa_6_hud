# Patch & Call API Reference

Source: `shared/Patch.h`, `shared/PluginBase.h`

## plugin::patch — static methods

```cpp
// NOP
patch::Nop(uintptr_t address, size_t size, bool vp = true);
patch::NopRestore(uintptr_t address, bool vp = true);  // restores original bytes

// Jumps / Calls
patch::RedirectCall(uintptr_t address, memory_pointer_raw func, bool vp = true);
patch::RedirectJump(uintptr_t address, memory_pointer_raw func, bool vp = true);
patch::RedirectShortJump(uintptr_t address, memory_pointer_raw dest, bool vp = true);

// Write typed values
patch::SetChar(uintptr_t address, char value, bool vp = true);
patch::SetUChar(uintptr_t address, unsigned char value, bool vp = true);
patch::SetShort(uintptr_t address, short value, bool vp = true);
patch::SetUShort(uintptr_t address, unsigned short value, bool vp = true);
patch::SetInt(uintptr_t address, uintptr_t value, bool vp = true);
patch::SetUInt(uintptr_t address, uintptr_t value, bool vp = true);
patch::SetFloat(uintptr_t address, float value, bool vp = true);
patch::SetPointer(uintptr_t address, memory_pointer_raw value, bool vp = true);
patch::SetRaw(uintptr_t address, void* value, size_t size, bool vp = true);

// Read typed values
patch::GetChar(uintptr_t address, bool vp = true) -> char;
patch::GetUChar(...) -> unsigned char;
patch::GetShort(...) -> short;
patch::GetUShort(...) -> unsigned short;
patch::GetInt(...) -> uintptr_t;
patch::GetUInt(...) -> uintptr_t;
patch::GetFloat(...) -> float;
patch::GetPointer(...) -> void*;
patch::GetRaw(uintptr_t address, void* ret, size_t size, bool vp = true);

// Generic typed read/write (template)
patch::Set<T>(uintptr_t address, T value, bool vp = true);
patch::Get<T>(uintptr_t address, bool vp = true) -> T;

// Return patches
patch::PutRetn(uintptr_t address, unsigned short bytesToPop = 0, bool vp = true);  // RETN / RETN imm16
patch::PutRetn0(uintptr_t address, unsigned short bytesToPop = 0, bool vp = true); // MOV EAX,0 + RETN
patch::PutRetn1(uintptr_t address, unsigned short bytesToPop = 0, bool vp = true); // MOV EAX,1 + RETN

// Function replacement
patch::ReplaceFunction(uintptr_t address, void* func, bool vp = true);
patch::ReplaceFunctionCall(uintptr_t address, void* func, bool vp = true);

// Mid-function register hook (wraps injector::MakeInline)
using RegPack = injector::reg_pack;
patch::StaticHook(uintptr_t start, uintptr_t end, void (*func)(RegPack&));
patch::StaticHook(uintptr_t address, void (*func)(RegPack&));  // uses address..address+5
patch::StaticHook<FunctorType>(uintptr_t start, uintptr_t end); // functor overload

// Offset translation
patch::TranslateCallOffset<T>(uintptr_t address) -> T;  // reads E8 rel32
patch::TranslateJumpOffset<T>(uintptr_t address) -> T;  // reads E9 rel32

// Batch overloads (vector of addresses)
patch::RedirectCall(vector<uintptr_t> const& addresses, func, vp);
patch::RedirectJump(vector<uintptr_t> const& addresses, func, vp);
patch::Nop(vector<uintptr_t> const& addresses, size_t size, vp);
patch::SetInt(vector<uintptr_t> const& addresses, uintptr_t value, vp);
patch::SetFloat(vector<uintptr_t> const& addresses, float value, vp);
patch::SetPointer(vector<uintptr_t> const& addresses, memory_pointer_raw value, vp);
patch::Set<T>(vector<uintptr_t> const& addresses, T value, vp);

// vtable override macro (use inside a method: overrides single vtable slot)
_override(index, classname, func);
```

## plugin:: — call templates

```cpp
// __cdecl
Call<address>(args...);
CallAndReturn<Ret, address>(args...) -> Ret;

// __stdcall
CallStd<address>(args...);
CallStdAndReturn<Ret, address>(args...) -> Ret;

// __thiscall (methods)
CallMethod<address, C>(thisPtr, args...);
CallMethodAndReturn<Ret, address, C>(thisPtr, args...) -> Ret;

// vtable dispatch
CallVirtualMethod<vtableIndex, C>(thisPtr, args...);
CallVirtualMethodAndReturn<Ret, vtableIndex, C>(thisPtr, args...) -> Ret;

// Dynamic address (runtime, translated via GetGlobalAddress)
CallDyn(uintptr_t address, args...);
CallAndReturnDyn<Ret>(uintptr_t address, args...) -> Ret;
CallStdDyn(uintptr_t address, args...);
CallStdAndReturnDyn<Ret>(uintptr_t address, args...) -> Ret;
CallMethodDyn(uintptr_t address, C thisPtr, args...);
CallMethodAndReturnDyn<Ret>(uintptr_t address, C thisPtr, args...) -> Ret;

// Dynamic global (no GetGlobalAddress translation)
CallDynGlobal(uintptr_t address, args...);
CallAndReturnDynGlobal<Ret>(uintptr_t address, args...) -> Ret;
CallMethodDynGlobal(uintptr_t address, C thisPtr, args...);
CallMethodAndReturnDynGlobal<Ret>(uintptr_t address, C thisPtr, args...) -> Ret;
```

## Address macros

```cpp
// All from PluginBase.h

addrof(func)                   // SA 1.0 US Compact address (compile-time)
gaddrof(func)                  // runtime-translated global address
argsof(func)                   // args_t type alias
refsof(func)                   // refs_t type alias
idof(func)                     // function id

addrof_o(func, decl)           // overloaded variants
gaddrof_o(func, decl)

ctor_addr(ClassName)           // constructor address
dtor_addr(ClassName)           // destructor address
del_dtor_addr(ClassName)       // deleting destructor address
op_new_addr(ClassName)         // operator new address
op_delete_addr(ClassName)      // operator delete address
vtableid(ClassName)            // vtable address

// Multi-version address selector
// SA: 6 args (Compact, Hoodlum, 1.0EU, 1.1US, 1.1EU, Steam)
ADDRESS_BY_VERSION(a, b, c, d, e, f)        // returns uintptr_t for current version
GLOBAL_ADDRESS_BY_VERSION(a, b, c, d, e, f) // same + GetGlobalAddress()

// Lambda cast helper
LAMBDA(Ret, Conv, Func, ...)   // e.g. LAMBDA(void, __cdecl, &MyFunc)
```

## injector::reg_pack (for StaticHook)

```cpp
struct reg_pack {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  // pushed order
    uint32_t eip;  // return address
};
// Access: regs.eax, regs.ecx, regs.edx, etc.
```
