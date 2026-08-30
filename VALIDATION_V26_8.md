# Validation - CustomVehicleGame v26.8 / VEK 2.6.1

## Passed source regression checks
- `DoorWorkshopEntryTests.py`
  - staged personnel-door sequence exists
  - enlarged handle and enlarged click bounds exist
  - handle rotation is linked to the grip target
  - damped arm spring and two-bone IK are present
  - the hand tracks the moving door grip
  - walk-through ends in automatic Build Mode
  - leaving the hangar ends the auto-build session
- `ArmRigOrientationTests.py`
- `FreeMousePolicyTests.py`
- `HairScalpFitTests.py`
- `IncrementalBuildPolicyTests.py`

## Secure VEK scripts
All files under `scripts/` were SHA-256 compared against the original v26.7 package. No script or `.sig` file changed.

## CMake
Offline CMake configuration passed using a local Raylib interface stub and a verified local VEK 2.6.1 test runtime. The build graph generates correctly.

## Structural checks
A comment/string-aware delimiter scan passed for the modified C++ headers and sources (`Game`, `Character`, and `World`).

## Environment limitation
The graphical executable was not linked in this Linux sandbox because a complete Raylib 6.0 source checkout is not installed locally. The Windows smart-build scripts remain unchanged and will reuse the user's v26.7+ incremental cache.
