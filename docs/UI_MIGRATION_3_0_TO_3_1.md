# Migrating VEK UI 3.0 styles to 3.1

Existing `.vek` widget declarations continue to work. No widget enum or existing `GuiFramework` method was removed.

## What changes visually

Modern styling is enabled by default. If a project relied on the old generic VEK 3.0 appearance, call `ui_modern_style(false)` or `GuiFramework::UseModernStyle(false)` temporarily.

`vek.dark` and `vek.light` still work as compatibility aliases. New projects should prefer `vek.modern.dark` and `vek.modern.light`.

## Stylesheet authors

You do not need to rewrite existing class/id selectors. VEK 3.1 additionally supports real widget-type selectors such as `button`, `viewport`, `tree_item`, and `button.primary:hover`.

Use tokens rather than literal colors when possible:

```text
style ".danger" {
  background: var(--danger);
  border_radius: var(--radius-md);
  transition_duration: var(--motion-fast);
}
```

New visual properties include `shadow_color`, `shadow_blur`, `shadow_x`, `shadow_y`, `focus_ring_color`, `focus_ring_width`, `icon_size`, `scale`, `translate_x`, `translate_y`, `transition_duration`, `easing`, `backdrop_blur`, `track_color`, and `thumb_color`.

## Render backends

Backends that only understand the original Box/Text/Image/Clip commands will continue compiling, but they should add support for Line, Circle, Shadow, Gradient, Icon, and semantic `Custom` parts to display the full 3.1 appearance.
