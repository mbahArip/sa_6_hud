# Font Loading & Typography Guide

`sa_ui` features an in-game TrueType/OpenType font loader using Direct3D 9 (`ID3DXFont`) with private process registration (`AddFontResourceEx`) and native `CFont` fallback.

---

## Supported Font Types

- **TrueType (`.ttf`)** — e.g. `Inter-Regular.ttf`, `Geist-Medium.ttf`, `Roboto.ttf`
- **OpenType (`.otf`)** — e.g. `FiraCode-Retina.otf`
- **System Installed Fonts** — e.g. `"Segoe UI"`, `"Arial"`, `"Consolas"`

---

## Loading Methods

### 1. Loading from File (`.ttf` / `.otf`)
```cpp
bool success = sa_ui::FontManager::LoadFromFile(
    "inter",                      // Font ID handle
    "fonts/Inter-Medium.ttf",     // File path
    "Inter",                      // Face Name inside TTF metadata
    16,                           // Pixel height
    FW_NORMAL                     // Weight: FW_LIGHT, FW_NORMAL, FW_MEDIUM, FW_SEMIBOLD, FW_BOLD
);
```

#### Automatic Path Resolution
`LoadFromFile` automatically checks locations in this order:
1. **Plugin directory (`PLUGIN_PATH`):** `modloader/YourMod/fonts/Inter-Medium.ttf`
2. **Game root directory (`GAME_PATH`):** `GTA San Andreas/fonts/Inter-Medium.ttf`
3. **Absolute path:** `C:/MyProject/fonts/Inter-Medium.ttf`

No Windows font installation required — font is registered privately for the GTA SA process only.

---

### 2. Loading System Fonts
```cpp
sa_ui::FontManager::LoadSystem(
    "heading",       // Font ID
    "Segoe UI",      // Windows font family name
    18,              // Height
    FW_BOLD          // Weight
);
```

---

## Typography API

### Native GTA font roles

The SDK's `CFont` exposes four San Andreas font styles. `sa_ui` maps them through
`Theme` and `GameFontStyle`:

```cpp
sa_ui::Theme theme = sa_ui::Theme::ZincDark();
theme.fontHeader = sa_ui::GameFontStyle::Pricedown;
theme.fontLabel = sa_ui::GameFontStyle::Menu;
theme.fontSubtitle = sa_ui::GameFontStyle::Subtitle;
theme.fontStyledHeader = sa_ui::GameFontStyle::Gothic;
sa_ui::SetTheme(theme);
```

Recommended roles:

- **Pricedown** — major display/header text
- **Menu** — labels, controls, and general UI text
- **Subtitles** — descriptions, captions, and supporting text
- **Gothic** — emphasized or styled headings

Use `TextProps::gameFont` to override the role for one text node. These are
native game fonts, not Windows font families, and therefore do not use
`D3DXCreateFontA`.

### Measuring Text
```cpp
float textW = 0.0f;
float textH = 0.0f;
sa_ui::FontManager::Measure("inter", "Spawn Infernus", &textW, &textH);
```

### Drawing Raw Text
```cpp
CRect textRect(100.0f, 100.0f, 300.0f, 120.0f);
sa_ui::FontManager::Draw(
    "inter",                         // Font ID (pass "" for default font)
    "Vehicle Options",               // String
    textRect,                        // Bounding box
    sa_ui::GetTheme().foreground,    // CRGBA color
    sa_ui::ALIGN_LEFT | DT_NOCLIP    // Alignment flags: ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT
);
```

---

## Font Fallback Mechanics

If a font file is missing, corrupted, or D3D device creation fails:
1. `sa_ui::FontManager` returns `false` without crashing.
2. All components gracefully fallback to RenderWare's native `CFont` engine.

---

## Device Lost / Reset Safety

Font textures are automatically managed across D3D9 device loss (Alt+Tab, resolution change):
- `plugin::Events::d3dLostEvent` calls `ID3DXFont::OnLostDevice()`.
- `plugin::Events::d3dResetEvent` calls `ID3DXFont::OnResetDevice()`.
- On shutdown / plugin reload, `RemoveFontResourceEx` frees GDI handles cleanly.
