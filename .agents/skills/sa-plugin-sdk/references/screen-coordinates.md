# Screen Coordinates & UI Scaling Reference (GTA SA & Plugin-SDK)

## Core Philosophy: Design in 1080p, Auto-Scale Anywhere

Plugin-SDK standardizes 2D coordinates to a **1920 × 1080** virtual canvas (`PLUGIN_SCREEN_RESOLUTION_DEFAULT = 1080.0f`).
Design your UI in Figma, Photoshop, or Illustrator at `1920x1080` and use exact pixel numbers wrapped in `SCREEN_COORD`.

---

## 3 Coordinate Layers in GTA SA

| Layer | Coordinate System | Base Resolution | Used By | Formula |
|---|---|---|---|---|
| **Vanilla Engine** | Virtual 4:3 canvas | `640.0 × 448.0` | `CFont::SetScale`, vanilla `CHud`, CLEO/SCM | `Pixel = Coord * (RsGlobal.maximumWidth / 640.0f)` |
| **RenderWare / D3D9** | Physical screen pixels | `0.0` to `RsGlobal.maximumWidth / Height` | `RwIm2DVertex`, `CSprite2d` raw coords | Physical hardware pixels |
| **Plugin-SDK** | Height-relative virtual px | `1920 × 1080` base (`1080p`) | `SCREEN_COORD`, `SCREEN_RECT`, modern mods | `Pixel = Value * (ScreenHeight / 1080.0f)` |

---

## Scaling Formula & Aspect Ratios

Plugin-SDK binds scaling to **vertical screen height** (`Screen.cpp`):
$$\text{ScaleFactor} = \frac{\min(\text{ScreenWidth}, \text{ScreenHeight})}{1080.0f} = \frac{\text{ScreenHeight}}{1080.0f} \quad (\text{for landscape monitors})$$

### Behavior across resolutions:
- **1920 × 1080 (16:9):** `Scale = 1.0` (1 Figma px = 1 screen px)
- **1366 × 768 (16:9):** `Scale = 768 / 1080 = 0.711` (scales down cleanly)
- **2560 × 1440 (16:9):** `Scale = 1440 / 1080 = 1.333` (scales up cleanly)
- **3840 × 2160 (16:9 4K):** `Scale = 2160 / 1080 = 2.0` (doubles pixels cleanly)
- **2560 × 1080 (21:9 Ultrawide):** `Scale = 1080 / 1080 = 1.0` (keeps exact physical size, no wide stretch!)

---

## Plugin-SDK Macro Reference (`shared/extensions/Screen.h`)

### 1. Element Sizing (Width, Height, Padding, Radius)
Always use `SCREEN_COORD`:
```cpp
float width   = SCREEN_COORD(128.0f); // 128px in Figma
float height  = SCREEN_COORD(64.0f);  // 64px in Figma
float padding = SCREEN_COORD(12.0f);  // 12px in Figma
```

### 2. Edge Anchors (Corner HUD Elements)
Never use fixed X/Y for right or bottom elements. Anchor from screen edges:

| Macro | Description | Formula |
|---|---|---|
| `SCREEN_COORD_LEFT(x)` | Offset from left edge | `GetCoord(x)` |
| `SCREEN_COORD_RIGHT(x)` | Offset from right edge | `ScreenWidth - GetCoord(x)` |
| `SCREEN_COORD_TOP(y)` | Offset from top edge | `GetCoord(y)` |
| `SCREEN_COORD_BOTTOM(y)` | Offset from bottom edge | `ScreenHeight - GetCoord(y)` |

### 3. Center Anchors (Menus, Modals, Reticles)
| Macro | Description |
|---|---|
| `SCREEN_COORD_CENTER_X` | Screen midpoint X (`ScreenWidth * 0.5f`) |
| `SCREEN_COORD_CENTER_Y` | Screen midpoint Y (`ScreenHeight * 0.5f`) |
| `SCREEN_COORD_CENTER_LEFT(x)` | `SCREEN_COORD_CENTER_X - GetCoord(x)` |
| `SCREEN_COORD_CENTER_RIGHT(x)`| `SCREEN_COORD_CENTER_X + GetCoord(x)` |
| `SCREEN_COORD_CENTER_UP(y)`   | `SCREEN_COORD_CENTER_Y - GetCoord(y)` |
| `SCREEN_COORD_CENTER_DOWN(y)` | `SCREEN_COORD_CENTER_Y + GetCoord(y)` |

---

## Layout Patterns

### Pattern A: Top-Right HUD Card (Cash, Wanted Stars, Weapon)
```cpp
float cardW  = SCREEN_COORD(240.0f);
float cardH  = SCREEN_COORD(60.0f);
float margin = SCREEN_COORD(20.0f);

float x2 = SCREEN_COORD_RIGHT(margin); // right boundary
float x1 = x2 - cardW;                 // left boundary
float y1 = SCREEN_COORD_TOP(margin);   // top boundary
float y2 = y1 + cardH;                 // bottom boundary

CRect hudRect(x1, y1, x2, y2);
```

### Pattern B: Bottom-Left HUD Card (Radar, Health/Armour, Speedometer)
```cpp
float cardW  = SCREEN_COORD(200.0f);
float cardH  = SCREEN_COORD(200.0f);
float margin = SCREEN_COORD(24.0f);

float x1 = SCREEN_COORD_LEFT(margin);
float x2 = x1 + cardW;
float y2 = SCREEN_COORD_BOTTOM(margin);
float y1 = y2 - cardH;

CRect radarRect(x1, y1, x2, y2);
```

### Pattern C: Center Modal / Dialog / Pause Menu
```cpp
float modalW = SCREEN_COORD(600.0f);
float modalH = SCREEN_COORD(400.0f);

float halfW = modalW * 0.5f;
float halfH = modalH * 0.5f;

CRect modalRect(
    SCREEN_COORD_CENTER_X - halfW,
    SCREEN_COORD_CENTER_Y - halfH,
    SCREEN_COORD_CENTER_X + halfW,
    SCREEN_COORD_CENTER_Y + halfH
);
```

---

## Common Pitfalls & Rules

1. **Never hardcode raw screen coordinates:**
   `CRect(10.0f, 10.0f, 110.0f, 110.0f)` appears huge on 720p and microscopic on 4K.
2. **Never scale width by `ScreenWidth / 1920.0f`:**
   Will distort squares into wide rectangles on 21:9 and 32:9 displays. Always scale both width and height by `SCREEN_COORD`.
3. **Subpixel antialiasing:**
   When rendering 1px border lines, cast or floor to integer pixels (`std::floor(SCREEN_COORD(1.0f))`) to prevent bilinear blur.
