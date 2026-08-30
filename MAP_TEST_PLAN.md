# Map verification checklist

Automated `MapSystemTests` verify the deterministic math/state layer. The following gameplay checklist is intended for the Windows raylib build.

1. Start game: rounded-square minimap appears upper-right below career/vitals.
2. Walk: player stays centered and local roads/markers move correctly.
3. Rotate human: PlayerUp world rotates while arrow stays up; NorthUp rotates arrow instead.
4. Enter vehicle: source switches to vehicle position and vehicle heading.
5. Drive faster: minimap zoom transitions outward smoothly.
6. Press M: minimap expands into full map rather than popping to an unrelated screen.
7. Press M again: full map shrinks back with GPS/waypoint/pan/zoom intact.
8. Pan/zoom full map: human/vehicle does not move underneath the map.
9. Accept job with J: cyan GPS route and active objective marker appear.
10. Move far enough from objective: edge indicator remains visible with distance.
11. Exit vehicle: player marker is source and parked vehicle remains visible.
12. Switch third/close/first-person cameras: map heading remains body/vehicle based.
13. Enter free inspection camera: free-camera yaw/position does not change gameplay map source or heading.
14. Open Character Creator: minimap is hidden and its state is preserved.
15. Resize window: minimap remains anchored and scales without the 1280x720 assumption.
16. Compare 30/60/120/144 FPS: transition and zoom smoothing remain approximately identical.
17. Right-click full map: waypoint appears on full map, minimap and world beacon.
18. RMB drag: map pans and does not accidentally set a waypoint after a real drag.
19. F/G: focus player and GPS destination.
20. F9: debug values and optional full-map region boundaries appear.
