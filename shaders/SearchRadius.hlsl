// SearchRadius.hlsl - Modern GTA 6 style wanted search radius & heat perimeter

// c0: (centerX, centerY, radiusX, radiusY) in UV space [0..1]
// c1: (heatScale, heatOpacity, copVisual, pulse)
// c2: (strobeRed: r, g, b, a)
// c3: (strobeBlue: r, g, b, a)
// c4: (searchBlur, heatBlur, innerSearchAlpha, heatAlpha)

float4 c0 : register(c0);
float4 c1 : register(c1);
float4 c2 : register(c2);
float4 c3 : register(c3);
float4 c4 : register(c4);

struct PS_INPUT {
    float4 Diffuse : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : COLOR0 {
    float2 delta = (input.TexCoord - c0.xy) / max(c0.zw, 0.0001f);
    float dist = length(delta);

    float heatScale = c1.x;
    float heatOpa   = c1.y * c4.w; // heatOpacity * heatAlpha
    float copVisual = c1.z;
    float pulse     = c1.w;

    // Search fill colors (calm searching blue vs. aggressive siren strobe)
    float4 strobeCol = (pulse > 0.5f) ? c2 : c3;
    float4 calmCol = float4(0.18f, 0.50f, 0.95f, 0.30f + 0.10f * pulse);
    float4 searchFill = (copVisual > 0.5f) ? strobeCol : calmCol;
    searchFill.a *= c4.z; // innerSearchAlpha

    // 1. Inner search radius mask: feathered boundary configured by c4.x (searchBlur)
    float innerMask = smoothstep(1.0f, max(1.0f - c4.x, 0.0f), dist);

    // 2. Outer heat perimeter: feathered falloff band configured by c4.y (heatBlur)
    float heatOuterStart = max(heatScale - c4.y, 0.0f);
    float heatOuterFade = smoothstep(heatScale, heatOuterStart, dist);
    float heatMask = heatOuterFade * heatOpa;

    float4 heatCol = float4(0.92f, 0.14f, 0.18f, 1.0f);

    // Composite non-premultiplied straight alpha & colors
    float fillAlpha = innerMask * searchFill.a;
    float totalAlpha = saturate(fillAlpha + heatMask * (1.0f - fillAlpha));

    if (totalAlpha < 0.001f) {
        discard;
    }

    // Smooth color blend from search fill to heat red
    float blendFactor = fillAlpha / max(totalAlpha, 0.0001f);
    float3 outRgb = lerp(heatCol.rgb, searchFill.rgb, blendFactor);

    return float4(outRgb, totalAlpha * input.Diffuse.a);
}
