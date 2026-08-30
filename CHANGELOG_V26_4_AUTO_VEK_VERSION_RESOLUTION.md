# CustomVehicleGame v26.4 - Automatic VEK Version Resolution

- Uses `C:\vek\versions` as the preferred installed VEK version store.
- Ignores `C:\vek\updates` for runtime selection because it is staging-only.
- Supports `CURRENT_VERSION`.
- Scans installed semantic versions and selects the newest verified compatible runtime.
- Queries the official GitHub repo for the newest stable `vX.Y.Z` release.
- Automatically uses the newer stable release at build time if local VEK is behind.
- Keeps offline fallback to the newest compatible verified local VEK.
- Defaults `VEK_GITHUB_TAG.txt` to `latest`.
- Fully wires the development keypad Easter egg:
  `37485 + Enter` opens the dev cheat panel before the normal passlock is evaluated.
