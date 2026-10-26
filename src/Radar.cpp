#include "Radar.h"
#include "Config.h"
#include "I18n.h"
#include "Menu.h"

#include <plugin.h>
#include <CWorld.h>
#include <CTheScripts.h>
#include <CScene.h>
#include <CEntity.h>
#include <CPlayerPed.h>
#include <CVehicle.h>
#include <CCopPed.h>
#include <CCarCtrl.h>
#include <CTaskComplexLeaveCar.h>
#include <CAutomobile.h>
#include <CBike.h>
#include <CHeli.h>
#include <CRadar.h>
#include <CHud.h>
#include <CClock.h>
#include <CFont.h>
#include <CPathFind.h>
#include <CWanted.h>
#include <CTimer.h>
#include <CCutsceneMgr.h>
#include <CMenuManager.h>
#include <CCamera.h>
#include <CPools.h>
#include <CTxdStore.h>
#include <TxdDef.h>
#include <CStreaming.h>
#include <CWeather.h>
#include <CGame.h>
#include <CTheZones.h>
#include <enums/eModelID.h>
#include <CZone.h>
#include <CText.h>
#include <CModelInfo.h>
#include <CVehicleModelInfo.h>
#include <CGeneral.h>
#include <CSprite.h>
#include <CReplay.h>
#include <enums/eVehicleType.h>
#include <enums/eVehicleClass.h>
#include <CPedClothesDesc.h>
#include <CCustomCarPlateMgr.h>
#include <CMenuSystem.h>
#include <CKeyGen.h>
#include <CAudioEngine.h>
#include <CCheat.h>
#include <common.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <RenderWare.h>
#include <rw/rtpng.h>
#include "SearchRadiusPS.h"
#include "PoliceAuraPS.h"
#include "BackdropBlurPS.h"

#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>

namespace hud6 {

// Include order matters: these are all pulled into this one translation unit and
// share file-static state, so a unit may only be included after everything it
// references. Grouping is by feature, ordering is by dependency.
#include "core/RadarPatches.inc"
#include "core/RadarState.inc"
#include "core/RadarHeatCore.inc"
#include "ui/wanted/RadarWanted.inc"
#include "ui/cards/RadarInfoCards.inc"
#include "ui/primitives/RadarRenderPrimitives.inc"
#include "ui/radar/RadarLifecycle.inc"
#include "ui/radar/RadarGps.inc"
#include "ext/heat/RadarHeatTracking.inc"
#include "ui/wanted/RadarWantedSearch.inc"
#include "ui/cards/RadarInfoCardsUpdate.inc"
#include "ui/cards/RadarRenderFuelServices.inc"
#include "ui/cards/RadarRenderPumpIcons.inc"
#include "ui/radar/RadarProcess.inc"
#include "ui/radar/RadarRenderMap.inc"
#include "ui/wanted/RadarRenderWanted.inc"
#include "ui/gps/RadarRenderGps.inc"
#include "ui/radar/RadarRenderBase.inc"
#include "ui/wanted/RadarRenderPolice.inc"
#include "ui/radar/RadarRenderNorth.inc"
#include "ui/radar/RadarRenderPlayer.inc"
#include "ui/radar/RadarRenderPin.inc"
#include "ui/gps/RadarRenderDistance.inc"
#include "ui/cards/RadarRenderCards.inc"
#include "ui/topright/RadarRenderTopRight.inc"
#include "ui/radar/RadarRenderOverlay.inc"
#include "ui/radar/RadarRenderMenuMap.inc"
#include "ui/wanted/RadarRenderAuras.inc"
#include "ui/wanted/RadarRenderArcs.inc"
#include "ui/radar/RadarRenderLifecycle.inc"

extern "C" char GtaMcpDebugLog[4096];
extern "C" int  GtaMcpDebugLen;

namespace hud6 {
void Radar::WriteDebugLog() {
    int n = snprintf(GtaMcpDebugLog, sizeof(GtaMcpDebugLog),
        "wantedActive=%d copHasVisual=%d searchRadius=%.1f "
        "heatScale=%.2f heatOpa=%.2f "
        "enableWanted=%d enableAuras=%d auraIntensity=%.2f "
        "radarDead=%d radarAlpha=%.3f warmup=%d",
        (int)g_wantedActive, (int)g_copHasVisual, g_searchRadius,
        g_config.heatRadiusScale, g_config.heatRadiusOpacity,
        (int)g_config.enableWantedSearchRadius, (int)g_config.enableCopAuras, g_config.copAuraIntensity,
        (int)g_radarDead, g_radarAlphaFade, g_warmupFrames);
    GtaMcpDebugLen = n;
}
} // namespace hud6
