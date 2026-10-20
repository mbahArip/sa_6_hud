# Injector API Reference

Source: `injector/injector.hpp`

## Memory types

```cpp
injector::memory_pointer          // smart pointer, supports ASLR translation
injector::memory_pointer_raw      // raw address, no translation
injector::memory_pointer_aslr     // always ASLR-translated
```

## Memory read/write

```cpp
// Typed read/write
injector::WriteMemory<T>(address, value, unprotect = true);
injector::ReadMemory<T>(address, unprotect = false) -> T;

// Raw bytes
injector::WriteMemoryRaw(address, void* data, size_t size, unprotect = true);
injector::ReadMemoryRaw(address, void* out, size_t size, unprotect = false);
injector::MemoryFill(address, uint8_t byte, size_t size, unprotect = true);
```

## Code patching

```cpp
injector::MakeJMP(address, dest, nopRest = true);          // E9 rel32
injector::MakeCALL(address, dest, nopRest = true);         // E8 rel32
injector::MakeNOP(address, count = 1);                     // 0x90 * count
injector::MakeRangedNOP(start, end);                       // NOP [start, end)
injector::MakeRET(address, pop = 0);                       // C3 or C2 imm16
injector::MakeJA(address, dest, nopRest = true);           // 77 rel8 (JA short)

// Offset helpers
injector::GetBranchDestination(address) -> memory_pointer; // reads rel32 target
injector::ReadRelativeOffset(address) -> int32_t;
injector::MakeRelativeOffset(address, dest) -> int32_t;

// Pointer adjustment (for embedded pointer relocations)
injector::AdjustPointer(address, offset);
```

## Memory protection

```cpp
injector::ProtectMemory(address, size, allowWrite);
injector::UnprotectMemory(address, size);

// RAII unprotect scope
{
    injector::scoped_unprotect<T> guard(address);
    // memory is writable here
}
```

## Lazy accessors (cached singletons)

```cpp
// Cached void* at address
injector::lazy_pointer<0xADDRESS> ptr;
void* raw = ptr.get();

// Cached object reference at address
injector::lazy_object<0xADDRESS, SomeType> obj;
SomeType& ref = obj.get();
```

## MakeInline (mid-function hook)

Used internally by `patch::StaticHook`. Can be used directly:

```cpp
struct MyHook {
    void operator()(injector::reg_pack& regs) {
        // read/write registers here
        // regs.eax, regs.ecx, regs.edx, regs.ebx
        // regs.esi, regs.edi, regs.ebp, regs.esp
    }
};
static MyHook hook;
injector::MakeInline<MyHook>(start, end);  // end defaults to start+5
injector::MakeInline(start, end, hook);    // pass instance
```

## Game version detection

```cpp
injector::game_version_manager gvm;
gvm.Detect();

// SA predicates
gvm.IsSA10US()   // 1.0 US Compact (entry == 0x824570)
gvm.IsSA10USH()  // 1.0 US Hoodlum
gvm.IsSA10EU()
gvm.IsSA11US()
gvm.IsSA11EU()
gvm.IsSASteam()
```
