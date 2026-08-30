# V5 - Humanoid, Torso Rotation, Run/Jump Animation

## New files
- `src/HumanoidSystem.h`
- `src/HumanoidSystem.cpp`
- `tests/HumanoidSystemTests.cpp`
- `HUMANOID_SYSTEM.md`
- `ANIMATION_UPGRADE.md`
- `CHANGELOG_V5.md`

## Modified files
- `src/Character.h`
- `src/Character.cpp`
- `src/Game.cpp`
- `CMakeLists.txt`
- `README_SETUP.txt`
- `CHARACTER_SYSTEM.md`

## Torso / body rendering
- Torso now uses true yawed rendering (`DrawCubePro`).
- Pelvis now rotates independently with a small locomotion counter-twist.
- Clothing overlays, backpack, feet and carried cargo follow their appropriate body layer.
- Chest can combine body facing, camera-look offset and running twist.
- Head remains independently constrained by the existing head-look system.

## Running
- Speed-scaled cadence.
- Run/sprint chest twist and pelvis counter-rotation.
- Shoulder motion.
- Bent-elbow run/sprint arm drive.
- Foot lift during swing phase.
- Vertical body bob.
- Turn-aware stride shortening remains integrated.

## Jumping
- `HumanoidSystem` owns grounded / vertical velocity / gravity.
- Jump pose reacts to actual vertical velocity.
- Separate rise, apex, fall and landing behavior.
- Knee tuck near apex.
- Landing compression scales with landing speed.

## Humanoid foundation
- Health/max health.
- Stamina.
- Damage/heal API.
- Alive/hurt state.
- Jump/grounded state.
- Airborne time and landing impact speed.
- Stable humanoid bone identifiers for future hitboxes, IK and body-part damage.

## Verification performed
- `HumanoidSystem tests: PASS`
- `Character + Camera rotation tests: PASS`
- Full C++ source syntax check: PASS using a local raylib API stub for header/type validation.

The normal Windows game build still uses the real raylib 6.0 fetched by CMake via `BUILD_WINDOWS.bat`.
