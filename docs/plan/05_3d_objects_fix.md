# Plan 05: 3D Objects Minimap Fix

## Objective
Fix rendering of experimental 3D radar objects to eliminate giant grey slabs covering the minimap.

## Specifications
- Fix vertex quad strip ordering in `CSprite2d::Draw2DPolygon`.
- Clip UV projection coordinates to strictly within the radar map bounding box.
- Enforce strict size and LOD filtering so world terrain and parent meshes are skipped.
