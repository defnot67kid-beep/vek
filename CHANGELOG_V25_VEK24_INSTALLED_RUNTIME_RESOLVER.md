# CustomVehicleGame v25 - VEK 2.4 Installed Runtime Resolver

The Windows build now checks `C:\vek` before downloading VEK.

A local installation is reused only when all of these are true:

- `C:\vek\vek.exe` exists.
- A complete VEK runtime checkout exists at `C:\vek\repo` or `C:\vek`.
- The runtime reports VEK 2.4.0 or newer.
- The checkout origin resolves to `https://github.com/defnot67kid-beep/vek`.
- Tracked source files are clean.
- The local Git HEAD exactly matches the current GitHub `main` HEAD.
- The runtime contains the VEK script, authority, and secondary-motion physics sources/headers.

If any check fails, CMake does not trust the installed runtime. It performs a
clean shallow FetchContent clone from the official VEK GitHub repository and
builds against that copy instead.

This avoids silently compiling CustomVehicleGame against an old, modified, or
wrong VEK installation while still avoiding an unnecessary clone when the
machine already has the exact current VEK runtime installed.
