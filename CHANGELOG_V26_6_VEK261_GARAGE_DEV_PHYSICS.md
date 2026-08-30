# Custom Vehicle Game v26.6 / VEK 2.6.1

- Added five new hair styles: Wavy, Locs, Bun, Bob and Coils, for 12 total.
- Added a visible animated personnel-door handle and E/mouse door interaction.
- Added an inside-wall main-garage OPEN/CLOSE control.
- Fixed the garage open latch so a manual close is not immediately overridden.
- Garage collision now clears once the opening is safely walkable (host caps the script threshold at 0.36).
- Expanded the workshop with cabinets, diagnostics, compressor tanks and tyre storage.
- Expanded the development-only cheat panel: fly, noclip, god mode, teleport, 3x movement, garage toggle, HUD and unlock.
- Every collision/damage bypass remains wrapped in VEK_DEVELOPMENT_MODE; secure release builds compile those paths out.
- Requires VEK 2.6.1, which adds authority hardening and Physics Definitions v0.2.
- Advanced Physics v0.2 is intentionally not enabled by this demo game.
