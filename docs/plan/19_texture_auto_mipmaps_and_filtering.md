# 19 - Texture Auto-Mipmaps & Anti-Aliasing Filtering

## Objective
Eliminate jagged edges on UI icon textures by enabling automatic mipmap generation in Direct3D 9 for loaded PNG assets and configuring trilinear texture sampling.

## Specifications
1. **D3DX Mipmap Generation (`LoadPngTexture`)**:
   - `MipLevels = D3DX_DEFAULT` (generates full mipmap chain down to 1x1).
   - `Filter = D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER` (high-quality downsampling filter).
   - `MipFilter = D3DX_FILTER_BOX` (high-quality box mip filter).
2. **Sampler States (`DrawD3DTexturedQuad` & `DrawD3DRotatedQuad`)**:
   - `D3DSAMP_MINFILTER = D3DTEXF_LINEAR`
   - `D3DSAMP_MAGFILTER = D3DTEXF_LINEAR`
   - `D3DSAMP_MIPFILTER = D3DTEXF_LINEAR` (trilinear filtering).
3. **Outcome**:
   - Eliminates pixel crawl, aliasing, and jagged edges on all downscaled PNG icons (engine, fuel, nitro, stars, profile badges, weapon icons) without requiring manual DDS conversion.
