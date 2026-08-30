# v24 - VEK 2.3 Hair Physics

- Replaced procedural global hair sway with VEK `SpringChain3D` physics.
- Every visible hair strand now has a constrained multi-particle backbone.
- Added VEK secondary-motion gravity, damping, stiffness, air drag, inertia and substepping.
- Hair responds to running acceleration, turning, jumping/landing and gait motion.
- Added head-sphere collision so simulated strands are projected outside the skull.
- Rebuilt scalp anchors mathematically on the head surface instead of placing roots inside the head.
- Removed the oversized embedded hair-cap sphere; crown coverage now uses small outside follicles.
- Hair styles use different physical material profiles (short, long, curly, straight, braids, ponytail, afro).
- Game now requires VEK 2.3.0+ and the `VekPhysicsSystems` runtime module.
