#pragma once

#include <CVector.h>
#include <CRect.h>

namespace hud6 {

class Radar {
public:
    static void Init();
    static void Shutdown();
    static void Process();
    static void Render();
    static void Restart();
    static void DrawMenuMapOverlay();
    static void WriteDebugLog();

    static void SetPin(float x, float y, float z);
    static void ClearPin();

    static void CreateShaders();
    static void DestroyShaders();
    static void OnD3dLost();
    static void OnD3dReset();
    static void SyncModulePatches();

private:
    static void UpdatePlayerData();
    static void UpdateCamera();
    static void UpdateGps();
    static void UpdateWantedSearch();
    static void UpdateInfoCards();
    static void UpdateFuelServices();
    static void DrawFuelServices();
    static void DrawPumpIcons(const CRect& rect, float alpha, bool drawAll);
    static void DrawPumpIconsOnMenuMap();
    static void DrawOverlay();

    static void DrawBackdrop(const CRect& rect, float alpha);
    static void DrawMap2D(const CRect& rect, float alpha);
    static void Draw3dObjects(const CRect& rect, float alpha);
    static void DrawWantedSearchRadius(const CRect& rect, float alpha);
    static void DrawPoliceTint(const CRect& rect, float alpha);
    static void DrawGpsPath(const CRect& rect, float alpha);
    static void DrawBorder(const CRect& rect, float alpha);
    static void DrawBlips(const CRect& rect, float alpha);
    static void DrawPoliceBlips(const CRect& rect, float alpha);
    static void DrawPinMarker(const CRect& rect, float alpha);
    static void DrawNorthBadge(const CRect& rect, float alpha);
    static void DrawPlayerPuck(const CRect& rect, float alpha);
    static void DrawGpsDistance(const CRect& rect, float alpha);
    static void DrawInfoCards(const CRect& rect, float alpha);
    static void DrawPoliceThreatAuras(float alpha);
    static void DrawPoliceThreatArcs(float alpha);

    static bool ProjectToUV(const CVector& worldPos, float& outU, float& outV);
    static void ProjectToRim(const CRect& rect, float dirX, float dirY, float& outX, float& outY);
    static bool IsInside(float x, float y, const CRect& rect, float radius);
    static float Radius();
};

} // namespace hud6
