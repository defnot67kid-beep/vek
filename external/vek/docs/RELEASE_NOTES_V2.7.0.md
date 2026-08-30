# VEK Programming Language v2.7.0

VEK 2.7.0 is a platform/runtime release. It moves VEK beyond a small gameplay
scripting layer by strengthening the language VM, diagnostics/debugger tooling,
and backend-neutral engine APIs while keeping the canonical runtime in C++20.

## Native VEK diagnostics, crash handling and debugger

- Structured `DiagnosticRecord` values with severity, domain, stable error code,
  source, line, function and VEK call stack.
- Diagnostic history with a bounded sinkable `DiagnosticHub`.
- `VekDebugger` with function breakpoints, source-line breakpoints, pause state,
  continue/resume behavior and bounded execution trace history.
- `VekCrashHandler` with append-only VEK crash logs, build/stage context,
  last structured diagnostic and VEK stack frames.
- `std::terminate` integration on all supported native hosts.
- Windows unhandled-exception filtering records the exception code/address.
- New CLI commands: `vek diagnose file.vek` and `vek trace file.vek`.
- The C ABI exposes diagnostics, debugger controls/trace and crash-handler hooks.

## VEK language additions

- `for item in collection { ... }` for arrays, maps and strings.
- `try { ... } catch(error) { ... }` for explicitly thrown VEK values.
- `throw value;`.
- `/* block comments */` in addition to `//` and `#` comments.
- Runtime statement line tracking and stack-aware errors.
- Structured recoverable script errors with `error(code, message, data)` and
  `is_error(value)`.
- Standard-library additions: assertions, array get/set/pop, map keys, contains,
  upper/lower/substrings, prefix/suffix checks, trigonometry, log/exp, lerp and
  monotonic `time_ms()`.

`try/catch` catches explicit VEK `throw` values only. Runtime faults, native
faults and security/sandbox-budget violations are intentionally not catchable by
scripts, preventing error handling from becoming a sandbox escape mechanism.

## Runtime platform APIs

New `VekRuntimeSystems` provides renderer-independent host models for:

- FPS/current FPS/average FPS/1% low/frame time/CPU time/GPU time tracking.
- GPU adapter/backend/capability and memory-budget reporting.
- GUI node definitions (windows, panels, labels, buttons, toggles, sliders,
  progress bars, text inputs, images, lists, scrolling and canvases).
- Gameplay definitions (abilities, input actions, camera modes, inventory,
  quests, spawn rules, states, interactions, damage types, teams/objectives).
- Runtime profiler zones and snapshots.

These APIs expose data and policy. They do not give scripts raw GPU pointers,
threads or operating-system handles.

## Physics Definitions v0.3

v0.3 keeps all v0.2 categories and adds definitions for:

- contact-material overrides and continuous collision detection
- articulations and articulated links
- ragdolls and IK chains
- physics particles and fluid volumes
- destruction clusters
- tire-friction and suspension models
- scene queries and physics LOD
- collision/trigger/sleep/wake/joint-break event policy

New generic native names are `physics_version`, `physics_definition_register`,
`physics_definition_exists` and `physics_definition_count`. Existing
`physics_v02_*` natives remain supported and `physics_v02_version()` continues
to report `0.2` for source compatibility.

## Host-language bindings

VEK still has one canonical implementation. 2.7 adds Go (cgo) and Java 22+
(Foreign Function & Memory API) binding foundations alongside C/C++, Python,
Rust, Node.js and C#/.NET. This avoids multiple interpreters drifting apart.

## Compatibility

VEK 2.7 preserves the 2.x C++ and C-ABI foundations and the older Physics v0.2
native names. Hosts should recompile native applications because the static
runtime and public C++ headers contain new types and methods.
