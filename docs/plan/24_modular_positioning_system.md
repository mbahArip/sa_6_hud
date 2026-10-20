# 24 - Modular Positioning System (Group & Free Layout)

## Objective
Implement a unified, robust layout positioning system for all 6HUD modules with two selectable modes:
1. **Group Stacking Mode** (Default): Automatic corner-anchored stack (`minimap`, `status`, `criminal`, `notification`) with automatic stack collapse when modules are hidden or set to Free mode.
2. **Free Positioning Mode**: Arbitrary screen positioning using independent screen anchor (`pos_anchor`), element pivot (`element_anchor`), and pixel offsets (`offset_x`, `offset_y`).

Also provide an in-game **Layout** tab in the settings menu (`Menu.cpp`) with collapsibles, per-module layout resets, live sliders, and a full mock preview mode.

---

## 1. Supported Modules & Default Layout
| Module ID | Display Name | Default Group | Default Index | Alignment Corner |
|---|---|---|---|---|
| `radar` | Radar / Minimap | `minimap` | 0 | Bottom-Left (Lowest) |
| `gps_nav` | GPS Navigation Card | `minimap` | 1 | Bottom-Left (Above Radar) |
| `place_info` | Place Information Card | `minimap` | 2 | Bottom-Left (Above GPS) |
| `vehicle_info` | Vehicle Information Card | `minimap` | 3 | Bottom-Left (Above Place) |
| `health_armor` | Health & Armor Bar | `status` | 0 | Top-Left (Highest) |
| `wanted` | Wanted Stars & Investigation | `criminal` | 0 | Top-Right (Highest) |
| `weapon_hud` | Weapon & Ammo HUD | `criminal` | 1 | Top-Right (Below Wanted) |
| `money_hud` | Money Counter HUD | `criminal` | 2 | Top-Right (Below Weapon) |

---

## 2. Configuration & Data Structures
Each module receives a `ModulePositionConfig` in `Config.h`:
```cpp
enum class PositionMode {
    Group = 0,
    Free = 1
};

enum class AnchorPoint {
    TopLeft = 0,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

enum class HudGroup {
    Minimap = 0,     // Bottom-Left, stacks upward
    Status,          // Top-Left, stacks downward
    Criminal,        // Top-Right, stacks downward
    Notification     // Bottom-Right, stacks upward
};

struct ModulePositionConfig {
    PositionMode mode          = PositionMode::Group;
    HudGroup     group         = HudGroup::Minimap;
    int          groupIndex    = 0;
    AnchorPoint  posAnchor     = AnchorPoint::BottomLeft;
    AnchorPoint  elementAnchor = AnchorPoint::BottomLeft;
    float        offsetX       = 0.0f; // + right, - left
    float        offsetY       = 0.0f; // + up, - down
};
```

INI format per module:
```ini
[layout.<module_name>]
position_mode = group       # 'group' or 'free'
group = minimap             # 'minimap', 'status', 'criminal', 'notification'
group_index = 0             # 0, 1, 2, ...
pos_anchor = bottom-left    # 'top-left', 'top-center', 'top-right', 'center-left', 'center', 'center-right', 'bottom-left', 'bottom-center', 'bottom-right'
element_anchor = bottom-left# 'top-left', 'top-center', 'top-right', 'center-left', 'center', 'center-right', 'bottom-left', 'bottom-center', 'bottom-right'
offset_x = 0.0              # screen pixel offset (+ right, - left)
offset_y = 0.0              # screen pixel offset (+ up, - down)
```

---

## 3. Positioning Math Specifications

### Free Mode Math
1. **Screen Anchor Point**:
   ```cpp
   float ax = ScreenW() * posAnchorFx;
   float ay = ScreenH() * posAnchorFy;
   ```
2. **Target Coords (Inverted Y for standard +up convention)**:
   ```cpp
   float targetX = ax + offsetX;
   float targetY = ay - offsetY; // Inverted because Screen Y goes downward
   ```
3. **Element Pivot Coords**:
   ```cpp
   float left = targetX - (elemWidth * elemAnchorFx);
   float top  = targetY - (elemHeight * elemAnchorFy);
   CRect rect(left, top, left + elemWidth, top + elemHeight);
   ```

### Group Mode Math & Stack Collapse
1. Filter only active visible elements assigned to `group` in `PositionMode::Group`.
2. Sort elements ascending by `groupIndex`.
3. Stack calculations:
   - **`minimap` & `notification` (Bottom Groups)**:
     - Starting baseline: `curBottom = ScreenH() - marginY`.
     - For each element in order:
       - `elemTop = curBottom - elemHeight`.
       - Element rect resolved at `(left, elemTop, right, curBottom)`.
       - Advance: `curBottom = elemTop - gap`.
   - **`status` & `criminal` (Top Groups)**:
     - Starting baseline: `curTop = marginY`.
     - For each element in order:
       - `elemBottom = curTop + elemHeight`.
       - Element rect resolved at `(left, curTop, right, elemBottom)`.
       - Advance: `curTop = elemBottom + gap`.

If any element is switched to `Free` mode or hidden, it is excluded from the group list and remaining elements collapse automatically to fill the space.

---

## 4. Settings Menu Integration (`Menu.cpp`)
1. Add new **Layout** tab to `sa_ui::Tabs`.
2. Display 8 collapsible sections (`sa_ui::Collapsible`), one for each module.
3. Inside each collapsible:
   - `Position Mode`: Stepper `[ < Group | Free > ]`.
   - In Group mode: `Group` selector (`Minimap`, `Status`, `Criminal`, `Notification`) and `Group Index` stepper.
   - In Free mode: `Screen Anchor` (9 options), `Element Anchor` (9 options), `Offset X` slider (`[-ScreenW, ScreenW]`), and `Offset Y` slider (`[-ScreenH, ScreenH]`).
   - `Reset Layout` button restoring that specific module's default config.
4. **Layout Preview Mode**:
   - While `s_activeTab == TAB_LAYOUT`, mock data is activated so all 8 modules are continuously visible on screen, allowing live visual tuning as sliders and anchors are adjusted.

---

## 5. Verification & Testing Steps
1. Build Win32 Release binary (`MSBuild.exe 6hud.vcxproj /p:Configuration=Release /p:Platform=Win32`).
2. Verify game launch without layout drift on default settings (0-pixel change from current HUD).
3. Open menu (`/`), switch to Layout tab, verify mock preview displays all elements.
4. Switch Radar to Free mode; verify GPS Nav, Place Info, and Vehicle Info collapse smoothly to bottom-left screen edge.
5. Move Health bar to `criminal` group; verify it stacks cleanly in top-right corner.
6. Test Free mode anchor combinations: Center-Center, Top-Right with padding, Bottom-Center.
7. Save INI, close and restart game, verify INI reload preserves layout overrides.
