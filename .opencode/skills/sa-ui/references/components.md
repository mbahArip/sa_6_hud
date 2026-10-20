# Component Reference & Usage Guide

All `sa_ui` components follow an immediate-mode execution model. Call them directly inside `Events::drawHudEvent` between `sa_ui::BeginFrame()` and `sa_ui::EndFrame()`.

For HTML-like composition, load `references/layout-and-images.md`. It documents
the stack-based `Flex`, `Grid`, `Text`, compound `Card`, and `Image` helpers.

---

## 1. Card Container (Freeform Dashboard / Surface)
Renders a panel surface with drop shadow, rounded corners, optional header banner, title, and description. Best for arbitrary multi-column interfaces, grids, tabs, sliders, forms, and mixed widgets without row limits:

```cpp
// Explicit rect / classic:
CRect cardRect(100.0f, 100.0f, 480.0f, 520.0f);
sa_ui::BeginCard(cardRect, "Vehicle Spawner", "Select and customize vehicles");

// Compound Card with position presets & trainer banner:
sa_ui::Card({
    .position = sa_ui::CardPosition::TopLeft,
    .marginX = 50.0f,
    .marginY = 50.0f,
    .width = 460.0f,
    .height = 560.0f,
    .banner = {
        .color = CRGBA{20, 90, 60, 255},
        .height = 80.0f,
        .align = sa_ui::CardHeaderAlign::Center
    },
    .title = "SAN ANDREAS"
}, [&] {
    // Arbitrary layout: Tabs, Grid, Sliders, Checkboxes...
});
```

---

## 2. Button
Buttons support 7 visual variants and 3 sizes with hover and click state handling.

```cpp
// Variants: Primary, Secondary, Outline, Ghost, Destructive, Link
// Sizes: Sm, Md, Lg
if (sa_ui::Button("Spawn Bullet", CRect(120.0f, 200.0f, 280.0f, 236.0f), sa_ui::ButtonVariant::Primary)) {
    // Spawn vehicle logic
}

if (sa_ui::Button("Delete Vehicle", CRect(290.0f, 200.0f, 450.0f, 236.0f), sa_ui::ButtonVariant::Destructive)) {
    // Destroy vehicle logic
}
```

---

## 3. InputText (Single-Line Textbox)
Features focus ring outline, blinking cursor caret, backspace, and text input.

```cpp
static char playerName[64] = "Carl Johnson";
sa_ui::InputText("NameInput", playerName, sizeof(playerName), CRect(120.0f, 160.0f, 460.0f, 196.0f), "Enter name...");
```

---

## 4. Textarea (Multi-Line Textbox)
Multi-line text editor supporting newline typing and word formatting.

```cpp
static char notes[256] = "Default mod notes...";
sa_ui::Textarea("NotesInput", notes, sizeof(notes), CRect(120.0f, 210.0f, 460.0f, 320.0f), "Enter notes here...");
```

---

## 5. Checkbox
Interactive toggle box with anti-aliased checkmark icon and label.

```cpp
static bool godMode = false;
if (sa_ui::Checkbox("God Mode", &godMode, CVector2D{120.0f, 340.0f})) {
    // Value toggled
}
```

---

## 6. RadioButton
Single-choice selector within a group.

```cpp
static int drivingStyle = 0;
sa_ui::RadioButton("Normal", &drivingStyle, 0, CVector2D{120.0f, 370.0f});
sa_ui::RadioButton("Aggressive", &drivingStyle, 1, CVector2D{220.0f, 370.0f});
sa_ui::RadioButton("Slow", &drivingStyle, 2, CVector2D{340.0f, 370.0f});
```

---

## 7. Tabs (Pill Segmented Switcher)
Horizontal tab bar with animated active pill indicator.

```cpp
static int activeTab = 0;
std::vector<std::string> tabLabels = { "Player", "Weapons", "Vehicles", "Config" };
activeTab = sa_ui::Tabs(tabLabels, activeTab, CRect(120.0f, 150.0f, 460.0f, 185.0f));

if (activeTab == 0) {
    // Draw Player controls
} else if (activeTab == 1) {
    // Draw Weapons controls
}
```

---

## 8. Collapsible (Accordion)
Expandable container for collapsing detailed sub-menus.

```cpp
static bool isEngineOpen = false;
sa_ui::Collapsible("Engine Tuning", &isEngineOpen, CRect(120.0f, 200.0f, 460.0f, 234.0f));

if (isEngineOpen) {
    // Draw sub-items below with offset Y
}
```

---

## 9. Select (Dropdown / Combobox)
Combo box trigger with floating popover list rendered above sibling controls.

```cpp
static int selectedCar = 0;
static bool isDropdownOpen = false;
std::vector<std::string> carList = { "Infernus", "Turismo", "Bullet", "Cheetah", "Banshee" };

sa_ui::Select("CarSelect", carList, &selectedCar, &isDropdownOpen, CRect(120.0f, 250.0f, 460.0f, 286.0f), "Choose car...");
```

---

## 10. Dialog (Modal Overlay)
Full-screen backdrop overlay with centered card and close button.

```cpp
static bool isDialogOpen = false;

sa_ui::Dialog("Confirm Vehicle Replacement", &isDialogOpen, [&](CRect contentArea) {
    sa_ui::FontManager::Draw("", "Are you sure you want to replace current vehicle?", CRect(contentArea.left, contentArea.top + 10.0f, contentArea.right, contentArea.top + 40.0f), sa_ui::GetTheme().foreground);

    if (sa_ui::Button("Confirm", CRect(contentArea.left, contentArea.bottom - 36.0f, contentArea.left + 120.0f, contentArea.bottom), sa_ui::ButtonVariant::Primary)) {
        isDialogOpen = false;
    }
    if (sa_ui::Button("Cancel", CRect(contentArea.left + 130.0f, contentArea.bottom - 36.0f, contentArea.left + 230.0f, contentArea.bottom), sa_ui::ButtonVariant::Ghost)) {
        isDialogOpen = false;
    }
}, 440.0f, 220.0f);
```

---

## 11. Slider (Number & Percent, Fine-Tuning, Default Snap)
Draggable value bar with knob thumb, step snapping, mouse-wheel support, inline text edit, and default reset.

- **Percent:** Config value `0.0` – `1.0`. Default step `0.05` (5%). Displays as `0%` – `100%`.
- **Number:** Min/Max can span `-xxx` to `+yyy`. Default step `1.0`.
- **Fine-Tuning:** Click value pill to type exact number, or press Up/Down arrows to step.
- **Default Snap:** Double-click thumb/track resets to `defaultValue`.

```cpp
// Percent mode with default snap:
static float opacity = 0.85f;
sa_ui::Slider("Opacity", &opacity, {
    .type = sa_ui::SliderType::Percent,
    .defaultValue = 0.85f,
    .hasDefault = true
}, CRect(120.0f, 300.0f, 460.0f, 336.0f));

// Number mode with negative range:
static float pitchOffset = 0.0f;
sa_ui::Slider("Pitch Offset", &pitchOffset, {
    .type = sa_ui::SliderType::Number,
    .min = -45.0f,
    .max = 45.0f,
    .step = 1.0f
}, CRect(120.0f, 350.0f, 460.0f, 386.0f));
```

---

## 12. IndexSelect / Stepper (Enum Left-Right Switch)
Preferred Shadcn/GTA-native pattern for enums (compact, faster than dropdown popovers). Cycles options horizontally (`[ < ] Current Option [ > ]`) with mouse-wheel and gamepad support. `wrap` is `false` by default.

```cpp
// Enum / string list:
static int weatherIdx = 0;
sa_ui::IndexSelect("Weather", &weatherIdx, { "Sunny", "Extra Sunny", "Cloudy", "Rainy", "Foggy" }, CRect(120.0f, 430.0f, 460.0f, 464.0f));

// Numeric integer stepper with loop enabled:
static int stars = 3;
sa_ui::Stepper("Wanted Level", &stars, 0, 6, CRect(120.0f, 475.0f, 460.0f, 509.0f), /*step=*/1, /*wrap=*/true);
```

---

## 13. Toasts (Floating Notifications)
```cpp
sa_ui::Toast::Show("Profile Loaded", "Config applied successfully", sa_ui::ToastVariant::Success);
```

---

## 14. Badge & Separator
```cpp
// Badge tag
sa_ui::Badge("PRO", CRect(400.0f, 110.0f, 450.0f, 130.0f), sa_ui::BadgeVariant::Secondary);

// Horizontal divider line
sa_ui::Separator(CVector2D{120.0f, 200.0f}, 340.0f);
```

---

## 15. MenuItem (Full-Width Flex Justify-Between)
Full-width row widget with native GTA `FONT_MENU` label on left and right-aligned text value or child control:
- **Auto width:** fills card/container (`width = 0.0f`).
- **Marquee:** labels exceeding available width scroll smoothly after 500ms hover/nav-focus (`wrap = false`). Or wrap if `wrap = true`. Hardware-clipped via D3D9 scissor rect with immediate `CFont::RenderFontBuffer()` flush.
- **Disabled:** `disabled = true` grays row, prevents click/confirm and marquee.
- **Dividers:** `showDivider = true` adds bottom border line.

```cpp
// Declarative (inside Menu or Card):
sa_ui::MenuItem("Teleport to Marker", ">", { .showDivider = true });

// Marquee scroll test on long label:
sa_ui::MenuItem("Super Extreme High Density Realistic Population Traffic Mode", "ACTIVE", { .showDivider = true });

// Custom right control:
sa_ui::MenuItem("Infinite Ammo", {}, [&](CRect rightRc) {
    sa_ui::Checkbox("", &ammo, { rightRc.right - 20.0f, rightRc.top + 8.0f });
});

// Explicit rect (outside Menu context):
sa_ui::MenuItem("Weather", "Extra Sunny", rect, { .disabled = false });
```

---

## 16. Menu (Trainer-Style Vertical List Manager)
NativeUI / Menyoo trainer stack with auto-windowed scrolling, header banner, subtitle bar, index counter footer (`X / Y`), and scroll indicator chevrons (`^ v`):
- Clamps rendering to `maxVisible` items (default 8).
- Automatically tracks `selectedIndex` and `scrollOffset`.
- D-pad / Arrow keys cycle items with wraparound.
- Child `MenuItem(...)` calls auto-position without explicit rectangles.

```cpp
sa_ui::Menu({
    .position = sa_ui::CardPosition::TopLeft,
    .marginX = 40.0f,
    .marginY = 40.0f,
    .width = 380.0f,
    .maxVisible = 8,
    .title = "TRAINER",
    .subtitle = "TELEPORT MENU",
    .banner = { .color = CRGBA{180, 20, 20, 255} },
    .showFooter = true
}, [&] {
    if (sa_ui::MenuItem("Los Santos Airport", ">")) { TeleportTo(LS_AIRPORT); }
    if (sa_ui::MenuItem("Grove Street", ">"))        { TeleportTo(GROVE_ST); }
    if (sa_ui::MenuItem("Mount Chiliad", ">"))       { TeleportTo(CHILIAD); }
    if (sa_ui::MenuItem("Las Venturas Strip", ">"))  { TeleportTo(LV_STRIP); }
    if (sa_ui::MenuItem("San Fierro Pier 69", ">"))  { TeleportTo(SF_PIER); }
    // Handles 50+ items transparently with auto-scrolling window & counter!
});
```
