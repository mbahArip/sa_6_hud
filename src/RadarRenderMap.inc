void Radar::DrawMap2D(const CRect& rect, float alpha) {
    float rw = rect.right - rect.left;
    float rh = rect.bottom - rect.top;

    float unifiedMapOpacity = g_config.radarOpacity * alpha;

    int px = (int)floorf((g_playerPos.x + 3000.0f) / 500.0f);
    int py = (int)floorf((3000.0f - g_playerPos.y) / 500.0f);

    int* gRadarTxdIds = (int*)0xBA8478;
    int* radarTextures = gRadarTxdIds ? gRadarTxdIds : (int*)0xBA8478;

    int grid = g_config.mapStreamGrid;
    if (grid < 1) grid = 1;
    if (grid > 6) grid = 6;
    int minD = -(grid / 2);
    int maxD = minD + grid - 1;

    CRadar::StreamRadarSections(g_playerPos);
    for (int dy = minD; dy <= maxD; ++dy) {
        for (int dx = minD; dx <= maxD; ++dx) {
            int tx = px + dx, ty = py + dy;
            if (tx >= 0 && tx < 12 && ty >= 0 && ty < 12) {
                int tileIndex = tx + 12 * ty;
                int txdSlot = radarTextures[tileIndex];
                if (txdSlot >= 0) {
                    CStreaming::RequestTxdModel(txdSlot, 0x16);
                }
            }
        }
    }
    CStreaming::LoadRequestedModels();

    unsigned char tileAlpha = (unsigned char)(255.0f * unifiedMapOpacity);
    CRGBA tileColor(255, 255, 255, tileAlpha);

    // Water is day/night blended once per frame, matching the custom tile crossfade.
    float seaNightBlend = GetDayNightBlend();
    CRGBA seaCol = CRGBA(
        (unsigned char)Lerpf((float)g_config.radarSeaColor.r, (float)g_config.radarSeaColorNight.r, seaNightBlend),
        (unsigned char)Lerpf((float)g_config.radarSeaColor.g, (float)g_config.radarSeaColorNight.g, seaNightBlend),
        (unsigned char)Lerpf((float)g_config.radarSeaColor.b, (float)g_config.radarSeaColorNight.b, seaNightBlend),
        (unsigned char)Lerpf((float)g_config.radarSeaColor.a, (float)g_config.radarSeaColorNight.a, seaNightBlend)
    );

    auto setMapVert = [](RwIm2DVertex& v, float sx, float sy, float uTex, float vTex, const CRGBA& col) {
        RwIm2DVertexSetScreenX(&v, sx);
        RwIm2DVertexSetScreenY(&v, sy);
        RwIm2DVertexSetScreenZ(&v, RwIm2DGetNearScreenZ());
        RwIm2DVertexSetRecipCameraZ(&v, RwIm2DGetNearScreenZ());
        RwIm2DVertexSetIntRGBA(&v, col.r, col.g, col.b, col.a);
        RwIm2DVertexSetU(&v, uTex, RwIm2DGetNearScreenZ());
        RwIm2DVertexSetV(&v, vTex, RwIm2DGetNearScreenZ());
    };

    for (int dy = minD; dy <= maxD; ++dy) {
        for (int dx = minD; dx <= maxD; ++dx) {
            int tx = px + dx, ty = py + dy;

            float wx0 = -3000.0f + (float)tx * 500.0f;
            float wx1 = wx0 + 500.0f;
            float wy0 = 3000.0f - (float)(ty + 1) * 500.0f;
            float wy1 = 3000.0f - (float)ty * 500.0f;

            float u0, v0, u1, v1, u2, v2, u3, v3;
            if (!ProjectToUV(CVector(wx0, wy0, 0.0f), u0, v0)) continue;
            if (!ProjectToUV(CVector(wx1, wy0, 0.0f), u1, v1)) continue;
            if (!ProjectToUV(CVector(wx1, wy1, 0.0f), u2, v2)) continue;
            if (!ProjectToUV(CVector(wx0, wy1, 0.0f), u3, v3)) continue;

            float sx0 = rect.left + rw * u0, sy0 = rect.top + rh * v0;
            float sx1 = rect.left + rw * u1, sy1 = rect.top + rh * v1;
            float sx2 = rect.left + rw * u2, sy2 = rect.top + rh * v2;
            float sx3 = rect.left + rw * u3, sy3 = rect.top + rh * v3;

            if ((sx0 < rect.left && sx1 < rect.left && sx2 < rect.left && sx3 < rect.left) ||
                (sx0 > rect.right && sx1 > rect.right && sx2 > rect.right && sx3 > rect.right) ||
                (sy0 < rect.top && sy1 < rect.top && sy2 < rect.top && sy3 < rect.top) ||
                (sy0 > rect.bottom && sy1 > rect.bottom && sy2 > rect.bottom && sy3 > rect.bottom)) {
                continue;
            }

            // Outside San Andreas 12x12 grid: render untextured flat sea quad if fillEmptySea enabled
            if (tx < 0 || tx >= 12 || ty < 0 || ty >= 12) {
                if (g_config.fillEmptySea) {
                    RwIm2DVertex verts[4];
                    CRGBA flatSeaCol = ModColor(seaCol, unifiedMapOpacity);
                    setMapVert(verts[0], sx0, sy0, 0.0f, 1.0f, flatSeaCol);
                    setMapVert(verts[1], sx1, sy1, 1.0f, 1.0f, flatSeaCol);
                    setMapVert(verts[2], sx3, sy3, 0.0f, 0.0f, flatSeaCol);
                    setMapVert(verts[3], sx2, sy2, 1.0f, 0.0f, flatSeaCol);

                    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
                    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
                    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
                    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
                    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, 4);
                }
                continue;
            }

            int tileIndex = tx + 12 * ty;

            // Item 8: Fill water base under map tiles with minimap.radar.sea (#5daece)
            if (g_config.fillEmptySea) {
                RwIm2DVertex verts[4];
                CRGBA flatSeaCol = ModColor(seaCol, unifiedMapOpacity);
                setMapVert(verts[0], sx0, sy0, 0.0f, 1.0f, flatSeaCol);
                setMapVert(verts[1], sx1, sy1, 1.0f, 1.0f, flatSeaCol);
                setMapVert(verts[2], sx3, sy3, 0.0f, 0.0f, flatSeaCol);
                setMapVert(verts[3], sx2, sy2, 1.0f, 0.0f, flatSeaCol);

                RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
                RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
                RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, 4);
            }

            // Item 6: Load from assets/radar/map/<day|night>/<tile>.png with smooth blend (if USE_CUSTOM_MAP_TILE is true)
            IDirect3DTexture9* pDayTex = nullptr;
            IDirect3DTexture9* pNightTex = nullptr;
            if (g_config.useCustomMapTile) {
                GetMapTileTextures(tileIndex, pDayTex, pNightTex);
            }
            if (pDayTex || pNightTex) {
                float nightBlend = GetDayNightBlend();
                auto drawCustomMapTile = [&](IDirect3DTexture9* pTex, float opacity) {
                    if (!pTex || opacity <= 0.005f) return;
                    IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
                    if (!dev) return;

                    unsigned char a = (unsigned char)(255.0f * opacity);
                    DWORD dwCol = D3DCOLOR_ARGB(a, 255, 255, 255);
                    D3D2DVert quad[4] = {
                        { sx0 - 0.5f, sy0 - 0.5f, 0.0f, 1.0f, dwCol, 0.0f, 1.0f },
                        { sx1 - 0.5f, sy1 - 0.5f, 0.0f, 1.0f, dwCol, 1.0f, 1.0f },
                        { sx3 - 0.5f, sy3 - 0.5f, 0.0f, 1.0f, dwCol, 0.0f, 0.0f },
                        { sx2 - 0.5f, sy2 - 0.5f, 0.0f, 1.0f, dwCol, 1.0f, 0.0f },
                    };

                    dev->SetFVF(D3DFVF_2DVERT);
                    dev->SetTexture(0, pTex);
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
                    dev->SetPixelShader(NULL);

                    dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(D3D2DVert));
                    dev->SetTexture(0, NULL);
                };

                if (pDayTex && pNightTex) {
                    drawCustomMapTile(pDayTex, unifiedMapOpacity * (1.0f - nightBlend));
                    drawCustomMapTile(pNightTex, unifiedMapOpacity * nightBlend);
                } else if (pDayTex) {
                    drawCustomMapTile(pDayTex, unifiedMapOpacity);
                } else {
                    drawCustomMapTile(pNightTex, unifiedMapOpacity);
                }
                continue;
            }

            int txdSlot = radarTextures[tileIndex];
            if (txdSlot < 0) {
                continue;
            }

            TxdDef* def = CTxdStore::ms_pTxdPool->GetAt(txdSlot);
            if (!def || !def->m_pRwDictionary) continue;

            RwTexture* tex = ::GetFirstTexture(def->m_pRwDictionary);
            if (!tex || !RwTextureGetRaster(tex)) continue;

            RwIm2DVertex verts[4];
            setMapVert(verts[0], sx0, sy0, 0.0f, 1.0f, tileColor);
            setMapVert(verts[1], sx1, sy1, 1.0f, 1.0f, tileColor);
            setMapVert(verts[2], sx3, sy3, 0.0f, 0.0f, tileColor);
            setMapVert(verts[3], sx2, sy2, 1.0f, 0.0f, tileColor);

            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwTextureGetRaster(tex));
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
            RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
            RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);

            RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, 4);
        }
    }
}

void Radar::Draw3dObjects(const CRect& rect, float alpha) {
    if (!g_config.experimentalShow3dObjects || alpha <= 0.01f) return;

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

    float rw = rect.right - rect.left;
    float rh = rect.bottom - rect.top;
    CRGBA objCol = ModColor(g_config.color3dObjects, alpha * 0.35f);
    CRGBA safehouseCol(245, 166, 35, (unsigned char)(160 * alpha));

    auto renderEntity3dBox = [&](CEntity* ent) {
        if (!ent) return;

        // Skip LOD parent models (districts, low-poly world meshes)
        if (ent->m_nNumLodChildren > 0) return;

        CBaseModelInfo* binfo = CModelInfo::GetModelInfo(ent->m_nModelIndex);
        if (binfo) {
            // bIsLod = bit 5, bIsRoad = bit 8
            if (binfo->bIsLod || binfo->bIsRoad) return;
        }

        CColModel* col = ent->GetColModel();
        if (!col) return;

        CVector minB = col->m_boundBox.m_vecMin;
        CVector maxB = col->m_boundBox.m_vecMax;
        float sizeX = maxB.x - minB.x;
        float sizeY = maxB.y - minB.y;
        float sizeZ = maxB.z - minB.z;

        // Filter: must be actual structure, not tiny prop or giant road/terrain tile
        if (sizeX < 2.0f || sizeY < 2.0f || sizeX > 80.0f || sizeY > 80.0f) return;
        if (sizeZ < 2.5f) return; // Flat ground/road slabs

        CVector entPos = ent->GetPosition();
        // Skip underground foundations
        if (entPos.z + maxB.z < g_playerPos.z - 4.0f) return;

        float distToPlayer = (entPos - g_playerPos).Magnitude2D();
        float maxExtent = std::max(sizeX, sizeY) * 0.5f;
        if (distToPlayer > (g_curDist + maxExtent + 15.0f)) return;

        float midZ = (minB.z + maxB.z) * 0.5f;
        CVector c[4];
        c[0] = ent->TransformFromObjectSpace(CVector(minB.x, minB.y, midZ));
        c[1] = ent->TransformFromObjectSpace(CVector(maxB.x, minB.y, midZ));
        c[2] = ent->TransformFromObjectSpace(CVector(maxB.x, maxB.y, midZ));
        c[3] = ent->TransformFromObjectSpace(CVector(minB.x, maxB.y, midZ));

        float u[4], v[4];
        bool ok = true;
        for (int k = 0; k < 4; ++k) {
            if (!ProjectToUV(c[k], u[k], v[k])) { ok = false; break; }
            if (u[k] < -0.2f || u[k] > 1.2f || v[k] < -0.2f || v[k] > 1.2f) { ok = false; break; }
        }
        if (!ok) return;

        float sx[4], sy[4];
        for (int k = 0; k < 4; ++k) {
            sx[k] = rect.left + rw * u[k];
            sy[k] = rect.top + rh * v[k];
        }

        float minSx = std::min({sx[0], sx[1], sx[2], sx[3]});
        float maxSx = std::max({sx[0], sx[1], sx[2], sx[3]});
        float minSy = std::min({sy[0], sy[1], sy[2], sy[3]});
        float maxSy = std::max({sy[0], sy[1], sy[2], sy[3]});
        if (maxSx < rect.left || minSx > rect.right || maxSy < rect.top || minSy > rect.bottom) {
            return;
        }

        bool isSafehouse = (ent->m_nModelIndex >= 14457 && ent->m_nModelIndex <= 14465);
        CRGBA finalCol = isSafehouse ? safehouseCol : objCol;

        IDirect3DDevice9* dev = (IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice();
        if (dev) {
            DWORD dwCol = D3DCOLOR_ARGB(finalCol.a, finalCol.r, finalCol.g, finalCol.b);
            D3D2DVert triVerts[6] = {
                { sx[0], sy[0], 0.0f, 1.0f, dwCol, 0.0f, 0.0f },
                { sx[1], sy[1], 0.0f, 1.0f, dwCol, 0.0f, 0.0f },
                { sx[2], sy[2], 0.0f, 1.0f, dwCol, 0.0f, 0.0f },

                { sx[0], sy[0], 0.0f, 1.0f, dwCol, 0.0f, 0.0f },
                { sx[2], sy[2], 0.0f, 1.0f, dwCol, 0.0f, 0.0f },
                { sx[3], sy[3], 0.0f, 1.0f, dwCol, 0.0f, 0.0f }
            };
            dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
            dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, triVerts, sizeof(D3D2DVert));
        }
    };

    // 1. Buildings Pool (skyscrapers, major structures)
    if (CPools::ms_pBuildingPool) {
        int bSize = CPools::ms_pBuildingPool->m_nSize;
        for (int i = 0; i < bSize; ++i) {
            CBuilding* b = CPools::ms_pBuildingPool->GetAt(i);
            renderEntity3dBox(b);
        }
    }

    // 2. Dummy Pool (bridges, towers, landmarks, overpasses)
    if (CPools::ms_pDummyPool) {
        int dSize = CPools::ms_pDummyPool->m_nSize;
        for (int i = 0; i < dSize; ++i) {
            CDummy* d = CPools::ms_pDummyPool->GetAt(i);
            renderEntity3dBox(d);
        }
    }

    // 3. Static World Objects (barriers, ramps, gas stations, large POIs)
    if (CPools::ms_pObjectPool) {
        int oSize = CPools::ms_pObjectPool->m_nSize;
        for (int i = 0; i < oSize; ++i) {
            CObject* obj = CPools::ms_pObjectPool->GetAt(i);
            if (obj && obj->bIsStatic) {
                renderEntity3dBox(obj);
            }
        }
    }
}

