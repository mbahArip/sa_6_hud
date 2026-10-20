# 20 - Wanted Star Animations Fine-Tune

## Objective
Fine-tune wanted star micro animations:
1. Slide animation occurs strictly upon first showing the wanted gauge (transitioning from 0 wanted to 1+).
2. When wanted level raises while gauge is already visible (e.g. 1 -> 2 stars), the new star appears directly in place without sliding; only its micro-glow animation triggers.
3. Slide animation duration increased to 300ms (`dt / 0.30f`).
4. Micro-glow visibility improved:
   - Duration: 300ms.
   - Opacity follows a 0% -> 100% -> 0% bell curve (`sin(progress * PI)`).
   - Dual-layer visual: inner additive star flare (`DrawD3DTexturedQuadAdditive`) and outer soft radial glow corona (`DrawSoftGlow`).
