static float ScreenW() { return (float)RsGlobal.maximumWidth; }
static float ScreenH() { return (float)RsGlobal.maximumHeight; }

static void UpdateRadarRect() {
    float sH = ScreenH();
    float sW = ScreenW();
    if (sH < 10.0f || sW < 10.0f) return;

    const Config& activeCfg = (Menu::IsOpen() ? g_pendingConfig : g_config);
    LayoutResult layout = SolveLayout(activeCfg.modulePositions, sW, sH, activeCfg.radarSizeScale);
    LayoutResult anim   = SolveAnimatedLayout(activeCfg.modulePositions, g_moduleAlpha, sW, sH, activeCfg.radarSizeScale);

    for (int i = 0; i < (int)HudModuleId::Count; ++i) {
        g_moduleRects[i]         = layout.rects[i];
        g_animatedModuleRects[i] = anim.rects[i];
    }
    g_radarRect = g_moduleRects[(int)HudModuleId::Radar];
}

struct RibbonPoint {
    float x, y;
    float alpha;
    bool valid;
};

static void RenderTriangleStripRibbon(const RibbonPoint* pts, int count, float halfWidth, CRGBA col, bool dotted = false) {
    if (count < 2 || col.a == 0 || halfWidth <= 0.1f) return;

    static RwIm2DVertex verts[8192];
    int vCount = 0;
    float accDist = 0.0f;

    for (int i = 0; i < count && vCount + 2 <= 8180; ++i) {
        float x = pts[i].x, y = pts[i].y;
        if (i > 0) {
            float dx = x - pts[i - 1].x;
            float dy = y - pts[i - 1].y;
            accDist += sqrtf(dx * dx + dy * dy);
        }
        float dashFactor = 1.0f;
        if (dotted) {
            dashFactor = (fmodf(accDist, 24.0f) < 14.0f) ? 1.0f : 0.0f;
        }

        float mx = 0.0f, my = 0.0f;

        if (i == 0) {
            float dx = pts[1].x - pts[0].x, dy = pts[1].y - pts[0].y;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 0.001f) {
                mx = -dy / len;
                my = dx / len;
            }
        } else if (i == count - 1) {
            float dx = pts[count - 1].x - pts[count - 2].x, dy = pts[count - 1].y - pts[count - 2].y;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 0.001f) {
                mx = -dy / len;
                my = dx / len;
            }
        } else {
            float d1x = pts[i].x - pts[i - 1].x, d1y = pts[i].y - pts[i - 1].y;
            float d2x = pts[i + 1].x - pts[i].x, d2y = pts[i + 1].y - pts[i].y;
            float l1 = sqrtf(d1x * d1x + d1y * d1y);
            float l2 = sqrtf(d2x * d2x + d2y * d2y);
            if (l1 > 0.001f && l2 > 0.001f) {
                float n1x = -d1y / l1, n1y = d1x / l1;
                float n2x = -d2y / l2, n2y = d2x / l2;
                mx = (n1x + n2x) * 0.5f;
                my = (n1y + n2y) * 0.5f;
                float mlen2 = mx * mx + my * my;
                if (mlen2 > 0.001f) {
                    float inv = 1.0f / sqrtf(mlen2);
                    if (inv > 1.35f) inv = 1.35f;
                    mx *= inv;
                    my *= inv;
                }
            }
        }

        float a = pts[i].alpha * dashFactor;
        unsigned char vertAlpha = (unsigned char)(col.a * a);

        RwIm2DVertexSetScreenX(&verts[vCount], x + mx * halfWidth);
        RwIm2DVertexSetScreenY(&verts[vCount], y + my * halfWidth);
        RwIm2DVertexSetScreenZ(&verts[vCount], RwIm2DGetNearScreenZ());
        RwIm2DVertexSetRecipCameraZ(&verts[vCount], RwIm2DGetNearScreenZ());
        RwIm2DVertexSetIntRGBA(&verts[vCount], col.r, col.g, col.b, vertAlpha);
        vCount++;

        RwIm2DVertexSetScreenX(&verts[vCount], x - mx * halfWidth);
        RwIm2DVertexSetScreenY(&verts[vCount], y - my * halfWidth);
        RwIm2DVertexSetScreenZ(&verts[vCount], RwIm2DGetNearScreenZ());
        RwIm2DVertexSetRecipCameraZ(&verts[vCount], RwIm2DGetNearScreenZ());
        RwIm2DVertexSetIntRGBA(&verts[vCount], col.r, col.g, col.b, vertAlpha);
        vCount++;
    }

    if (vCount >= 4) {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
        RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, vCount);
    }
}

struct Vec2 { float x, y; };

struct RoundedPerim {
    Vec2 pts[160];
    int n = 0;
    void Add(float x, float y) { if (n < 160) { pts[n].x = x; pts[n].y = y; ++n; } }
};

static RoundedPerim RoundedRectPerim(const CRect& r, float rad, float inset, int perCorner) {
    RoundedPerim out;
    float l = r.left + inset, t = r.top + inset;
    float rr = r.right - inset, b = r.bottom - inset;
    float radI = rad - inset;
    if (radI < 0.5f) {
        out.Add(l, t); out.Add(rr, t); out.Add(rr, b); out.Add(l, b); out.Add(l, t);
        return out;
    }
    float cxTL = l + radI, cyTL = t + radI;
    float cxTR = rr - radI, cyTR = t + radI;
    float cxBR = rr - radI, cyBR = b - radI;
    float cxBL = l + radI, cyBL = b - radI;
    auto arc = [&](float cx, float cy, float a0deg, float a1deg) {
        for (int i = 1; i <= perCorner; ++i) {
            float a = (a0deg + (a1deg - a0deg) * i / (float)perCorner) * kPi / 180.0f;
            out.Add(cx + radI * cosf(a), cy + radI * sinf(a));
        }
    };
    out.Add(cxTL, t);
    out.Add(cxTR, t);
    arc(cxTR, cyTR, 270.0f, 360.0f);
    out.Add(rr, b - radI);
    arc(cxBR, cyBR, 0.0f, 90.0f);
    out.Add(cxBL, b);
    arc(cxBL, cyBL, 90.0f, 180.0f);
    out.Add(l, t + radI);
    arc(cxTL, cyTL, 180.0f, 270.0f);
    return out;
}

static RoundedPerim TopRoundedRectPerim(const CRect& r, float rad, float inset, int perCorner) {
    RoundedPerim out;
    float l = r.left + inset, t = r.top + inset;
    float rr = r.right - inset, b = r.bottom - inset;
    float radI = rad - inset;
    if (radI < 0.5f) {
        out.Add(l, t); out.Add(rr, t); out.Add(rr, b); out.Add(l, b); out.Add(l, t);
        return out;
    }
    float cxTL = l + radI, cyTL = t + radI;
    float cxTR = rr - radI, cyTR = t + radI;
    auto arc = [&](float cx, float cy, float a0deg, float a1deg) {
        for (int i = 1; i <= perCorner; ++i) {
            float a = (a0deg + (a1deg - a0deg) * i / (float)perCorner) * kPi / 180.0f;
            out.Add(cx + radI * cosf(a), cy + radI * sinf(a));
        }
    };
    out.Add(cxTL, t);
    out.Add(cxTR, t);
    arc(cxTR, cyTR, 270.0f, 360.0f);
    out.Add(rr, b);
    out.Add(l, b);
    out.Add(l, t + radI);
    arc(cxTL, cyTL, 180.0f, 270.0f);
    return out;
}

static RoundedPerim RightRoundedRectPerim(const CRect& r, float rad, float inset, int perCorner) {
    RoundedPerim out;
    float l = r.left + inset, t = r.top + inset;
    float rr = r.right - inset, b = r.bottom - inset;
    float radI = rad - inset;
    if (radI < 0.5f) {
        out.Add(l, t); out.Add(rr, t); out.Add(rr, b); out.Add(l, b); out.Add(l, t);
        return out;
    }
    float cxTR = rr - radI, cyTR = t + radI;
    float cxBR = rr - radI, cyBR = b - radI;
    auto arc = [&](float cx, float cy, float a0deg, float a1deg) {
        for (int i = 1; i <= perCorner; ++i) {
            float a = (a0deg + (a1deg - a0deg) * i / (float)perCorner) * kPi / 180.0f;
            out.Add(cx + radI * cosf(a), cy + radI * sinf(a));
        }
    };
    out.Add(l, t);
    out.Add(cxTR, t);
    arc(cxTR, cyTR, 270.0f, 360.0f);
    out.Add(rr, b - radI);
    arc(cxBR, cyBR, 0.0f, 90.0f);
    out.Add(l, b);
    return out;
}

static RoundedPerim BottomRoundedRectPerim(const CRect& r, float rad, float inset, int perCorner) {
    RoundedPerim out;
    float l = r.left + inset, t = r.top + inset;
    float rr = r.right - inset, b = r.bottom - inset;
    float radI = rad - inset;
    if (radI < 0.5f) {
        out.Add(l, t); out.Add(rr, t); out.Add(rr, b); out.Add(l, b); out.Add(l, t);
        return out;
    }
    float cxBR = rr - radI, cyBR = b - radI;
    float cxBL = l + radI, cyBL = b - radI;
    auto arc = [&](float cx, float cy, float a0deg, float a1deg) {
        for (int i = 1; i <= perCorner; ++i) {
            float a = (a0deg + (a1deg - a0deg) * i / (float)perCorner) * kPi / 180.0f;
            out.Add(cx + radI * cosf(a), cy + radI * sinf(a));
        }
    };
    out.Add(l, t);
    out.Add(rr, t);
    out.Add(rr, b - radI);
    arc(cxBR, cyBR, 0.0f, 90.0f);
    out.Add(cxBL, b);
    arc(cxBL, cyBL, 90.0f, 180.0f);
    return out;
}

float Radar::Radius() {
    return (g_radarRect.bottom - g_radarRect.top) * (g_config.radarCornerRadius / 100.0f);
}

bool Radar::IsInside(float x, float y, const CRect& r, float rad) {
    if (x < r.left || x > r.right || y < r.top || y > r.bottom) return false;
    if (x < r.left + rad && y < r.top + rad) {
        float dx = x - (r.left + rad), dy = y - (r.top + rad);
        return (dx * dx + dy * dy) <= (rad * rad);
    }
    if (x > r.right - rad && y < r.top + rad) {
        float dx = x - (r.right - rad), dy = y - (r.top + rad);
        return (dx * dx + dy * dy) <= (rad * rad);
    }
    if (x < r.left + rad && y > r.bottom - rad) {
        float dx = x - (r.left + rad), dy = y - (r.bottom - rad);
        return (dx * dx + dy * dy) <= (rad * rad);
    }
    if (x > r.right - rad && y > r.bottom - rad) {
        float dx = x - (r.right - rad), dy = y - (r.bottom - rad);
        return (dx * dx + dy * dy) <= (rad * rad);
    }
    return true;
}

void Radar::ProjectToRim(const CRect& r, float dirX, float dirY, float& outX, float& outY) {
    float cx = (r.left + r.right) * 0.5f;
    float cy = r.top + (r.bottom - r.top) * g_curVCenter;
    float len = sqrtf(dirX * dirX + dirY * dirY);
    if (len < 0.0001f) { outX = cx; outY = cy; return; }
    float nx = dirX / len;
    float ny = dirY / len;

    float rad = Radius();
    if (rad < 0.5f) rad = 0.0f;

    float bestT = 1e9f;

    if (ny < -1e-5f) {
        float t = (r.top - cy) / ny;
        if (t > 0.0f && t < bestT) {
            float x = cx + t * nx;
            if (x >= r.left + rad && x <= r.right - rad) bestT = t;
        }
    }
    if (ny > 1e-5f) {
        float t = (r.bottom - cy) / ny;
        if (t > 0.0f && t < bestT) {
            float x = cx + t * nx;
            if (x >= r.left + rad && x <= r.right - rad) bestT = t;
        }
    }
    if (nx < -1e-5f) {
        float t = (r.left - cx) / nx;
        if (t > 0.0f && t < bestT) {
            float y = cy + t * ny;
            if (y >= r.top + rad && y <= r.bottom - rad) bestT = t;
        }
    }
    if (nx > 1e-5f) {
        float t = (r.right - cx) / nx;
        if (t > 0.0f && t < bestT) {
            float y = cy + t * ny;
            if (y >= r.top + rad && y <= r.bottom - rad) bestT = t;
        }
    }

    if (rad > 0.5f) {
        struct Corner { float ox, oy; bool xMin, yMin; } corners[4] = {
            { r.left + rad,  r.top + rad,    true,  true  },
            { r.right - rad, r.top + rad,    false, true  },
            { r.right - rad, r.bottom - rad, false, false },
            { r.left + rad,  r.bottom - rad, true,  false }
        };

        for (int i = 0; i < 4; ++i) {
            float dx = cx - corners[i].ox;
            float dy = cy - corners[i].oy;
            float bCoeff = 2.0f * (dx * nx + dy * ny);
            float cCoeff = dx * dx + dy * dy - rad * rad;
            float discr = bCoeff * bCoeff - 4.0f * cCoeff;
            if (discr >= 0.0f) {
                float sqrtD = sqrtf(discr);
                float tRoots[2] = { (-bCoeff - sqrtD) * 0.5f, (-bCoeff + sqrtD) * 0.5f };
                for (int k = 0; k < 2; ++k) {
                    float t = tRoots[k];
                    if (t > 0.001f && t < bestT) {
                        float ix = cx + t * nx;
                        float iy = cy + t * ny;
                        bool matchX = corners[i].xMin ? (ix <= corners[i].ox + 0.01f) : (ix >= corners[i].ox - 0.01f);
                        bool matchY = corners[i].yMin ? (iy <= corners[i].oy + 0.01f) : (iy >= corners[i].oy - 0.01f);
                        if (matchX && matchY) {
                            bestT = t;
                        }
                    }
                }
            }
        }
    }

    if (bestT < 1e8f) {
        outX = cx + bestT * nx;
        outY = cy + bestT * ny;
    } else {
        outX = cx;
        outY = cy;
    }
}

static inline void Set2DVert(RwIm2DVertex& v, float x, float y, const CRGBA& col) {
    RwIm2DVertexSetScreenX(&v, x);
    RwIm2DVertexSetScreenY(&v, y);
    RwIm2DVertexSetScreenZ(&v, 0.0f);
    RwIm2DVertexSetRecipCameraZ(&v, 1.0f);
    RwIm2DVertexSetIntRGBA(&v, col.r, col.g, col.b, col.a);
    RwIm2DVertexSetU(&v, 0.0f, 1.0f);
    RwIm2DVertexSetV(&v, 0.0f, 1.0f);
}

static void DrawD3DTexturedRoundedFill(IDirect3DDevice9* dev, IDirect3DTexture9* tex, const CRect& r, float rad, float sw, float sh, CRGBA col) {
    if (!dev || !tex || col.a == 0 || sw < 1.0f || sh < 1.0f) return;
    RoundedPerim p = RoundedRectPerim(r, rad, 0.0f, 24);
    if (p.n < 3) return;

    static D3D2DVert verts[192];
    float cx = (r.left + r.right) * 0.5f;
    float cy = (r.top + r.bottom) * 0.5f;

    auto setV = [&](D3D2DVert& v, float px, float py, float uOffset, float vOffset, DWORD dw) {
        v.x = px - 0.5f;
        v.y = py - 0.5f;
        v.z = 0.0f;
        v.rhw = 1.0f;
        v.color = dw;
        v.u = (px + uOffset) / sw;
        v.v = (py + vOffset) / sh;
    };

    dev->SetFVF(D3DFVF_2DVERT);
    dev->SetTexture(0, tex);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_LIGHTING, FALSE);

    if (g_pBackdropBlurPS) {
        float cOffset[4] = { 1.0f / sw, 1.0f / sh, g_config.blurIntensity * 3.5f, 1.0f };
        dev->SetPixelShaderConstantF(0, cOffset, 1);
        dev->SetPixelShader((IDirect3DPixelShader9*)g_pBackdropBlurPS);

        DWORD dwCol = D3DCOLOR_ARGB(col.a, col.r, col.g, col.b);
        setV(verts[0], cx, cy, 0.0f, 0.0f, dwCol);
        for (int i = 0; i < p.n; ++i) {
            setV(verts[i + 1], p.pts[i].x, p.pts[i].y, 0.0f, 0.0f, dwCol);
        }
        setV(verts[p.n + 1], p.pts[0].x, p.pts[0].y, 0.0f, 0.0f, dwCol);

        dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, p.n, verts, sizeof(D3D2DVert));
        dev->SetPixelShader(NULL);
    } else {
        dev->SetPixelShader(NULL);
        // Multi-tap blur pass with radius proportional to blurIntensity
        float rSpread = g_config.blurIntensity * 4.0f;

        const struct { float du, dv; float weight; } kOffsets[9] = {
            {  0.0f,          0.0f,          0.24f },
            { -rSpread,       0.0f,          0.11f },
            {  rSpread,       0.0f,          0.11f },
            {  0.0f,         -rSpread,       0.11f },
            {  0.0f,          rSpread,       0.11f },
            { -rSpread * 0.7f, -rSpread * 0.7f, 0.08f },
            {  rSpread * 0.7f, -rSpread * 0.7f, 0.08f },
            { -rSpread * 0.7f,  rSpread * 0.7f, 0.08f },
            {  rSpread * 0.7f,  rSpread * 0.7f, 0.08f },
        };

        for (int tap = 0; tap < 9; ++tap) {
            unsigned char tapA = (unsigned char)(col.a * kOffsets[tap].weight);
            if (tapA == 0) continue;
            DWORD dwCol = D3DCOLOR_ARGB(tapA, col.r, col.g, col.b);

            setV(verts[0], cx, cy, kOffsets[tap].du, kOffsets[tap].dv, dwCol);
            for (int i = 0; i < p.n; ++i) {
                setV(verts[i + 1], p.pts[i].x, p.pts[i].y, kOffsets[tap].du, kOffsets[tap].dv, dwCol);
            }
            setV(verts[p.n + 1], p.pts[0].x, p.pts[0].y, kOffsets[tap].du, kOffsets[tap].dv, dwCol);

            dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, p.n, verts, sizeof(D3D2DVert));
        }
    }

    dev->SetTexture(0, NULL);
}

static void DrawCircle(float cx, float cy, float radius, CRGBA col, int segments = 24) {
    if (radius <= 0.0f || col.a == 0) return;
    if (segments < 8) segments = 8;
    if (segments > 48) segments = 48;

    static RwIm2DVertex verts[64];
    Set2DVert(verts[0], cx, cy, col);
    for (int i = 0; i <= segments; ++i) {
        float a = (float)i * (2.0f * kPi / (float)segments);
        Set2DVert(verts[i + 1], cx + radius * cosf(a), cy + radius * sinf(a), col);
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts, segments + 2);
}

static void DrawRing(float cx, float cy, float innerR, float outerR, CRGBA col, int segments = 32) {
    if (innerR <= 0.0f && outerR <= 0.0f) return;
    if (col.a == 0) return;
    if (segments < 8) segments = 8;
    if (segments > 64) segments = 64;

    static RwIm2DVertex verts[132];
    int vCount = 0;
    for (int i = 0; i <= segments; ++i) {
        float a = (float)i * (2.0f * kPi / (float)segments);
        float ca = cosf(a);
        float sa = sinf(a);
        Set2DVert(verts[vCount++], cx + outerR * ca, cy + outerR * sa, col);
        Set2DVert(verts[vCount++], cx + innerR * ca, cy + innerR * sa, col);
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, vCount);
}

static void DrawEllipse(float cx, float cy, float radX, float radY, CRGBA col, int segments = 24) {
    if (radX <= 0.0f || radY <= 0.0f || col.a == 0) return;
    if (segments < 8) segments = 8;
    if (segments > 48) segments = 48;

    static RwIm2DVertex verts[64];
    Set2DVert(verts[0], cx, cy, col);
    for (int i = 0; i <= segments; ++i) {
        float a = (float)i * (2.0f * kPi / (float)segments);
        Set2DVert(verts[i + 1], cx + radX * cosf(a), cy + radY * sinf(a), col);
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts, segments + 2);
}

// additive=true uses SRC_ALPHA/ONE blend (glow effect); false uses normal alpha blend
static void DrawSoftGlow(float cx, float cy, float radX, float radY, CRGBA centerCol, int segments = 20, bool additive = false) {
    if (radX <= 0.0f || radY <= 0.0f || centerCol.a == 0) return;
    if (segments < 8) segments = 8;
    if (segments > 36) segments = 36;

    static RwIm2DVertex verts[48];
    CRGBA outerCol = centerCol;
    outerCol.a = 0;

    Set2DVert(verts[0], cx, cy, centerCol);
    for (int i = 0; i <= segments; ++i) {
        float a = (float)i * (2.0f * kPi / (float)segments);
        Set2DVert(verts[i + 1], cx + radX * cosf(a), cy + radY * sinf(a), outerCol);
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, additive ? (void*)rwBLENDONE : (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts, segments + 2);
    // Restore normal blend so subsequent draws aren't affected
    if (additive)
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
}

static void DrawArcStrip(float cx, float cy, float innerRad, float outerRad, float startAngleRad, float endAngleRad, CRGBA col, int segments = 24) {
    if (outerRad <= innerRad || col.a == 0 || segments < 3) return;
    if (segments > 36) segments = 36;
    static RwIm2DVertex verts[80];
    int vCount = 0;
    float delta = (endAngleRad - startAngleRad) / (float)segments;
    for (int i = 0; i <= segments; ++i) {
        float a = startAngleRad + delta * (float)i;
        float cosA = cosf(a), sinA = sinf(a);
        Set2DVert(verts[vCount++], cx + outerRad * cosA, cy + outerRad * sinA, col);
        Set2DVert(verts[vCount++], cx + innerRad * cosA, cy + innerRad * sinA, col);
    }
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, vCount);
}

static void DrawRoundedFill(const CRect& r, float rad, CRGBA col) {
    if (col.a == 0) return;
    RoundedPerim p = RoundedRectPerim(r, rad, 0.0f, 24);
    if (p.n < 3) return;

    static RwIm2DVertex verts[192];
    float cx = (r.left + r.right) * 0.5f;
    float cy = (r.top + r.bottom) * 0.5f;
    Set2DVert(verts[0], cx, cy, col);
    for (int i = 0; i < p.n; ++i) {
        Set2DVert(verts[i + 1], p.pts[i].x, p.pts[i].y, col);
    }
    Set2DVert(verts[p.n + 1], p.pts[0].x, p.pts[0].y, col);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts, p.n + 2);
}

static void DrawRoundedGradientFill(const CRect& r, float rad, CRGBA topCol, CRGBA botCol) {
    if (topCol.a == 0 && botCol.a == 0) return;
    RoundedPerim p = RoundedRectPerim(r, rad, 0.0f, 24);
    if (p.n < 3) return;

    static RwIm2DVertex verts[192];
    float cx = (r.left + r.right) * 0.5f;
    float cy = (r.top + r.bottom) * 0.5f;

    auto lerpCol = [&](float y) -> CRGBA {
        float t = Clampf((y - r.top) / (r.bottom - r.top), 0.0f, 1.0f);
        return CRGBA(
            (unsigned char)(topCol.r + (botCol.r - topCol.r) * t),
            (unsigned char)(topCol.g + (botCol.g - topCol.g) * t),
            (unsigned char)(topCol.b + (botCol.b - topCol.b) * t),
            (unsigned char)(topCol.a + (botCol.a - topCol.a) * t)
        );
    };

    Set2DVert(verts[0], cx, cy, lerpCol(cy));
    for (int i = 0; i < p.n; ++i) {
        Set2DVert(verts[i + 1], p.pts[i].x, p.pts[i].y, lerpCol(p.pts[i].y));
    }
    Set2DVert(verts[p.n + 1], p.pts[0].x, p.pts[0].y, lerpCol(p.pts[0].y));

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts, p.n + 2);
}

static void DrawTopRoundedHorizontalGradientFill(const CRect& r, float rad, CRGBA leftCol, CRGBA rightCol) {
    if (leftCol.a == 0 && rightCol.a == 0) return;
    float w = r.right - r.left;
    float h = r.bottom - r.top;
    if (w < 1.0f || h < 1.0f) return;

    const int kSlices = 64;
    static RwIm2DVertex verts[(kSlices + 1) * 2];

    float radClamped = std::min(rad, h);
    radClamped = std::min(radClamped, w * 0.5f);

    auto lerpCol = [&](float t) -> CRGBA {
        return CRGBA(
            (unsigned char)(leftCol.r + (rightCol.r - leftCol.r) * t),
            (unsigned char)(leftCol.g + (rightCol.g - leftCol.g) * t),
            (unsigned char)(leftCol.b + (rightCol.b - leftCol.b) * t),
            (unsigned char)(leftCol.a + (rightCol.a - leftCol.a) * t)
        );
    };

    for (int i = 0; i <= kSlices; ++i) {
        float t = (float)i / (float)kSlices;
        float x = r.left + w * t;
        float topY = r.top;

        if (radClamped > 0.5f) {
            if (x < r.left + radClamped) {
                float dx = (r.left + radClamped) - x;
                float dy = sqrtf(std::max(0.0f, radClamped * radClamped - dx * dx));
                topY = (r.top + radClamped) - dy;
            } else if (x > r.right - radClamped) {
                float dx = x - (r.right - radClamped);
                float dy = sqrtf(std::max(0.0f, radClamped * radClamped - dx * dx));
                topY = (r.top + radClamped) - dy;
            }
        }

        CRGBA col = lerpCol(t);
        Set2DVert(verts[i * 2 + 0], x, topY, col);
        Set2DVert(verts[i * 2 + 1], x, r.bottom, col);
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, (kSlices + 1) * 2);
}

static void DrawLeftRoundedHorizontalGradientFill(const CRect& r, float rad, CRGBA leftCol, CRGBA rightCol) {
    if (leftCol.a == 0 && rightCol.a == 0) return;
    float w = r.right - r.left;
    float h = r.bottom - r.top;
    if (w < 1.0f || h < 1.0f) return;

    const int kSlices = 64;
    static RwIm2DVertex verts[(kSlices + 1) * 2];

    float radClamped = std::min(rad, h * 0.5f);
    radClamped = std::min(radClamped, w);

    auto lerpCol = [&](float t) -> CRGBA {
        return CRGBA(
            (unsigned char)(leftCol.r + (rightCol.r - leftCol.r) * t),
            (unsigned char)(leftCol.g + (rightCol.g - leftCol.g) * t),
            (unsigned char)(leftCol.b + (rightCol.b - leftCol.b) * t),
            (unsigned char)(leftCol.a + (rightCol.a - leftCol.a) * t)
        );
    };

    for (int i = 0; i <= kSlices; ++i) {
        float t = (float)i / (float)kSlices;
        float x = r.left + w * t;
        float topY = r.top;
        float botY = r.bottom;

        if (radClamped > 0.5f && x < r.left + radClamped) {
            float dx = (r.left + radClamped) - x;
            float dy = sqrtf(std::max(0.0f, radClamped * radClamped - dx * dx));
            topY = (r.top + radClamped) - dy;
            botY = (r.bottom - radClamped) + dy;
        }

        CRGBA col = lerpCol(t);
        Set2DVert(verts[i * 2 + 0], x, topY, col);
        Set2DVert(verts[i * 2 + 1], x, botY, col);
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, (kSlices + 1) * 2);
}

static void DrawRightRoundedFill(const CRect& r, float rad, CRGBA col) {
    if (col.a == 0) return;
    RoundedPerim p = RightRoundedRectPerim(r, rad, 0.0f, 24);
    if (p.n < 3) return;

    static RwIm2DVertex verts[192];
    float cx = (r.left + r.right) * 0.5f;
    float cy = (r.top + r.bottom) * 0.5f;
    Set2DVert(verts[0], cx, cy, col);
    for (int i = 0; i < p.n; ++i) {
        Set2DVert(verts[i + 1], p.pts[i].x, p.pts[i].y, col);
    }
    Set2DVert(verts[p.n + 1], p.pts[0].x, p.pts[0].y, col);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts, p.n + 2);
}

static void DrawBottomRoundedFill(const CRect& r, float rad, CRGBA col) {
    if (col.a == 0) return;
    RoundedPerim p = BottomRoundedRectPerim(r, rad, 0.0f, 24);
    if (p.n < 3) return;

    static RwIm2DVertex verts[192];
    float cx = (r.left + r.right) * 0.5f;
    float cy = (r.top + r.bottom) * 0.5f;
    Set2DVert(verts[0], cx, cy, col);
    for (int i = 0; i < p.n; ++i) {
        Set2DVert(verts[i + 1], p.pts[i].x, p.pts[i].y, col);
    }
    Set2DVert(verts[p.n + 1], p.pts[0].x, p.pts[0].y, col);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts, p.n + 2);
}

static void DrawManeuverIcon(GpsManeuver maneuver, float cx, float cy, float size, CRGBA col) {
    if (col.a == 0) return;
    float halfS = size * 0.5f;
    float arrowW = size * 0.38f;
    float shaftW = size * 0.16f;

    switch (maneuver) {
        case GpsManeuver::Straight: {
            CSprite2d::Draw2DPolygon(
                cx - shaftW * 0.5f, cy + halfS * 0.7f,
                cx + shaftW * 0.5f, cy + halfS * 0.7f,
                cx - shaftW * 0.5f, cy - halfS * 0.1f,
                cx + shaftW * 0.5f, cy - halfS * 0.1f,
                col
            );
            CSprite2d::Draw2DPolygon(
                cx, cy - halfS * 0.8f,
                cx - arrowW, cy - halfS * 0.05f,
                cx + arrowW, cy - halfS * 0.05f,
                cx, cy - halfS * 0.05f,
                col
            );
            break;
        }
        case GpsManeuver::TurnRight: {
            CSprite2d::Draw2DPolygon(
                cx - halfS * 0.45f - shaftW * 0.5f, cy + halfS * 0.7f,
                cx - halfS * 0.45f + shaftW * 0.5f, cy + halfS * 0.7f,
                cx - halfS * 0.45f - shaftW * 0.5f, cy - halfS * 0.25f,
                cx - halfS * 0.45f + shaftW * 0.5f, cy - halfS * 0.25f,
                col
            );
            CSprite2d::Draw2DPolygon(
                cx - halfS * 0.45f - shaftW * 0.5f, cy - halfS * 0.25f - shaftW * 0.5f,
                cx + halfS * 0.20f, cy - halfS * 0.25f - shaftW * 0.5f,
                cx - halfS * 0.45f - shaftW * 0.5f, cy - halfS * 0.25f + shaftW * 0.5f,
                cx + halfS * 0.20f, cy - halfS * 0.25f + shaftW * 0.5f,
                col
            );
            CSprite2d::Draw2DPolygon(
                cx + halfS * 0.80f, cy - halfS * 0.25f,
                cx + halfS * 0.15f, cy - halfS * 0.25f - arrowW,
                cx + halfS * 0.15f, cy - halfS * 0.25f + arrowW,
                cx + halfS * 0.15f, cy - halfS * 0.25f,
                col
            );
            break;
        }
        case GpsManeuver::TurnLeft: {
            CSprite2d::Draw2DPolygon(
                cx + halfS * 0.45f - shaftW * 0.5f, cy + halfS * 0.7f,
                cx + halfS * 0.45f + shaftW * 0.5f, cy + halfS * 0.7f,
                cx + halfS * 0.45f - shaftW * 0.5f, cy - halfS * 0.25f,
                cx + halfS * 0.45f + shaftW * 0.5f, cy - halfS * 0.25f,
                col
            );
            CSprite2d::Draw2DPolygon(
                cx - halfS * 0.20f, cy - halfS * 0.25f - shaftW * 0.5f,
                cx + halfS * 0.45f + shaftW * 0.5f, cy - halfS * 0.25f - shaftW * 0.5f,
                cx - halfS * 0.20f, cy - halfS * 0.25f + shaftW * 0.5f,
                cx + halfS * 0.45f + shaftW * 0.5f, cy - halfS * 0.25f + shaftW * 0.5f,
                col
            );
            CSprite2d::Draw2DPolygon(
                cx - halfS * 0.80f, cy - halfS * 0.25f,
                cx - halfS * 0.15f, cy - halfS * 0.25f - arrowW,
                cx - halfS * 0.15f, cy - halfS * 0.25f + arrowW,
                cx - halfS * 0.15f, cy - halfS * 0.25f,
                col
            );
            break;
        }
        case GpsManeuver::SlightRight: {
            CRGBA faintCol = ModColor(col, 0.40f);
            CSprite2d::Draw2DPolygon(
                cx - shaftW * 0.5f, cy + halfS * 0.7f,
                cx + shaftW * 0.5f, cy + halfS * 0.7f,
                cx - shaftW * 0.5f, cy - halfS * 0.7f,
                cx + shaftW * 0.5f, cy - halfS * 0.7f,
                faintCol
            );
            CSprite2d::Draw2DPolygon(
                cx - shaftW * 0.5f, cy + halfS * 0.2f,
                cx + shaftW * 0.5f, cy + halfS * 0.2f,
                cx + halfS * 0.35f - shaftW * 0.5f, cy - halfS * 0.35f,
                cx + halfS * 0.35f + shaftW * 0.5f, cy - halfS * 0.35f,
                col
            );
            CSprite2d::Draw2DPolygon(
                cx + halfS * 0.75f, cy - halfS * 0.75f,
                cx + halfS * 0.10f, cy - halfS * 0.65f,
                cx + halfS * 0.65f, cy - halfS * 0.10f,
                cx + halfS * 0.35f, cy - halfS * 0.35f,
                col
            );
            break;
        }
        case GpsManeuver::SlightLeft: {
            CRGBA faintCol = ModColor(col, 0.40f);
            CSprite2d::Draw2DPolygon(
                cx - shaftW * 0.5f, cy + halfS * 0.7f,
                cx + shaftW * 0.5f, cy + halfS * 0.7f,
                cx - shaftW * 0.5f, cy - halfS * 0.7f,
                cx + shaftW * 0.5f, cy - halfS * 0.7f,
                faintCol
            );
            CSprite2d::Draw2DPolygon(
                cx - shaftW * 0.5f, cy + halfS * 0.2f,
                cx + shaftW * 0.5f, cy + halfS * 0.2f,
                cx - halfS * 0.35f - shaftW * 0.5f, cy - halfS * 0.35f,
                cx - halfS * 0.35f + shaftW * 0.5f, cy - halfS * 0.35f,
                col
            );
            CSprite2d::Draw2DPolygon(
                cx - halfS * 0.75f, cy - halfS * 0.75f,
                cx - halfS * 0.10f, cy - halfS * 0.65f,
                cx - halfS * 0.65f, cy - halfS * 0.10f,
                cx - halfS * 0.35f, cy - halfS * 0.35f,
                col
            );
            break;
        }
        case GpsManeuver::UTurnLeft: {
            DrawArcStrip(cx, cy - halfS * 0.10f, halfS * 0.35f, halfS * 0.35f + shaftW, -kPi, 0.0f, col, 16);
            CSprite2d::Draw2DPolygon(
                cx + halfS * 0.35f, cy + halfS * 0.7f,
                cx + halfS * 0.35f + shaftW, cy + halfS * 0.7f,
                cx + halfS * 0.35f, cy - halfS * 0.10f,
                cx + halfS * 0.35f + shaftW, cy - halfS * 0.10f,
                col
            );
            CSprite2d::Draw2DPolygon(
                cx - halfS * 0.35f - shaftW, cy + halfS * 0.25f,
                cx - halfS * 0.35f, cy + halfS * 0.25f,
                cx - halfS * 0.35f - shaftW, cy - halfS * 0.10f,
                cx - halfS * 0.35f, cy - halfS * 0.10f,
                col
            );
            CSprite2d::Draw2DPolygon(
                cx - halfS * 0.35f - shaftW * 0.5f, cy + halfS * 0.75f,
                cx - halfS * 0.35f - shaftW * 0.5f - arrowW * 0.75f, cy + halfS * 0.20f,
                cx - halfS * 0.35f - shaftW * 0.5f + arrowW * 0.75f, cy + halfS * 0.20f,
                cx - halfS * 0.35f - shaftW * 0.5f, cy + halfS * 0.20f,
                col
            );
            break;
        }
        case GpsManeuver::UTurnRight: {
            DrawArcStrip(cx, cy - halfS * 0.10f, halfS * 0.35f, halfS * 0.35f + shaftW, -kPi, 0.0f, col, 16);
            CSprite2d::Draw2DPolygon(
                cx - halfS * 0.35f - shaftW, cy + halfS * 0.7f,
                cx - halfS * 0.35f, cy + halfS * 0.7f,
                cx - halfS * 0.35f - shaftW, cy - halfS * 0.10f,
                cx - halfS * 0.35f, cy - halfS * 0.10f,
                col
            );
            CSprite2d::Draw2DPolygon(
                cx + halfS * 0.35f, cy + halfS * 0.25f,
                cx + halfS * 0.35f + shaftW, cy + halfS * 0.25f,
                cx + halfS * 0.35f, cy - halfS * 0.10f,
                cx + halfS * 0.35f + shaftW, cy - halfS * 0.10f,
                col
            );
            CSprite2d::Draw2DPolygon(
                cx + halfS * 0.35f + shaftW * 0.5f, cy + halfS * 0.75f,
                cx + halfS * 0.35f + shaftW * 0.5f - arrowW * 0.75f, cy + halfS * 0.20f,
                cx + halfS * 0.35f + shaftW * 0.5f + arrowW * 0.75f, cy + halfS * 0.20f,
                cx + halfS * 0.35f + shaftW * 0.5f, cy + halfS * 0.20f,
                col
            );
            break;
        }
        case GpsManeuver::Arrival:
        default: {
            DrawCircle(cx, cy, halfS * 0.70f, col, 18);
            DrawCircle(cx, cy, halfS * 0.40f, CRGBA(232, 74, 213, col.a), 14);
            DrawCircle(cx, cy, halfS * 0.18f, col, 10);
            break;
        }
    }
}

static void DrawBorderRing(const RoundedPerim& outer, const RoundedPerim& inner, CRGBA col) {
    if (outer.n != inner.n || outer.n < 3 || col.a == 0) return;
    int count = outer.n;

    static RwIm2DVertex verts[384];
    int vCount = 0;
    for (int i = 0; i < count; ++i) {
        Set2DVert(verts[vCount++], outer.pts[i].x, outer.pts[i].y, col);
        Set2DVert(verts[vCount++], inner.pts[i].x, inner.pts[i].y, col);
    }
    Set2DVert(verts[vCount++], outer.pts[0].x, outer.pts[0].y, col);
    Set2DVert(verts[vCount++], inner.pts[0].x, inner.pts[0].y, col);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, vCount);
}

static void DrawRoundedBorder(const CRect& r, float rad, float stroke, CRGBA col) {
    if (stroke <= 0.05f || col.a == 0) return;
    RoundedPerim pShadow = RoundedRectPerim(r, rad, -1.0f, 24);
    RoundedPerim pBezel  = RoundedRectPerim(r, rad, 0.0f, 24);
    RoundedPerim pInner  = RoundedRectPerim(r, rad, stroke, 24);

    DrawBorderRing(pShadow, pBezel, CRGBA(0, 0, 0, (unsigned char)(col.a * 0.5f)));
    DrawBorderRing(pBezel, pInner, col);
}

bool Radar::ProjectToUV(const CVector& wp, float& u, float& v) {
    float dx = wp.x - g_playerPos.x;
    float dy = wp.y - g_playerPos.y;

    float rx = dx * g_camRight.x + dy * g_camRight.y;
    float ry = dx * g_camUp.x + dy * g_camUp.y;

    float hh = (g_radarRect.bottom - g_radarRect.top) * 0.5f;
    float hw = (g_radarRect.right - g_radarRect.left) * 0.5f;
    float radarRange = g_curDist;
    if (radarRange < 10.0f) radarRange = 150.0f;

    float cosP = cosf(g_curTiltRad);
    if (cosP < 0.2f) cosP = 0.2f;
    float sinP = sinf(g_curTiltRad);

    float dz = g_config.lockPlayerElevation ? 0.0f : (wp.z - g_playerPos.z);

    float scale = hh / radarRange;
    float sx = rx * scale;
    float sy = (-ry * cosP - dz * sinP) * scale;

    u = (sx / (hw * 2.0f)) + 0.5f;
    v = (sy / (hh * 2.0f)) + g_curVCenter;
    return true;
}

