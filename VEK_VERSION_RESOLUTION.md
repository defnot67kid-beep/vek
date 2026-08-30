# VEK Version Resolution - CustomVehicleGame v26.4

Use `C:\vek\versions` for installed VEK runtimes.

Recommended layout:

```text
C:\vek\
  CURRENT_VERSION
  repo\
  versions\
    v2.6.0\
      repo\
    v2.7.0\
      repo\
  updates\
    staging\
```

`versions` is the trusted version store. `updates` is only a temporary
download/staging area and is never used directly by the game build.

At configure time CustomVehicleGame:
1. Reads `CURRENT_VERSION` when present.
2. Scans all installed version directories.
3. Checks the legacy/current `C:\vek\repo` checkout.
4. Rejects incomplete, too-old, dirty, or wrong-origin runtimes.
5. Checks the official GitHub repo for the newest stable `vX.Y.Z` tag.
6. Uses the newest verified local runtime when it is current.
7. Automatically fetches the newer stable release when the installed runtime is behind.
8. Falls back to the newest verified local runtime if GitHub cannot be reached.

Because VEK is linked into the game executable, this update selection happens
at build/configure time. The running game does not hot-swap its C++ runtime.
