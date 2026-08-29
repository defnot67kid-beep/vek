# VEK Programming Language v2.1.0 — Portable VEK

VEK 2.1.0 turns VEK into a relocatable Windows language distribution.

## User installation

The Windows release asset is built as `VEK-v2.1.0-windows-x64.zip`.

A normal user can:

1. extract the ZIP;
2. rename the folder to `vek` if desired;
3. move it to `C:\vek`, `D:\Languages\vek`, or another location;
4. run `INSTALL_PATH.cmd` once;
5. open a new Command Prompt and run `vek --version`.

No Git clone, CMake or Visual Studio is needed to *use* the portable release.

## New CLI commands

- `vek home` — prints the detected installation root
- `vek info` — prints version/platform/executable/home information
- `vek doctor` — checks runtime files, version and PATH state
- `vek verify` — checks package files against `manifest.sha256`

## Relocation behavior

The runtime does not contain a fixed `C:\vek` path. It discovers its installation from the running executable and can therefore be moved or renamed. `VEK_HOME` remains optional and is only a fallback when it points to a valid VEK tree.

## Release automation

A tag such as `v2.1.0` triggers `.github/workflows/release-portable.yml`. The workflow:

- builds Windows x64 in Release mode;
- uses the static MSVC runtime option for a more self-contained CLI;
- runs the complete test suite;
- assembles the portable folder;
- creates `manifest.sha256`;
- runs `vek doctor` and `vek verify` on the staged release;
- creates the ZIP and ZIP SHA-256 file;
- uploads both as GitHub Release assets.

## Test reliability fix

The Release test suite now keeps assertions enabled in the tests themselves. This fixes a pre-existing problem where side-effectful test expressions inside `assert(...)` could disappear under `NDEBUG`, causing the interaction test to crash instead of actually testing the code.
