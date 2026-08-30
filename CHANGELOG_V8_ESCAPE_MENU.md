# v8 — No Console + Escape Menu + Free Mouse

## Windows game window
- `CustomVehicleGame.exe` is built as a Windows GUI executable (`WIN32_EXECUTABLE`).
- `main.cpp` now supplies a Windows `WinMain` entry point while keeping a normal `main()` on non-Windows platforms.
- Launching the compiled EXE no longer creates a separate black console window.
- `RUN_GAME.bat` now starts the GUI executable detached.
- `RUN_GAME_NO_CONSOLE.vbs` is included for a launch path with no command window.

## Escape Menu
`EscapeMenuSystem.h/.cpp` adds a modular modal menu.

Press **Esc** to open:
- **RESUME** — continue from the exact paused state.
- **RESET** — return the player and finalized vehicle to the workshop, restore humanoid health/stamina, and return to third person. Career/job/map/save progress is preserved.
- **LEAVE** — exit to desktop cleanly through the normal game shutdown/save path.

Raylib's default ESC-to-close behavior is disabled with `SetExitKey(0)`, so Esc belongs to the game UI.

## Mouse behavior
- Gameplay mouse-look has been removed.
- Right mouse no longer rotates any gameplay camera.
- The cursor stays enabled, visible and free during gameplay.
- Build Mode, World Map and UI continue using mouse input normally.
- Physical Left/Right Arrow keys remain the camera yaw-alignment controls.
- Full map and Character Creator retain their existing context-specific arrow controls.
