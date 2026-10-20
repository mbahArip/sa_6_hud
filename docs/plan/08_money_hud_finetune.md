# Plan 08: Money HUD Finetune

## Objective
Add module controls, 20px font, interior detection, outdoor auto-hide, and rolling number animation.

## Specifications
- **Config**:
  - `[money]` -> `USE_MONEY_UI = 1` (boolean toggle for money text + gauge)
- **Font Size**:
  - Scale Pricedown font to 20px (`sH * (20.0f / 720.0f)`).
  - Align text baseline with gauge center.
- **Interior & Outdoor Auto-Hide**:
  - Check `CGame::currArea > 0` or `player->m_nAreaCode > 0`.
  - Indoors: permanently visible (`alpha = 1.0`).
  - Outdoors (`area == 0`): auto-hides after `UI_IDLE_TIMEOUT` seconds idle.
  - Active triggers: money balance change (`delta != 0`), 'J' key peek hold.
- **Rolling Number Animation**:
  - Lerp `m_renderMoney` towards player money `CWorld::Players[0].m_nMoney`.
  - Adaptive speed: fast scroll on large deltas, incremental stepping on small deltas.
