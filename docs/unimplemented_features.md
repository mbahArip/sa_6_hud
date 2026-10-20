# 6hud: Unimplemented Features & Next-Gen Roadmap

This document outlines all planned, audited, and pending features required to bring `6hud` to full GTA 6 visual and functional parity.

---

## 1. 3D Buildings & Mesh Pipeline

### 1.1 Custom DFF Replacement Proxies (`6hud/models/`)
* **Concept:** Load hand-cleaned replacement models for buildings from `GTA San Andreas/6hud/models/<model_name>.dff`.
* **Mechanism:**
  * Intercept model retrieval in `ProcessEntity`.
  * If `<model_name>.dff` exists in the `6hud/models/` directory, load geometry via RenderWare `RpClumpStreamRead()` into RAM cache.
  * Transform clean vertices using the live in-game entity matrix `entity->m_matrix`.
  * Eliminates all terrain bleeding, attached sidewalk slabs, billboards, and fences without editing game `.img` archives or consuming IDE IDs.
  * Fall back to live pool extraction if no custom DFF is provided.

### 1.2 Spatial Indexing & Performance Acceleration
* **Current:** Iterates all active slots in `CPools::ms_pBuildingPool` and `CPools::ms_pDummyPool` every frame.
* **Target:** 2D spatial grid (hash grid or quadtree) partitioned across San Andreas world bounds. Query only the $3\times3$ spatial cells overlapping the radar's active draw radius, reducing vertex transformation overhead from thousands of entities to only visible candidates.

### 1.3 Day/Night Dynamic Palette Tinting
* **Concept:** Adapt building wall and rooftop base colors to current game time (`CClock::ms_nGameClockHours`) and weather conditions (`CWeather::WeatherTypeInList`).
* **Visuals:** Warm amber/golden illumination during sunset, cool desaturated slate-blue at night with window illumination highlights, crisp neutral silver under midday sun.

### 1.4 Screen-Space Ambient Base Occlusion
* **Concept:** Soft ambient contact shadow gradient where building footprints meet the ground plane.

---

## 2. Frame, Glass Backdrop & Bezel

### 2.1 Outer Drop Shadow & Vignette
* **Concept:** Multi-layered blurred black/smoked drop shadow quad rendered behind the frosted glass backdrop to lift the radar off the game screen.
* **Backdrop Vignette:** Subtle radial darkening towards the outer bezel edges to simulate modern curved glass refraction.

### 2.2 Micro Silver Bezel
* **Concept:** Replace the single 1.0px flat stroke with a dual-pass composite:
  * Outer: 1.0px crisp semi-transparent silver/white rim (`RGBA(225, 230, 240, 180)`).
  * Inner: 1.5px subtle dark inset glow (`RGBA(0, 0, 0, 120)`).

### 2.3 Dynamic Aspect Ratio Framing
* **Concept:** Smoothly expand radar aspect ratio (from 16:10 rectangle to wide 21:9 letterbox frame) when flying aircraft (planes/helicopters) or driving at high speed to grant wider forward navigation vision.

---

## 3. Typography & Street/District Display

### 3.1 Modern Sans-Serif HUD Font Engine
* **Current:** Relies on default GTA SA Old English / Gothic font (`CFont::SetFontStyle(FONT_PRICEDOWN)`).
* **Target:** Custom high-DPI raster or Direct3D 9 texture font engine (DIN 1451 / Helvetica Neue / Chalet style) for:
  * Street name and neighborhood labels below radar frame.
  * Route remaining distance badge (`1.2 KM`, `450 M`).
  * Cardinal compass badges (`N`, `S`, `E`, `W`).

### 3.2 GPS Next-Turn Banner
* **Concept:** Modern turn-by-turn notification banner (e.g. `↱ Turn Right on Vinewood Blvd in 200m`) sliding into view above or below the radar frame when following an active waypoint.

---

## 4. Player Indicator & Navigation Blips

### 4.1 Modern GTA 6 Chevron Puck
* **Concept:** Sharp geometric chevron arrow with:
  * Crisp white interior fill.
  * Subtle outer glow / contact shadow underneath.
  * Smooth rotation physics dampening.

### 4.2 Ambient Field-of-View (FOV) Vision Cone
* **Concept:** Soft, semi-transparent flashlight-style cone projecting forward from the player chevron (45° arc, 15–20% opacity white-to-transparent gradient) indicating CJ's look/facing direction.

### 4.3 Vector Blip Suite & Altitude Indicators
* **Concept:** Modern flat vector icon set for shops, safehouses, mission markers, and destinations.
* **Altitude Chevrons:** Display small up/down triangular arrows on blip markers when the target is physically $>10\text{m}$ above or below CJ's current elevation.

---

## 5. GPS Navigation & Visuals

### 5.1 Multi-Pass Glowing Neon Route Ribbon
* **Concept:** Enhance the GPS road spline with a multi-pass bloom effect:
  * Pass 1: Wide blurred translucent neon glow underlay (`RGBA(232, 74, 213, 80)`).
  * Pass 2: Solid core high-contrast ribbon (`RGBA(255, 120, 240, 240)`).
  * Pass 3: Crisp directional animated chevron pulses traversing along the path towards the destination.

### 5.2 Dynamic Look-Ahead Center ($V_{\text{center}}$)
* **Concept:** Dynamically shift the radar vertical anchor from $0.50$ (center) down to $0.65 - 0.70$ (lower third) proportionally with vehicle forward speed, giving 50%+ more forward visibility when driving fast.

---

## 6. Integrated Vitals HUD (Health / Armor / Stamina)

### 6.1 Sub-Radar Micro-Bars
* **Concept:** Anchor sleek, minimalist 3px micro-bars directly beneath the radar bezel:
  * **Health Bar:** Vivid Green (`RGBA(40, 200, 80, 240)`), flashing red on low health.
  * **Armor Bar:** Deep Sky Blue (`RGBA(40, 140, 245, 240)`).
  * **Stamina / Special Bar:** Pure Yellow / White (`RGBA(255, 210, 40, 240)`).

### 6.2 Vanilla HUD Suppression Toggle
* **Concept:** Option to cleanly suppress GTA SA's classic 2004 top-right health, armor, weapon, and breath icons (`CHud::Draw()` patch) to achieve a complete standalone next-gen minimalist HUD.

---

## 7. Wanted & Police Threat System (Implemented)

* **7.1 Non-Penetrating Line-of-Sight Raycasting:** Multi-entity collision checks (`CWorld::GetIsLineOfSightClear`) for cops on foot (with 160° vision cone / 8m sound awareness) and chasing cop vehicles/helicopters.
* **7.2 Dynamic Search Center Evasion:** Real-time search radius following player during active visual contact; locks to last known position upon losing line-of-sight with an escape countdown timer.
* **7.3 Strobe Search Area & Feathered Heat Perimeter:** Flashing red/blue strobe fill and outer feathered red heat ring scaling up to 150%+ with wanted intensity.
* **7.4 Screen-Edge Vehicle Siren Auras:** Directional emergency vehicle beacon glows cast across screen boundaries with tangent offset red/blue sirens and distance attenuation.
* **7.5 Directional Threat Chevrons:** Screen-space curved threat arcs and pointed chevrons tracking nearby on-foot officers.

