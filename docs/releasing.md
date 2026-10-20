# Release Process

## Prerequisites

- Visual Studio 2022 / 2026 with the Win32 (x86) toolset
- WinRAR (`C:\Program Files\WinRAR\Rar.exe`) — RAR 5.x or later
- The `plugin-sdk-sa` clone reachable via `$(SdkRoot)` (see `6hud.vcxproj`)

## Packaging

One command builds, stages, archives and deploys a release:

```powershell
powershell -ExecutionPolicy Bypass -File tools/package-release.ps1
```

The script:

1. Builds `6hud.vcxproj` in `Release|Win32` (skip with `-SkipBuild`).
2. Stages `dist/6hud/` containing `6hud.asi`, `6hud.ini`, `6hud/colors.ini`, `6hud/assets/**` and a generated `README.txt`.
3. Creates `dist/6hud_v<version>.rar` with the archive root flattened so extraction drops the files straight into the game folder.
4. Deploys to the game directory, seeding `6hud.ini` and `6hud/colors.ini` **only when absent** so in-game menu settings are never clobbered.
5. Prints the archive path, size and SHA-256.

### Version

The version comes from `VERSION.txt` in the repo root. Bump it before packaging:

```powershell
Set-Content VERSION.txt "0.2.0"
powershell -ExecutionPolicy Bypass -File tools/package-release.ps1
```

Or override per-run without touching the file:

```powershell
powershell -ExecutionPolicy Bypass -File tools/package-release.ps1 -Version 0.2.0
```

## Publishing

```powershell
git tag v0.2.0
git push origin v0.2.0
gh release create v0.2.0 "dist\6hud_v0.2.0.rar" `
    --title "6HUD v0.2.0" `
    --notes-file "docs\release-notes-v0.2.0.md" `
    --prerelease
```

Mark the release as a **prerelease** while the mod is in WIP. Promote it to a full release once the feature set stabilises.

## Archive layout

```text
6hud.asi
6hud.ini
6hud\
    colors.ini          ; palette, read and written by the plugin
    assets\
        wallet.png
        radar\*.png
        wanted\*.png
        wanted\profile\*.png
        weapon\          ; optional custom weapon icons
```

`6hud/colors.ini` must sit at that exact path — `GetColorsIniPath()` in `src/Config.cpp` resolves it relative to the game root. The post-build event in `6hud.vcxproj` seeds it on first install only.

## Verification checklist

- [ ] `dist/6hud_v<version>.rar` extracts into the game root with the layout above
- [ ] Game boots, `6hud.log` shows `6hud initialized successfully`
- [ ] All shaders report `created OK` and all textures report `loaded`
- [ ] Radar, GPS ribbon, info cards and the settings menu render correctly
- [ ] The in-game settings menu opens, saves, and the changes survive a restart
