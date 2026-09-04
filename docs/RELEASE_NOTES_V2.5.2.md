# VEK Programming Language v2.5.2

VEK 2.5.2 focuses on the Windows installation experience and background update behavior.

## Installer

- Borderless fullscreen presentation using the complete primary display.
- Responsive UI layout based on the actual client size instead of a fixed 1120-pixel drawing area.
- Smaller static 3D VEK logo.
- Removed continuous VEK logo spin.
- Removed verbose console-style version/source/status text from the visible installer.
- Three DOWNLOAD controls remain.
- AUTO and MANUAL update controls remain.
- The extruded 3D progress bar is now the primary download indicator.
- Git clone/fetch percentage updates feed the 3D progress bar.
- Bottom runner remains playable with Space.
- Escape or the top-right X closes the installer when no install operation is active.

## Background auto-update

AUTO mode now starts the update worker using the installer's `--background` mode instead of forcing the foreground installer workflow. Background mode creates no installer window and does not create a console window.

MANUAL mode never starts an update without a DOWNLOAD action.

## VEK runtime

`VekInteractiveUiSystems` now includes:

- `ComputeFullscreenInstallerLayout()`
- `ShouldBackgroundUpdate()`

These keep responsive installer geometry and auto-update decisions renderer-independent and reusable by other VEK hosts.
