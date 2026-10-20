# Best Practices & Architecture Patterns

Guidelines for writing stable, responsive, crash-free GTA San Andreas mods using `sa_ui` and `plugin-sdk`.

---

## 1. Lifecycle Hook Points

Never initialize fonts or UI state before RenderWare is ready.

| Phase | Hook Event | What to do |
|---|---|---|
| **Initialization** | `Events::initRwEvent` | Call `sa_ui::Init()`, load `.ttf` fonts via `FontManager::LoadFromFile`. |
| **Input / Logic** | `Events::gameProcessEvent` | Check toggle keys (`KeyPressed(VK_F4)`), perform game mutations. |
| **Rendering** | `Events::drawHudEvent` | Call `sa_ui::BeginFrame(open)`, draw components, call `sa_ui::EndFrame(open)`. |
| **Device Lost** | `Events::d3dLostEvent` | Handled internally by `FontManager`. |
| **Device Reset** | `Events::d3dResetEvent` | Handled internally by `FontManager`. |
| **Cleanup** | `Events::shutdownRwEvent` | Call `sa_ui::Shutdown()`. |

---

## 2. Game Control & Input Capture

When UI menus are open:
1. `sa_ui::BeginFrame(true)` automatically zeroes `CPad::NewMouseControllerState` and `CPad::PCTempMouseControllerState` to lock in-game camera look from moving while browsing the UI.
2. Automatically disables weapon cycling, ducking, jumping, and weapon fire via `CPad::GetPad(0)`.
3. Time scale control: Setting `theme.slowTimeWhenOpen = true` automatically freezes (`timeScaleOnOpen = 0.0f`) or slows time (`timeScaleOnOpen = 0.1f`), and smoothly restores timescale on close.
4. Backdrop overlay: Automatically applies a full-screen dim overlay (`theme.backdropOverlayCol`, default 10% black opacity `CRGBA{0, 0, 0, 26}`).
5. Authentic Cursor: Automatically loads and renders the native GTA SA frontend cursor sprite from `models/fronten_pc.txd` (`mouse`).

---

## 3. Z-Index & Overlay Flushing

Dropdown menus (`sa_ui::Select`) and Modals (`sa_ui::Dialog`) must render **on top of** other buttons and cards on the screen.

- `sa_ui::Select` and `sa_ui::Dialog` push their floating content into an internal overlay queue (`PushOverlay`).
- `sa_ui::EndFrame()` automatically flushes all overlays after the main card/body pass finishes, followed by drawing the custom cursor.
- **Rule:** Never manually render popovers outside the `BeginFrame()` / `EndFrame()` wrapper.

---

## 4. Layout Stacking Pattern

Maintain a running `curY` accumulator for clean vertical component stacks:

```cpp
float curY = 160.0f;
float padding = 10.0f;

// Input Box (Height: 36px)
sa_ui::InputText("Name", nameBuf, sizeof(nameBuf), CRect(120.0f, curY, 460.0f, curY + 36.0f));
curY += 36.0f + padding;

// Checkbox (Height: 20px)
sa_ui::Checkbox("Option", &val, CVector2D{120.0f, curY});
curY += 20.0f + padding;

// Button (Height: 36px)
if (sa_ui::Button("Submit", CRect(120.0f, curY, 460.0f, curY + 36.0f))) {
    // Action
}
```

---

## 5. Multi-Resolution Aspect Ratio Support

For HUD widgets and menus that adapt across 4:3, 16:9, and ultrawide resolutions:
- Use `SCREEN_WIDTH` and `SCREEN_HEIGHT` macros from `shared/extensions/Screen.h`.
- Center dialogs dynamically:
  ```cpp
  float cardWidth = 400.0f;
  float cardHeight = 300.0f;
  float left = (SCREEN_WIDTH - cardWidth) * 0.5f;
  float top = (SCREEN_HEIGHT - cardHeight) * 0.5f;
  CRect centerRect(left, top, left + cardWidth, top + cardHeight);
  ```

---

## 6. Safe Hot-Reloading

For hot-reloading `.asi` plugins without restarting GTA SA via MCP bridge:
```cpp
extern "C" __declspec(dllexport) void GtaMcpUnload() {
    sa_ui::Shutdown();
    Events::gameProcessEvent.RemoveById(MY_PROCESS_HOOK);
    Events::drawHudEvent.RemoveById(MY_DRAW_HOOK);
}
```
Always register event listeners with an explicit hook ID to allow clean removal.
