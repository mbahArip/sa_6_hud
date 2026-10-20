# 6HUD Project Instructions

## Project Paths & Environment
- **Game Directory**: `D:\Games\gta sa\GTA San Andreas`
- **Plugin SDK Repository**: `C:\Users\me\dev\sa_project\plugin-sdk-sa`

## Planning & Documentation Rules
- For every implementation or confirmed plan, write a markdown file under `docs/plan/` using the naming format `<number>_<planned_feature>.md`.
- If a plan spans multiple features, split into separate plan files so each feature has exactly 1 plan file.

## SDK & Safety Auditing Rules
- Always audit or recheck game structures, hook points, memory offsets, and API signatures against the `sa-plugin-sdk` skill or its source repository (`C:\Users\me\dev\sa_project\plugin-sdk-sa`) to verify safety and compatibility with GTA San Andreas 1.0 US.
- When performing an audit, invoke the `audit-loop` skill to conduct a rigorous, iterative review-and-fix pass before finalizing changes.
