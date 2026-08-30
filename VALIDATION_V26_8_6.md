# v26.8.6 / VEK 2.7.1 validation

- PASS: VEK 2.7.1 runtime build and 12/12 CTest suites.
- PASS: VEK editor-system presentation-schema test (`part_presentation`, icon/view/world model/material/tint/view transform).
- PASS: CustomVehicle part registry loads all current VEK part scripts: 28 parts.
- PASS: Every current part has a distinct explicit presentation mapping and all built-in view-model IDs are handled by the CustomVehicle renderer policy test.
- PASS: Seat, fuel tank, battery and petrol engine resolve to different dedicated view models/icons.
- PASS: secure VEK script hash/signature manifest regenerated for 23 scripts.
- PASS: VEK Guard manifest/security regression test.
- PASS: PartPresentationRenderer C++ syntax check against a Raylib API stub.
- PASS: existing Python regressions: arm rig, stale-runtime mirror, Build Mode HUD, 3D collision height, crash logs, door workshop entry/arm IK, free mouse, hair, incremental build.
- PASS: VEKBP3 blueprint format persists presentation metadata while VEKBP2 load compatibility remains implemented.

The final graphical Windows executable was not launched in this Linux sandbox; the project still downloads/uses Raylib 6.0 on the user's Windows build machine. The source release includes an offline-compatible bundled VEK 2.7.1 fallback.
