# VEK Secondary-Motion Physics

VEK 2.3 adds an engine-independent deterministic spring-chain solver for hair,
cloth strips, ropes, cables, antennae and other secondary motion.

## C++ API

Use `vek::SpringChain3D` with `vek::SpringChainSettings`. The solver uses
bounded Verlet integration, fixed-length constraints, optional sphere collision,
substepping, damping, stiffness, inertia and external acceleration.

The solver deliberately uses `vek::PhysicsVec3` instead of a renderer-specific
vector type, so VEK stays embeddable in raylib, SDL, custom engines and servers.

## VEK script profiles

`SecondaryMotionProfileRegistry` exposes:

- `secondary_motion_profile_register(map)`
- `secondary_motion_profile_exists(id)`
- `secondary_motion_profile_count()`

Profiles clamp unsafe/non-finite values before storage.
