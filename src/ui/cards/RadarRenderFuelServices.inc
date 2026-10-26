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
const float kServiceScale = 0.72f;

// Reserved width for the row block, so the block does not shift when a row swaps
// between its enabled and disabled label. Measured from the widest possible text.
float ServiceBlockWidth(float radius) {
    const float sH = ScreenH();
    const float scale = (sH / 720.0f) * kServiceScale;
    const float labelScale = 0.62f * scale;

    CFont::SetFontStyle(FONT_MENU);
    CFont::SetProportional(true);
    CFont::SetScale(labelScale, labelScale * 1.9f);

    float widest = CFont::GetStringWidth("No repair needed", true, false);
    widest = std::max(widest, CFont::GetStringWidth("Refuel ($99999)", true, false));
    widest = std::max(widest, CFont::GetStringWidth("Tank is full", true, false));
    return radius * 2.0f + radius * 0.45f + widest;
}

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
    // Gap so the fill ring reads as separate from the background disc.
    const float gap = trackW * 0.7f;
    const float innerR = radius - trackW - gap;
    const float outerR = radius - gap;

    DrawCircle(cx, cy, radius, fillCol, 48);

    // Track ring, then the filled portion on top of it.
    DrawArcStrip(cx, cy, innerR, outerR, 0.0f, 2.0f * kPi, ringCol, 48);

    if (progress > 0.001f) {
        const float startAngle = -kPi * 0.5f;
        const float endAngle   = startAngle + (2.0f * kPi * Clampf(progress, 0.0f, 1.0f));
        DrawArcStrip(cx, cy, innerR, outerR, startAngle, endAngle, textCol, 48);

        // Round cap so the leading edge matches the telemetry gauges.
        const float midRad = (innerR + outerR) * 0.5f;
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
    CFont::SetEdge(1);
    CFont::SetDropShadowPosition(0);
    CFont::SetScale(fontScale, fontScale * 1.9f);

    const float maxWidth = radius * 1.30f;
    float textW = CFont::GetStringWidth(badge, true, false);
    if (textW > maxWidth && textW > 0.0f) {
        float fit = maxWidth / textW;
        CFont::SetScale(fontScale * fit, fontScale * 1.9f * fit);
    }

    CFont::SetColor(textCol);
    CFont::SetDropColor(fillCol);
    CFont::PrintString(cx, cy - radius * 0.30f, badge);
}

// Builds the display line for a row: label plus " ($12)" when priced.
void FormatServiceLine(const char* label, int price, char* out, size_t outSize) {
    if (price >= 0) snprintf(out, outSize, "%s ($%d)", label, price);
    else            snprintf(out, outSize, "%s", label);
}

// Measures a row's total width without drawing it, so the block can be centred
// while the rows inside it stay left-aligned against each other.
float MeasureServiceRow(float radius, const char* label, int price) {
    const float sH = ScreenH();
    const float scale = (sH / 720.0f) * kServiceScale;

    char line[64] = {};
    FormatServiceLine(label, price, line, sizeof(line));

    float labelScale = 0.62f * scale;
    CFont::SetFontStyle(FONT_MENU);
    CFont::SetProportional(true);
    CFont::SetScale(labelScale, labelScale * 1.9f);

    return radius * 2.0f + radius * 0.45f + CFont::GetStringWidth(line, true, false);
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
    FormatServiceLine(label, price, line, sizeof(line));

    float labelScale = 0.62f * scale;
    CFont::SetFontStyle(FONT_MENU);
    CFont::SetProportional(true);
    CFont::SetJustify(false);
    CFont::SetOrientation(ALIGN_LEFT);
    CFont::SetEdge(1);
    CFont::SetDropColor(fillCol);
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
        float totalW = MeasureServiceRow(radius, "Services", -1);
        float holdProgress = Clampf(s_serviceHoldTime, 0.0f, 1.0f);
        DrawServiceRow(sx - totalW * 0.5f, sy, radius, badge, "Services", -1,
                       trackCol, discBg, textCol, holdProgress);
        return;
    }

    // While an operation runs, the rows are replaced by the SAME row geometry: disc
    // where the badge was, operation name where the price text was. Nothing jumps.
    if (s_serviceOp != SERVICE_OP_NONE) {
        float progress = (s_serviceOpFillTime > 0.0f)
                            ? Clampf(s_serviceOpTimer / s_serviceOpFillTime, 0.0f, 1.0f)
                            : 1.0f;
        const char* label = (s_serviceOp == SERVICE_OP_REFUEL) ? "Refuelling..." : "Repairing...";

        char badge[16] = {};
        KeyBadgeName(g_config.serviceSelectKey, g_config.serviceSelectPad, s_serviceUsingPad,
                     badge, sizeof(badge));

        float radius = 22.0f * scale;
        float blockW = ServiceBlockWidth(radius);
        float rowLeft = sx - blockW * 0.5f;

        DrawServiceRow(rowLeft, sy, radius, badge, label, -1, trackCol, discBg, accent, progress);
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

    // s_vehFuel is what the telemetry gauge shows, so the menu agrees with it. The
    // ext copy can lag behind, which is what made a half-full tank read as half empty.
    bool tankFull = s_vehFuel >= 0.99f;
    bool needsRepair = NeedsRepair(veh);

    snprintf(rows[SERVICE_ROW_REFUEL].label, sizeof(rows[SERVICE_ROW_REFUEL].label), "%s",
             tankFull ? "Tank is full" : "Refuel");
    rows[SERVICE_ROW_REFUEL].enabled = !tankFull;
    if (!tankFull) rows[SERVICE_ROW_REFUEL].price = RefuelPrice(s_serviceAnchor, s_vehFuel);

    snprintf(rows[SERVICE_ROW_REPAIR].label, sizeof(rows[SERVICE_ROW_REPAIR].label), "%s",
             needsRepair ? "Repair" : "No repair needed");
    rows[SERVICE_ROW_REPAIR].enabled = needsRepair;
    if (needsRepair) rows[SERVICE_ROW_REPAIR].price = RepairPrice(veh);

    float radius = 22.0f * scale;
    float rowH = radius * 2.6f;
    float rowY = sy;

    // Centre the block on the anchor using a RESERVED width, so the position does
    // not shift when a row swaps between its enabled and disabled label.
    float blockW = ServiceBlockWidth(radius);
    float rowLeft = sx - blockW * 0.5f;

    // Progress of a running operation drives the active row's ring.
    float opProgress = 0.0f;
    if (s_serviceOp != SERVICE_OP_NONE && s_serviceOpDuration > 0.0f) {
        opProgress = Clampf(s_serviceOpTimer / s_serviceOpDuration, 0.0f, 1.0f);
    }

    for (int i = 0; i < SERVICE_ROW_COUNT; ++i) {
        bool selected = (i == s_serviceRow);
        CRGBA col = rows[i].enabled ? (selected ? accent : textCol) : dimCol;
        // The track is always the dim ring. Drawing it in the accent colour hid the
        // fill arc against it, so the progress sweep was invisible.
        CRGBA ring = trackCol;

        // Only the row whose operation is running shows a fill.
        float ringProgress = 0.0f;
        if (s_serviceOp == SERVICE_OP_REFUEL && i == SERVICE_ROW_REFUEL) ringProgress = opProgress;
        if (s_serviceOp == SERVICE_OP_REPAIR && i == SERVICE_ROW_REPAIR) ringProgress = opProgress;

        DrawServiceRow(rowLeft, rowY, radius, rows[i].badge, rows[i].label, rows[i].price,
                       ring, discBg, col, ringProgress);
        rowY += rowH;
    }

    // Bottom-right back hint, screen space.
    char backBadge[16] = {};
    KeyBadgeName(g_config.serviceBackKey, g_config.serviceBackPad, s_serviceUsingPad, backBadge, sizeof(backBadge));

    float backRadius = 16.0f * scale;
    float backW = MeasureServiceRow(backRadius, "Back", -1);

    DrawServiceRow(ScreenW() - backW - 40.0f * scale, ScreenH() - 60.0f * scale,
                   backRadius, backBadge, "Back", -1, trackCol, discBg, textCol, 0.0f);
}
