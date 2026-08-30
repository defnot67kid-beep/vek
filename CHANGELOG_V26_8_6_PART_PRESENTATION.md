# v26.8.6 - VEK Part Icons + Viewmodels

## VEK 2.7.1 integration
- CustomVehicle now requires VEK 2.7.1 and ships a bundled 2.7.1 fallback for offline source builds.
- VEK `PartDefinition` now supports renderer-neutral `presentation` metadata: icon, editor view model, world model, material, view scale/rotation/offset, and tint policy.
- Added the `part_presentation(icon, view_model, world_model, options)` VEK helper while keeping legacy `visual` fully compatible.

## CustomVehicle renderer
- Added a dedicated part-presentation renderer shared by Build Mode and finalized vehicles.
- Every current VEK part has a distinct presentation ID. Seats, fuel tanks, batteries, engines, motors, suspension, steering, winches, lights, cargo, wheels, tracks, aircraft, marine and experimental parts no longer collapse to the same generic box.
- The part catalog now uses VEK-defined vector icons instead of the old category-colored square.
- Build-mode ghost/placed parts use `view_model`; constructed vehicles use `world_model`.
- Blueprint format VEKBP3 persists icon/model/material/view-transform presentation data while VEKBP2 remains loadable.

## Security/build
- Part and editor VEK scripts were intentionally changed and therefore require a new signing identity/compiled script hash manifest for this release.
- Smart incremental build and authoritative runtime mirroring remain enabled.
