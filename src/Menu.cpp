#include "Menu.h"
#include "Config.h"
#include <plugin.h>
#include <sa_ui/sa_ui.h>
#include <vector>
#include <string>

namespace hud6 {

static bool s_isOpen = false;
static int s_activeTab = 0;
static bool s_keyWasDown = false;
static char s_saveNotice[64] = {};
static int s_saveNoticeTimer = 0;
static bool s_uiInitialized = false;

static sa_ui::Theme CreateGtaVITheme() {
    sa_ui::Theme t = sa_ui::Theme::ZincDark();

    // GTA VI Color Palette (Vice Slate & Neon Pink/Cyan Accents)
    t.card                = CRGBA{15, 18, 26, 245};   // Deep translucent slate glass
    t.cardForeground      = CRGBA{255, 255, 255, 255};
    t.foreground          = CRGBA{248, 250, 252, 255};
    t.muted               = CRGBA{30, 36, 51, 255};
    t.mutedForeground     = CRGBA{148, 163, 184, 255}; // Slate gray
    t.primary             = CRGBA{244, 63, 142, 255};  // Vice Magenta / Pink
    t.primaryForeground   = CRGBA{255, 255, 255, 255};
    t.secondary           = CRGBA{30, 36, 51, 255};    // Slate button/track fill
    t.secondaryForeground = CRGBA{226, 232, 240, 255};
    t.accent              = CRGBA{236, 72, 153, 50};   // Neon magenta hover
    t.accentForeground    = CRGBA{255, 255, 255, 255};
    t.border              = CRGBA{51, 65, 85, 220};    // Glass stroke
    t.input               = CRGBA{30, 36, 51, 255};
    t.ring                = CRGBA{6, 182, 212, 255};   // Neon Cyan focus

    // Geometry
    t.radiusSm            = 4.0f;
    t.radiusMd            = 6.0f;
    t.radiusLg            = 10.0f;

    // Slow-Mo & Overlay Settings
    t.slowTimeWhenOpen    = true;
    t.timeScaleOnOpen     = 0.1f;                       // 10% speed slow-mo
    t.showBackdropOverlay = true;
    t.backdropOverlayCol  = CRGBA{6, 8, 12, 50};       // 20% dark slate dim
    t.useGameDefaultCursor= true;

    return t;
}

void Menu::Init() {
    sa_ui::Init(CreateGtaVITheme());
    sa_ui::FontManager::LoadSystem("default", "Segoe UI", 15, FW_SEMIBOLD);
    s_uiInitialized = true;
}

void Menu::Shutdown() {
    sa_ui::Shutdown();
    s_uiInitialized = false;
}

bool Menu::IsOpen() {
    return s_isOpen;
}

bool Menu::IsLayoutPreview() {
    return s_isOpen && s_activeTab == 7;
}

void Menu::Process() {
    // If menu is open, handle ESC press to close menu instead of opening GTA SA pause menu
    if (s_isOpen) {
        if (sa_ui::Input::keyEscape || (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) {
            s_isOpen = false;
            sa_ui::Input::keyEscape = false;
            FrontEndMenuManager.m_bStartUpFrontEndRequested = false;
            FrontEndMenuManager.m_bMenuActive = false;
            CPad::NewKeyState.standardKeys[VK_ESCAPE] = 0;
            CPad::OldKeyState.standardKeys[VK_ESCAPE] = 0;
            LogWrite("menu: closed via ESC");
            return;
        }
    }

    int key = g_pendingConfig.menuToggleKey;
    if (key <= 0) key = 0xBF; // VK_OEM_2 ('/')

    bool keyDown = (GetAsyncKeyState(key) & 0x8000) != 0 ||
                   (GetAsyncKeyState(VK_OEM_2) & 0x8000) != 0 ||
                   (GetAsyncKeyState(VK_DIVIDE) & 0x8000) != 0 ||
                   plugin::KeyPressed(key) ||
                   plugin::KeyPressed(VK_OEM_2) ||
                   plugin::KeyPressed(VK_DIVIDE);

    if (keyDown && !s_keyWasDown) {
        s_isOpen = !s_isOpen;
        LogWrite("menu: state changed -> %d", s_isOpen ? 1 : 0);
        if (s_isOpen) {
            s_saveNotice[0] = '\0';
            s_saveNoticeTimer = 0;
        }
    }
    s_keyWasDown = keyDown;

    if (s_saveNoticeTimer > 0) {
        --s_saveNoticeTimer;
    }
}

void Menu::Render() {
    if (!s_uiInitialized) {
        Init();
    }

    sa_ui::BeginFrame(s_isOpen);

    if (!s_isOpen) {
        sa_ui::EndFrame(s_isOpen);
        return;
    }

    float sW = (float)RsGlobal.maximumWidth;
    float sH = (float)RsGlobal.maximumHeight;

    float cardW = 660.0f;
    float cardH = 500.0f;
    float left = (sW - cardW) * 0.5f;
    float top = (sH - cardH) * 0.5f;
    CRect panel(left, top, left + cardW, top + cardH);

    sa_ui::BeginCard(panel, "6HUD Settings", "Configure modules, perspective, lighting & threat radars");

    // Tabs container
    std::vector<std::string> tabNames = { "Radar", "GPS", "Cards", "Search", "Heat", "Sirens", "Arcs/Snitch", "Layout" };
    CRect tabRect(left + 20.0f, top + 55.0f, left + cardW - 20.0f, top + 88.0f);
    s_activeTab = sa_ui::Tabs(tabNames, s_activeTab, tabRect);

    float curY = top + 102.0f;
    float colW = 280.0f;
    float leftCol = left + 25.0f;
    float rightCol = left + 345.0f;

    static std::string s_activeEditField = "";
    static char s_editBuf[32] = "";

    auto DrawSliderField = [&](const char* label, float* val, float minV, float maxV, float x, float y, const char* fmt) {
        float labelW = colW - 75.0f;
        float pillW = 70.0f;
        float pillH = 18.0f;
        CRect pillRc(x + colW - pillW, y - 2.0f, x + colW, y + pillH - 2.0f);

        sa_ui::FontManager::Draw("", label, CRect(x, y, x + labelW, y + 16.0f), sa_ui::GetTheme().foreground, sa_ui::ALIGN_LEFT | DT_NOCLIP);

        unsigned int pillId = sa_ui::HashId((std::string(label) + "_pill").c_str());

        if (s_activeEditField == label) {
            sa_ui::InputText(label, s_editBuf, sizeof(s_editBuf), pillRc);

            bool commit = false;
            if (sa_ui::Input::keyEnter) {
                commit = true;
                sa_ui::Input::keyEnter = false;
            } else if (sa_ui::Input::focusId != sa_ui::HashId(label)) {
                commit = true;
            }

            if (commit) {
                float v = (float)atof(s_editBuf);
                if (v < minV) v = minV;
                if (v > maxV) v = maxV;
                *val = v;
                s_activeEditField.clear();
                if (sa_ui::Input::focusId == sa_ui::HashId(label)) {
                    sa_ui::Input::focusId = 0;
                }
            }
        } else {
            bool hovered = sa_ui::Input::IsHovered(pillRc);
            if (hovered) sa_ui::Input::hotId = pillId;
            if (hovered && sa_ui::Input::mouseLeftDown) {
                sa_ui::Input::activeId = pillId;
            }
            if (sa_ui::Input::activeId == pillId && sa_ui::Input::mouseLeftUp && hovered) {
                s_activeEditField = label;
                snprintf(s_editBuf, sizeof(s_editBuf), "%.2f", *val);
                sa_ui::Input::focusId = sa_ui::HashId(label);
                sa_ui::Input::textQueue.clear();
                sa_ui::Input::activeId = 0;
            }

            CRGBA pillBg = hovered ? sa_ui::GetTheme().secondary : CRGBA{25, 30, 42, 180};
            sa_ui::Renderer::DrawRoundedRect(pillRc, pillBg, 3.0f);
            sa_ui::Renderer::DrawRoundedBorder(pillRc, hovered ? sa_ui::GetTheme().primary : sa_ui::GetTheme().border, 3.0f, 1.0f);

            char valBuf[32];
            snprintf(valBuf, sizeof(valBuf), fmt, *val);
            CRect valTextRc(pillRc.left, pillRc.top + 2.0f, pillRc.right, pillRc.bottom);
            sa_ui::FontManager::Draw("", valBuf, valTextRc, hovered ? sa_ui::GetTheme().primary : sa_ui::GetTheme().mutedForeground, sa_ui::ALIGN_CENTER | DT_NOCLIP);
        }

        CRect sRc(x, y + 20.0f, x + colW, y + 38.0f);
        sa_ui::Slider(label, val, minV, maxV, sRc);
    };

    auto DrawStepper = [&](const char* label, int* currentIdx, const std::vector<const char*>& options, float x, float y, float width) {
        float labelW = width * 0.40f;
        float controlW = width - labelW;
        float btnW = 20.0f;
        float textW = controlW - (btnW * 2.0f);
        float h = 20.0f;

        sa_ui::FontManager::Draw("", label, CRect(x, y + 2.0f, x + labelW, y + h + 2.0f), sa_ui::GetTheme().foreground, sa_ui::ALIGN_LEFT | DT_NOCLIP);

        float cx = x + labelW;
        CRect btnPrev(cx, y, cx + btnW, y + h);
        CRect textRc(cx + btnW, y, cx + btnW + textW, y + h);
        CRect btnNext(cx + btnW + textW, y, cx + controlW, y + h);

        int n = (int)options.size();
        if (n <= 0) return;

        if (*currentIdx < 0) *currentIdx = 0;
        if (*currentIdx >= n) *currentIdx = n - 1;

        sa_ui::Renderer::DrawRoundedRect(CRect(cx, y, cx + controlW, y + h), CRGBA{25, 30, 42, 180}, 3.0f);
        sa_ui::Renderer::DrawRoundedBorder(CRect(cx, y, cx + controlW, y + h), sa_ui::GetTheme().border, 3.0f, 1.0f);

        if (sa_ui::Button("<", btnPrev, sa_ui::ButtonVariant::Ghost, sa_ui::ButtonSize::Sm)) {
            *currentIdx = (*currentIdx - 1 + n) % n;
        }

        sa_ui::FontManager::Draw("", options[*currentIdx], textRc, sa_ui::GetTheme().primary, sa_ui::ALIGN_CENTER | sa_ui::ALIGN_VCENTER | DT_NOCLIP);

        if (sa_ui::Button(">", btnNext, sa_ui::ButtonVariant::Ghost, sa_ui::ButtonSize::Sm)) {
            *currentIdx = (*currentIdx + 1) % n;
        }
    };

    if (s_activeTab == 0) {
        // [Minimap.Radar]
        sa_ui::Checkbox("Enable Radar", &g_pendingConfig.enabled, CVector2D(leftCol, curY));
        sa_ui::Checkbox("Show on Foot", &g_pendingConfig.showOnFoot, CVector2D(leftCol, curY + 28.0f));
        sa_ui::Checkbox("Show in Vehicle", &g_pendingConfig.showInVehicle, CVector2D(leftCol, curY + 56.0f));
        sa_ui::Checkbox("Lock Elevation Z", &g_pendingConfig.lockPlayerElevation, CVector2D(leftCol, curY + 84.0f));
        sa_ui::Checkbox("Dynamic Altitude Zoom", &g_pendingConfig.enableAltitudeZoom, CVector2D(leftCol, curY + 112.0f));
        sa_ui::Checkbox("Custom Map Tiles", &g_pendingConfig.useCustomMapTile, CVector2D(leftCol, curY + 140.0f));
        sa_ui::Checkbox("Show 3D Objects", &g_pendingConfig.experimentalShow3dObjects, CVector2D(leftCol, curY + 168.0f));
        bool is24H = (g_pendingConfig.gameClockFormat == 1);
        if (sa_ui::Checkbox("24H Clock Format", &is24H, CVector2D(leftCol, curY + 196.0f))) {
            g_pendingConfig.gameClockFormat = is24H ? 1 : 0;
        }
        sa_ui::Checkbox("Backdrop Blur", &g_pendingConfig.enableBackdropBlur, CVector2D(leftCol, curY + 224.0f));

        DrawSliderField("Radar Opacity", &g_pendingConfig.radarOpacity, 0.10f, 1.00f, rightCol, curY, "%.0f%%");
        DrawSliderField("Radar Zoom Mult", &g_pendingConfig.peekZoomMult, 1.0f, 3.0f, rightCol, curY + 36.0f, "%.2fx");
        DrawSliderField("Altitude Zoom Mult", &g_pendingConfig.altitudeMaxZoomMult, 1.0f, 3.0f, rightCol, curY + 72.0f, "%.2fx");
        DrawSliderField("Closed Space Zoom", &g_pendingConfig.closedSpaceZoomMultiplier, 0.5f, 1.5f, rightCol, curY + 108.0f, "%.2fx");
        DrawSliderField("Corner Radius", &g_pendingConfig.radarCornerRadius, 0.0f, 25.0f, rightCol, curY + 144.0f, "%.1f%%");
        DrawSliderField("Radar Scale", &g_pendingConfig.radarSizeScale, 0.5f, 2.0f, rightCol, curY + 180.0f, "%.2fx");
        DrawSliderField("Camera 2.5D Tilt", &g_pendingConfig.radarTiltDeg, 0.0f, 90.0f, rightCol, curY + 216.0f, "%.1f deg");
    }
    else if (s_activeTab == 1) {
        // [Minimap.GPSNavigation]
        sa_ui::Checkbox("Radar GPS Route", &g_pendingConfig.enableGps, CVector2D(leftCol, curY));
        sa_ui::Checkbox("Menu Map GPS Route", &g_pendingConfig.enableMenuMapGps, CVector2D(leftCol, curY + 28.0f));
        sa_ui::Checkbox("Turn Nav Indicator", &g_pendingConfig.enableGpsNavigator, CVector2D(leftCol, curY + 56.0f));
        sa_ui::Checkbox("Walking Node Path", &g_pendingConfig.useWalkingNode, CVector2D(leftCol, curY + 84.0f));
        sa_ui::Checkbox("Show Distance Badge", &g_pendingConfig.showGpsDistance, CVector2D(leftCol, curY + 112.0f));

        DrawSliderField("GPS Ribbon Width", &g_pendingConfig.gpsLineWidth, 4.0f, 30.0f, rightCol, curY, "%.1fpx");
        DrawSliderField("GPS Stroke Width", &g_pendingConfig.gpsCasingWidth, 0.5f, 10.0f, rightCol, curY + 45.0f, "%.1fpx");
        DrawSliderField("Distance Scale", &g_pendingConfig.distanceTextScale, 0.5f, 2.0f, rightCol, curY + 90.0f, "%.2fx");
        DrawSliderField("Distance Stroke", &g_pendingConfig.distanceStrokeThickness, 0.0f, 4.0f, rightCol, curY + 135.0f, "%.1fpx");
    }
    else if (s_activeTab == 2) {
        // [Minimap.PlaceInformation] & [Minimap.VehicleInformation] & [Minimap.Extension.VehicleFuel]
        sa_ui::Checkbox("Place Information Card", &g_pendingConfig.usePlaceInfo, CVector2D(leftCol, curY));
        sa_ui::Checkbox("Vehicle Information Card", &g_pendingConfig.useVehicleInfo, CVector2D(leftCol, curY + 28.0f));
        sa_ui::Checkbox("Vehicle Fuel System", &g_pendingConfig.useVehicleFuel, CVector2D(leftCol, curY + 56.0f));
        sa_ui::Checkbox("Fuel Active On Mission", &g_pendingConfig.useVehicleFuelOnMission, CVector2D(leftCol, curY + 84.0f));
        sa_ui::Checkbox("Engine Health Gauge", &g_pendingConfig.showGaugeHealth, CVector2D(leftCol, curY + 112.0f));
        sa_ui::Checkbox("Fuel Level Gauge", &g_pendingConfig.showGaugeFuel, CVector2D(leftCol, curY + 140.0f));
        sa_ui::Checkbox("Nitro Boost Gauge", &g_pendingConfig.showGaugeNitro, CVector2D(leftCol, curY + 168.0f));
        sa_ui::Checkbox("Controllable Nitrous", &g_pendingConfig.enableControllableNitro, CVector2D(leftCol, curY + 196.0f));

        DrawSliderField("Fuel Burn Rate", &g_pendingConfig.vehicleFuelConsumption, 0.1f, 5.0f, rightCol, curY, "%.2fx");
        DrawSliderField("Fuel RNG Min", &g_pendingConfig.vehicleFuelRngMin, 0.05f, 0.50f, rightCol, curY + 36.0f, "%.2f");
        DrawSliderField("Fuel RNG Max", &g_pendingConfig.vehicleFuelRngMax, 0.50f, 1.00f, rightCol, curY + 72.0f, "%.2f");
        DrawSliderField("Gauge Size Scale", &g_pendingConfig.gaugeScale, 0.5f, 2.0f, rightCol, curY + 108.0f, "%.2fx");
        DrawSliderField("Card Opacity", &g_pendingConfig.cardBackdropOpacity, 0.1f, 1.0f, rightCol, curY + 144.0f, "%.0f%%");
        DrawSliderField("Border Thickness", &g_pendingConfig.cardBorderThickness, 0.0f, 4.0f, rightCol, curY + 180.0f, "%.1fpx");
    }
    else if (s_activeTab == 3) {
        // [Wanted.Visual] & [Wanted.Extension.SearchArea]
        sa_ui::Checkbox("Wanted Star HUD", &g_pendingConfig.useWantedStar, CVector2D(leftCol, curY));
        sa_ui::Checkbox("Search Area Evasion", &g_pendingConfig.useWantedSearchArea, CVector2D(leftCol, curY + 28.0f));
        sa_ui::Checkbox("Search Area On Mission", &g_pendingConfig.useWantedSearchAreaOnMission, CVector2D(leftCol, curY + 56.0f));
        sa_ui::Checkbox("Allow Hide In Search", &g_pendingConfig.allowHideInSearchArea, CVector2D(leftCol, curY + 84.0f));

        DrawSliderField("Search Area Zoom", &g_pendingConfig.extWantedSearchAreaZoom, 1.0f, 2.5f, rightCol, curY, "%.2fx");
        DrawSliderField("Search Radius Base", &g_pendingConfig.extWantedSearchRadiusBase, 20.0f, 300.0f, rightCol, curY + 42.0f, "%.0fm");
        DrawSliderField("Radius / Star Add", &g_pendingConfig.extWantedSearchRadiusMultiplier, 20.0f, 400.0f, rightCol, curY + 84.0f, "%.0fm");
        DrawSliderField("Escape Base Duration", &g_pendingConfig.extWantedEscapeBaseDuration, 2.0f, 60.0f, rightCol, curY + 126.0f, "%.1fs");
        DrawSliderField("Escape / Star Add", &g_pendingConfig.extWantedEscapeMultiplier, 1.0f, 30.0f, rightCol, curY + 168.0f, "%.1fs");
    }
    else if (s_activeTab == 4) {
        // [Wanted.Extension.HeatArea]
        sa_ui::Checkbox("Heat Area System", &g_pendingConfig.useWantedHeatArea, CVector2D(leftCol, curY));
        sa_ui::Checkbox("Heat Area On Mission", &g_pendingConfig.useWantedHeatAreaOnMission, CVector2D(leftCol, curY + 28.0f));
        sa_ui::Checkbox("Show Heat On ESC Map", &g_pendingConfig.showHeatOnMap, CVector2D(leftCol, curY + 56.0f));

        DrawSliderField("Heat Base Duration", &g_pendingConfig.extWantedHeatAreaBaseDuration, 5.0f, 90.0f, rightCol, curY, "%.1fs");
        DrawSliderField("Heat Duration / Star", &g_pendingConfig.extWantedHeatAreaDurationMultiplier, 2.0f, 40.0f, rightCol, curY + 45.0f, "%.1fs");
        DrawSliderField("Heat Zone Multiplier", &g_pendingConfig.extWantedHeatAreaZoneMultiplier, 1.1f, 3.0f, rightCol, curY + 90.0f, "%.2fx");
        DrawSliderField("Heat Area Opacity", &g_pendingConfig.heatRadiusOpacity, 0.05f, 1.0f, rightCol, curY + 135.0f, "%.0f%%");
        DrawSliderField("Heat Perimeter Blur", &g_pendingConfig.heatRadiusBlur, 0.05f, 1.0f, rightCol, curY + 180.0f, "%.0f%%");
    }
    else if (s_activeTab == 5) {
        // [Wanted.Extension.PoliceSirenIndicator]
        sa_ui::Checkbox("Siren Screen Auras", &g_pendingConfig.usePoliceSirenIndicator, CVector2D(leftCol, curY));
        sa_ui::Checkbox("Siren On Mission", &g_pendingConfig.usePoliceSirenIndicatorOnMission, CVector2D(leftCol, curY + 28.0f));
        sa_ui::Checkbox("On-Foot Siren Pod", &g_pendingConfig.extWantedPoliceSirenOnFoot, CVector2D(leftCol, curY + 56.0f));
        sa_ui::Checkbox("Roadblock Sirens", &g_pendingConfig.extWantedPoliceSirenRoadblock, CVector2D(leftCol, curY + 84.0f));
        sa_ui::Checkbox("Helicopter Sirens", &g_pendingConfig.extWantedPoliceSirenHelicopter, CVector2D(leftCol, curY + 84.0f + 28.0f));
        sa_ui::Checkbox("Cull In-View Cruisers", &g_pendingConfig.copAuraVehCull, CVector2D(leftCol, curY + 84.0f + 56.0f));

        DrawSliderField("On-Foot Distance", &g_pendingConfig.extWantedPoliceSirenOnFootDist, 10.0f, 100.0f, rightCol, curY, "%.1fm");
        DrawSliderField("Max Aura Distance", &g_pendingConfig.copAuraMaxDist, 30.0f, 300.0f, rightCol, curY + 36.0f, "%.1fm");
        DrawSliderField("Min Intensity", &g_pendingConfig.extWantedPoliceSirenMinIntensity, 0.05f, 1.0f, rightCol, curY + 72.0f, "%.2f");
        DrawSliderField("Max Intensity", &g_pendingConfig.extWantedPoliceSirenMaxIntensity, 0.20f, 2.0f, rightCol, curY + 108.0f, "%.2f");
        DrawSliderField("Min Size Scale", &g_pendingConfig.extWantedPoliceSirenMinSize, 0.20f, 1.5f, rightCol, curY + 144.0f, "%.2fx");
        DrawSliderField("Max Size Scale", &g_pendingConfig.extWantedPoliceSirenMaxSize, 0.50f, 3.0f, rightCol, curY + 180.0f, "%.2fx");
        DrawSliderField("Far Speed Mult", &g_pendingConfig.extWantedPoliceSirenFarSpeedMult, 0.10f, 2.0f, rightCol, curY + 216.0f, "%.2fx");
    }
    else if (s_activeTab == 6) {
        // [Wanted.Extension.PoliceArcIndicator] & [Wanted.Extension.CriminalProfile] & [Wanted.Extension.SnitchSystem]
        sa_ui::Checkbox("Police Arc Threat", &g_pendingConfig.usePoliceArcIndicator, CVector2D(leftCol, curY));
        sa_ui::Checkbox("Arcs On Mission", &g_pendingConfig.usePoliceArcIndicatorOnMission, CVector2D(leftCol, curY + 28.0f));
        sa_ui::Checkbox("Criminal Profile (Heat)", &g_pendingConfig.useCriminalProfile, CVector2D(leftCol, curY + 56.0f));
        sa_ui::Checkbox("Profile On Mission", &g_pendingConfig.useCriminalProfileOnMission, CVector2D(leftCol, curY + 84.0f));
        sa_ui::Checkbox("Gun Heat Decay", &g_pendingConfig.extWantedGunHeatDecay, CVector2D(leftCol, curY + 112.0f));
        sa_ui::Checkbox("Snitch 911 Calls", &g_pendingConfig.useSnitchSystem, CVector2D(leftCol, curY + 140.0f));
        sa_ui::Checkbox("Snitch On Mission", &g_pendingConfig.useSnitchSystemOnMission, CVector2D(leftCol, curY + 168.0f));

        DrawSliderField("Arc Base Dist", &g_pendingConfig.extWantedPoliceArcDistance, 10.0f, 100.0f, rightCol, curY, "%.1fm");
        DrawSliderField("Arc Max Dist", &g_pendingConfig.extWantedPoliceArcMaxDist, 20.0f, 150.0f, rightCol, curY + 42.0f, "%.1fm");
        DrawSliderField("Arc Min Dist", &g_pendingConfig.extWantedPoliceArcMinDist, 5.0f, 50.0f, rightCol, curY + 84.0f, "%.1fm");
        DrawSliderField("Snitch Radius", &g_pendingConfig.extWantedSnitchRadius, 20.0f, 200.0f, rightCol, curY + 126.0f, "%.1fm");
        DrawSliderField("Min Dispatch Delay", &g_pendingConfig.extWantedSnitchMinDispatchTime, 5.0f, 60.0f, rightCol, curY + 168.0f, "%.1fs");
        DrawSliderField("Max Dispatch Delay", &g_pendingConfig.extWantedSnitchMaxDispatchTime, 10.0f, 90.0f, rightCol, curY + 210.0f, "%.1fs");
    }
    else if (s_activeTab == 7) {
        // Tab: Layout Positioning
        float fullW = cardW - 50.0f;
        float headerH = 22.0f;
        float gapY = 3.0f;
        static int s_openModuleIdx = 0;

        static const std::vector<const char*> s_posModeOpts = { "Group", "Free" };
        static const std::vector<const char*> s_groupOpts   = { "Minimap (BL)", "Status (TL)", "Criminal (TR)", "Notification (BR)" };
        static const std::vector<const char*> s_indexOpts   = { "0", "1", "2", "3", "4", "5" };
        static const std::vector<const char*> s_anchorOpts  = {
            "Top-Left", "Top-Center", "Top-Right",
            "Center-Left", "Center", "Center-Right",
            "Bottom-Left", "Bottom-Center", "Bottom-Right"
        };

        for (int i = 0; i < (int)HudModuleId::Count; ++i) {
            HudModuleId modId = (HudModuleId)i;
            auto& pos = g_pendingConfig.modulePositions[i];
            bool isOpen = (s_openModuleIdx == i);

            std::string title = std::string(GetModuleDisplayName(modId)) + " [" + PositionModeToString(pos.mode) + "]";
            CRect hRect(leftCol, curY, leftCol + fullW, curY + headerH);

            bool toggle = isOpen;
            if (sa_ui::Collapsible(title.c_str(), &toggle, hRect)) {
                if (!isOpen) s_openModuleIdx = i;
            } else {
                if (isOpen) s_openModuleIdx = -1;
            }
            curY += headerH + gapY;

            if (s_openModuleIdx == i) {
                float innerY = curY + 2.0f;
                float halfColW = (fullW - 20.0f) * 0.5f;
                float rightColX = leftCol + halfColW + 20.0f;

                int modeIdx = (pos.mode == PositionMode::Free) ? 1 : 0;
                DrawStepper("Position Mode", &modeIdx, s_posModeOpts, leftCol + 10.0f, innerY, halfColW - 10.0f);
                pos.mode = (modeIdx == 1) ? PositionMode::Free : PositionMode::Group;

                CRect btnResetMod(rightColX + halfColW - 100.0f, innerY - 1.0f, rightColX + halfColW, innerY + 20.0f);
                if (sa_ui::Button("Reset Layout", btnResetMod, sa_ui::ButtonVariant::Outline, sa_ui::ButtonSize::Sm)) {
                    pos = GetDefaultModulePosition(modId);
                    snprintf(s_saveNotice, sizeof(s_saveNotice), "Reset %s layout!", GetModuleName(modId));
                    s_saveNoticeTimer = 180;
                }
                innerY += 24.0f;

                if (pos.mode == PositionMode::Group) {
                    int grpIdx = (int)pos.group;
                    DrawStepper("Target Group", &grpIdx, s_groupOpts, leftCol + 10.0f, innerY, halfColW - 10.0f);
                    pos.group = (HudGroup)grpIdx;

                    int gIdx = pos.groupIndex;
                    if (gIdx < 0) gIdx = 0;
                    if (gIdx > 5) gIdx = 5;
                    DrawStepper("Stack Index", &gIdx, s_indexOpts, rightColX, innerY, halfColW);
                    pos.groupIndex = gIdx;

                    innerY += 24.0f;
                } else {
                    int posAncIdx = (int)pos.posAnchor;
                    DrawStepper("Screen Anchor", &posAncIdx, s_anchorOpts, leftCol + 10.0f, innerY, halfColW - 10.0f);
                    pos.posAnchor = (AnchorPoint)posAncIdx;

                    int elemAncIdx = (int)pos.elementAnchor;
                    DrawStepper("Element Pivot", &elemAncIdx, s_anchorOpts, rightColX, innerY, halfColW);
                    pos.elementAnchor = (AnchorPoint)elemAncIdx;
                    innerY += 24.0f;

                    float maxOffsetX = sW;
                    float maxOffsetY = sH;

                    DrawSliderField("Offset X (px)", &pos.offsetX, -maxOffsetX, maxOffsetX, leftCol + 10.0f, innerY, "%.0f");
                    DrawSliderField("Offset Y (px)", &pos.offsetY, -maxOffsetY, maxOffsetY, rightColX, innerY, "%.0f");
                    innerY += 40.0f;
                }

                curY = innerY + 4.0f;
            }
        }
    }

    // Bottom Action Bar
    sa_ui::Separator(CVector2D(left + 20.0f, top + cardH - 55.0f), cardW - 40.0f);

    CRect btnSave(left + 25.0f, top + cardH - 45.0f, left + 125.0f, top + cardH - 15.0f);
    if (sa_ui::Button("Save INI", btnSave, sa_ui::ButtonVariant::Primary)) {
        g_pendingConfig.enableWantedSearchRadius = g_pendingConfig.useWantedSearchArea;
        g_pendingConfig.useNewWantedSystem = g_pendingConfig.useWantedStar;
        g_pendingConfig.enableHeatArea = g_pendingConfig.useWantedHeatArea;
        g_pendingConfig.enableCopAuras = g_pendingConfig.usePoliceSirenIndicator;
        g_pendingConfig.enableCopArcs = g_pendingConfig.usePoliceArcIndicator;
        g_pendingConfig.enableHeatIdentification = g_pendingConfig.useCriminalProfile;
        SaveConfig();
        snprintf(s_saveNotice, sizeof(s_saveNotice), "Saved to 6hud.ini!");
        s_saveNoticeTimer = 180;
    }

    CRect btnReload(left + 135.0f, top + cardH - 45.0f, left + 235.0f, top + cardH - 15.0f);
    if (sa_ui::Button("Reload INI", btnReload, sa_ui::ButtonVariant::Secondary)) {
        RequestConfigReload();
        snprintf(s_saveNotice, sizeof(s_saveNotice), "Reloaded 6hud.ini!");
        s_saveNoticeTimer = 180;
    }

    CRect btnReset(left + 245.0f, top + cardH - 45.0f, left + 365.0f, top + cardH - 15.0f);
    if (sa_ui::Button("Reset to Default", btnReset, sa_ui::ButtonVariant::Outline)) {
        ResetConfig();
        snprintf(s_saveNotice, sizeof(s_saveNotice), "Reset to defaults!");
        s_saveNoticeTimer = 180;
    }

    if (s_saveNotice[0] && s_saveNoticeTimer > 0) {
        sa_ui::FontManager::Draw("", s_saveNotice, CRect(left + 375.0f, top + cardH - 40.0f, left + cardW - 135.0f, top + cardH - 15.0f), sa_ui::GetTheme().primary, sa_ui::ALIGN_LEFT | DT_NOCLIP);
    }

    CRect btnClose(left + cardW - 125.0f, top + cardH - 45.0f, left + cardW - 25.0f, top + cardH - 15.0f);
    if (sa_ui::Button("Close ( / )", btnClose, sa_ui::ButtonVariant::Outline)) {
        s_isOpen = false;
    }

    sa_ui::EndFrame(s_isOpen);
}

} // namespace hud6
