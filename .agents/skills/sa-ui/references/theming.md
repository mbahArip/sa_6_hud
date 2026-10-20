# Theming & Design Tokens Guide

`sa_ui` uses a token-based theming model mirroring Tailwind CSS / shadcn UI. Colors use RenderWare's native `CRGBA{r, g, b, a}` structure (0-255).

---

## Token Reference

```cpp
struct Theme {
    // Surfaces & Canvas
    CRGBA background;           // Semi-transparent backdrop / fullscreen tint
    CRGBA foreground;           // Default body text
    CRGBA card;                 // Card / panel surface background
    CRGBA cardForeground;       // Card header and body text
    CRGBA popover;              // Dropdown menu / modal dialog background
    CRGBA popoverForeground;    // Dropdown item text

    // Primary & Action Colors
    CRGBA primary;              // Primary button fill / active tab / checkmark
    CRGBA primaryForeground;    // Primary button text
    CRGBA secondary;            // Secondary button fill / inactive track
    CRGBA secondaryForeground;  // Secondary button text

    // Utility & State
    CRGBA muted;                // Inactive tabs, disabled items, sub-surfaces
    CRGBA mutedForeground;      // Placeholder text, descriptions, icons
    CRGBA accent;               // Hover highlight color
    CRGBA accentForeground;     // Hover text color
    CRGBA destructive;          // Danger / delete actions
    CRGBA destructiveForeground;// Danger button text

    // Borders & Focus Rings
    CRGBA border;               // Card and container borders
    CRGBA input;                // Textbox borders
    CRGBA ring;                 // Focused input / active outline ring

    // Geometry
    float radiusSm;             // Small badge / checkbox corner radius (e.g. 4.0f)
    float radiusMd;             // Buttons, inputs, tabs corner radius (e.g. 6.0f)
    float radiusLg;             // Cards, dialogs corner radius (e.g. 8.0f)

    // Behavior & Effects
    bool slowTimeWhenOpen;      // Slows/pauses in-game time when UI is open
    float timeScaleOnOpen;      // 0.0f = full freeze, 0.1f = slow-mo
    bool showBackdropOverlay;   // Fullscreen screen tint when menu is active
    CRGBA backdropOverlayCol;   // e.g. CRGBA{0, 0, 0, 26} (10% black opacity)
    bool useGameDefaultCursor;  // Uses authentic models/fronten_pc.txd mouse cursor sprite
};
```

---

## Built-in Presets

### 1. Zinc Dark (Default)
Clean, modern neutral dark theme.
```cpp
sa_ui::SetTheme(sa_ui::Theme::ZincDark());
```

### 2. Zinc Light
High-contrast daylight theme.
```cpp
sa_ui::SetTheme(sa_ui::Theme::ZincLight());
```

### 3. Slate Dark
Cool blue-gray dark theme.
```cpp
sa_ui::SetTheme(sa_ui::Theme::SlateDark());
```

### 4. Cyberpunk
Sharp retro-futuristic yellow & cyan theme (0px border-radius).
```cpp
sa_ui::SetTheme(sa_ui::Theme::Cyberpunk());
```

---

## Creating Custom Themes

### Emerald Dark Example
```cpp
sa_ui::Theme emeraldTheme;
emeraldTheme.background          = CRGBA{6, 20, 15, 240};
emeraldTheme.card                = CRGBA{10, 30, 22, 255};
emeraldTheme.cardForeground      = CRGBA{236, 253, 245, 255};
emeraldTheme.primary             = CRGBA{16, 185, 129, 255}; // emerald-500
emeraldTheme.primaryForeground   = CRGBA{0, 0, 0, 255};
emeraldTheme.secondary           = CRGBA{20, 50, 38, 255};
emeraldTheme.secondaryForeground = CRGBA{209, 250, 229, 255};
emeraldTheme.muted               = CRGBA{20, 50, 38, 255};
emeraldTheme.mutedForeground     = CRGBA{110, 160, 140, 255};
emeraldTheme.border              = CRGBA{30, 70, 50, 255};
emeraldTheme.input               = CRGBA{30, 70, 50, 255};
emeraldTheme.ring                = CRGBA{52, 211, 153, 255};
emeraldTheme.radiusMd            = 6.0f;

sa_ui::SetTheme(emeraldTheme);
```

### Runtime Theme Toggle
```cpp
static bool isLight = false;
if (sa_ui::Button("Toggle Theme", rect)) {
    isLight = !isLight;
    sa_ui::SetTheme(isLight ? sa_ui::Theme::ZincLight() : sa_ui::Theme::ZincDark());
}
```
