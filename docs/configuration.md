# Configuration Guide (`6hud.ini`)

`6hud.ini` is located in the root game directory alongside `6hud.asi`. All values can be tuned in real-time with hot-reloading.

## Configuration Keys

```ini
[radar]
; Master toggle for the 6hud radar overlay (1 = enabled, 0 = disabled)
enabled=1

; Units for waypoint distance label: "m" for metric (m / km) or "ft" for imperial (ft / mi)
unit=m

; Radar frame size multiplier percentage (100 = default 100%)
radar_size=100

; Radar bezel corner radius percentage (0 = sharp rectangle, 6.5 = modern subtle curve default, 18 = circular)
radar_corner_radius=6.5

; Bezel outline border thickness in screen pixels (0 = no border)
border_thickness=1.0

; Bezel outline border color: R, G, B, Alpha (0..255)
border_color=205, 205, 205, 170

; Smooth show/hide fade transition speed (0.01 = slow, 0.03 = default smooth, 0.10 = fast)
fade_speed=0.03


; --- Camera Perspective & Elevation ---
; Lock player Z elevation to flat 2D plane in projection (1 = locked flat, 0 = elements follow 3D elevation)
lock_player_elevation=1

; Base camera zoom distance in meters
radar_zoom=50.0

; Maximum dynamic camera zoom distance at high speed
radar_max_zoom=70.0

; Vehicle speed threshold in km/h before dynamic zoom begins (matches SpeedFX motion blur threshold)
speed_zoom_threshold=70.0

; Camera elevation angle in degrees (90.0 = top-down, 35.0 = 2.5D perspective)
radar_tilt=35.0

; Smooth camera interpolation speed
transition_speed=0.025

; Tilt player navigation puck in 2.5D perspective with camera pitch
tilt_player_blip=1


; --- Visibility Triggers ---
; Render radar when player is on foot (1 = on, 0 = hidden on foot by default)
show_on_foot=0

; Render radar when driving vehicle/car/bike/heli/boat (1 = on, 0 = off)
show_in_vehicle=1

; Render radar inside shops, casinos, and interior rooms (1 = on, 0 = off)
show_in_interior=1

; Render radar inside tunnels & underground roads (1 = on, 0 = off)
show_in_tunnel=1

; Force render radar when wanted level > 0 (1 = on, 0 = off)
show_when_wanted=1

; Force render radar when destination waypoint route is active (1 = on, 0 = off)
show_when_gps_active=1


; --- Stacked Info Cards (Location & Vehicle) ---
; Enable GTA 6 stacked location and vehicle info cards on top of radar (1 = enabled, 0 = disabled)
show_info_cards=1

; Keyboard hotkey to show/refresh info cards on demand (e.g. J, TAB, M)
info_hotkey=J

; Info cards backdrop color: R, G, B, Alpha (0..255)
card_backdrop_color=14, 18, 24, 230

; Info cards background opacity percentage (0..100)
card_backdrop_opacity=90

; Info cards slide and fade animation transition speed (0.02 = slow, 0.08 = smooth default, 0.20 = fast)
card_transition_speed=0.08

; Location card display duration in seconds on entering a new area outdoors
location_duration=3.5

; Vehicle name banner display duration in seconds on entering a vehicle
vehicle_name_duration=4.0

; Vehicle name text size scale percentage (100 = default 100%)
vehicle_name_scale=100

; Vehicle class subtitle text size scale percentage (100 = default 100%)
vehicle_class_scale=100

; Vehicle name to class vertical spacing percentage (100 = default 100%)
vehicle_text_gap=100

; Circular vector gauges size scale percentage (100 = default 100%)
gauge_scale=100

; Circular vector gauges horizontal spacing percentage (100 = default 100%)
gauge_spacing=100

; Info cards bezel border outline thickness in pixels
card_border_thickness=1.0

; Info cards bezel border outline color: R, G, B, Alpha (0..255)
card_border_color=205, 205, 205, 120

; Info cards text foreground color: R, G, B, Alpha (0..255)
card_text_color=255, 255, 255, 255


; --- Overview Peek Mode ---
; Keyboard key to hold down for overview zoom-out
peek_key=G

; Camera zoom-out multiplier while holding the peek key (keeps same 2.5D tilt angle)
peek_zoom_mult=1.5

; Dynamic radar zoom-in when driving inside tunnels or underground
enable_tunnel_zoom=1

; Radar zoom multiplier inside tunnels (0.65 = 35% closer zoom)
tunnel_zoom_mult=0.65


; --- GPS Navigation Road Ribbon ---
; Core GPS road ribbon fill color: R, G, B, Alpha
gps_color=232, 74, 213, 240

; Outer lighter casing stroke color: R, G, B, Alpha
gps_casing_color=248, 140, 238, 200

; Radar core road ribbon width in pixels (fixed screen space)
gps_line_width=14.0

; Radar outer casing stroke width in pixels
gps_casing_width=2.0

; ESC Menu map core road ribbon width in pixels
map_gps_line_width=12.0

; ESC Menu map outer casing stroke width in pixels
map_gps_casing_width=2.0

; Radial mask inner radius in meters (0% opacity around player puck)
gps_mask_inner=10.0

; Radial mask outer radius in meters (100% opacity road fade)
gps_mask_outer=50.0

; Distance in meters to auto-clear destination waypoint on arrival (0 to disable)
waypoint_reach_radius=10.0


; --- Backdrop Styling ---
; Dark acrylic radar backdrop color (R, G, B)
backdrop_color=14, 18, 24

; Radar background opacity percentage (0 = fully transparent, 90 = acrylic glass default, 100 = solid)
backdrop_opacity=90

; Fill empty/unstreamed radar map bounds with sea water color (1 = on, 0 = off)
fill_empty_sea=1


; --- Police Detection & Wanted Search Area ---
; Show police officers on foot on radar during wanted level
show_police_foot=1

; Show police vehicles on radar during wanted level
show_police_vehicle=1

; Enable GTA V / VI style police visual search radius on wanted level
enable_wanted_search_radius=1

; Time in seconds player must stay outside police visual search radius to clear wanted level
wanted_escape_time=8.0

; Outer red heat search area radius scale multiplier percentage (150 = 150% of search radius)
heat_radius_scale=1.50

; Outer red heat search area opacity percentage (0 = off, 14 = 14% subtle red aura)
heat_radius_opacity=14

; Enable GTA 6 screen edge police vehicle emergency siren beacon auras (1 = on, 0 = off)
enable_cop_auras=1

; Maximum world distance in meters to detect police vehicle threat auras
cop_aura_max_dist=150.0

; Police vehicle emergency siren aura brightness percentage (100 = default 100%)
cop_aura_intensity=100

; Enable GTA 6 on-foot police curved directional threat chevrons (1 = on, 0 = off)
enable_cop_arcs=1

; Maximum world distance in meters to detect on-foot police threat arcs
cop_arc_max_dist=70.0

; On-foot police threat arc and chevron size scale percentage (100 = default 100%)
cop_arc_scale=100
```
