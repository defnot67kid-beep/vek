# v26.8.3 - Authoritative Smart Cache / Stale Script Fix

The v26.7+ incremental compiler cache was correctly deleting removed files from
its canonical source mirror, but two later runtime-copy steps were merge-only:
CMake `copy_directory` and the Windows `xcopy` publish. That meant a renamed or
deleted `.vek` file could remain in an older runtime output directory.

This was especially dangerous for `scripts/parts`, because the vehicle part
registry intentionally enumerates every `.vek` file in that directory. A stale
part script could therefore keep running even though it no longer existed in
the newly downloaded source ZIP.

v26.8.3 changes the policy:

- Persistent **compiled objects/libraries remain incremental**.
- Persistent canonical source remains SHA-256 synchronized and removes files
  missing from the current ZIP.
- Temporary/editor leftovers (`*.tmp`, `*.bak`, `*.old`, `*.orig`, `*.rej`,
  editor `~` files) are excluded from the canonical source mirror.
- `scripts/` and `assets/` beside the cached executable are deleted and copied
  fresh on every build. They are small runtime data trees, so this does not
  trigger a C++ recompile.
- Local `build/Release` and `build-dev/Release` are published through an
  ownership manifest instead of raw `xcopy` accumulation.
- The first v26.8.3 publish forcibly purges old `scripts/` and `assets/` trees,
  cleaning stale files left by v26.7-v26.8.2 automatically.
- `saves/` and `crashlogs.txt` are explicitly preserved by the publisher.
- Removed build-owned files are deleted on later publishes using
  `.vek-build-output-manifest.txt`.

Result: the build cache reuses expensive compiled work, while runtime scripts
and assets always represent the **current source ZIP only**.
