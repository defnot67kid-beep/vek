# VEK 2.8.0 Validation

Validation environment: Linux x86_64, GCC 14.2, CMake source build with CLI, static runtime, shared C ABI and tests enabled.

Results:

- VEK runtime and CLI compiled successfully.
- Shared C ABI compiled successfully.
- `vek --version` reports `VEK 2.8.0`.
- 13/13 CTest suites passed.
- Dedicated GUI framework test covers retained hierarchy, row/column/fill layout, hit testing, click events, focus traversal, animation, themes, draw-list generation, snapshots and VEK-script `ui_*` natives.
- C ABI test verifies the C-hosted runtime exposes VEK GUI 2.8 natives to loaded scripts.
- Existing language, gameplay, editor, interaction GUI, authority/security, physics, interactive UI, diagnostics/debugger, VEK 2.7 language and runtime-platform suites remain green.

This validation does not claim a Windows GUI renderer implementation. `GuiFramework` is intentionally renderer-neutral; Windows/raylib/SDL/DirectX/Vulkan hosts consume its draw list and feed it input.
