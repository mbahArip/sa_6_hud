// Gas pump services overlay: a world-anchored "(KEY) Text" prompt, and when open,
// a floating 1.5-row sliding menu with connected track strip and top/bottom gradient fade.
//
// The active row is centered at the anchor Y. When the player navigates Up/Down, the
// list slides smoothly. The active item displays the full circular button badge, while
// inactive items display a connected pip node with a vertical strip between them.
//
// The anchor is the pump position projected through the game camera each frame, so
// the text sticks to the world instead of the screen.

namespace {

// Hardcoded UI scale for the services overlay. Kept as a constant because the
// layout is tuned by eye, not by players.
const float kServiceScale = 0.72f;

// Builds the display line for a row: label plus " ($12)" when priced, converted to uppercase.
void FormatServiceLine(const char* label, int price, char* out, size_t outSize) {
    char buf[64] = {};
    if (price >= 0) snprintf(buf, sizeof(buf), "%s ($%d)", label, price);
    else            snprintf(buf, sizeof(buf), "%s", label);
    for (size_t i = 0; i < outSize - 1 && buf[i]; ++i) {
        out[i] = (char)toupper((unsigned char)buf[i]);
    }
}

// Reserved width for the row block, so the block does not shift when a row swaps
// between its enabled and disabled label. Measured from the widest possible text.
float ServiceBlockWidth(float radius) {
    const float sH = ScreenH();
    const float scale = (sH / 720.0f) * kServiceScale;
    const float labelScale = 0.62f * scale;

    CFont::SetFontStyle(FONT_MENU);
    CFont::SetProportional(true);
    CFont::SetScale(labelScale, labelScale * 1.9f);

    float widest = CFont::GetStringWidth("NO REPAIR NEEDED", true, false);
    widest = std::max(widest, CFont::GetStringWidth("REFUEL ($99999)", true, false));
    widest = std::max(widest, CFont::GetStringWidth("TANK IS FULL", true, false));
    return radius * 2.0f + 48.0f * scale + widest;
}

// Circular key badge: a filled disc with a track ring, the key name centred inside.
// Mirrors the telemetry gauge look at roughly 1.8x the radius.
// `progress` fills the ring from 12 o'clock clockwise, 0..1.
void DrawKeyBadge(float cx, float cy, float radius, const char* badge,
                  CRGBA ringCol, CRGBA fillCol, CRGBA textCol, float progress) {
    const float trackW = radius * 0.16f;
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

        const float midRad = (innerR + outerR) * 0.5f;
        DrawCircle(cx + midRad * cosf(endAngle), cy + midRad * sinf(endAngle), trackW * 0.5f, textCol, 24);
    }

    if (!badge || badge[0] == '\0') return;

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

// Segmented vertical strip connecting the badge/pip column between rows with hardware Gouraud fade.
template<typename FadeFunc>
void DrawVerticalStrip(float cx, float topY, float botY, float width,
                       FadeFunc fadeFn, float alphaMod) {
    if (topY >= botY) return;

    IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
    if (!dev) return;

    BeginRawD3DQuad(dev);
    dev->SetTexture(0, nullptr);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    dev->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    const int kSegments = 24;
    static D3D2DVert verts[(kSegments + 1) * 2];
    float halfW = width * 0.5f;

    for (int i = 0; i <= kSegments; ++i) {
        float t = (float)i / (float)kSegments;
        float y = Lerpf(topY, botY, t);
        float a = fadeFn(y) * alphaMod;
        CRGBA col = ModColor(CRGBA(200, 205, 215, 175), a);
        DWORD dwCol = D3DCOLOR_ARGB(col.a, col.r, col.g, col.b);

        verts[i * 2 + 0] = { cx - halfW - 0.5f, y - 0.5f, 0.0f, 1.0f, dwCol, 0.0f, 0.0f };
        verts[i * 2 + 1] = { cx + halfW - 0.5f, y - 0.5f, 0.0f, 1.0f, dwCol, 1.0f, 0.0f };
    }

    dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, kSegments * 2, verts, sizeof(D3D2DVert));

    // Restore Texture Stage 0 states so subsequent font/HUD rendering doesn't get corrupted
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    dev->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
}

// Three-dot vertical grip separator with circular gauge background (disc + track ring, no radial fill).
void DrawServiceGrip(float cx, float cy, float scale, CRGBA dotCol, CRGBA discBg, CRGBA trackCol) {
    if (dotCol.a == 0) return;
    float chipRad = 9.5f * scale;
    float trackW  = 1.5f * scale;

    // Background disc
    DrawCircle(cx, cy, chipRad, discBg, 24);
    // Outer border track ring
    DrawArcStrip(cx, cy, chipRad - trackW, chipRad, 0.0f, 2.0f * kPi, trackCol, 24);

    // Three centered dots
    float dotR = 1.15f * scale;
    float spread = 3.6f * scale;
    DrawCircle(cx, cy - spread, dotR, dotCol, 12);
    DrawCircle(cx, cy,          dotR, dotCol, 12);
    DrawCircle(cx, cy + spread, dotR, dotCol, 12);
}

// Measures single prompt width for closed state or Back prompt.
float MeasureServicePrompt(float radius, const char* label, int price) {
    const float sH = ScreenH();
    const float scale = (sH / 720.0f) * kServiceScale;

    char line[64] = {};
    FormatServiceLine(label, price, line, sizeof(line));

    float labelScale = 0.62f * scale;
    CFont::SetFontStyle(FONT_MENU);
    CFont::SetProportional(true);
    CFont::SetScale(labelScale, labelScale * 1.9f);

    return radius * 2.0f + 10.0f * scale + CFont::GetStringWidth(line, true, false);
}

// Draws static single row prompt (for initial closed state and Back hint).
void DrawServicePromptRow(float leftX, float centerY, float radius, const char* badge,
                          const char* label, int price, CRGBA ringCol, CRGBA fillCol,
                          CRGBA textCol, float progress, float alphaMod) {
    if (alphaMod <= 0.005f) return;
    const float sH = ScreenH();
    const float scale = (sH / 720.0f) * kServiceScale;

    float badgeX = leftX + radius;
    float labelX = leftX + radius * 2.0f + 10.0f * scale;

    DrawKeyBadge(badgeX, centerY, radius, badge, ModColor(ringCol, alphaMod),
                 ModColor(fillCol, alphaMod), ModColor(textCol, alphaMod), progress);

    char line[64] = {};
    FormatServiceLine(label, price, line, sizeof(line));

    float labelScale = 0.62f * scale;
    CFont::SetFontStyle(FONT_MENU);
    CFont::SetProportional(true);
    CFont::SetJustify(false);
    CFont::SetOrientation(ALIGN_LEFT);
    CFont::SetEdge(1);
    CFont::SetDropColor(ModColor(fillCol, alphaMod));
    CFont::SetDropShadowPosition(0);
    CFont::SetScale(labelScale, labelScale * 1.9f);
    CFont::SetColor(ModColor(textCol, alphaMod));
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
    CRGBA discBg   = ModColor(CRGBA(14, 18, 24, 210), alphaMod);
    CRGBA trackCol = ModColor(CRGBA(60, 68, 80, 140), alphaMod);

    CPlayerPed* player = FindPlayerPed(0);
    CVehicle* veh = player ? GetStablePlayerVehicle(player) : nullptr;

    if (!s_serviceOpen) {
        char badge[16] = {};
        KeyBadgeName(g_config.serviceKey, g_config.servicePad, s_serviceUsingPad, badge, sizeof(badge));

        float radius = 20.0f * scale;
        float totalW = MeasureServicePrompt(radius, "Services", -1);
        float holdProgress = Clampf(s_serviceHoldTime, 0.0f, 1.0f);
        DrawServicePromptRow(sx - totalW * 0.5f, sy, radius, badge, "Services", -1,
                             trackCol, discBg, textCol, holdProgress, alphaMod);
        return;
    }

    // While an operation runs, the active row is locked at the center with a circular progress fill.
    if (s_serviceOp != SERVICE_OP_NONE) {
        float progress = (s_serviceOpFillTime > 0.0f)
                            ? Clampf(s_serviceOpTimer / s_serviceOpFillTime, 0.0f, 1.0f)
                            : 1.0f;
        const char* label = (s_serviceOp == SERVICE_OP_REFUEL) ? "Refuelling..." : "Repairing...";

        float radius = 22.0f * scale;
        float blockW = ServiceBlockWidth(radius);
        float rowLeft = sx - blockW * 0.5f;

        DrawServicePromptRow(rowLeft, sy, radius, "", label, -1,
                             trackCol, discBg, accent, progress, alphaMod);
        return;
    }

    // Menu open: 1.5-row viewport sliding carousel.
    struct RowView { char badge[16]; char label[48]; bool enabled; int price; };
    RowView rows[SERVICE_ROW_COUNT] = {};

    for (int i = 0; i < SERVICE_ROW_COUNT; ++i) {
        KeyBadgeName(g_config.serviceSelectKey, g_config.serviceSelectPad, s_serviceUsingPad,
                     rows[i].badge, sizeof(rows[i].badge));
        rows[i].price = -1;
    }

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
    float rowH   = radius * 2.45f;

    float blockW  = ServiceBlockWidth(radius);
    float rowLeft = sx - blockW * 0.5f;

    // Viewport Scissor Rect: allows adjacent item to sit naturally in view with its outer edge fading out.
    IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
    RECT oldScissor = {};
    DWORD scissorEnabled = 0;
    if (dev) {
        dev->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissorEnabled);
        dev->GetScissorRect(&oldScissor);

        RECT menuScissor;
        menuScissor.left   = (LONG)(rowLeft - 15.0f);
        menuScissor.right  = (LONG)(rowLeft + blockW + 40.0f);
        menuScissor.top    = (LONG)(sy - 1.25f * rowH);
        menuScissor.bottom = (LONG)(sy + 1.25f * rowH);

        menuScissor.left   = std::max<LONG>(0, menuScissor.left);
        menuScissor.top    = std::max<LONG>(0, menuScissor.top);
        menuScissor.right  = std::min<LONG>((LONG)ScreenW(), menuScissor.right);
        menuScissor.bottom = std::min<LONG>((LONG)ScreenH(), menuScissor.bottom);

        dev->SetScissorRect(&menuScissor);
        dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    }

    // Row positions based on smooth scroll index
    float rowY[SERVICE_ROW_COUNT];
    for (int i = 0; i < SERVICE_ROW_COUNT; ++i) {
        rowY[i] = sy + ((float)i - s_serviceScrollRow) * rowH;
    }

    // Alpha fade helper: 100% opacity for center item, ramping to 0% at 75% of adjacent item (d = 1.25 * rowH)
    auto CalcRowFade = [&](float y) -> float {
        float d = fabsf(y - sy);
        if (d <= 0.50f * rowH) return 1.0f;
        if (d >= 1.25f * rowH) return 0.0f;
        float t = (d - 0.50f * rowH) / (0.75f * rowH);
        return Clampf(1.0f - t, 0.0f, 1.0f);
    };

    // 1. Draw vertical connecting strip between row nodes without overlapping into node circles
    float badgeX = rowLeft + radius;
    int topIdx = (rowY[0] <= rowY[1]) ? 0 : 1;
    int botIdx = (rowY[0] <= rowY[1]) ? 1 : 0;

    float topDist = fabsf(rowY[topIdx] - sy);
    float topWeight = Clampf(1.0f - (topDist / (0.85f * rowH)), 0.0f, 1.0f);
    float topR = Lerpf(radius * 0.28f, radius, topWeight);

    float botDist = fabsf(rowY[botIdx] - sy);
    float botWeight = Clampf(1.0f - (botDist / (0.85f * rowH)), 0.0f, 1.0f);
    float botR = Lerpf(radius * 0.28f, radius, botWeight);

    float stripTop = rowY[topIdx] + topR;
    float stripBot = rowY[botIdx] - botR;

    DrawVerticalStrip(badgeX, stripTop, stripBot, 2.0f * scale, CalcRowFade, alphaMod);

    // 2. Draw each row (Badge / Pip + Grip + Label)
    float gripX  = rowLeft + radius * 2.0f + 16.0f * scale;
    float labelX = gripX + 18.0f * scale;

    for (int i = 0; i < SERVICE_ROW_COUNT; ++i) {
        float y = rowY[i];
        float fade = CalcRowFade(y);
        float itemAlpha = fade * alphaMod;
        if (itemAlpha <= 0.005f) continue;

        float distFromCenter = fabsf(y - sy);
        float selectWeight   = Clampf(1.0f - (distFromCenter / (0.85f * rowH)), 0.0f, 1.0f);

        // A. Active Key Badge vs Inactive Pip
        if (selectWeight > 0.05f) {
            CRGBA bRing = ModColor(trackCol, selectWeight * itemAlpha);
            CRGBA bFill = ModColor(discBg,   selectWeight * itemAlpha);
            CRGBA bText = ModColor(textCol,  selectWeight * itemAlpha);
            float bRad  = radius * Lerpf(0.65f, 1.0f, selectWeight);
            DrawKeyBadge(badgeX, y, bRad, rows[i].badge, bRing, bFill, bText, 0.0f);
        }
        if (selectWeight < 0.95f) {
            float pipAlpha = (1.0f - selectWeight) * itemAlpha;
            CRGBA pipCol = ModColor(CRGBA(215, 220, 230, 220), pipAlpha);
            DrawCircle(badgeX, y, radius * 0.28f, pipCol, 24);
        }

        // B. Grip Dots with gauge circular background (Image 1 style)
        CRGBA gripDotCol   = ModColor(CRGBA(215, 220, 230, 220), itemAlpha);
        CRGBA gripDiscBg   = ModColor(discBg, itemAlpha);
        CRGBA gripTrackCol = ModColor(trackCol, itemAlpha);
        DrawServiceGrip(gripX, y, scale, gripDotCol, gripDiscBg, gripTrackCol);

        // C. Label Text
        char line[64] = {};
        FormatServiceLine(rows[i].label, rows[i].price, line, sizeof(line));

        bool isSelected = (i == s_serviceRow);
        CRGBA baseCol = rows[i].enabled ? (isSelected ? accent : textCol) : dimCol;

        float labelScale = 0.62f * scale;
        CFont::SetFontStyle(FONT_MENU);
        CFont::SetProportional(true);
        CFont::SetJustify(false);
        CFont::SetOrientation(ALIGN_LEFT);
        CFont::SetEdge(1);
        CFont::SetDropColor(ModColor(discBg, itemAlpha));
        CFont::SetDropShadowPosition(0);
        CFont::SetScale(labelScale, labelScale * 1.9f);
        CFont::SetColor(ModColor(baseCol, itemAlpha));
        CFont::PrintString(labelX, y - radius * 0.42f, line);
    }

    // Restore scissor state
    if (dev) {
        dev->SetScissorRect(&oldScissor);
        dev->SetRenderState(D3DRS_SCISSORTESTENABLE, scissorEnabled);
    }

    // Bottom-right back hint, screen space.
    char backBadge[16] = {};
    KeyBadgeName(g_config.serviceBackKey, g_config.serviceBackPad, s_serviceUsingPad, backBadge, sizeof(backBadge));

    float backRadius = 16.0f * scale;
    float backW = MeasureServicePrompt(backRadius, "Back", -1);

    DrawServicePromptRow(ScreenW() - backW - 40.0f * scale, ScreenH() - 60.0f * scale,
                         backRadius, backBadge, "Back", -1, trackCol, discBg, textCol, 0.0f, alphaMod);
}
