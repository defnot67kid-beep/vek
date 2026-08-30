# Game v18 / VEK 1.8

- Added signed `scripts/camera_world.vek`.
- VEK now defines camera distance/FOV/pitch/sensitivity/smoothing/alignment and camera cycle order.
- Hold RMB to look around in gameplay, vehicle cameras, free inspection and the engineering editor; the mouse is never captured/warped.
- Added VEK-defined procedural skybox/environment colors, sun and fog metadata rendered natively by raylib.
- Added VEK-defined global world policy for FPS, frame-delta safety and player movement/interaction speeds.
- Expanded humanoid rig to pelvis, lower/upper spine, chest, neck/head, clavicles, elbow/wrist joints, knee/ankle and toe sections.
- VEK humanoid rig joint weights/flexibility now influence ragdoll pose and damping/recovery behavior.
- F11 DEV reload now reloads camera/sky/rig/world policy too.
- VEK Guard signs the new camera/world script in secure releases.
