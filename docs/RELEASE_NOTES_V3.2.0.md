# VEK 3.2.0 — Camera + Physics Expansion

VEK 3.2.0 expands the backend-neutral camera and physics contracts while keeping existing VEK 3.1 UI and scripting APIs compatible.

## Camera Framework 1.0

Adds camera lens, rig, shake, constraint, shot, and sequence definitions plus CPU-side blend helpers. Hosts still own matrices, ray casts, GPU rendering, and collision queries.

Native registry API:

- `camera_version()` → `1.0`
- `camera_definition_register(category, definition)`
- `camera_definition_exists(category, id)`
- `camera_definition_count(category)`

Supported categories: `lens`, `rig`, `shake`, `constraint`, `shot`, `sequence`.

## Physics Definitions v0.4

Adds scene settings, advanced materials, mass properties, contact solver settings, constraint graphs, sensors, drivetrain/differential definitions, aero surfaces, wheel-contact models, debug draw, snapshots, and network-sync descriptors.

The generic `physics_*` API now reports `0.4`. v0.3 and v0.2 compatibility aliases remain available.

## Lightweight Vehicle Dynamics

`VehicleDynamicsModel::Step` provides a deterministic backend-neutral vehicle dynamics helper with longitudinal force, braking, drag, rolling resistance, tire-limited traction, steering response, yaw dynamics, lateral damping, body roll/pitch, suspension compression, and engine RPM estimates. Engines may use it directly or translate VEK definitions into a more advanced solver.
