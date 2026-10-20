# Plan 03: Money Counter & Telemetry Gauge

## Objective
Display money amount and a circular money telemetry gauge directly below the weapon icon in the top-right HUD stack.

## Specifications
- **Placement**: Top-right corner directly below weapon icon.
- **Structure**: `[money] [gauge]` side by side.
- **Money Text**:
  - Value: `CWorld::Players[0].m_nMoney`.
  - Format: `$123,456,789` (comma-separated).
  - Font: Pricedown font (`FONT_PRICEDOWN`), 16px size, 1px black outline.
- **Telemetry Gauge**:
  - Circular arc gauge following vehicle telemetry style from `RadarRenderCards.inc`.
  - Maximum value: `999,999,999` (GTA SA money cap).
  - Center icon: `assets/wallet.png` (size 14px; fallback if file missing).
  - Arc color: Green.
- **Colors**:
  - `money.text` = `#FFFFFF`.
  - `money.stroke` = `#000000`.
  - `money.gauge` = `#2ECC71` (green).

## Safety & Audit
- Check `CWorld::Players` memory structure in `plugin-sdk-sa`.
