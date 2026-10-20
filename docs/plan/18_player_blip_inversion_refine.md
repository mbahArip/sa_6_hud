# 18 - Player Blip Inversion Refinement

## Objective
Refine player blip color inversion so the blip remains inverted during active pursuit and loss-of-sight flashing stars, while reverting back to default black when in hollow stars search, heat area, or criminal profile investigation states.

## Specifications
1. **Inverted (Red/White / `player_wanted.png`)**:
   - `wantedLevel > 0` AND (`g_wantedStage == WANTED_STAGE_PURSUIT` || `g_wantedStage == WANTED_STAGE_LKP`).
   - Cops have active pursuit or are searching last known position with flashing stars.
2. **Default Black (`player.png` / normal black disc arrow)**:
   - Evaded / hollow stars state (`g_hollowStarsCount > 0`).
   - Search radius stage (`WANTED_STAGE_SEARCH`, `WANTED_STAGE_ESCAPE`).
   - Passive heat area (`WANTED_STAGE_HEAT_AREA` or `g_wantedStage == WANTED_STAGE_HEAT`).
   - Criminal profile investigation without active pursuit stars.
3. **Smooth Cross-fade**:
   - Continuous smoothstep transition between `g_pTexPlayer` and `g_pTexPlayerWanted`.
