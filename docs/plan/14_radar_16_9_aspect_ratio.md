# 14 - Radar 16:9 Aspect Ratio & Normalized Base Scale

## Objective
Standardize the radar frame geometry to a 16:9 ($1.7778:1$) aspect ratio while preserving the 0.85 scale width as the new default baseline scale of 1.0.

## Mathematical Formulation
- Base Width ($W_{base}$ at scale 1.0):
  $W = sH \times 0.25 \times 0.85 \times 1.52 = sH \times 0.3230$
- Base Height ($H_{base}$ for 16:9 ratio):
  $H = \frac{W}{16 / 9} = W \times \frac{9}{16} = sH \times 0.3230 \times 0.5625 = sH \times 0.1816875$
- Formula in `UpdateRadarRect()`:
  ```cpp
  float w = sH * 0.3230f * g_config.radarSizeScale;
  float h = w * (9.0f / 16.0f);
  ```
- Scale Normalization:
  - Default `radarSizeScale = 1.0f`.
  - `6hud.ini`: `RADAR_SIZE_SCALE = 1.00`.
  - At 1.00x scale, radar displays exact 0.85 original width in standard 16:9 form factor.
