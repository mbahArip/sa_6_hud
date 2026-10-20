sampler2D s0 : register(s0);
float4 cOffset : register(c0); // (texelX, texelY, blurIntensity, alpha)

float4 main(float2 uv : TEXCOORD0, float4 vColor : COLOR0) : COLOR0 {
    float2 r = cOffset.xy * cOffset.z;
    float2 rIn = r * 0.55;
    float2 rOut = r * 1.25;

    // Center tap
    float4 col = tex2D(s0, uv) * 0.16;

    // Inner ring (8 taps at 0.55r)
    col += tex2D(s0, uv + float2( rIn.x,  0.0)) * 0.06;
    col += tex2D(s0, uv - float2( rIn.x,  0.0)) * 0.06;
    col += tex2D(s0, uv + float2( 0.0,  rIn.y)) * 0.06;
    col += tex2D(s0, uv - float2( 0.0,  rIn.y)) * 0.06;
    col += tex2D(s0, uv + float2( rIn.x,  rIn.y) * 0.7071) * 0.06;
    col += tex2D(s0, uv - float2( rIn.x,  rIn.y) * 0.7071) * 0.06;
    col += tex2D(s0, uv + float2(-rIn.x,  rIn.y) * 0.7071) * 0.06;
    col += tex2D(s0, uv + float2( rIn.x, -rIn.y) * 0.7071) * 0.06;

    // Outer ring (8 taps at 1.25r, rotated 22.5 deg)
    col += tex2D(s0, uv + float2( rOut.x * 0.9239,  rOut.y * 0.3827)) * 0.045;
    col += tex2D(s0, uv - float2( rOut.x * 0.9239,  rOut.y * 0.3827)) * 0.045;
    col += tex2D(s0, uv + float2(-rOut.x * 0.3827,  rOut.y * 0.9239)) * 0.045;
    col += tex2D(s0, uv - float2(-rOut.x * 0.3827,  rOut.y * 0.9239)) * 0.045;
    col += tex2D(s0, uv + float2( rOut.x * 0.3827,  rOut.y * 0.9239)) * 0.045;
    col += tex2D(s0, uv - float2( rOut.x * 0.3827,  rOut.y * 0.9239)) * 0.045;
    col += tex2D(s0, uv + float2(-rOut.x * 0.9239,  rOut.y * 0.3827)) * 0.045;
    col += tex2D(s0, uv - float2(-rOut.x * 0.9239,  rOut.y * 0.3827)) * 0.045;

    col.rgb *= vColor.rgb;
    col.a = vColor.a * cOffset.w;
    return col;
}
