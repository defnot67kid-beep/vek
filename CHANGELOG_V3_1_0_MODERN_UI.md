# VEK 3.1.0 — Modern UI visual/UX overhaul

- Added complete dark/light token design system.
- Added widget-type selectors and recursive `var(--token, fallback)` resolution.
- Modern styling for all existing widget types without changing their enum/API.
- Added semantic modern draw primitives and component sub-parts.
- Added focus-visible keyboard behavior and smoothed interaction state progress.
- Added modern overlay/classic scrollbars, thumb dragging, scroll shadows and virtualization metrics.
- Added viewport frame/backdrop/loading/empty/toolbar/corner-gizmo contract without touching 3D rendering.
- Added modern graph/timeline grid/crosshair/keyframe rendering contracts.
- Added sticky table-header/column rendering contract and property-grid row contract.
- Added renderer-independent `UiInteractionMotion` helper.
- UI API version advanced to 3.1; VEK release version 3.1.0.
- Existing `.vek` widget declarations and public GuiFramework methods remain compatible.
