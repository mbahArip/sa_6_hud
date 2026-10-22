#pragma once

#include <CRect.h>
#include <CVector2D.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

namespace hud6 {

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

enum class HudModuleId {
    Radar = 0,
    GpsNav,
    PlaceInfo,
    VehicleInfo,
    HealthArmor,
    Wanted,
    WeaponHud,
    MoneyHud,
    Count
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

inline const char* PositionModeToString(PositionMode m) {
    return (m == PositionMode::Free) ? "free" : "group";
}

inline PositionMode StringToPositionMode(const std::string& str) {
    if (str == "free") return PositionMode::Free;
    return PositionMode::Group;
}

inline const char* HudGroupToString(HudGroup g) {
    switch (g) {
        case HudGroup::Minimap:      return "minimap";
        case HudGroup::Status:       return "status";
        case HudGroup::Criminal:     return "criminal";
        case HudGroup::Notification: return "notification";
        default:                     return "minimap";
    }
}

inline HudGroup StringToHudGroup(const std::string& str) {
    if (str == "status")       return HudGroup::Status;
    if (str == "criminal")     return HudGroup::Criminal;
    if (str == "notification") return HudGroup::Notification;
    return HudGroup::Minimap;
}

inline const char* AnchorPointToString(AnchorPoint a) {
    switch (a) {
        case AnchorPoint::TopLeft:      return "top-left";
        case AnchorPoint::TopCenter:    return "top-center";
        case AnchorPoint::TopRight:     return "top-right";
        case AnchorPoint::CenterLeft:   return "center-left";
        case AnchorPoint::Center:       return "center";
        case AnchorPoint::CenterRight:  return "center-right";
        case AnchorPoint::BottomLeft:   return "bottom-left";
        case AnchorPoint::BottomCenter: return "bottom-center";
        case AnchorPoint::BottomRight:  return "bottom-right";
        default:                        return "top-left";
    }
}

inline AnchorPoint StringToAnchorPoint(const std::string& str) {
    if (str == "top-center")    return AnchorPoint::TopCenter;
    if (str == "top-right")     return AnchorPoint::TopRight;
    if (str == "center-left")   return AnchorPoint::CenterLeft;
    if (str == "center")        return AnchorPoint::Center;
    if (str == "center-right")  return AnchorPoint::CenterRight;
    if (str == "bottom-left")   return AnchorPoint::BottomLeft;
    if (str == "bottom-center") return AnchorPoint::BottomCenter;
    if (str == "bottom-right")  return AnchorPoint::BottomRight;
    return AnchorPoint::TopLeft;
}

inline void GetAnchorFactors(AnchorPoint a, float& fx, float& fy) {
    switch (a) {
        case AnchorPoint::TopLeft:      fx = 0.0f; fy = 0.0f; break;
        case AnchorPoint::TopCenter:    fx = 0.5f; fy = 0.0f; break;
        case AnchorPoint::TopRight:     fx = 1.0f; fy = 0.0f; break;
        case AnchorPoint::CenterLeft:   fx = 0.0f; fy = 0.5f; break;
        case AnchorPoint::Center:       fx = 0.5f; fy = 0.5f; break;
        case AnchorPoint::CenterRight:  fx = 1.0f; fy = 0.5f; break;
        case AnchorPoint::BottomLeft:   fx = 0.0f; fy = 1.0f; break;
        case AnchorPoint::BottomCenter: fx = 0.5f; fy = 1.0f; break;
        case AnchorPoint::BottomRight:  fx = 1.0f; fy = 1.0f; break;
        default:                        fx = 0.0f; fy = 0.0f; break;
    }
}

inline const char* GetModuleName(HudModuleId id) {
    switch (id) {
        case HudModuleId::Radar:       return "radar";
        case HudModuleId::GpsNav:      return "gps_nav";
        case HudModuleId::PlaceInfo:   return "place_info";
        case HudModuleId::VehicleInfo: return "vehicle_info";
        case HudModuleId::HealthArmor: return "health_armor";
        case HudModuleId::Wanted:      return "wanted";
        case HudModuleId::WeaponHud:   return "weapon_hud";
        case HudModuleId::MoneyHud:    return "money_hud";
        default:                       return "module";
    }
}

inline const char* GetModuleDisplayName(HudModuleId id) {
    switch (id) {
        case HudModuleId::Radar:       return "Radar";
        case HudModuleId::GpsNav:      return "GPS Navigation Card";
        case HudModuleId::PlaceInfo:   return "Place Information Card";
        case HudModuleId::VehicleInfo: return "Vehicle Information Card";
        case HudModuleId::HealthArmor: return "Health & Armor Bar";
        case HudModuleId::Wanted:      return "Wanted & Investigation";
        case HudModuleId::WeaponHud:   return "Weapon & Ammo HUD";
        case HudModuleId::MoneyHud:    return "Money Counter HUD";
        default:                       return "Module";
    }
}

inline ModulePositionConfig GetDefaultModulePosition(HudModuleId id) {
    ModulePositionConfig c;
    switch (id) {
        case HudModuleId::Radar:
            c.mode          = PositionMode::Group;
            c.group         = HudGroup::Minimap;
            c.groupIndex    = 0;
            c.posAnchor     = AnchorPoint::BottomLeft;
            c.elementAnchor = AnchorPoint::BottomLeft;
            break;
        case HudModuleId::GpsNav:
            c.mode          = PositionMode::Group;
            c.group         = HudGroup::Minimap;
            c.groupIndex    = 1;
            c.posAnchor     = AnchorPoint::BottomLeft;
            c.elementAnchor = AnchorPoint::BottomLeft;
            break;
        case HudModuleId::PlaceInfo:
            c.mode          = PositionMode::Group;
            c.group         = HudGroup::Minimap;
            c.groupIndex    = 2;
            c.posAnchor     = AnchorPoint::BottomLeft;
            c.elementAnchor = AnchorPoint::BottomLeft;
            break;
        case HudModuleId::VehicleInfo:
            c.mode          = PositionMode::Group;
            c.group         = HudGroup::Minimap;
            c.groupIndex    = 3;
            c.posAnchor     = AnchorPoint::BottomLeft;
            c.elementAnchor = AnchorPoint::BottomLeft;
            break;
        case HudModuleId::HealthArmor:
            c.mode          = PositionMode::Group;
            c.group         = HudGroup::Status;
            c.groupIndex    = 0;
            c.posAnchor     = AnchorPoint::TopLeft;
            c.elementAnchor = AnchorPoint::TopLeft;
            break;
        case HudModuleId::Wanted:
            c.mode          = PositionMode::Group;
            c.group         = HudGroup::Criminal;
            c.groupIndex    = 0;
            c.posAnchor     = AnchorPoint::TopRight;
            c.elementAnchor = AnchorPoint::TopRight;
            break;
        case HudModuleId::WeaponHud:
            c.mode          = PositionMode::Group;
            c.group         = HudGroup::Criminal;
            c.groupIndex    = 1;
            c.posAnchor     = AnchorPoint::TopRight;
            c.elementAnchor = AnchorPoint::TopRight;
            break;
        case HudModuleId::MoneyHud:
            c.mode          = PositionMode::Group;
            c.group         = HudGroup::Criminal;
            c.groupIndex    = 2;
            c.posAnchor     = AnchorPoint::TopRight;
            c.elementAnchor = AnchorPoint::TopRight;
            break;
        default:
            break;
    }
    return c;
}

struct LayoutResult {
    CRect rects[(int)HudModuleId::Count];
};

inline LayoutResult SolveLayout(
    const ModulePositionConfig configs[(int)HudModuleId::Count],
    float sW,
    float sH,
    float radarScale = 1.0f
) {
    LayoutResult res;
    if (sW < 10.0f || sH < 10.0f) return res;

    float radarW  = sH * 0.3230f * radarScale;
    float radarH  = radarW * (9.0f / 16.0f);
    float navH    = sH * 0.064f * radarScale;
    float locH    = sH * 0.040f * radarScale;
    float vehH    = sH * 0.115f * radarScale;
    float hpW     = 120.0f * (sH / 720.0f);
    float hpH     = 18.0f * (sH / 720.0f);
    float wantedW = 150.0f * (sH / 720.0f);
    float wantedH = 49.0f * (sH / 720.0f);
    float wepW    = 120.0f * (sH / 720.0f);
    float wepH    = 76.0f * (sH / 720.0f);
    float moneyW  = 140.0f * (sH / 720.0f);
    float moneyH  = 24.0f * (sH / 720.0f);

    auto GetDim = [&](HudModuleId id, float& outW, float& outH) {
        switch (id) {
            case HudModuleId::Radar:       outW = radarW;  outH = radarH;  break;
            case HudModuleId::GpsNav:      outW = radarW;  outH = navH;    break;
            case HudModuleId::PlaceInfo:   outW = radarW;  outH = locH;    break;
            case HudModuleId::VehicleInfo: outW = radarW;  outH = vehH;    break;
            case HudModuleId::HealthArmor: outW = hpW;     outH = hpH;     break;
            case HudModuleId::Wanted:      outW = wantedW; outH = wantedH; break;
            case HudModuleId::WeaponHud:   outW = wepW;    outH = wepH;    break;
            case HudModuleId::MoneyHud:    outW = moneyW;  outH = moneyH;  break;
            default:                       outW = 100.0f;  outH = 50.0f;   break;
        }
    };

    // 1. Solve Free elements
    for (int i = 0; i < (int)HudModuleId::Count; ++i) {
        if (configs[i].mode != PositionMode::Free) continue;

        float w = 0.0f, h = 0.0f;
        GetDim((HudModuleId)i, w, h);

        float posFx = 0.0f, posFy = 0.0f;
        GetAnchorFactors(configs[i].posAnchor, posFx, posFy);
        float elemFx = 0.0f, elemFy = 0.0f;
        GetAnchorFactors(configs[i].elementAnchor, elemFx, elemFy);

        float targetX = sW * posFx + configs[i].offsetX;
        float targetY = sH * posFy - configs[i].offsetY; // Screen Y goes down, user +Y goes up

        float left = targetX - (w * elemFx);
        float top  = targetY - (h * elemFy);
        res.rects[i] = CRect(left, top, left + w, top + h);
    }

    // 2. Solve Group elements with automatic stack collapse
    struct GroupItem {
        HudModuleId id;
        int         groupIndex;
        float       width;
        float       height;
    };

    std::vector<GroupItem> groups[4]; // 0: Minimap, 1: Status, 2: Criminal, 3: Notification

    for (int i = 0; i < (int)HudModuleId::Count; ++i) {
        if (configs[i].mode != PositionMode::Group) continue;

        int gIdx = (int)configs[i].group;
        if (gIdx < 0 || gIdx >= 4) gIdx = 0;

        float w = 0.0f, h = 0.0f;
        GetDim((HudModuleId)i, w, h);

        groups[gIdx].push_back({ (HudModuleId)i, configs[i].groupIndex, w, h });
    }

    auto SortItems = [](std::vector<GroupItem>& items) {
        std::sort(items.begin(), items.end(), [](const GroupItem& a, const GroupItem& b) {
            if (a.groupIndex != b.groupIndex) return a.groupIndex < b.groupIndex;
            return (int)a.id < (int)b.id;
        });
    };

    for (int g = 0; g < 4; ++g) {
        SortItems(groups[g]);
    }

    // Group 0: Minimap (Bottom-Left, stacks upward)
    {
        float pad = 16.0f * (sH / 720.0f);
        if (pad < 16.0f) pad = 16.0f;
        float gap = sH * 0.008f * radarScale;
        float curBottom = sH - pad;

        for (const auto& item : groups[(int)HudGroup::Minimap]) {
            float left   = pad;
            float top    = curBottom - item.height;
            float right  = left + item.width;
            float bottom = curBottom;
            res.rects[(int)item.id] = CRect(left, top, right, bottom);
            curBottom = top - gap;
        }
    }

    // Group 1: Status (Top-Left, stacks downward)
    {
        float pad = 16.0f * (sH / 720.0f);
        if (pad < 16.0f) pad = 16.0f;
        float gap = 8.0f * (sH / 720.0f);
        float curTop = pad;

        for (const auto& item : groups[(int)HudGroup::Status]) {
            float left   = pad;
            float top    = curTop;
            float right  = left + item.width;
            float bottom = top + item.height;
            res.rects[(int)item.id] = CRect(left, top, right, bottom);
            curTop = bottom + gap;
        }
    }

    // Group 2: Criminal (Top-Right, stacks downward)
    {
        float rightMargin = 28.0f * (sW / 1280.0f);
        float topMargin   = 26.0f * (sH / 720.0f);
        float gap         = 8.0f * (sH / 720.0f);
        float curTop      = topMargin;
        float right       = sW - rightMargin;

        for (const auto& item : groups[(int)HudGroup::Criminal]) {
            float left   = right - item.width;
            float top    = curTop;
            float bottom = top + item.height;
            res.rects[(int)item.id] = CRect(left, top, right, bottom);
            curTop = bottom + gap;
        }
    }

    // Group 3: Notification (Bottom-Right, stacks upward)
    {
        float rightMargin = 28.0f * (sW / 1280.0f);
        float pad         = 16.0f * (sH / 720.0f);
        if (pad < 16.0f) pad = 16.0f;
        float gap         = 8.0f * (sH / 720.0f);
        float curBottom   = sH - pad;
        float right       = sW - rightMargin;

        for (const auto& item : groups[(int)HudGroup::Notification]) {
            float left   = right - item.width;
            float top    = curBottom - item.height;
            float bottom = curBottom;
            res.rects[(int)item.id] = CRect(left, top, right, bottom);
            curBottom = top - gap;
        }
    }

    return res;
}

} // namespace hud6
