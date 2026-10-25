// Gas pump services overlay: a world-anchored "(KEY) Text" prompt, and when open,
// a floating two-row list. No card, no backdrop — text only, like a pickup prompt.
//
// The key badge is drawn as a circular gauge-style disc, matching the vehicle
// telemetry gauges but at a larger radius so it reads at a glance.
//
// The anchor is the pump position projected through the game camera each frame, so
// the text sticks to the world instead of the screen.

namespace {

// Hardcoded UI scale for the services overlay. Kept as a constant because the
// layout is tuned by eye, not by players.
const float kServiceScale = 0.80f;

// Projects a world position to screen pixels. Returns false when the point is
// behind the camera or off screen.
bool ProjectAnchor(const CVector& worldPos, float& outX, float& outY) {
    RwV3d in = { worldPos.x, worldPos.y, worldPos.z };
    RwV3d out = {};
    float w = 0.0f, h = 0.0f;
    if (!CSprite::CalcScreenCoors(in, &out, &w, &h, true, true)) return false;
    outX = out.x;
    outY = out.y;
    return true;
}

// Circular key badge: a filled disc with a track ring, the key name centred
// inside. Mirrors the telemetry gauge look at roughly 1.8x the radius.
// `progress` fills the ring from 12 o'clock clockwise, 0..1.
void DrawKeyBadge(float cx, float cy, float radius, const char* badge,
                  CRGBA ringCol, CRGBA fillCol, CRGBA textCol, float progress) {
    const float trackW = radius * 0.16f;

    DrawCircle(cx, cy, radius, fillCol, 48);

    // Track ring, then the filled portion on top of it.
    DrawArcStrip(cx, cy, radius - trackW, radius, 0.0f, 2.0f * kPi, ringCol, 48);

    if (progress > 0.001f) {
        const float startAngle = -kPi * 0.5f;
        const float endAngle   = startAngle + (2.0f * kPi * Clampf(progress, 0.0f, 1.0f));
        DrawArcStrip(cx, cy, radius - trackW, radius, startAngle, endAngle, textCol, 48);

        // Round cap so the leading edge matches the telemetry gauges.
        const float midRad = radius - trackW * 0.5f;
        DrawCircle(cx + midRad * cosf(endAngle), cy + midRad * sinf(endAngle), trackW * 0.5f, textCol, 24);
    }

    // Shrink the label to fit inside the disc. Long names like SPACE, ENTER and
    // DDOWN need to be measured, not assumed: the naive radius-proportional scale
    // overflowed the ring for anything past ~3 characters.
    float fontScale = radius * 0.030f;
    CFont::SetFontStyle(FONT_MENU);
    CFont::SetProportional(true);
    CFont::SetJustify(false);
    CFont::SetOrientation(ALIGN_CENTER);
    CFont::SetEdge(0);
    CFont::SetDropShadowPosition(0);
    CFont::SetScale(fontScale, fontScale * 1.9f);

    const float maxWidth = radius * 1.30f;
    float textW = CFont::GetStringWidth(badge, true, false);
    if (textW > maxWidth && textW > 0.0f) {
        float fit = maxWidth / textW;
        CFont::SetScale(fontScale * fit, fontScale * 1.9f * fit);
    }

    CFont::SetColor(textCol);
    CFont::PrintString(cx, cy - radius * 0.30f, badge);
}

// Draws a full row: badge disc, then the label to its right. `price` is appended
// as " ($12)" when non-negative.
void DrawServiceRow(float leftX, float centerY, float radius, const char* badge,
                    const char* label, int price, CRGBA ringCol, CRGBA fillCol,
                    CRGBA textCol, float progress) {
    const float sH = ScreenH();
    const float scale = (sH / 720.0f) * kServiceScale;

    DrawKeyBadge(leftX + radius, centerY, radius, badge, ringCol, fillCol, textCol, progress);

    char line[64] = {};
    if (price >= 0) snprintf(line, sizeof(line), "%s ($%d)", label, price);
    else            snprintf(line, sizeof(line), "%s", label);

    float labelScale = 0.62f * scale;
    CFont::SetFontStyle(FONT_MENU);
    CFont::SetProportional(true);
    CFont::SetJustify(false);
    CFont::SetOrientation(ALIGN_LEFT);
    CFont::SetEdge(0);
    CFont::SetDropShadowPosition(0);
    CFont::SetScale(labelScale, labelScale * 1.9f);
    CFont::SetColor(textCol);

    float labelX = leftX + radius * 2.0f + radius * 0.45f;
    CFont::PrintString(labelX, centerY - radius * 0.42f, line);
}

} // namespace

void Radar::DrawFuelServices() {
    if (!g_config.useVehicleFuel) return;
    if (s_servicePromptAlpha <= 0.005f) return;

    float sx = 0.0f, sy = 0.0f;
    if (!ProjectAnchor(s_serviceAnchor, sx, sy)) return;

    const float sH = ScreenH();
    const float scale = (sH / 720.0f) * kServiceScale;
    const float alphaMod = s_servicePromptAlpha;

    CRGBA textCol  = ModColor(CRGBA(240, 240, 245, 255), alphaMod);
    CRGBA dimCol   = ModColor(CRGBA(140, 138, 150, 255), alphaMod);
    CRGBA accent   = ModColor(g_config.palettePrimary, alphaMod);
    CRGBA discBg   = ModColor(CRGBA(14, 18, 24, 190), alphaMod);
    CRGBA trackCol = ModColor(CRGBA(60, 68, 80, 140), alphaMod);

    CPlayerPed* player = FindPlayerPed(0);
    CVehicle* veh = player ? GetStablePlayerVehicle(player) : nullptr;

    if (!s_serviceOpen) {
        char badge[16] = {};
        KeyBadgeName(g_config.serviceKey, g_config.servicePad, s_serviceUsingPad, badge, sizeof(badge));

        float radius = 20.0f * scale;
        float holdProgress = Clampf(s_serviceHoldTime, 0.0f, 1.0f);
        DrawServiceRow(sx, sy, radius, badge, "Services", -1, trackCol, discBg, textCol, holdProgress);
        return;
    }

    // Menu open: two rows stacked below the anchor, left-aligned on it.
    struct RowView { char badge[16]; char label[48]; bool enabled; int price; };
    RowView rows[SERVICE_ROW_COUNT] = {};

    for (int i = 0; i < SERVICE_ROW_COUNT; ++i) {
        KeyBadgeName(g_config.serviceSelectKey, g_config.serviceSelectPad, s_serviceUsingPad,
                     rows[i].badge, sizeof(rows[i].badge));
        rows[i].price = -1;
    }

    VehicleFuelExt& ext = s_vehicleFuelExt.Get(veh);
    bool tankFull = ext.fuelLevel >= 0.99f;
    bool needsRepair = NeedsRepair(veh);

    snprintf(rows[SERVICE_ROW_REFUEL].label, sizeof(rows[SERVICE_ROW_REFUEL].label), "%s",
             tankFull ? "Tank is full" : "Refuel");
    rows[SERVICE_ROW_REFUEL].enabled = !tankFull;
    if (!tankFull) rows[SERVICE_ROW_REFUEL].price = RefuelPrice(s_serviceAnchor, ext.fuelLevel);

    snprintf(rows[SERVICE_ROW_REPAIR].label, sizeof(rows[SERVICE_ROW_REPAIR].label), "%s",
             needsRepair ? "Repair" : "No repair needed");
    rows[SERVICE_ROW_REPAIR].enabled = needsRepair;
    if (needsRepair) rows[SERVICE_ROW_REPAIR].price = RepairPrice(veh);

    float radius = 22.0f * scale;
    float rowH = radius * 2.6f;
    float rowY = sy;

    for (int i = 0; i < SERVICE_ROW_COUNT; ++i) {
        bool selected = (i == s_serviceRow);
        CRGBA col = rows[i].enabled ? (selected ? accent : textCol) : dimCol;
        CRGBA ring = rows[i].enabled ? (selected ? accent : trackCol) : trackCol;

        DrawServiceRow(sx, rowY, radius, rows[i].badge, rows[i].label, rows[i].price,
                       ring, discBg, col, 0.0f);
        rowY += rowH;
    }

    // Bottom-right back hint, screen space.
    char backBadge[16] = {};
    KeyBadgeName(g_config.serviceBackKey, g_config.serviceBackPad, s_serviceUsingPad, backBadge, sizeof(backBadge));

    float backRadius = 16.0f * scale;
    const float backScale = 0.62f * scale;
    CFont::SetFontStyle(FONT_MENU);
    CFont::SetProportional(true);
    CFont::SetScale(backScale, backScale * 1.9f);
    float backW = CFont::GetStringWidth("Back", true, false) + backRadius * 2.45f;

    DrawServiceRow(ScreenW() - backW - 40.0f * scale, ScreenH() - 60.0f * scale,
                   backRadius, backBadge, "Back", -1, trackCol, discBg, textCol, 0.0f);
}
