# VEK 2.8.0 — GUI Framework

VEK 2.8.0 is a GUI-focused major release. It adds a backend-neutral retained-mode UI framework while keeping the existing immediate GUI command APIs and the 2.7 `gui_define` registry compatible.

Highlights:

- persistent UI tree with parent/child ownership and cycle-safe reparenting
- 50+ widget definitions covering runtime HUD, menus and editor/tool interfaces
- row, column, grid, overlay, absolute and dock-oriented layout primitives
- fixed/content/fill/percentage sizing, min/max constraints, margin, padding and gaps
- viewport DPI scale and safe-area support
- theme registry, class styles, per-state styles, typography, radius, border and opacity
- hover/pressed/focus/disabled visual states
- pointer, keyboard, text, scroll, drag/drop and submit event model
- bounded event queue for script polling
- hit testing and tab-order focus navigation
- accessibility roles, labels, hints and navigation order
- animation/tween engine with easing and bounded active-animation count
- renderer-neutral draw list and complete UI snapshot export
- built-in dark/light themes
- script-native `ui_*` API registered through `RuntimePlatformPack`
- renderer/input isolation: VEK scripts never receive raw GPU/window handles

The new framework API version is `2.8` and the VEK runtime version is `2.8.0`.
