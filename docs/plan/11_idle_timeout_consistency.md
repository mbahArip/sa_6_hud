# 11 - Idle and Hold Timeout Consistency

## Objective
Unify all temporary HUD element timers across the codebase to adhere to `[Global]` `UI_IDLE_TIMEOUT` (`g_config.uiIdleTimeout`), removing hardcoded legacy 8.0s values.

## Audit Findings & Changes
1. **Vehicle Name & Class Card**:
   - Previously: Hardcoded `8.0f` seconds (`location_duration` / `vehicle_name_duration`).
   - Now: Inherits `g_config.uiIdleTimeout` (default `4.00s`). Optional override via `[Minimap.VehicleInformation]` `VEHICLE_INFO_DURATION`.
2. **Location / Zone Card**:
   - Previously: Hardcoded `8.0f` seconds.
   - Now: Inherits `g_config.uiIdleTimeout` (default `4.00s`). Optional override via `[Minimap.PlaceInformation]` `PLACE_INFO_DURATION`.
3. **Weapon HUD**:
   - `s_weaponIdleTimer` initialized to `0.0f` and set to `g_config.uiIdleTimeout` on weapon switch, aiming, firing, or ammo change.
4. **Money HUD**:
   - `s_moneyIdleTimer` initialized to `0.0f` and set to `g_config.uiIdleTimeout` on money delta.
5. **HUD Key Hold/Tap**:
   - `s_hudKeyHoldTimer` triggers `g_config.uiIdleTimeout` on key release/tap.
