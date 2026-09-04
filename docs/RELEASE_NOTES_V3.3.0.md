# VEK 3.3.0 — Docking, Colors, Shaders and UI Reliability

VEK 3.3.0 advances the VEK 3 UI platform without changing `.vek` language semantics or removing the existing widget enum/API.

## UI reliability
- Fixed retained UI clip ordering: draw commands are no longer globally re-sorted across `ClipPush`/`ClipPop` boundaries.
- Scroll containers clamp stale offsets after layout/content changes.
- Wheel scroll is bounded to the real content extent.
- Scrollbar pointer handling uses the actual thumb/track geometry and supports track jumps plus thumb dragging.
- Virtualized content can continue to supply `content_height` / `content_width`, so scrollbar range represents the whole logical data set.

## Advanced docking
- Added `VekUiDocking` / `DockManager` with open/close, active tab, pinning, floating panels, edge/center docking, split ratio, persistence snapshots and restore.
- Added workspace layout computation and five drop-zone calculations.
- Added script natives: `ui_dock_register`, `ui_dock`, `ui_dock_float`, `ui_dock_close`, `ui_dock_open`, `ui_dock_active`, `ui_dock_pin`, `ui_dock_resize`, `ui_dock_snapshot`, `ui_dock_restore`, `ui_dock_layout`, `ui_dock_drop_zones`.
- `dock_space` / `split_pane` draw lists now expose semantic drag handles and theme-aware drop-zone primitives.

## VEK colors
- Added renderer-neutral `VekColorSystems` helpers and a VEK palette (`vek-blue`, cyan, teal, green, lime, yellow, orange, red, pink, violet and grey 50–900).
- Script helpers: `color_hex`, `color_named`, `color_mix`, `color_alpha`, `color_lighten`, `color_darken`, `color_hsl`, `color_luminance`, `color_contrast`.

## Shader/material descriptors
- Added safe renderer-neutral shader descriptors and bounded `ShaderRegistry`.
- Built-in presets include frosted glass, vignette, outline/selection glow, wireframe, heat haze and color grading.
- Script helpers: `shader_preset`, `shader_effect`, `material`.
- VEK never gives scripts raw GPU handles; compilation/execution remains a host capability.
- `GuiVisualStyle` can carry `shader` and `shader_params`, and `ui_draw_list()` exposes them to the host renderer.

## Themes
- Modern dark/light themes now expose the VEK palette, docking tokens and semantic shader tokens such as `--shader-panel` and `--shader-viewport`.

## Compatibility
- Existing `GuiWidgetType`, `GuiFramework` methods and VEK scripting semantics remain source-compatible.
- GUI API reports `3.3`.
