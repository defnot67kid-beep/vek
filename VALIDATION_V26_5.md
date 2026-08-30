# Validation - v26.5

Build-script invariants:
- Source paths remain fully quoted.
- Paths containing spaces and `(1)` remain supported.
- Short MSBuild path remains under `%LOCALAPPDATA%\VEK\builds`.
- A unique build directory is selected for every invocation.
- Existing/locked old build directories are not deleted or reused.
- Both development and release scripts use the same safe strategy.
- `C:\vek\versions` remains the preferred VEK runtime store.
- `C:\vek\updates` remains staging-only.
