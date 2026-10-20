# Radar Rendering Pipeline & Geometry Math

## 1. Direct3D 9 Stencil Masking
To render custom rounded rectangle frames without texture bleeding or jagged edges, `6hud` uses hardware stencil masking:
1. Clear the radar bounding box in the stencil buffer with value `0`.
2. Configure Direct3D 9 render states:
   - `D3DRS_STENCILENABLE = TRUE`
   - `D3DRS_STENCILFUNC = D3DCMP_ALWAYS`
   - `D3DRS_STENCILPASS = D3DSTENCILOP_REPLACE` (write ref value `1`).
   - `D3DRS_COLORWRITEENABLE = 0` (disable color writes).
3. Draw a rounded rectangle fan covering the interior area.
4. Enable color writes (`D3DRS_COLORWRITEENABLE = 0xF`) and set stencil test to `D3DCMP_EQUAL` with ref `1`.
5. Draw map tiles and police tint. All out-of-bounds pixels are discarded at the hardware level.
6. Disable stencil test before drawing borders, blips, and HUD badges.

## 2. Linear Affine Projection
Vanilla 2D radar tiles and blips use flat orthogonal coordinates. To create the GTA 6 angled perspective without introducing non-linear texture bending or quad shearing:
- Relative world offset:
  $$\Delta x = P_x - C_x, \quad \Delta y = P_y - C_y$$
- Rotate into camera space:
  $$r_x = \Delta x \cdot \text{camRight}_x + \Delta y \cdot \text{camRight}_y$$
  $$r_y = \Delta x \cdot \text{camUp}_x + \Delta y \cdot \text{camUp}_y$$
- Linear tilt compression:
  $$s_x = r_x \cdot \frac{h_h}{\text{range}}$$
  $$s_y = -r_y \cdot \cos(\text{pitch}) \cdot \frac{h_h}{\text{range}}$$
- Final screen UV:
  $$u = \frac{s_x}{2 \cdot h_w} + 0.5$$
  $$v = \frac{s_y}{2 \cdot h_h} + V_{\text{center}}$$

Where $V_{\text{center}} = 0.50$ on foot and smoothly transitions to $0.65$ when driving vehicles.

## 3. Perimeter Boundary Clamping (`ProjectToRim`)
Out-of-range blips and compass indicators are clamped directly onto the rounded bezel curve rather than snapping to square box edges:
1. Compute ray from radar center $(c_x, c_y)$ along direction vector $(d_x, d_y)$.
2. Calculate bounding box intersection $(e_x, e_y)$.
3. Sample 24-point rounded perimeter curve and find closest orthogonal projection point $(q_x, q_y)$.
4. Position blip or badge at the calculated perimeter coordinate.
