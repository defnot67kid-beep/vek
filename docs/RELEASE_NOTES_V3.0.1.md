# VEK 3.0.1 — Installer and Version-System Hotfix

VEK 3.0.1 fixes a release-engineering bug in the 3.0.0 source package: `VERSION` and CMake identified 3.0.0 while the public runtime header and GitHub installer still embedded 2.8.0. As a result, a checkout could look like VEK 3.0.0 while a compiled/installed `vek.exe` still reported 2.8.0.

## What changed

- `VERSION` is now the single release-version source for CMake.
- CMake generates `vek/VekVersion.h`; `VekScriptEngine.h` no longer hard-codes a version.
- The GitHub installer installs a release package that must declare exactly the tag it is installing.
- The installer no longer records a newer `C:\vek\VERSION` while copying an older executable from itself.
- Source updates use the exact semantic-version tag instead of blindly resetting to `origin/main`.
- `vek doctor`/`vek info` can expose binary-vs-install metadata mismatches.
- Tag releases now have a workflow that builds and publishes the runtime and installer assets expected by the updater.

## Important for v3.0.0

A Git tag alone is not an installable binary release. The legacy updater expected `VekInstaller.zip` and its SHA-256 asset under the GitHub Release for that tag. VEK 3.0.1 makes that contract explicit and automates it for future tags.
