# Advanced Minimap + World Map System (v6)

This project now uses a dedicated `MapSystem` rather than drawing the full map directly from `Game.cpp`.
The old `World::DrawMap()` function is intentionally left in the project as a legacy fallback so existing world functionality has not been removed, but normal gameplay now uses the shared minimap/world-map architecture.

## Shared architecture

`MapSystem` owns one cached set of world-map data used by both display modes:

- `MapRoad`
- `MapArea`
- `MapBuilding`
- `MapRegion`
- `MapMarker`
- `MapRoute`
- `MapFilterSettings`
- `MapSaveData`

Display mode is only presentation state:

```cpp
enum class MapDisplayMode
{
    Minimap,
    Fullscreen
};
```

Pressing `M` therefore does not create another map or discard any state. It expands the same map data and preserves GPS, custom waypoint, filters, discovery state, minimap orientation, map pan and zoom.

## Minimap

The minimap is a resolution-aware rounded-square HUD element anchored at the upper-right, below the existing money/XP/health panel.

It uses cached 2D map geometry instead of re-rendering the 3D scene.

It displays the current prototype's:

- roads and test road
- workshop pad and workshop structures
- industrial warehouse and delivery yard
- mud, sand and test/ramp areas
- discovered regions
- player/current vehicle
- parked vehicle when the player is on foot
- active mission objective
- custom waypoint
- GPS route
- discovered/unknown markers
- N/E/S/W compass headings

Content is clipped with raylib scissor mode inside an inset minimap viewport so map geometry cannot spill into the surrounding HUD.

### Orientation modes

`PlayerUp` is the default. The player/vehicle heading remains at the top and the world rotates around it.

`NorthUp` keeps north at the top and rotates the player arrow instead.

The minimap never uses free-camera yaw as gameplay heading. Heading source is:

```text
Driving -> vehicle heading
On foot -> CharacterRotationSystem body yaw
```

This remains correct in first person, cockpit and free inspection cameras.

Press `O` while the full map is open to switch minimap orientation. The setting is saved.

### Dynamic minimap zoom

The system smoothly changes zoom based on gameplay context:

- walking: close
- running: normal
- sprinting: slightly wider
- slow road vehicle: medium-wide
- fast road vehicle: wider
- boat: wide architecture preset
- aircraft: very wide architecture preset

Zoom is exponential/delta-time based rather than a frame-dependent instant jump.

## GPS and objective markers

The existing `Jobs` state remains the source of the active delivery objective.
`Game::SyncMapState()` forwards the current job target into `MapSystem`.

Current prototype routing is intentionally direct-point routing, matching the previous GPS architecture. `MapRouteMode` already supports:

- `Direct`
- `Road`
- `Water`
- `Air`

so a future road graph, harbour route or air checkpoint network can replace the route generator without rewriting minimap rendering.

If the objective leaves minimap range, an edge indicator remains visible and shows distance rather than simply disappearing.

## Custom waypoint

Full map:

- right-click an empty map position to set a waypoint
- right-drag to pan instead of placing a waypoint
- select a marker and click `SET WAYPOINT`
- `Delete`/`Backspace` or the `REMOVE WAYPOINT` button clears it

The waypoint appears on:

- full map
- minimap
- world as a small beacon
- GPS route whenever no mission objective has higher priority

## Full world map

`M` animates the minimap into the full world map in about 0.24 seconds.
The transition interpolates:

- position
- size
- background opacity
- map center
- map scale/zoom
- player-up rotation toward north-up full-map presentation

Controls:

```text
M                 close/minimize
WASD / arrows     pan
Mouse wheel       zoom toward cursor
RMB drag          pan
RMB click         set waypoint
LMB               select marker
F                 focus player/current vehicle
G                 focus GPS objective/waypoint
O                 toggle PlayerUp/NorthUp minimap setting
1                 jobs filter
2                 services filter
3                 vehicles filter
4                 events filter
Delete/Backspace  remove custom waypoint
F9                map debug
```

While the full map or its transition is active, gameplay input is blocked before `UpdatePlayer()`, `Vehicle::UpdateDriving()` and free-camera movement. Map WASD cannot move the human or vehicle underneath the UI.

## Selectable markers

A selected discovered marker can expose:

- name
- region
- distance
- services
- available job count
- example reward
- difficulty
- description
- set waypoint action

The current prototype includes cached markers for the starter workshop, job board, industrial cargo warehouse, delivery company yard and engineering test grounds.

## Discovery and regions

Current playable map regions:

- Starter Town
- Industrial District
- Vehicle Test Grounds

The enum also prepares the requested future world regions:

- Main City
- Countryside
- Mountains
- Airport
- Harbour
- Desert
- Forest
- Islands

Undiscovered regions are darkened. Discoverable locations reveal themselves when the player comes within their discovery radius.

## Filters

`MapFilterSettings` currently supports:

- jobs
- services
- vehicles
- events

Active objectives and the custom waypoint remain visible even if a normal category filter is disabled.

## Future vehicle route support

`MapActorMode` supports:

```cpp
OnFoot
RoadVehicle
Boat
Aircraft
```

`MapAreaType` includes water, and marker types already include airport and harbour. The current prototype world simply does not contain a real water/airport region yet, so the map does not invent one.

## Performance

Static map geometry and markers are built once in `MapSystem::Initialize()` and vectors reserve their expected capacity.

The draw/update loop does not rebuild the world-map database each frame. Dynamic state is limited to:

- player location/heading
- current/parked vehicle
- active GPS target
- waypoint
- discovery checks
- smooth zoom/pan/transition values

This leaves room for later distance-based marker updates and NPC/event pooling.

## Save integration

Map state is saved through the existing `Save` class to:

```text
saves/map_state.txt
```

Saved data includes:

- discovered markers
- discovered regions
- fast-travel-capable discovered markers
- custom waypoint
- minimap orientation
- map filters
- full-map pan
- full-map zoom
- tracked marker ID

It is saved on shutdown and when the player closes the full map.

## Debug

Press `F9` to show:

- player world coordinates
- normalized map coordinates
- gameplay heading
- minimap zoom
- world map zoom
- visible marker count
- GPS target
- waypoint state

When the full map is open, F9 also outlines region boundaries.
