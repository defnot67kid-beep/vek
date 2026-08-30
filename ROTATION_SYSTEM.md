# Advanced Player Character Rotation System

> **v8 input update:** gameplay mouse-look is disabled. Camera targets are changed with physical Left/Right Arrow alignment. The cursor remains free; RMB is only used by context-specific UI such as the full map.


This is an in-place upgrade of the existing C++20 + raylib character build. Existing avatar customization, equipment, procedural animation, vehicles, jobs, build mode, camera modes, career saving and blueprint saving remain present.

## Architecture

### `CharacterRotationSystem`
Dedicated root-facing controller. It owns:

- current body yaw
- target body yaw
- angular velocity
- camera yaw snapshot
- normalized movement direction/magnitude
- head yaw offset
- upper-body yaw offset
- rotation mode
- turn-in-place state
- interaction-facing override
- seated vehicle yaw

The camera never writes body yaw directly.

### `CameraRotationSystem`
Dedicated smooth camera-facing controller. It owns:

- current camera yaw/pitch
- target camera yaw/pitch
- yaw/pitch angular velocity
- configurable mouse sensitivity
- angular acceleration/deceleration
- shortest-angle yaw wrapping
- pitch limits
- optional yaw clamps for cockpit look

Mouse input updates target angles first. The visible camera then accelerates toward
those targets, so camera orbit no longer snaps directly to raw mouse deltas. Separate
instances are used for normal on-foot view, free inspection view and cockpit view.

### `Game`
Supplies:

- independent smooth camera yaw / pitch
- ground-projected camera forward/right movement basis
- input magnitude
- desired movement direction
- current movement state
- vehicle seat/entry context

Translation and facing are separated. `playerVelocity` handles movement smoothing, while `CharacterRotationSystem` handles orientation.

### `CharacterAnimationSystem`
Consumes rotation data:

- `movementSpeed`
- `movementDirection`
- `turnAngle`
- `rotationAngularVelocity`
- `isTurning`
- `isMoving`
- `isSprinting`
- `isCrouching`
- `isCarrying`
- `isRepairing`
- `isSeated`
- `turnState`
- head and upper-body offsets

Sharp turns reduce stride slightly. The inside leg gets a shorter stride than the outside leg. Foot-yaw offsets are maintained as a future IK hook.

## Rotation modes

`CharacterMovementRotationMode` supports:

- `OrientToMovement` — normal open-world locomotion
- `FaceCameraDirection` — strafe/aim-style architecture for future tools/aiming
- `LockedDirection` — smooth interaction/repair/vehicle-entry facing

## Default tunable values

`CharacterRotationSettings` currently defaults to:

| Setting | Value |
|---|---:|
| Walk rotation | 240 deg/s |
| Run rotation | 360 deg/s |
| Sprint rotation | 450 deg/s |
| Crouch rotation | 180 deg/s |
| Medium carry | 170 deg/s |
| Heavy carry | 120 deg/s |
| Hurt movement | 140 deg/s |
| Interaction facing | 210 deg/s |
| Turn in place | 220 deg/s |
| First-person body catch-up | 250 deg/s |
| Angular acceleration | 900 deg/s² |
| Angular deceleration | 1200 deg/s² |
| Movement deadzone | 0.08 |
| Third-person turn-in-place threshold | 70 deg |
| First-person body threshold | 55 deg |
| Strong turn threshold | 120 deg |
| Head yaw limit | ±65 deg |
| Upper-body yaw limit | ±25 deg |
| Head look smoothing | 8.0 |
| Upper-body smoothing | 6.0 |

All are public values in `CharacterRotationSettings` and can be tuned without rewriting the algorithm.

## Shortest-angle math

The system provides:

- `NormalizeAngle()`
- `DeltaAngle()`
- `MoveTowardsAngle()`
- `YawFromDirection()`

Example: 350° → 10° produces a +20° turn, not -340°. 359° → 0° produces +1°.

## Angular acceleration

Body turning does not instantly jump to its max state speed. Angular velocity accelerates toward a braking-limited desired speed. Near the target, the desired speed drops using a braking-distance calculation, preventing overshoot and oscillation.

## Camera-relative movement

On-foot input is generated from horizontal camera basis vectors:

- camera forward Y is ignored
- camera right Y is ignored
- both are normalized
- W/S use camera forward/back
- A/D use camera left/right

Camera yaw and body yaw remain independent.

The previous build's left/right convention was inconsistent with the raylib world
basis. A/D input has been inverted where required so visible movement/steering is
consistent across the on-foot controller, vehicle steering and free-camera strafing.

## Turn in place and first person

Stationary third-person camera orbit does not continuously drag the avatar. Body catch-up begins only after the camera/body yaw difference exceeds the configured threshold.

In first person, head/upper-body look is allowed first. Once the view exceeds the 55° body threshold, the root begins turning smoothly toward the view.

## Head / upper-body look

Head yaw is clamped to ±65°. Upper-body yaw is clamped to ±25°. Both use frame-rate-independent exponential smoothing and are applied as local procedural pose offsets, not separate world-space root rotations.

## Sprint and carrying weight

Sprint turn differences above 90° reduce movement speed. Differences above 150° temporarily reduce the requested sprint speed toward run speed while the character completes the turn.

Heavy cargo uses slower angular speed, slower translation acceleration, lower movement speed and additional sharp-turn speed reduction.

## Repair / inspect facing

`FaceTarget(characterPosition, targetPosition)` creates a locked smooth-facing request. Repair and inspection use the nearby vehicle position. The facing override is released when the interaction pose completes.

## Vehicle entry / seating

Entry is staged:

1. move toward generic driver entry point
2. smoothly face the approach direction
3. smoothly align to vehicle heading
4. switch to driving state
5. seated root follows vehicle heading directly
6. head remains independently viewable

Normal walking rotation is disabled while seated.

## Build / free camera / creator

- Build camera does not copy its yaw into the avatar. A part placement may request smooth facing toward the placement point.
- Free inspection camera freezes character root facing and cannot rotate the avatar.
- Avatar creator uses its own `avatarPreviewYaw`. `SHIFT + LEFT/RIGHT` rotates preview 360° without changing gameplay yaw.

## Debug mode

Press `F8`.

HUD shows:

- Body Yaw
- Body Target Yaw
- Camera/View Yaw
- Angle Difference
- Angular Velocity
- Movement Direction
- Rotation Mode
- Turn State
- Camera Current Yaw/Pitch
- Camera Target Yaw/Pitch
- Camera Yaw/Pitch Angular Velocity

3D lines:

- GREEN = character forward
- YELLOW = desired movement direction
- BLUE = camera forward

## Automated verification

`tests/CharacterRotationTests.cpp` checks both body and camera rotation:

- -1° normalization to 359°
- 359° → 0° shortest angle
- 350° → 10° shortest angle
- deadzone stability
- non-snapping first frame turn
- heavy-carry rotation slower than run
- stationary turn-in-place threshold
- first-person body threshold
- interaction target locking
- seated heading ownership
- free-camera isolation
- 30 / 60 / 120 / 144 FPS body turn consistency
- non-snapping camera target/current separation
- shortest camera yaw across 359°/0°
- cockpit yaw and pitch clamping
- 30 / 60 / 120 / 144 FPS camera consistency

## Manual gameplay test checklist

1. Hold W — avatar smoothly faces camera-forward movement.
2. W then D — root turns right smoothly rather than snapping.
3. Orbit camera around a stationary avatar — no continuous body copying; turn-in-place only after threshold.
4. Run a WASD circle — root follows the arc smoothly.
5. Sprint then reverse — speed reduces during the sharp turn and root has momentum.
6. Crouch and change direction — rotation is visibly slower.
7. Toggle heavy carry with H — rotation is slower than normal locomotion.
8. Enter vehicle with E — approach and alignment occur before seating.
9. Drive — seated root follows vehicle heading.
10. First-person — view/head can move before full-body catch-up.
11. Free camera — avatar root orientation stays unchanged.
12. Cross 359°/0° — F8 debug should show the short correction, not a full spin.
13. Test at 30/60/120/144+ FPS — response should remain approximately equivalent.

## Files changed in this upgrade

Modified:

- `CMakeLists.txt`
- `README_SETUP.txt`
- `CHARACTER_SYSTEM.md`
- `src/Character.h`
- `src/Character.cpp`
- `src/Game.h`
- `src/Game.cpp`
- `src/Vehicle.h`
- `src/Vehicle.cpp`
- `src/VehicleBuilder.h`

Created in the previous body-rotation upgrade:

- `src/CharacterRotationSystem.h`
- `src/CharacterRotationSystem.cpp`
- `tests/CharacterRotationTests.cpp`
- `ROTATION_SYSTEM.md`

Created in this camera-rotation upgrade:

- `src/CameraRotationSystem.h`
- `src/CameraRotationSystem.cpp`
