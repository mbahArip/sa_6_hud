$ErrorActionPreference = "Stop"
$root = "C:\Users\me\dev\sa_project\6hud"
$raw  = Join-Path $root "assets_raw"
$out  = Join-Path $root "assets"
$exe  = "C:\Users\me\dev\sa_project\txd-cli\target\release\txd-cli.exe"

function Convert-One($src, $dst, $comp) {
    $dir = Split-Path $dst -Parent
    if (!(Test-Path $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
    & $exe convert $src $dst --compression $comp --mipmaps
}

# ── icons: dxt5 ──
$icons = @(
    @("wallet.png",              "icons\wallet.dds"),
    @("radar\engine.png",        "icons\engine.dds"),
    @("radar\fuel.png",          "icons\fuel.dds"),
    @("radar\nitro.png",         "icons\nitro.dds"),
    @("radar\ifruit.png",        "icons\ifruit.dds"),
    @("wanted\wantedStar.png",   "icons\wantedStar.dds"),
    @("wanted\profile\identity.png", "icons\profile\identity.dds"),
    @("wanted\profile\outfit.png",   "icons\profile\outfit.dds"),
    @("wanted\profile\vehicle.png",  "icons\profile\vehicle.dds"),
    @("wanted\profile\weapon.png",   "icons\profile\weapon.dds")
)
foreach ($i in $icons) { Convert-One (Join-Path $raw $i[0]) (Join-Path $out $i[1]) "dxt5" }

# ── blips: dxt3 ──
$blips = @(
    @("radar\mission.png",       "blips\mission.dds"),
    @("radar\north.png",         "blips\north.dds"),
    @("radar\player.png",        "blips\player.dds"),
    @("radar\player_wanted.png", "blips\player_wanted.dds"),
    @("radar\z_above.png",       "blips\z_above.dds"),
    @("radar\z_below.png",       "blips\z_below.dds")
)
foreach ($b in $blips) { Convert-One (Join-Path $raw $b[0]) (Join-Path $out $b[1]) "dxt3" }

# ── maps: dxt3, preserve day/night subdirs ──
Get-ChildItem (Join-Path $raw "radar\map") -Recurse -Filter *.png | ForEach-Object {
    $rel = $_.FullName.Substring((Join-Path $raw "radar\map").Length).TrimStart('\')
    $dst = Join-Path $out ("maps\" + [IO.Path]::ChangeExtension($rel, ".dds"))
    Convert-One $_.FullName $dst "dxt3"
}

Write-Output "=== done ==="
