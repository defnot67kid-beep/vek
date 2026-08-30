# v6.1 Camera Input Update

This patch modifies the existing v6 project only.

## Changes

- Camera mouse-look now requires **holding Right Mouse Button**.
- Releasing RMB stops new mouse input without snapping or resetting camera yaw/pitch.
- Added `<` / comma camera alignment key: next 45-degree bracket left.
- Added `>` / period camera alignment key: next 45-degree bracket right.
- Alignment uses the existing smooth CameraRotationSystem target/current architecture.
- Third-person driving camera now supports RMB orbit around the moving vehicle while preserving a relative orbit offset.
- Driving `<` / `>` alignment resolves to 45-degree **world-space** camera brackets.
- Free inspection camera follows the same RMB + alignment-key behavior.
- First-person/cockpit look follows the same RMB + alignment-key behavior within its existing yaw clamp.
- Full World Map keeps its existing RMB drag/click behavior because gameplay camera input is blocked while the map owns input.

## Files modified

- `src/Game.cpp`
- `src/Game.h`
- `src/CameraRotationSystem.h`
- `src/CameraRotationSystem.cpp`
- `tests/CharacterRotationTests.cpp`
- `README_SETUP.txt`
- `CAMERA_ROTATION_SYSTEM.md`

## New file

- `CHANGELOG_V6_1_CAMERA_INPUT.md`
