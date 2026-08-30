# Validation - CustomVehicleGame v26.8.1 / VEK 2.6.1

Validated source-policy/regression checks:
- Door/workshop entry state machine
- Personnel-door live collision refresh
- Scripted traversal collision bypass and release
- Incremental smart-build policy
- Arm-rig orientation
- Free-mouse input policy
- Hair/scalp-fit policy
- Signed `.vek` scripts remain unchanged from v26.8

The graphical executable was not rebuilt in this Linux sandbox because this project fetches Raylib 6.0 during CMake configuration and the sandbox does not provide that dependency locally.
