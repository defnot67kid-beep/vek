# CustomVehicleGame v26.1 - Windows MSBuild Path Fix

The Visual Studio/MSBuild generator could fail during CMake's compiler test with
`FileTracker : error FTK1011` when the game was extracted into a deeply nested
Downloads directory.

## Fix

`BUILD_WINDOWS.bat` and `BUILD_WINDOWS_DEV.bat` now configure and compile in a
short external build directory:

- `%LOCALAPPDATA%\VEK\builds\CVG-v26-release`
- `%LOCALAPPDATA%\VEK\builds\CVG-v26-dev`

After a successful build, the complete Release output is copied back into the
project's normal local output directory:

- `build\Release`
- `build-dev\Release`

This avoids long CMake scratch/MSBuild `.tlog` paths without requiring registry
changes, administrator privileges, or global Windows long-path configuration.
