# VEK Physics

VEK 2.6.1 contains two deliberately separate physics layers.

## Secondary motion (runtime solver)

`vek::SpringChain3D` is the deterministic Verlet-style solver used for hair, cloth strips, ropes, cables and antennae. It supports bounded substeps, damping, stiffness, inertia, wind/external acceleration and optional sphere collision.

## Advanced Physics Definitions v0.2 (descriptor API)

Physics v0.2 adds engine-independent C++ descriptors for materials, colliders, rigid bodies, joints/motors/limits, solver/world settings, character controllers, vehicles/wheels, soft bodies, cloth, ropes, aerodynamics, buoyancy, breakables, force fields and query filters.

This release **does not silently install or activate a rigid-body solver**. Games can map these definitions into Jolt, PhysX, Bullet or a custom server/client backend. The Custom Vehicle Game demo intentionally keeps its existing gameplay physics.

Script hosts can expose a bounded `PhysicsDefinitionRegistry` with:

- `physics_v02_version()`
- `physics_v02_definition_register(category, map)`
- `physics_v02_definition_exists(category, id)`
- `physics_v02_definition_count(category)`

Definitions require a safe `id`, a known category, a serializable map no larger than 64 KiB, and each category is capped at 4096 definitions.

The existing secondary-motion profile natives remain available: `secondary_motion_profile_register`, `secondary_motion_profile_exists`, and `secondary_motion_profile_count`.
