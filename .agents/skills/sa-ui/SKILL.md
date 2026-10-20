---
name: sa-ui
description: >
  Modern shadcn-style native in-game UI library for GTA San Andreas (classic 1.0 US)
  using plugin-sdk. Use when building in-game menus, trainers, HUD cards, dialogs,
  buttons, inputs, textareas, tabs, select dropdowns, collapsible sections, sliders,
  checkboxes, declarative Flex/Grid layout, GTA game-font typography, texture images,
  custom fonts (.ttf/.otf), or custom themes in GTA SA C++ plugins.
metadata:
  version: 1.0.0
  repo: https://github.com/mbahArip/sa_ui
  local_repo: vendor/sa_ui
  underlying_sdk: sa-plugin-sdk (https://github.com/Dryxio/plugin-sdk-sa)
  include_path: vendor/sa_ui/include
---

# SA UI — Modern Native In-Game UI Library for GTA San Andreas

Header-only C++ library for GTA San Andreas (1.0 US). Built directly on RenderWare 2D (`RwIm2D`) & Direct3D 9. Native in-game rendering (not ImGui overlay).

Companion skill: `sa-plugin-sdk` (for entity manipulation, game hooks, memory patches, streaming).

---

## References & Deep-Dive Guides

Load references on demand:

| Need | Load |
|---|---|
| Palette tokens, custom themes, dark/light switching | [references/theming.md](references/theming.md) |
| TTF/OTF font loading, path resolution, text measuring | [references/font-loading.md](references/font-loading.md) |
| All components API (Card, Button, Input, Select, Dialog, etc.) | [references/components.md](references/components.md) |
| Declarative Flex/Grid layout, Text, compound cards, and Image | [references/layout-and-images.md](references/layout-and-images.md) |
| Hook points, Z-index overlays, game control blocking, hot-reload | [references/best-practices.md](references/best-practices.md) |

---

## Quick Integration

### 1. Visual Studio Project Setup
Add include path in `.vcxproj`:
```xml
<AdditionalIncludeDirectories>vendor\sa_ui\include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
```
Link libraries:
```xml
<AdditionalDependencies>gdi32.lib;user32.lib;$(SdkRoot)\shared\dxsdk\d3dx9.lib;%(AdditionalDependencies)</AdditionalDependencies>
```

### 2. Standard Plugin Lifecycle Pattern
```cpp
#include <plugin.h>
#include <sa_ui/sa_ui.h>

using namespace plugin;

class MyModUI {
    bool isMenuOpen = false;

public:
    MyModUI() {
        // 1. Initialize sa_ui + load fonts at RenderWare init
        Events::initRwEvent += [] {
            sa_ui::Init(sa_ui::Theme::ZincDark());
            // Auto-resolves relative to .asi directory or game folder
            sa_ui::FontManager::LoadFromFile("default", "fonts/Inter-Medium.ttf", "Inter", 16);
        };

        // 2. Input toggle
        Events::gameProcessEvent += [this] {
            if (KeyPressed(VK_F4)) isMenuOpen = !isMenuOpen;
        };

        // 3. Render inside HUD draw pass
        Events::drawHudEvent += [this] {
            sa_ui::BeginFrame(isMenuOpen);

            if (isMenuOpen) {
                CRect panel(100.0f, 100.0f, 480.0f, 460.0f);
                sa_ui::BeginCard(panel, "My Mod Menu", "Config and options");

                if (sa_ui::Button("Click Me", CRect(120.0f, 160.0f, 260.0f, 196.0f), sa_ui::ButtonVariant::Primary)) {
                    // Action
                }
            }

            sa_ui::EndFrame(isMenuOpen);
        };

        // 4. Cleanup
        Events::shutdownRwEvent += [] {
            sa_ui::Shutdown();
        };
    }
} myModUI;
```

---

## Component Quick Reference

| Component | Function Signature |
|---|---|
| **Card** | `sa_ui::Card(CardProps props, [&] { ... })` or `sa_ui::BeginCard(rect, title, desc)` |
| **Menu** | `sa_ui::Menu(MenuProps props, [&] { ... })` |
| **MenuItem** | `bool clicked = sa_ui::MenuItem(label, valueText, [rect], [props])` |
| **Button** | `bool clicked = sa_ui::Button(label, rect, variant, size, disabled)` |
| **InputText** | `bool focused = sa_ui::InputText(label, charBuffer, bufferSize, rect, placeholder)` |
| **Textarea** | `bool focused = sa_ui::Textarea(label, charBuffer, bufferSize, rect, placeholder)` |
| **Checkbox** | `bool changed = sa_ui::Checkbox(label, bool* value, CVector2D pos)` |
| **RadioButton** | `bool changed = sa_ui::RadioButton(label, int* currentVal, int myVal, CVector2D pos)` |
| **Tabs** | `int activeIdx = sa_ui::Tabs(std::vector<std::string> names, int activeIdx, CRect rect)` |
| **Collapsible** | `bool isOpen = sa_ui::Collapsible(title, bool* isOpen, CRect headerRect)` |
| **Select** | `bool changed = sa_ui::Select(label, optionsVec, int* selectedIdx, bool* isOpen, CRect rect)` |
| **Dialog** | `sa_ui::Dialog(title, bool* isOpen, [](CRect contentArea) { ... }, width, height)` |
| **Slider** | `bool dragging = sa_ui::Slider(label, float* val, float min, float max, CRect rect)` |
| **Badge** | `sa_ui::Badge(label, rect, variant)` |
| **Separator** | `sa_ui::Separator(CVector2D startPos, float width)` |
