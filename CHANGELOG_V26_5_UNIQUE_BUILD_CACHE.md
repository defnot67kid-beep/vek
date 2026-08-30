# CustomVehicleGame v26.5 - Unique Windows Build Cache

Fixes Windows sharing/locking errors from reusing one fixed CMake/MSBuild directory.

Previous behavior:
- Every development build reused `%LOCALAPPDATA%\VEK\builds\CVG-v26-dev`.
- The script tried to delete that directory before configuring.
- CMake, Git, raylib, MSBuild, Visual Studio, antivirus scanning, or another process
  could still hold generated files open.
- Windows then returned sharing errors such as:
  `The process cannot access the file because it is being used by another process.`

New behavior:
- Every build receives a unique short directory.
- Development example:
  `%LOCALAPPDATA%\VEK\builds\CVG-v26-dev-1234-5678-9012`
- Release example:
  `%LOCALAPPDATA%\VEK\builds\CVG-v26-release-1234-5678-9012`
- No previous build directory has to be deleted before a new build can start.
- Local output folders are no longer deleted before copying either.
- Added `CLEAN_OLD_VEK_BUILD_CACHE.bat` for optional manual cleanup.
- Build banner now correctly says the resolver falls back to the latest stable
  GitHub VEK release rather than GitHub `main`.
