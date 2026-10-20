# 22 - Wanted Star Smooth Glow and Synchronized Animation

## Objective
Smooth wanted star glow edges and synchronize glow pulses across all active stars over a 500ms duration.

## Problem Analysis
1. Glow edge appeared stiff and harsh due to:
   - Polished star mesh quad being scaled up (`bloomScale = 1.15f -> 1.50f`) with additive blending, projecting hard vector polygon corners.
   - `DrawSoftGlow` using a linear-falloff triangle fan with visible chords and non-zero boundary gradients.
2. Glow animations previously triggered sequentially per-star as each individual star finished its slide ("one star at a time"), looking disconnected.

## Changes
1. **Procedural Gaussian Glow Texture (`g_pTexSoftGlow`)**:
   - Create a 64x64 D3D9 texture with Gaussian falloff `exp(-3.2 * dist^2)` smoothly tapered to zero at boundary using quadratic smoothstep `taper = (1-d)^2 * (3 - 2*(1-d))`.
   - Guaranteed zero derivative at boundary: completely eliminates stiff or abrupt edges.
2. **Layered Smooth Bloom**:
   - Outer soft warm halo (`DrawD3DTexturedQuad`) with alpha blending for ambient dispersion.
   - Inner core corona (`DrawD3DTexturedQuadAdditive`) with additive blending.
   - Unscaled additive flash on active star quad at native size to intensify brightness without distorting or scaling star geometry.
3. **Synchronized 500ms Animation**:
   - Animation progress duration set to 500ms (`dt / 0.50f`).
   - All active stars initiate glow synchronously at the exact same time upon gauge appearance or wanted level increase.
