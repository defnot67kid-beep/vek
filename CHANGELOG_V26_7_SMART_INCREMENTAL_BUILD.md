# CustomVehicleGame v26.7 - Smart Incremental Builds Across ZIP Downloads

## Problem
Previous v26.5/v26.6 Windows scripts intentionally created a brand-new CMake/MSBuild
folder for every run to avoid stale-folder deletion/locking problems. That was safe,
but it also threw away every compiled object and made each newly downloaded/unzipped
version rebuild from the beginning.

CMake cannot directly reuse one build directory when the source folder path changes
from one downloaded ZIP name to another, because its cache records the original
source directory.

## New design
`BUILD_WINDOWS.bat` and `BUILD_WINDOWS_DEV.bat` now keep a stable canonical source
mirror and stable configuration-specific build caches under:

`%LOCALAPPDATA%\VEK\incremental\CustomVehicleGame`

The source ZIP can live anywhere and its folder name can change every update.

### Build flow
1. `build_support/Sync-IncrementalSource.ps1` hashes the newly unzipped source files.
2. Files with identical content are left untouched in the canonical mirror.
3. New/changed files are copied and touched so MSBuild cannot miss a change because
   an archive preserved an older timestamp.
4. Deleted/renamed source files are removed from the mirror.
5. CMake configures against the stable mirror path.
6. MSBuild's normal dependency graph recompiles only invalidated `.cpp` files and
   anything that depends on changed headers/configuration.
7. Development and release builds have separate persistent build caches because
   `VEK_DEVELOPMENT_MODE` changes compilation.

## VEK scripts and assets
A `.vek` script or runtime asset change should not compile unrelated C++ files.
The game now has an always-run lightweight runtime sync target so scripts/assets are
refreshed beside the executable even when the executable itself is already up to date.

## Expected behavior
- First build: full build.
- Download/unzip a new version with one changed `.cpp`: primarily that translation
  unit (plus affected dependents) rebuilds, then the game relinks.
- Change a shared header: dependent C++ files correctly rebuild.
- Change only `.vek`: no unnecessary C++ compile; runtime files still update.
- Raylib/FetchContent data remains in the persistent build tree instead of being
  downloaded/rebuilt from scratch every run.
- Dev and release caches never contaminate one another.

## Recovery
`RESET_INCREMENTAL_BUILD_CACHE.bat` deletes only the generated smart cache. Use it
when the compiler/toolchain changes incompatibly or a cache is genuinely damaged.
The next build will be full once, then incremental behavior resumes.
