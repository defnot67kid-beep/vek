# VEK Runtime Platform APIs v2.7

VEK 2.7 adds engine-facing definitions without coupling the language runtime to
a renderer, windowing toolkit, GPU API or physics backend.

## Performance/FPS

`FramePerformanceTracker` accepts host frame samples and calculates current FPS,
average FPS, 1% low FPS, frame milliseconds, CPU/GPU milliseconds and the worst
frame in the configured rolling window. Registering its natives exposes
`perf_stats()` to VEK scripts.

## GPU

`GpuRuntimeInfo` lets the trusted native host report the active backend, adapter,
vendor, VRAM/shared memory, feature flags and current memory budget. VEK scripts
can read `gpu_info()` and `gpu_supports(name)` but receive no device pointer or
raw command access.

## GUI

`GuiDefinitionRegistry` is a bounded, renderer-neutral definition tree. Hosts
map definitions to raylib, SDL, Dear ImGui, WinUI, a custom renderer, etc.

Script natives:

```text
gui_define(map)
gui_count()
gui_exists(id)
gui_remove(id)
```

## Gameplay definitions

`GameplayDefinitionRegistry` stores backend-neutral definitions for abilities,
input actions, camera modes, inventory items, quests, spawn rules, states,
interactions, damage types, teams and objectives.

## Profiler

`RuntimeProfiler` aggregates host-provided named timing zones and exposes a
snapshot to VEK. It does not instrument arbitrary native code by itself.

## RuntimePlatformPack

`RuntimePlatformPack` groups performance, GPU, GUI, gameplay and profiler
registries and registers their natives in one call. The pack must outlive the
script engine because registered natives refer to the pack's registries.
