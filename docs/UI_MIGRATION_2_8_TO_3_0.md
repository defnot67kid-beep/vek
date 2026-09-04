# Migrating from VEK 2.8 to VEK 3.0.0

## TL;DR

Nothing breaks. VEK 3.0.0 is additive: the VEK 2.8 `GuiFramework` and all
`gui_*`/`ui_*` script APIs are unchanged. You do not need to change any
existing `.vek` GUI script to build or run on 3.0.0.

## What's new to opt into

Four new C++ runtime modules are available under `<vek/VekUi*.h>`. They are
standalone (no dependency on `GuiFramework`), so you can adopt them
independently and incrementally:

| Module | Header | Use it for |
|---|---|---|
| Reactive state | `vek/VekUiReactive.h` | `Signal`/`Computed`/`Effect` — replace manual "poll and diff" state management with dependency-tracked updates. |
| Style cascade | `vek/VekUiStyle.h` | Parse `.vek` `style`/`theme` blocks, resolve class/id/pseudo-state cascades and theme tokens into a property map. |
| Commands | `vek/VekUiCommands.h` | Register one `CommandDescriptor` per action and invoke it from toolbar/menu/shortcut/palette call sites instead of duplicating logic. |
| Virtualization | `vek/VekUiVirtualization.h` | Compute the visible row/item range for large lists/trees/tables/grids before creating `GuiNode`s for them. |

None of these modules currently write into `GuiNode` for you — that
integration (e.g. "apply `StyleSheet::Resolve()` output onto a node's
`GuiVisualStyle`" or "only call `ui.CreateValue()` for the range returned
by `ComputeVirtualRange`") is left to calling code in this release. See
`docs/UI_NEXT.md` for exactly what is and isn't wired up yet.

## No API removals

No `gui_*`, `ui_*`, C ABI, or host-binding symbol was removed, renamed, or
changed in this release. There is no deprecation list for 3.0.0 because
nothing from 2.8 was deprecated.

## Versioning

`VERSION`, the CMake `project()` version, and `README.md` now read
`3.0.0`. If your build or packaging pins the exact VEK version string,
update that pin; no other change is required.
