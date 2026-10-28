# ==============================================================================
# 6HUD - Release Packaging Script
# ==============================================================================
# Assembles a distributable RAR archive containing everything a player needs:
#   6hud.asi        - the compiled plugin
#   6hud.ini        - fully-commented configuration template
#   6hud/colors.ini - runtime colour palette (written by the in-game menu)
#   6hud/assets/    - radar / wanted / profile textures
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools/package-release.ps1
#   powershell -ExecutionPolicy Bypass -File tools/package-release.ps1 -Version 0.2.0
# ==============================================================================

param(
    [string]$Version = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot    = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $RepoRoot "6hud.vcxproj"
$GameDir     = "D:\Games\gta sa\GTA San Andreas"
$DistDir     = Join-Path $RepoRoot "dist"
$StageDir    = Join-Path $DistDir "6hud"
$RarExe      = "C:\Program Files\WinRAR\Rar.exe"

# --- Version ----------------------------------------------------------------
if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = (Get-Content (Join-Path $RepoRoot "VERSION.txt") -ErrorAction SilentlyContinue | Select-Object -First 1)
}
if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = "0.1.0"
}
$Version = $Version.Trim()

Write-Host "=== 6HUD Release Packaging ===" -ForegroundColor Cyan
Write-Host "Version: $Version"

# --- Build ------------------------------------------------------------------
if (-not $SkipBuild) {
    $msbuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
    if (-not (Test-Path $msbuild)) {
        $msbuild = (Get-Command msbuild -ErrorAction SilentlyContinue).Source
    }
    if (-not $msbuild -or -not (Test-Path $msbuild)) {
        throw "MSBuild not found. Build the project manually or pass -SkipBuild."
    }
    Write-Host "`n[1/5] Building Release..." -ForegroundColor Yellow
    & $msbuild $ProjectFile /p:Configuration=Release /p:Platform=Win32 /v:m /nologo
    if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }
}

$AsiPath = Join-Path $RepoRoot "Release\6hud.asi"
if (-not (Test-Path $AsiPath)) { throw "Built plugin not found at $AsiPath" }

# --- Stage ------------------------------------------------------------------
Write-Host "`n[2/5] Staging files..." -ForegroundColor Yellow
if (Test-Path $StageDir) { Remove-Item -Recurse -Force $StageDir }
New-Item -ItemType Directory -Force -Path $StageDir | Out-Null

Copy-Item $AsiPath (Join-Path $StageDir "6hud.asi") -Force
Copy-Item (Join-Path $RepoRoot "6hud.ini") (Join-Path $StageDir "6hud.ini") -Force

# 6hud/colors.ini is the file the plugin reads (GetColorsIniPath) and writes.
# The repo keeps the template in assets/ so it ships with the texture tree.
New-Item -ItemType Directory -Force -Path (Join-Path $StageDir "6hud") | Out-Null
Copy-Item (Join-Path $RepoRoot "assets\colors.ini") (Join-Path $StageDir "6hud\colors.ini") -Force

# Texture & data tree
$assetsSrc = Join-Path $RepoRoot "assets"
$assetsDst = Join-Path $StageDir "6hud\assets"
New-Item -ItemType Directory -Force -Path $assetsDst | Out-Null
Copy-Item (Join-Path $assetsSrc "*") $assetsDst -Recurse -Force
Remove-Item (Join-Path $assetsDst "colors.ini") -Force -ErrorAction SilentlyContinue

# --- Install instructions ----------------------------------------------------
$readme = @"
6HUD - Modern Minimalist Radar & HUD for GTA San Andreas
Version $Version  |  100% AI-coded, Work In Progress

INSTALLATION
1. Make sure you have an ASI Loader installed (e.g. Silent's ASI Loader).
2. Copy the contents of this folder into your GTA San Andreas root directory
   (the folder containing gta_sa.exe):

       6hud.asi
       6hud.ini
       6hud\
           colors.ini
           assets\

3. Start the game.

IMPORTANT: CONFIG UPGRADE & MIGRATION
The configuration structure in 6hud.ini has been reorganized into [UI], [EXT], and [KEYS] sections.
If upgrading from an earlier version (e.g. v0.1.0), do NOT overwrite your settings blindly:
1. Rename your existing '6hud.ini' to '6hud.old.ini' before extracting.
2. Extract the new '6hud.ini' and files.
3. 6HUD automatically detects '6hud.old.ini' on startup and migrates all your customized settings
   into the new structure seamlessly.

REQUIREMENTS
- GTA San Andreas 1.0 US "HOODLUM" only
  gta_sa.exe  14,383,616 bytes
  MD5         07B6C7496D10ADA10A7AFB930BF1C709
  SHA-256     21903BCD40957F130A0BD802A44FC4CDBEC2A8224F757B03EC3CB7467CC06EF3
- An ASI Loader
- CLEO and MoonLoader are NOT required

The mod patches fixed addresses in the executable, so only the 1.0 US HOODLUM
build is supported. 1.0 EU, 1.1 US/EU, Steam and Compact builds will not work.

DEFAULT KEYS
  J      Reveal hidden HUD elements and info cards
  K      Hold to zoom the radar out for a tactical overview
  G      Hold to peek a zoomed-out radar view
  /      Open the in-game settings menu (ESC closes it)
  LCTRL  Hold near a gas pump to open fuel & repair services (Gamepad: LB)

NOTES
- The radar is hidden on foot by default. Press J to show it.
- All options are documented inline in 6hud.ini.
- Colours are written to 6hud\colors.ini when you save from the in-game menu.
- This is a work in progress. Please report bugs in the GitHub issue tracker.

CREDITS
- 100% AI-generated and iterated via LLM agents.
- SDK:      Dryxio/plugin-sdk-sa
- UI:       mbahArip/sa_ui
- Reference: v-hud by gennariarmando
- GTA San Andreas and its data files belong to Rockstar Games.
"@
Set-Content -Path (Join-Path $StageDir "README.txt") -Value $readme -Encoding UTF8

# --- Archive ----------------------------------------------------------------
Write-Host "`n[3/5] Creating archive..." -ForegroundColor Yellow
$RarPath = Join-Path $DistDir "6hud_v$Version.rar"
if (Test-Path $RarPath) { Remove-Item -Force $RarPath }
if (-not (Test-Path $RarExe)) { throw "WinRAR not found at $RarExe" }
& $RarExe a -ep1 -m5 -r -x"*.git*" -x"*.log" $RarPath (Join-Path $StageDir "*")
if ($LASTEXITCODE -ne 0) { throw "RAR creation failed with exit code $LASTEXITCODE" }

# --- Deploy -----------------------------------------------------------------
Write-Host "`n[4/5] Deploying to game directory..." -ForegroundColor Yellow
Copy-Item (Join-Path $StageDir "6hud.asi") $GameDir -Force

# Seed config and palette only when absent: both are rewritten by the in-game
# menu, so an existing file holds the developer's own saved settings.
$gameIni = Join-Path $GameDir "6hud.ini"
if (-not (Test-Path $gameIni)) {
    Copy-Item (Join-Path $StageDir "6hud.ini") $gameIni -Force
    Write-Host "  seeded 6hud.ini (new install)"
} else {
    Write-Host "  kept   6hud.ini (existing settings preserved)"
}

$gameColors = Join-Path $GameDir "6hud\colors.ini"
New-Item -ItemType Directory -Force -Path (Join-Path $GameDir "6hud") | Out-Null
if (-not (Test-Path $gameColors)) {
    Copy-Item (Join-Path $StageDir "6hud\colors.ini") $gameColors -Force
    Write-Host "  seeded 6hud\colors.ini (new install)"
} else {
    Write-Host "  kept   6hud\colors.ini (existing settings preserved)"
}

$gameAssets = Join-Path $GameDir "6hud\assets"
New-Item -ItemType Directory -Force -Path $gameAssets | Out-Null
Copy-Item (Join-Path $assetsSrc "*") $gameAssets -Recurse -Force

# --- Report -----------------------------------------------------------------
Write-Host "`n[5/5] Done" -ForegroundColor Green
$RarInfo = Get-Item $RarPath
Write-Host ""
Write-Host "  Archive : $($RarInfo.FullName)"
Write-Host ("  Size    : {0:N0} bytes" -f $RarInfo.Length)
Write-Host "  Staged  : $StageDir"
Write-Host "  Deployed: $GameDir"
Write-Host ""
Write-Host "  SHA256  : $((Get-FileHash $RarPath -Algorithm SHA256).Hash)"
