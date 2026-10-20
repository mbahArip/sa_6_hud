# 13 - Backdrop Blur Quality & Intensity Upgrade

## Objective
Upgrade the D3D9 live backdrop blur to be thicker, creamier, and higher quality with configurable global settings (`BACKDROP_BLUR_INTENSITY` and `BACKDROP_BLUR_QUALITY`).

## Specifications
1. **Configurable Tokens (`[Global]`)**:
   - `BACKDROP_BLUR_INTENSITY = 1.50` (float multiplier for blur spread radius).
   - `BACKDROP_BLUR_QUALITY = 3` (int: 1 = Low 8x, 2 = Medium 4x, 3 = High 2x, 4 = Ultra native resolution).
2. **Shader Architecture (`BackdropBlur.hlsl` / `BackdropBlurPS.h`)**:
   - Upgraded from 9-tap single ring to 17-tap dual-ring Gaussian distribution kernel:
     - 1 center tap (weight 0.16)
     - 8 inner ring taps at 0.55r (weight 0.06 each = 0.48)
     - 8 outer ring taps at 1.25r rotated 22.5 deg (weight 0.045 each = 0.36)
     - Sum of weights = 1.00.
   - Compiled with `fxc.exe /T ps_2_0 /E main /Fh src/BackdropBlurPS.h /Vn g_psBackdropBlur`.
3. **Reusability**:
   - Centralized blur shader and parameters available for minimap and future translucent UI layers.
