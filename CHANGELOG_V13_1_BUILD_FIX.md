# v13.1 Windows Build Fix

This patch preserves the v13 VEK 1.3 hangar/editor/SDK feature set and fixes the MSVC/raylib 6 compile errors reported from the Windows development build.

## Fixes

- Updated three five-argument rounded rectangle outline calls to raylib 6's `DrawRectangleRoundedLinesEx(...)` API.
- Updated `VekJobRules` to use VEK 1.3's zero-argument `VekValue::AsString()` API while preserving default job-definition values when a string field is missing or has the wrong type.
- Kept the GitHub VEK dependency unchanged:
  - repository: `https://github.com/defnot67kid-beep/vek.git`
  - tag: `v1.3.0`

## Verification

All non-Windows-specific game `.cpp` translation units pass a C++20 syntax compilation against the VEK 1.3 headers and the raylib-compatible API stub after this patch. `WindowsPlatformHooks.cpp` remains platform-specific and is compiled by the normal Windows/MSVC build.
