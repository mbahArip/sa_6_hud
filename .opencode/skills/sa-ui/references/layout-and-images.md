# Declarative Layout, Typography, and Images

`sa_ui` includes a lightweight declarative layer inspired by HTML flex/grid
composition while preserving GTA SA's immediate-mode rendering model.

## Flex

```cpp
sa_ui::Flex({
    .rect = CRect(100.0f, 100.0f, 520.0f, 420.0f),
    .direction = sa_ui::FlexDirection::Column,
    .gap = 12.0f,
    .padding = 20.0f
}, [&] {
    sa_ui::CardHeader("Player Settings", "Configure player controls");
    sa_ui::Checkbox("God Mode", &godMode);
    sa_ui::Button("Save Changes", {
        .variant = sa_ui::ButtonVariant::Primary,
        .width = 180.0f
    });
});
```

`FlexProps` supports `Row`/`Column`, `gap`, `padding`, `rect`, `width`,
`height`, `justify`, and `align`.

## Grid

```cpp
sa_ui::Grid({
    .rect = CRect(100.0f, 100.0f, 700.0f, 320.0f),
    .columns = 2,
    .gap = 12.0f,
    .padding = 16.0f
}, [&] {
    sa_ui::Checkbox("Show on Foot", &showOnFoot);
    sa_ui::Checkbox("Show in Vehicle", &showInVehicle);
    sa_ui::Checkbox("Lock Elevation", &lockElevation);
    sa_ui::Checkbox("Dynamic Tunnel", &dynamicTunnel);
});
```

## Text and GTA font roles

```cpp
sa_ui::Text("Radar Settings", {
    .size = 18,
    .color = sa_ui::GetTheme().foreground,
    .gameFont = sa_ui::GameFontStyle::Pricedown
});
```

Recommended global roles:

```cpp
theme.fontHeader = sa_ui::GameFontStyle::Pricedown;
theme.fontLabel = sa_ui::GameFontStyle::Menu;
theme.fontSubtitle = sa_ui::GameFontStyle::Subtitle;
theme.fontStyledHeader = sa_ui::GameFontStyle::Gothic;
```

The roles map to plugin-sdk's `FONT_PRICEDOWN`, `FONT_MENU`,
`FONT_SUBTITLES`, and `FONT_GOTHIC` styles.

## Images

Images use textures already available in the current RenderWare TXD:

```cpp
sa_ui::ImageManager::LoadFromCurrentTxd("logo", "my_logo");
sa_ui::Image("logo", CRect(100.0f, 100.0f, 260.0f, 160.0f),
             CRGBA{255, 255, 255, 255}, sa_ui::ImageFit::Contain);
```

Supported fit modes are `Stretch`, `Contain`, and `Cover`. `ImageManager` owns
and destroys loaded textures during `sa_ui::Shutdown()`.

## Declarative Sliders & Steppers

Inside `Flex` or `Grid`, call `Slider` or `IndexSelect` directly:

```cpp
sa_ui::Flex({ .gap = 12.0f }, [&] {
    // Percent slider (config: 0.0 - 1.0, step: 0.05, display: 0% - 100%)
    sa_ui::Slider("Radar Opacity", &opacity, { .type = sa_ui::SliderType::Percent });

    // Number slider with negative range (-45 to +45, step: 1.0)
    sa_ui::Slider("Pitch Offset", &pitch, {
        .type = sa_ui::SliderType::Number,
        .min = -45.0f,
        .max = 45.0f,
        .step = 1.0f
    });

    // Enum left-right switch with mouse wheel and wrap support
    sa_ui::IndexSelect("Weather", &weatherIdx, { "Sunny", "Extra Sunny", "Cloudy", "Rainy", "Foggy" });
});
```

## Immediate-mode limitation

This is not a browser layout engine. A container establishes a layout scope and
children acquire deterministic rectangles as they are drawn. Use explicit
child widths in horizontal rows when intrinsic text measurement matters. The
legacy explicit-rectangle APIs remain available and can be mixed with the
declarative layer.
