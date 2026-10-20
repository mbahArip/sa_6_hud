/*
    GTA SA Plugin SDK — HUD / 2D Drawing
    Demonstrates: drawHudEvent, CFont, SCREEN_COORD macros, CRGBA,
                  RwIm2D rendering, CSprite2d
    Source: examples/GPS/Main.cpp, examples/Speedometer/Main.cpp
*/
#include <plugin.h>
#include "CFont.h"
#include "CRadar.h"

using namespace plugin;

class HudDemo {
public:
    HudDemo() {
        Events::drawHudEvent += [] {
            // --- CFont text ---
            CFont::SetOrientation(ALIGN_CENTER);
            CFont::SetColor(CRGBA(255, 255, 255, 200));
            CFont::SetBackground(false, false);
            CFont::SetWrapx(500.0f);
            CFont::SetScale(
                0.6f * static_cast<float>(RsGlobal.maximumWidth)  / 640.0f,
                1.2f * static_cast<float>(RsGlobal.maximumHeight) / 448.0f
            );
            CFont::SetFontStyle(FONT_SUBTITLES);  // or FONT_STANDARD, FONT_MENU
            CFont::SetProportional(true);
            CFont::SetDropShadowPosition(1);
            CFont::SetDropColor(CRGBA(0, 0, 0, 255));

            // Print at screen-space coords (pixels at 640x448 base resolution)
            // SCREEN_COORD scales to actual resolution
            CFont::PrintString(
                SCREEN_COORD_CENTER_X,
                SCREEN_COORD(50.0f),
                "Hello HUD"
            );
        };

        Events::drawRadarOverlayEvent += [](bool /*expanded*/) {
            // Draw on top of the radar (before radar blips)
            // Use RwIm2DVertex + RwIm2DRenderPrimitive for custom lines/shapes
            // Use CRadar::TransformRealWorldPointToRadarSpace to convert world->radar
            // Use CRadar::TransformRadarPointToScreenSpace to convert radar->screen
        };
    }
} hudDemo;

/*
  SCREEN_COORD MACROS (shared/extensions/Screen.h):
  SCREEN_COORD(x)           — scale x from 640-base to actual width
  SCREEN_COORD_Y(y)         — scale y from 448-base to actual height
  SCREEN_COORD_CENTER_X     — horizontal center
  SCREEN_COORD_CENTER_Y     — vertical center
  SCREEN_COORD_RIGHT(x)     — from right edge: actualWidth - SCREEN_COORD(x)
  SCREEN_COORD_BOTTOM(y)    — from bottom edge: actualHeight - SCREEN_COORD_Y(y)
  SCREEN_RECT(x, y, w, h)   — CRect scaled to screen

  CRGBA(r, g, b, a)         — colour struct (shared/Color.h)

  CFont styles:
    FONT_SUBTITLES = 0
    FONT_STANDARD  = 1
    FONT_MENU      = 2

  CFont orientations:
    ALIGN_LEFT   = 0
    ALIGN_CENTER = 1
    ALIGN_RIGHT  = 2

  RwIm2D (2D immediate mode rendering):
    RwIm2DVertex  — vertex: { float x, y, z, rhw; uint32_t emissiveColor; float u, v; }
    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, count)
    RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, verts, count)
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster)  — NULL for no texture
    CSprite2d::NearScreenZ     — z value for 2D overlay verts
    CSprite2d::RecipNearClip   — rhw value for 2D overlay verts
*/
