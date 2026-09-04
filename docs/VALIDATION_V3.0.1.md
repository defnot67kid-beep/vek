# VEK 3.0.1 Validation

Validated on 2026-09-04 in the build sandbox.

- `tools/version_manager.py check`: PASS (`3.0.1` synchronized).
- CMake configure: PASS with `project(VEK VERSION 3.0.1)` derived from `VERSION`.
- Full native runtime/CLI/C ABI build: PASS.
- CTest: **15/15 PASS**.
- `vek --version`: `VEK 3.0.1`.
- Installed generated `include/vek/VekVersion.h`: major 3, minor 0, patch 1.
- `vek doctor` on staged install: runtime and disk `VERSION` match.
- Windows GitHub installer cross-build (`GOOS=windows GOARCH=amd64`): PASS.
- Windows installer PE subsystem: **Windows GUI**.
- Portable launcher Windows cross-build: PASS.
- No stale hard-coded `2.8.0` release version remains in executable version declarations.

The Windows release workflow itself must run on GitHub's `windows-latest` runner after the source is pushed/tagged because this Linux validation environment does not contain MSVC/Windows SDK and therefore cannot produce the final C++ Windows runtime package locally.
