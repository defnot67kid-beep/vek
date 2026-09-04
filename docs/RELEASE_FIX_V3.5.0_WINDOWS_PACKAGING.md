# VEK 3.5.0 Windows release packaging fix

This source tree contains a release-system correction for VEK 3.5.0.

## Fixed

- Added the missing `UPDATE_POLICY` file (`manual` by default).
- Added `.github/workflows/release-windows.yml` so a `v3.5.0` tag builds the actual MSVC Windows x64 runtime on `windows-latest`.
- Added `tools/validate_release.py`; release creation now fails if the package is missing the compiled runtime library, generated version header, CMake package files, or required EXEs.
- Added installed CMake package support (`VEKConfig.cmake`, `VEKConfigVersion.cmake`).
- Exported the C ABI as `VEK::CABI` in addition to `VEK::Runtime`.
- `BUILD_PORTABLE_WINDOWS.bat` now validates the staged runtime before and after compression.
- Portable packages now include `UPDATE_POLICY`, `release-version.json`, and `REPOSITORY.txt`.
- The Windows workflow verifies that the Git tag exactly matches `VERSION`.

## Important

A GitHub `VEK-v3.5.0-windows-x64.zip` must be produced by `BUILD_PORTABLE_WINDOWS.bat` on a real Windows/MSVC runner. A source tree plus the Go bootstrap launcher is **not** a production VEK runtime package.
