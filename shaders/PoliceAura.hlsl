// PoliceAura.hlsl - Modern GTA 6 dynamic police threat lightbar auras
// Features: 4 discrete strobe pods (2 Blue / 2 Red) with independent wave scanning,
// dynamic size/blur/opacity per pod (big-small plume pop), pure additive optical bloom,
// and brilliant white core highlights.

// c0: (pulse0, pulse1, pulse2, pulse3) - individual head opacities / pulses
// c1: (rad0, rad1, rad2, rad3)         - individual head dynamic X-radii
// c2: (sirenRed: r, g, b, 1.0)
// c3: (sirenBlue: r, g, b, 1.0)
// c4: (pod0_x, pod1_x, pod2_x, pod3_x) - pod X positions along tangent axis
// c5: (baseRadY, whiteCoreStrength, 0.0, 0.0)

float4 c0 : register(c0);
float4 c1 : register(c1);
float4 c2 : register(c2);
float4 c3 : register(c3);
float4 c4 : register(c4);
float4 c5 : register(c5);

struct PS_INPUT {
    float4 Diffuse : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : COLOR0 {
    float2 uv = input.TexCoord;

    // Smooth edge fade on all 4 sides of the quad (feathered to 0 to prevent hard geometry cuts)
    float sideFade     = saturate(uv.x * (1.0f - uv.x) * 7.0f);
    float verticalFade = saturate(uv.y * (1.0f - uv.y) * 7.0f);
    float quadFade     = sideFade * verticalFade;

    // Y coordinate centered along lightbar forward axis (uv.y = 0.5f is center of pods)
    float dy = (uv.y - 0.5f) / c5.x;
    float dySq = dy * dy;

    // Pod 0 (Blue outer)
    float dx0 = (uv.x - c4.x) / c1.x;
    float g0 = exp(-(dx0 * dx0 + dySq) * 2.2f) * c0.x;

    // Pod 1 (Blue inner)
    float dx1 = (uv.x - c4.y) / c1.y;
    float g1 = exp(-(dx1 * dx1 + dySq) * 2.2f) * c0.y;

    // Pod 2 (Red inner)
    float dx2 = (uv.x - c4.z) / c1.z;
    float g2 = exp(-(dx2 * dx2 + dySq) * 2.2f) * c0.z;

    // Pod 3 (Red outer)
    float dx3 = (uv.x - c4.w) / c1.w;
    float g3 = exp(-(dx3 * dx3 + dySq) * 2.2f) * c0.w;

    float blueGlow = g0 + g1;
    float redGlow  = g2 + g3;

    // Additive color accumulation
    float3 outRgb = c3.rgb * (blueGlow * quadFade) + c2.rgb * (redGlow * quadFade);

    // Brilliant white core at center of each active beam (additive optical bloom)
    float coreFactor = (blueGlow * blueGlow + redGlow * redGlow) * quadFade * c5.y * 0.45f;
    outRgb += float3(coreFactor, coreFactor, coreFactor);

    // Modulate by vertex alpha (master opacity / distance attenuation)
    outRgb *= input.Diffuse.a;

    // Discard completely dark pixels
    if (dot(outRgb, float3(1.0f, 1.0f, 1.0f)) < 0.002f) {
        discard;
    }

    return float4(outRgb, 1.0f);
}
