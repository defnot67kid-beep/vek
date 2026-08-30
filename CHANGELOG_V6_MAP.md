# v6 - Advanced Minimap + World Map

Built directly on the v5.1 humanoid/torso/run-jump build-fix project.

## Added

- `MapSystem.h/.cpp`
- persistent rounded-square minimap
- shared minimap/full-map data model
- PlayerUp and NorthUp orientations
- smooth movement/vehicle-dependent minimap zoom
- full-map M-key expand/shrink transition
- map pan and cursor-centered zoom
- selectable location side panel
- custom waypoint + world beacon
- objective edge indicator
- direct GPS route rendered on both map modes
- player vs parked-vehicle markers
- discovery regions/locations
- filters
- compass
- resolution-aware layout
- F9 map debug
- `MapSaveData` persisted by existing `Save` system
- road/water/air route-mode architecture
- road vehicle/boat/aircraft actor-mode architecture
- automated map math/state/transition tests

## Preserved

The existing humanoid, character customization, torso animation, character/camera rotation, equipment, clothing, vehicle builder, vehicles, job system, economy, blueprint saving, workshop, Build Mode and all camera modes remain in the project.

`World::DrawMap()` is retained as legacy source functionality even though the normal game now uses `MapSystem`.
