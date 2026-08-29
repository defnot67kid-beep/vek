# VEK Programming Language v2.3.0

VEK 2.3.0 advances the runtime with deterministic secondary-motion physics for
hair, cloth strips, ropes, cables and other lightweight articulated systems.

## Highlights

- New `VekPhysicsSystems` runtime module.
- New `vek::SpringChain3D` constrained Verlet solver.
- Gravity, damping, stiffness, air drag, inertia and external-force controls.
- Bounded substepping and constraint iterations for stable frame-rate independent motion.
- Optional sphere collision for character heads and other rounded colliders.
- `SecondaryMotionProfileRegistry` for validated VEK-configurable physics materials.
- VEK script natives for registering and querying secondary-motion profiles.
- New `hair_physics.vek` example.
- New physics regression suite; full runtime suite is 8/8 passing.
- Stable `VEK::Runtime` CMake export remains available to external games.
