/*
    GTA SA Plugin SDK — Radar Blip Custom Coloring & Channel-Mask Shaders
    Demonstrates:
      1. Single-color vertex modulation (Texture * TintColor multiply blending).
      2. Multi-channel RGB mask shader (D3D9 HLSL ps_3_0) for independent R/G/B tinting.
      3. Hooking CRadar::ShowRadarTraceWithHeight (0x584070) to replace RADAR_SPRITE_NONE.
    Target: GTA SA 1.0 US Compact (0x400000)
*/
#include <plugin.h>
#include "CRadar.h"
#include "CSprite2d.h"
#include <d3d9.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace plugin;

/*
    ===========================================================================
    MULTIPLY BLENDING RULES (RenderWare Fixed-Function Pipeline)
    ===========================================================================
    CSprite2d::Draw uses D3DTOP_MODULATE:
        FinalColor = TextureColor * VertexColor (values normalized 0.0 to 1.0)

    - Pure White  (1.0, 1.0, 1.0) * Tint = Tint        (Clean 100% color replacement)
    - Pure Black  (0.0, 0.0, 0.0) * Tint = Pure Black  (Stays black regardless of tint)
    - Gray/Color  (0.5, 0.5, 0.5) * Tint = Darker Tint (Multiplied, darkened hue)
    - Blue Pixel  (0.0, 0.0, 1.0) * Red  = Black       (0 * 1 = 0, cancels out)

    RULE: For single-color tinting to work cleanly, your icon fill MUST be pure white (#FFFFFF),
    and strokes/borders must be pure black (#000000).
*/

// ===========================================================================
// METHOD 1: Single-Color Modulation (Pure White Fill + Black Stroke)
// ===========================================================================
static CSprite2d gSingleColorBlip;

void DrawSingleColorBlip(CRect const& rect, CRGBA tintColor) {
    // White center turns into tintColor; black border remains pure black
    gSingleColorBlip.Draw(rect, tintColor);
}

// ===========================================================================
// METHOD 2: Multi-Color RGB Channel-Mask Shader (D3D9 HLSL)
// ===========================================================================
/*
    Texture acts as an ID/Channel mask:
      - Red channel   (1,0,0) = Mask for Color 1 (e.g. border / stroke)
      - Green channel (0,1,0) = Mask for Color 2 (e.g. background fill)
      - Blue channel  (0,0,1) = Mask for Color 3 (e.g. icon / symbol)
      - Alpha channel         = Master cutout / transparency
*/
static CSprite2d gChannelMaskBlip;
static IDirect3DPixelShader9* gChannelMaskShader = nullptr;

const char* gShaderHLSL = R"(
    sampler2D s0 : register(s0);
    float4 cColorR : register(c0); // Replacement for Red mask
    float4 cColorG : register(c1); // Replacement for Green mask
    float4 cColorB : register(c2); // Replacement for Blue mask

    float4 main(float2 uv : TEXCOORD0) : COLOR0 {
        float4 mask = tex2D(s0, uv);
        float3 rgb = (mask.r * cColorR.rgb) 
                   + (mask.g * cColorG.rgb) 
                   + (mask.b * cColorB.rgb);
        return float4(rgb, mask.a);
    }
)";

void InitChannelMaskShader() {
    IDirect3DDevice9* dev = *reinterpret_cast<IDirect3DDevice9**>(0xC97C18);
    if (!dev) return;

    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(
        gShaderHLSL, strlen(gShaderHLSL),
        nullptr, nullptr, nullptr,
        "main", "ps_3_0", 0, 0,
        &shaderBlob, &errorBlob
    );

    if (SUCCEEDED(hr) && shaderBlob) {
        dev->CreatePixelShader(
            reinterpret_cast<DWORD*>(shaderBlob->GetBufferPointer()),
            &gChannelMaskShader
        );
        shaderBlob->Release();
    }
}

void DrawChannelMaskBlip(CRect const& rect, CRGBA colorR, CRGBA colorG, CRGBA colorB) {
    IDirect3DDevice9* dev = *reinterpret_cast<IDirect3DDevice9**>(0xC97C18);
    if (!dev || !gChannelMaskShader) return;

    // Convert CRGBA (0-255) to float4 (0.0-1.0)
    float c0[4] = { colorR.r / 255.0f, colorR.g / 255.0f, colorR.b / 255.0f, 1.0f };
    float c1[4] = { colorG.r / 255.0f, colorG.g / 255.0f, colorG.b / 255.0f, 1.0f };
    float c2[4] = { colorB.r / 255.0f, colorB.g / 255.0f, colorB.b / 255.0f, 1.0f };

    dev->SetPixelShaderConstantF(0, c0, 1);
    dev->SetPixelShaderConstantF(1, c1, 1);
    dev->SetPixelShaderConstantF(2, c2, 1);

    dev->SetPixelShader(gChannelMaskShader);
    gChannelMaskBlip.Draw(rect, CRGBA(255, 255, 255, 255));
    dev->SetPixelShader(nullptr); // Restore RenderWare state
}

// ===========================================================================
// HOOK: Intercept Default Blip Drawing (RADAR_SPRITE_NONE)
// ===========================================================================
// 0x584070: void CRadar::ShowRadarTraceWithHeight(float x, float y, uint size, uchar r, uchar g, uchar b, uchar a, uchar type)
void CustomShowRadarTraceWithHeight(float x, float y, unsigned int size,
                                    unsigned char r, unsigned char g, unsigned char b, unsigned char a,
                                    unsigned char type)
{
    float halfSize = static_cast<float>(size) * 0.5f;
    CRect rect(x - halfSize, y - halfSize, x + halfSize, y + halfSize);
    CRGBA blipColor(r, g, b, a);

    // Example A: Use single-color modulation with game's dynamic blip color
    DrawSingleColorBlip(rect, blipColor);

    /*
    // Example B: Use multi-channel RGB mask shader
    DrawChannelMaskBlip(
        rect,
        CRGBA(0, 0, 0, 255),       // Red map   -> Black border
        blipColor,                 // Green map -> Dynamic target fill (red/green/yellow)
        CRGBA(255, 255, 255, 255)  // Blue map  -> White inner icon
    );
    */
}

class RadarBlipColoringDemo {
public:
    RadarBlipColoringDemo() {
        Events::initRwEvent += [] {
            gSingleColorBlip.m_pTexture = RwTextureRead("blip_single", nullptr);
            gChannelMaskBlip.m_pTexture = RwTextureRead("blip_mask", nullptr);
            InitChannelMaskShader();
        };

        Events::shutdownRwEvent += [] {
            gSingleColorBlip.Delete();
            gChannelMaskBlip.Delete();

            if (gChannelMaskShader) {
                gChannelMaskShader->Release();
                gChannelMaskShader = nullptr;
            }
        };

        // Detour 0x584070 to our custom blip renderer
        patch::RedirectJump(0x584070, CustomShowRadarTraceWithHeight);
    }
} radarBlipColoringDemo;
