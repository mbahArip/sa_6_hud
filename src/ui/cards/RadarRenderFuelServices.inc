// Gas pump services overlay: a world-anchored "(KEY) Text" prompt, and when open,
// a floating two-row list. No card, no backdrop — text only, like a pickup prompt.
//
// The anchor is the pump position projected through the game camera each frame, so
// the text sticks to the world instead of the screen.

namespace {

struct ServiceRowView {
    char badge[16];
    char label[48];
    bool enabled;
};

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

// Draws "(KEY) Text" starting at the left edge x, vertically centred on y.
// Returns the total width so callers can right-align if needed.
float DrawKeyPrompt(float x, float y, const char* badge, const char* label, CRGBA col) {
    const float sH = ScreenH();
    const float scale = sH / 720.0f;
    const float fontScale = 0.52f * scale;

    CFont::SetFontStyle(FONT_MENU);
    CFont::SetProportional(true);
    CFont::SetJustify(false);
    CFont::SetOrientation(ALIGN_LEFT);
    CFont::SetEdge(0);
    CFont::SetDropShadowPosition(0);
    CFont::SetScale(fontScale, fontScale * 1.9f);

    char buf[96] = {};
    snprintf(buf, sizeof(buf), "(%s)", badge);

    CFont::SetColor(ModColor(g_config.palettePrimary, col.a / 255.0f));
    float badgeW = CFont::GetStringWidth(buf, true, false);
    CFont::PrintString(x, y, buf);

    CFont::SetColor(col);
    float textX = x + badgeW + 6.0f * scale;
    CFont::PrintString(textX, y, label);

    float labelW = CFont::GetStringWidth(label, true, false);
    return badgeW + 6.0f * scale + labelW;
}

} // namespace

void Radar::DrawFuelServices() {
    if (!g_config.useVehicleFuel) return;
    if (s_servicePromptAlpha <= 0.005f) return;

    float sx = 0.0f, sy = 0.0f;
    if (!ProjectAnchor(s_serviceAnchor, sx, sy)) return;

    const float sH = ScreenH();
    const float scale = sH / 720.0f;
    const float lineH = 22.0f * scale;
    const float alphaMod = s_servicePromptAlpha;

    CRGBA textCol = ModColor(CRGBA(240, 240, 245, 255), alphaMod);
    CRGBA dimCol  = ModColor(CRGBA(140, 138, 150, 255), alphaMod);
    CRGBA accent  = ModColor(g_config.palettePrimary, alphaMod);

    CPlayerPed* player = FindPlayerPed(0);
    CVehicle* veh = player ? GetStablePlayerVehicle(player) : nullptr;
    float fuel = veh ? s_vehicleFuelExt.Get(veh).fuelLevel : 1.0f;

    if (!s_serviceOpen) {
        char badge[16] = {};
        KeyBadgeName(g_config.serviceKey, g_config.servicePad, badge, sizeof(badge));

        // Centre the prompt horizontally on the anchor.
        char probe[96] = {};
        snprintf(probe, sizeof(probe), "(%s)", badge);
        CFont::SetFontStyle(FONT_MENU);
        CFont::SetProportional(true);
        CFont::SetScale(0.52f * scale, 0.99f * scale);
        float totalW = CFont::GetStringWidth(probe, true, false) + 6.0f * scale
                     + CFont::GetStringWidth("Services", true, false);

        DrawKeyPrompt(sx - totalW * 0.5f, sy, badge, "Services", textCol);
        return;
    }

    // Menu open: two rows stacked below the anchor.
    ServiceRowView rows[SERVICE_ROW_COUNT] = {};
    for (int i = 0; i < SERVICE_ROW_COUNT; ++i) {
        KeyBadgeName(g_config.serviceSelectKey, g_config.serviceSelectPad,
                     rows[i].badge, sizeof(rows[i].badge));
    }

    bool tankFull = fuel >= 0.99f;
    bool undamaged = veh && veh->m_fHealth >= 995.0f;

    snprintf(rows[SERVICE_ROW_REFUEL].label, sizeof(rows[SERVICE_ROW_REFUEL].label), "%s",
             tankFull ? "Tank is full" : "Refuel");
    rows[SERVICE_ROW_REFUEL].enabled = !tankFull;

    snprintf(rows[SERVICE_ROW_REPAIR].label, sizeof(rows[SERVICE_ROW_REPAIR].label), "%s",
             undamaged ? "No repair needed" : "Repair");
    rows[SERVICE_ROW_REPAIR].enabled = !undamaged;

    float rowY = sy;
    for (int i = 0; i < SERVICE_ROW_COUNT; ++i) {
        bool selected = (i == s_serviceRow);
        CRGBA col = rows[i].enabled ? (selected ? accent : textCol) : dimCol;

        char probe[96] = {};
        snprintf(probe, sizeof(probe), "(%s)", rows[i].badge);
        CFont::SetFontStyle(FONT_MENU);
        CFont::SetProportional(true);
        CFont::SetScale(0.52f * scale, 0.99f * scale);
        float totalW = CFont::GetStringWidth(probe, true, false) + 6.0f * scale
                     + CFont::GetStringWidth(rows[i].label, true, false);

        DrawKeyPrompt(sx - totalW * 0.5f, rowY, rows[i].badge, rows[i].label, col);
        rowY += lineH;
    }

    // Bottom-right back hint, screen space.
    char backBadge[16] = {};
    KeyBadgeName(g_config.serviceBackKey, g_config.serviceBackPad, backBadge, sizeof(backBadge));

    CFont::SetFontStyle(FONT_MENU);
    CFont::SetProportional(true);
    CFont::SetScale(0.52f * scale, 0.99f * scale);
    char backProbe[96] = {};
    snprintf(backProbe, sizeof(backProbe), "(%s)", backBadge);
    float backW = CFont::GetStringWidth(backProbe, true, false) + 6.0f * scale
                + CFont::GetStringWidth("Back", true, false);

    DrawKeyPrompt(ScreenW() - backW - 40.0f * scale, ScreenH() - 60.0f * scale,
                  backBadge, "Back", textCol);
}
