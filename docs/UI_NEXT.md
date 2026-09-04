# VEK UI Next (VEK 3.0.0)

## What this release actually is

VEK 2.8 shipped a solid retained-mode GUI: a node tree, layout, themes,
input, events, animation, accessibility metadata and draw lists. The
31-point brief for this release asks VEK to grow into something with the
architecture of React/Flutter/SwiftUI/Qt/UMG-class UI systems: declarative
components, a real style cascade, advanced layout (flex/grid/responsive),
professional text, vector icons, dozens of high-quality widgets,
virtualization, docking, a modern animation system, rich input (gamepad,
touch, IME), a command architecture, data binding, undo/redo integration,
deeper accessibility, a measurable performance/profiler story, a
GPU-neutral draw-list expansion, effects, world-space UI, a visual
designer, hot reload, diagnostics, security hardening, dev tools, and
compatibility/versioning/tests/docs to match.

That is a multi-quarter engineering program for a real UI framework team.
This release does **not** claim to have built all of it. What it does is
lay down four new, independently-designed, independently-tested runtime
modules that are the actual hard architectural core of a declarative/
virtualized/style-cascaded UI system — the parts that are easy to fake with
enum values and hard to fake with working code — and it is explicit below
about everything from the brief that is *not* yet built, so nothing here is
overstated.

## What is implemented in VEK 3.0.0 (real, tested code)

All four modules live under `include/vek/` + `src/`, are compiled into
`vek_runtime` (see `CMakeLists.txt`), and are covered by
`tests/ui_next_tests.cpp` (18 assertions across reactive state, style
cascade, commands, and virtualization — all passing, see the test-run
output in the delivery summary).

### 1. `VekUiReactive` — reactive state (brief section 1)
- `Signal<T>` — value cell with `Get()`/`Set()`/`Update()`.
- `Computed<T>` — derived values with **automatic dependency tracking**
  (reads inside the compute function register edges) and **lazy caching**
  (re-evaluates only when a dependency actually changed, not on every read).
- `Effect` / `Watch()` — side effects that automatically re-subscribe to
  whatever they read on their most recent run (so conditional dependencies
  work correctly, matching Vue/Solid-style fine-grained reactivity).
- `ReactiveScope::Batch()` — coalesces multiple writes inside a batch into
  a single notification per affected subscriber, which is exactly the
  mechanism `ui.begin_edit()/ui.commit_edit()`-style transactional editing
  (brief section 13) needs to build on: a slider dragged across 200 pointer
  events should produce one dependent recompute, not 200.
- This is real dependency-graph reactivity, not a dirty-poll-every-frame
  simulation: `TestComputedTracksAndCaches` asserts the compute function
  runs exactly once between writes even across multiple reads.

### 2. `VekUiStyle` — style cascade (brief section 2)
- Real selector parser: `#id`, `.class`, chained `.a.b`, pseudo-states
  `:hover`/`:disabled`/etc., and descendant selectors (`".toolbar
  .icon-button"`) matched against an explicit ancestor path the same way
  a browser matches CSS.
- CSS-style **specificity** (ID > class/pseudo > bare type) with
  source-order tie-breaking, so `#hero-card` correctly overrides `.card`
  regardless of which rule was added first.
- **Theme design tokens**: `theme "name" { --token: value }` blocks, an
  active theme, and `var("--token")` resolution inside any declaration.
- **Inherited properties**: `foreground`, `font_family`, `font_size`,
  `line_height`, `letter_spacing`, `text_align` cascade from parent to
  child when not overridden (`IsInheritedStyleProperty`), everything else
  does not, matching real CSS inheritance rules.
- A real parser for the `.vek` syntax shown in the brief (`style ".sel" {
  ... }` / `theme "name" { ... }`), not just a hand-built API — see
  `StyleSheet::Parse` and `TestStyleSheetParsing`.
- **Known limitation**: element-type selectors (a bare `Panel` token) are
  tokenized but not matched against `GuiWidgetType` yet — only ID/class/
  pseudo compound selectors are matched. Flagged here rather than silently
  claimed as complete.

### 3. `VekUiCommands` — command architecture (brief sections 10–11)
- `CommandDescriptor` with id/label/icon/shortcut/enabled/checked state and
  an `execute` callback, plus an optional `requiredCapability` so command
  execution can be gated by VEK's existing authority/capability system
  (`VekAuthoritySystems`) without adding a new security surface.
- `CommandRegistry::Execute(id)` and `ExecuteByShortcut(chord)` both run
  the same registered command — proving the "toolbar button, menu item,
  shortcut, and command palette all invoke the same code" requirement from
  section 11, rather than four separate call sites.
- A real key-chord parser (`"Ctrl+Shift+Delete"`, multi-step chords like
  `"Ctrl+K Ctrl+B"`) for shortcut binding.

### 4. `VekUiVirtualization` — list/tree/table/grid virtualization
   (brief section 7)
- `ComputeVirtualRange` (uniform row height), `ComputeVirtualRangeVariable`
  (mixed row heights, via binary search over a prefix-sum table), and
  `ComputeVirtualGridRange` (2D wrap grids for asset browsers).
- `TestVirtualListUniformRows` asserts that a 50,000-item list scrolled to
  an arbitrary offset renders **fewer than 40 nodes** (viewport + overscan)
  — the exact requirement stated in the brief.

## What is designed but not built in this generation

To be direct about scope, the following brief items have **no runtime
implementation yet** in this release. Where there's an obvious integration
point in the code above it's noted; otherwise this is future work:

- The declarative `component Foo(props) { ... }` **language/parser layer**
  itself (brief section 1's `.vek` component syntax, JSX-like tree
  building, keyed list reconciliation, lifecycle hooks). `VekUiReactive`
  is the state engine such a layer would sit on top of, but the parser/
  compiler that turns `component VehicleCard(vehicle) { Card { ... } }`
  into a tree of reactive bindings does not exist yet.
- Applying resolved `VekUiStyle` output onto `GuiNode`'s
  `GuiVisualStyle`/`GuiLayoutStyle` fields automatically (today
  `StyleSheet::Resolve` returns a plain property map; wiring that into
  `GuiFramework`'s per-frame style pass is not done).
- Flexbox/Grid-equivalent layout algorithms, responsive `breakpoint`
  blocks, sticky/anchor positioning (VEK 2.8's `GuiLayoutMode` row/column/
  grid/overlay/dock stays as-is in this release).
- Text shaping/IME/rich text architecture, the icon/SVG-path system,
  new widget implementations (ComboBox, DataGrid, NodeGraph, CurveEditor,
  etc.), the docking/workspace system, the modern animation system
  (springs/keyframes/sequences), data binding (`bind(...)`), undo/redo
  transactions, deeper accessibility tree/screen-reader bridge, the
  profiler/`ui_stats()` API, draw-list effects (shadows/blur/gradients),
  world-space UI, the visual designer, hot reload semantics, and the
  `VEK-UI-####` diagnostic code catalogue.
- The CustomVehicle Build Mode layout (brief section 25) as a real example
  — not built; would consume the widgets/docking above once they exist.

None of the above is claimed as implemented anywhere in this release's
code, tests, or version metadata. Treat this as the reactive/style/
command/virtualization foundation layer of VEK UI Next, with the rest of
the 31-point brief as a tracked backlog.

## Backward compatibility

Nothing in VEK 2.8's `GuiFramework`, `gui_*`/`ui_*` script API, or C ABI
was modified. The four new modules are additive translation units with no
dependency on `GuiFramework` internals, so all existing `.vek` scripts and
host-language bindings continue to work unchanged.

## Files added

```
include/vek/VekUiReactive.h        src/VekUiReactive.cpp
include/vek/VekUiStyle.h           src/VekUiStyle.cpp
include/vek/VekUiCommands.h        src/VekUiCommands.cpp
include/vek/VekUiVirtualization.h  src/VekUiVirtualization.cpp
tests/ui_next_tests.cpp
docs/UI_NEXT.md
docs/UI_MIGRATION_2_8_TO_3_0.md
```

`CMakeLists.txt` was modified to compile the four new sources into
`vek_runtime` and to build/register `vek_ui_next_tests` as a ctest target
(`vek_ui_next`). `VERSION`, `Versions.md` and `README.md` were updated to
3.0.0.
