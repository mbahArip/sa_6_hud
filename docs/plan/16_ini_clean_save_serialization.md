# 16 - INI Clean Save Serialization

## Objective
Prevent in-game menu and config serialization from saving developer/internal noise sections (`[debug]`, `[cop_ai]`, and `[menu]`) into `6hud.ini`.

## Changes
1. **SaveConfig Cleanup**:
   - Remove serialization of `[debug]`, `[cop_ai]`, and `[menu]` keys in `src/Config.cpp`.
   - Purge these legacy sections from existing INI on save via `WritePrivateProfileStringA(section, NULL, NULL, path)`.
2. **INI File Pruning**:
   - Strip `[debug]`, `[cop_ai]`, and `[menu]` sections from `6hud.ini`.
