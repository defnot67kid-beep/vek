# VEK Physics

VEK 2.7.0 contains two deliberately separate physics layers.

## Secondary motion (runtime solver)

`vek::SpringChain3D` is the deterministic Verlet-style solver used for hair, cloth strips, ropes, cables and antennae. It supports bounded substeps, damping, stiffness, inertia, wind/external acceleration and optional sphere collision.

## Advanced Physics Definitions v0.3 (descriptor API)

Physics v0.3 includes engine-independent C++ descriptors for materials, colliders, rigid bodies, joints/motors/limits, solver/world settings, character controllers, vehicles/wheels, soft bodies, cloth, ropes, aerodynamics, buoyancy, breakables, force fields and query filters.

This release **does not silently install or activate a rigid-body solver**. Games can map these definitions into Jolt, PhysX, Bullet or a custom server/client backend. The Custom Vehicle Game demo intentionally keeps its existing gameplay physics.

Script hosts can expose a bounded `PhysicsDefinitionRegistry` with:

- `physics_version()`
- `physics_definition_register(category, map)`
- `physics_definition_exists(category, id)`
- `physics_definition_count(category)`

The older `physics_v02_*` names are retained for compatibility.

Definitions require a safe `id`, a known category, a serializable map no larger than 64 KiB, and each category is capped at 4096 definitions.

The existing secondary-motion profile natives remain available: `secondary_motion_profile_register`, `secondary_motion_profile_exists`, and `secondary_motion_profile_count`.


### v0.3 additions

VEK 2.7 adds contact overrides, CCD, articulations, ragdolls, IK chains, particles, fluid volumes, destruction clusters, tire/suspension models, scene-query policy, physics LOD and physics-event definitions. The generic `physics_*` native names are preferred for new code; the `physics_v02_*` names remain as compatibility aliases.
