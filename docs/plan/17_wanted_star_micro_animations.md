# 17 - Wanted Star Micro Animations

## Objective
Implement micro animations for wanted stars: right-to-left slide animation on star acquisition (starting from rightmost star 1), followed by a snappy 100ms micro-glow behind newly active star slots.

## Specifications
1. **Star Slots Order**:
   - Slot 5 = Star 1 (rightmost).
   - Slot 4 = Star 2.
   - Slot 3 = Star 3.
   - Slot 2 = Star 4.
   - Slot 1 = Star 5.
   - Slot 0 = Star 6 (leftmost).
2. **Slide Animation**:
   - Duration: 180ms (`dt / 0.18f`) with smoothstep curve.
   - Offset: +14px to the right when inactive, slides left to resting slot position.
   - Stagger: sequential activation from rightmost slot (slot 5) to leftmost.
3. **Micro-Glow**:
   - Triggers when slide transition completes (`1.0`).
   - Decay duration: 100ms (`dt / 0.10f`).
   - Visual: Radial glow bloom quad behind star slot, expanding from 1.0x to 1.4x size while fading out.
