# VEK 3.1 Modern UI Framework

VEK 3.1 keeps the existing retained widget tree and scripting API, but replaces the old generic visual path with a modern token-driven styling and draw-command system suitable for game-engine editors and polished desktop apps.

## Theme cascade

The built-in themes are `vek.modern.dark` and `vek.modern.light`.

Tokens cascade in three layers:

1. **Base design tokens** — `--bg-*`, `--surface*`, `--text-*`, `--border*`, `--accent`, state colors, spacing, radii, typography, elevation, motion.
2. **Component tokens** — `--button-bg`, `--button-primary-bg`, `--input-bg`, `--popup-bg`, `--scrollbar-thumb`, `--viewport-bg`, etc. Most component tokens reference base tokens with `var()`.
3. **Widget/style overrides** — normal CSS-like rules such as `button.primary:hover`, `.my-class`, or `#specific-id` override component defaults.

Example:

```text
theme "my.dark" {
  --accent: #a879ff;
  --button-primary-bg: var(--accent);
  --radius-md: 12;
}
style "button.primary" { background: var(--button-primary-bg); }
style ".vehicle-card:hover" { border: var(--accent); }
```

`var(--missing, fallback)` is supported, and token references resolve recursively with a bounded recursion depth.

## Important tokens

### Color
`--bg-0..3`, `--surface`, `--surface-raised`, `--surface-hover`, `--surface-active`, `--border-subtle`, `--border`, `--border-strong`, `--text-1..3`, `--text-disabled`, `--accent`, `--accent-hover`, `--accent-active`, `--accent-soft`, `--success`, `--warning`, `--danger`, `--info`, `--focus`, `--selection`, `--shadow`, `--overlay`.

### Spacing / sizing
`--space-1`, `--space-2`, `--space-3`, `--space-4`, `--space-5`, `--space-6`, `--space-8`, `--control-h-sm`, `--control-h`, `--control-h-lg`, `--icon-sm`, `--icon`, `--icon-lg`.

### Radius / typography
`--radius-xs`, `--radius-sm`, `--radius-md`, `--radius-lg`, `--radius-xl`, `--radius-pill`, `--font-xs`, `--font-sm`, `--font-md`, `--font-lg`, `--font-xl`, `--font-xxl`, `--line-tight`, `--line-normal`, `--line-relaxed`.

### Motion / elevation
`--motion-fast`, `--motion-normal`, `--motion-slow`, `--ease-standard`, `--elevation-1-blur`, `--elevation-2-blur`, `--elevation-3-blur`, `--focus-ring`.

### Scrolling
`--scrollbar-size`, `--scrollbar-hover-size`, `--scrollbar-min-thumb`, `--scrollbar-track`, `--scrollbar-thumb`, `--scrollbar-thumb-hover`.

## Standard widgets

VEK 3.1 emits semantic sub-part draw commands for buttons, icon buttons, toggles, checkboxes, radios, sliders/range sliders, progress/spinner, all text fields, combo/dropdown/search, tabs, list/tree/table/property grid, menus/toolbars/status, popup/tooltip/context menu/toast/badges, color picker/keybind, scroll containers, dock/split panes, viewport, graph and timeline.

The host renderer should render `GuiDrawCommand::part` appropriately. Existing Box/Text/Image/Clip commands are preserved. New renderer-neutral primitives are appended to `GuiDrawType`: Line, Circle, Shadow, Gradient and Icon. `Custom` commands carry semantic data for complex components.

## Viewport widget

The `viewport` node keeps the embedded 3D pipeline untouched. VEK emits:

- `viewport.frame`
- `viewport.backdrop`
- `viewport.content` (host places the live 3D render here)
- `viewport.loading` / `viewport.empty`
- `viewport.toolbar`
- `viewport.toolbar.reset-view`
- `viewport.toolbar.wireframe`
- `viewport.toolbar.zoom`
- `viewport.corner-gizmo`

Set `data:{toolbar:true,state:"loading"}` or `state:"empty"`. Toolbar clicks emit a `select` event with `text` equal to `reset_view`, `toggle_wireframe`, or `zoom`.

## Scrollbars and virtualization

`VekUiVirtualization` now exposes `ComputeScrollbarMetrics`, `ComputeScrollShadowMetrics`, and `ComputeAdaptiveOverscan`. A virtualized 50,000-row list still materializes only the visible/overscan range, while scrollbar thumb size and scroll shadows use the full `totalContentSize` rather than live node count.

Virtual ranges now expose `clampedScrollOffset`, `beforeExtent` and `afterExtent`.

Scrollable nodes support `data.scrollbar_mode = "overlay"` (default) or `"classic"`. Classic mode reserves scrollbar space; overlay mode draws above content. The retained input system supports wheel scrolling and scrollbar thumb dragging.

## Interaction

- Mouse focus does not show a keyboard focus ring.
- Tab/Shift+Tab focus sets `:focus-visible`.
- Hover, press, focus, checked/selected, expanded and scrollbar visibility are smoothed in `GuiFramework::Step` using theme-driven durations.
- Dragging starts after a small movement threshold instead of immediately on pointer-down.
- Sliders update continuously while dragged.
- Timeline drag emits normalized `change` values so the host can move the active keyframe.

## Renderer backend note

No GPU/window/native handle is exposed. Shadows, gradients, blur requests, icons, viewport content and graph/timeline data are renderer-neutral requests. A host without blur/shader support may use a simpler fallback while preserving layout and interaction.
