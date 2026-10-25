// Gas pump services overlay: a world-anchored "(KEY) Text" prompt, and when open,
// a floating two-row list. No card, no backdrop — text only, like a pickup prompt.
//
// The key badge is drawn as a circular gauge-style disc, matching the vehicle
// telemetry gauges but at a larger radius so it reads at a glance.
//
// The anchor is the pump position projected through the game camera each frame, so
// the text sticks to the world instead of the screen.

namespace {

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

    // Shrink the label to fit inside the disc.
    float fontScale = radius * 0.055f;
    CFont::SetFontStyle(FONT_MENU);
    CFont::SetProportional(true);
    CFont::SetJustify(false);
    CFont::SetOrientation(ALIGN_CENTER);
    CFont::SetEdge(0);
    CFont::SetDropShadowPosition(0);
    CFont::SetScale(fontScale, fontScale * 1.9f);
    CFont::SetColor(textCol);
    CFont::PrintString(cx, cy - radius * 0.42f, badge);
}

// Draws a full row: badge disc, then the label to its right. Returns the row's
// total width so callers can centre the block.
float DrawServiceRow(float leftX, float centerY, float radius, const char* badge,
                     const char* label, CRGBA ringCol, CRGBA fillCol, CRGBA textCol,
                     float progress) {
    const float sH = ScreenH();
    const float scale = sH / 720.0f;

    DrawKeyBadge(leftX + radius, centerY, radius, badge, ringCol, fillCol, textCol, progress);

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
    CFont::PrintString(labelX, centerY - radius * 0.42f, label);

    float labelW = CFont::GetStringWidth(label, true, false);
    return radius * 2.0f + radius * 0.45f + labelW;
}

// Measures a row without drawing it, for centring.
float MeasureServiceRow(float radius, const char* label) {
    const float sH = ScreenH();
    const float scale = sH / 720.0f;
    const float labelScale = 0.62f * scale;

    CFont::SetFontStyle(FONT_MENU);
    CFont::SetProportional(true);
    CFont::SetScale(labelScale, labelScale * 1.9f);
    float labelW = CFont::GetStringWidth(label, true, false);
    return radius * 2.0f + radius * 0.45f + labelW;
}

} // namespace

void Radar::DrawFuelServices() {
    if (!g_config.useVehicleFuel) return;
    if (s_servicePromptAlpha <= 0.005f) return;

    float sx = 0.0f, sy = 0.0f;
    if (!ProjectAnchor(s_serviceAnchor, sx, sy)) return;

    const float sH = ScreenH();
    const float scale = sH / 720.0f;
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
        KeyBadgeName(g_config.serviceKey, g_config.servicePad, badge, sizeof(badge));

        float radius = 20.0f * scale;
        float totalW = MeasureServiceRow(radius, "Services");
        float holdProgress = Clampf(s_serviceHoldTime, 0.0f, 1.0f);
        DrawServiceRow(sx - totalW * 0.5f, sy, radius, badge, "Services", trackCol, discBg, textCol, holdProgress);
        return;
    }

    // Menu open: two rows stacked below the anchor.
    struct RowView { char badge[16]; char label[48]; bool enabled; };
    RowView rows[SERVICE_ROW_COUNT] = {};

    for (int i = 0; i < SERVICE_ROW_COUNT; ++i) {
        KeyBadgeName(g_config.serviceSelectKey, g_config.serviceSelectPad,
                     rows[i].badge, sizeof(rows[i].badge));
    }

    VehicleFuelExt& ext = s_vehicleFuelExt.Get(veh);
    bool tankFull = ext.fuelLevel >= 0.99f;
    bool bodyOk = !BodyNeedsRepair(veh);

    snprintf(rows[SERVICE_ROW_REFUEL].label, sizeof(rows[SERVICE_ROW_REFUEL].label), "%s",
             tankFull ? "Tank is full" : "Refuel");
    rows[SERVICE_ROW_REFUEL].enabled = !tankFull;

    snprintf(rows[SERVICE_ROW_REPAIR].label, sizeof(rows[SERVICE_ROW_REPAIR].label), "%s",
             bodyOk ? "No repair needed" : "Repair");
    rows[SERVICE_ROW_REPAIR].enabled = !bodyOk;

    float radius = 22.0f * scale;
    float rowH = radius * 2.6f;
    float rowY = sy;

    for (int i = 0; i < SERVICE_ROW_COUNT; ++i) {
        bool selected = (i == s_serviceRow);
        CRGBA col = rows[i].enabled ? (selected ? accent : textCol) : dimCol;
        CRGBA ring = rows[i].enabled ? (selected ? accent : trackCol) : trackCol;

        float totalW = MeasureServiceRow(radius, rows[i].label);
        DrawServiceRow(sx - totalW * 0.5f, rowY, radius, rows[i].badge, rows[i].label,
                       ring, discBg, col, 0.0f);
        rowY += rowH;
    }

    // Bottom-right back hint, screen space.
    char backBadge[16] = {};
    KeyBadgeName(g_config.serviceBackKey, g_config.serviceBackPad, backBadge, sizeof(backBadge));

    float backRadius = 16.0f * scale;
    float backW = MeasureServiceRow(backRadius, "Back");
    DrawServiceRow(ScreenW() - backW - 40.0f * scale, ScreenH() - 60.0f * scale,
                   backRadius, backBadge, "Back", trackCol, discBg, textCol, 0.0f);
}
