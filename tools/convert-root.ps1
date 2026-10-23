$ErrorActionPreference = "Stop"
$root = "C:\Users\me\dev\sa_project\6hud"
$raw  = Join-Path $root "assets_raw"
$out  = Join-Path $root "assets"
$exe  = "C:\Users\me\dev\sa_project\txd-cli\target\release\txd-cli.exe"

# png (root of assets_raw) -> dest relative path under assets/, compression
$jobs = @(
    @("mission.png",           "blips\mission.dds",           "dxt5"),
    @("north.png",             "blips\north.dds",             "dxt5"),
    @("player.png",            "blips\player.dds",            "dxt5"),
    @("zAbove.png",            "blips\z_above.dds",           "dxt5"),
    @("zBelow.png",            "blips\z_below.dds",           "dxt5"),
    @("ifruit.png",            "icons\ifruit.dds",            "dxt5"),
    @("wallet.png",            "icons\wallet.dds",            "dxt5"),
    @("wantedStar.png",        "icons\wantedStar.dds",        "dxt5"),
    @("wantedStarBorder.png",  "icons\wantedStarBorder.dds",  "dxt5"),
    @("policeArc.png",         "ui\cops_arrow.dds",           "dxt3")
)

foreach ($j in $jobs) {
    $src = Join-Path $raw $j[0]
    $dst = Join-Path $out $j[1]
    if (!(Test-Path $src)) { throw "missing source: $src" }
    $dir = Split-Path $dst -Parent
    if (!(Test-Path $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
    & $exe convert $src $dst --compression $j[2] --mipmaps
    if ($LASTEXITCODE -ne 0) { throw "convert failed: $src" }
}
