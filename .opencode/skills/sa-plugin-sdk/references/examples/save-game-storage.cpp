/*
    GTA SA Plugin SDK — Custom Save Game & Stats Persistence
    Demonstrates: CGenericGameStorage, companion sidecar file per slot,
                  extracting save slot index & checksum,
                  hooking save/load routines, handling New Game resets.
    Target: GTA SA 1.0 US Compact (0x400000)
*/
#include <plugin.h>
#include "CGenericGameStorage.h"
#include <fstream>
#include <string>

using namespace plugin;

struct CustomModStats {
    int   killCount     = 0;
    float driftDistance = 0.0f;
    int   driftScore    = 0;

    void Reset() {
        killCount     = 0;
        driftDistance = 0.0f;
        driftScore    = 0;
    }
} gModStats;

// Extract slot number 1-8 from "GTASAsf1.b"
int GetActiveSaveSlot() {
    const char* filename = CGenericGameStorage::ms_LoadFileName; // e.g. "GTASAsf2.b"
    if (filename && strstr(filename, "GTASAsf")) {
        char digit = filename[7];
        if (digit >= '1' && digit <= '8') {
            return digit - '0';
        }
    }
    return 1; // Default fallback slot
}

// Build sidecar file path: "mod_stats/slot_<N>.dat"
std::string GetSlotFilePath(int slot) {
    char buf[MAX_PATH];
    sprintf_s(buf, "mod_stats\\slot_%d.dat", slot);
    return std::string(buf);
}

void SaveCustomStats(int slot) {
    CreateDirectoryA("mod_stats", nullptr);
    std::string path = GetSlotFilePath(slot);

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return;

    // Optional: write checksum or timestamp header for integrity verification
    int checkSum = CGenericGameStorage::ms_CheckSum;
    file.write(reinterpret_cast<const char*>(&checkSum), sizeof(checkSum));
    file.write(reinterpret_cast<const char*>(&gModStats), sizeof(gModStats));
}

void LoadCustomStats(int slot) {
    std::string path = GetSlotFilePath(slot);

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        gModStats.Reset(); // File doesn't exist yet for this slot
        return;
    }

    int checkSum = 0;
    file.read(reinterpret_cast<char*>(&checkSum), sizeof(checkSum));
    file.read(reinterpret_cast<char*>(&gModStats), sizeof(gModStats));
}

class SaveGamePersistenceDemo {
public:
    SaveGamePersistenceDemo() {
        // --- 1. Hook Game Save ---
        // 0x619081: Call CGenericGameStorage::GenericSave(int slot)
        patch::RedirectCall(0x619081, [](int slot) -> bool {
            bool success = CGenericGameStorage::GenericSave(slot);
            if (success) {
                // Game save succeeded, flush companion data
                int slotNumber = (slot >= 0 && slot < 8) ? (slot + 1) : GetActiveSaveSlot();
                SaveCustomStats(slotNumber);
            }
            return success;
        });

        // --- 2. Hook Game Load ---
        // 0x53C70B: Call CGenericGameStorage::GenericLoad(bool* flag)
        patch::RedirectCall(0x53C70B, [](bool* flag) -> bool {
            bool success = CGenericGameStorage::GenericLoad(flag);
            if (success) {
                // Game load succeeded, load companion data for active slot
                int slotNumber = GetActiveSaveSlot();
                LoadCustomStats(slotNumber);
            }
            return success;
        });

        // --- 3. Handle New Game / Restart ---
        // Starting New Game resets campaign without triggering GenericLoad
        Events::restartGameEvent += [] {
            gModStats.Reset();
        };
    }
} saveGameDemo;
