# VEK

VEK 3.0.2 ("VEK UI Next") is the current stable line. VEK 3.0.2 is the installer reliability hotfix for the 3.0 generation. It builds on the VEK 2.8 retained GUI framework with foundational modern-UI systems: reactive state, a CSS-like style cascade, a centralized command architecture, and list/tree/table virtualization. See `docs/UI_NEXT.md` for full scope and `docs/UI_MIGRATION_2_8_TO_3_0.md` for migration notes.


## VEK 2.8 — GUI Framework major update

VEK 2.8 is a GUI-focused major release. It adds a retained-mode, renderer-neutral `GuiFramework` with a persistent UI tree, 50+ widget types, row/column/grid/overlay/dock layouts, fixed/content/fill/percentage sizing, DPI and safe-area handling, themes and state styles, hit testing, keyboard focus, pointer/text/scroll input, bounded events, drag/drop metadata, accessibility roles, tween animations, UI snapshots and renderer-neutral draw lists.

The older immediate GUI command API and VEK 2.7 `gui_define` registry remain available for source compatibility. New projects can use the `ui_*` native API or the C++ `vek::GuiFramework` directly. The C ABI runtime also registers the same `ui_*` natives so Python/Rust/Node/C#/Go/Java hosts can drive one canonical GUI runtime instead of reimplementing it.

See `docs/GUI_FRAMEWORK_V2.8.md` and `docs/RELEASE_NOTES_V2.8.0.md`.

## VEK 2.7 — language/runtime platform update

VEK 2.7 adds first-class structured diagnostics, a VEK debugger, append-only crash reporting, FPS/GPU/profiler runtime APIs, renderer-neutral GUI/gameplay definition registries, Physics Definitions v0.3, and new language syntax (`for ... in`, `try/catch`, `throw`, block comments). The CLI adds `vek diagnose` and `vek trace`.

VEK still uses **one canonical C++20 runtime**. Host bindings now include C/C++, Python, Rust, Node.js, C#/.NET, Go, and Java 22+. Keeping one interpreter avoids security and behavior drift between languages.

See `docs/RELEASE_NOTES_V2.7.1.md` for the part-presentation update, plus `docs/RELEASE_NOTES_V2.7.0.md`, `docs/DIAGNOSTICS_DEBUGGER_V2.7.md`, `docs/LANGUAGE_V2.7.md`, and `docs/RUNTIME_PLATFORM_V2.7.md` for the 2.7 platform foundation.


## Windows GitHub installer

VEK 3.0.2 further hardens the installer and release-version system. The repository `VERSION` file is the canonical version source. The installer resolves the newest **published release** (not merely the newest tag), checks out that exact release tag into `C:\vek\repo`, downloads the matching `VEK-vX.Y.Z-windows-x64.zip` GitHub Release asset, verifies its SHA-256 companion, verifies the package `VERSION`, archives the runtime under `C:\vek\versions\vX.Y.Z`, and only then deploys the matching runtime.

A Git tag by itself is not treated as an installable Windows update. Version checks now use the published `release-version.json`, so tag-only releases without Windows assets are ignored. The GUI installer can be launched from Downloads and stages itself safely into `C:\vek`; Git is optional because exact-tag source archives are used as a fallback. `.github/workflows/release-windows.yml` automatically builds the required runtime and `VekInstaller.zip` assets when a matching `vX.Y.Z` tag is pushed. See `docs/VERSION_AND_INSTALLER_SYSTEM_V3.md` and `FIXING_V3_0_0_INSTALL.md`.



## VEK 2.5.4 — safer Windows installer architecture

VEK 2.5.4 redesigns the Windows installer to reduce antivirus false-positive risk. The installer no longer performs silent/background self-updates, no longer launches a hidden copy of itself, and no longer replaces its own executable. AUTO mode performs version checks and notifications only; actual downloads/updates require a visible user click. For the safest install flow, extract `VekInstaller.zip` directly to `C:\vek` and run `VekInstaller.exe` from there. Git operations are still executed directly and their progress is captured inside the GUI so no console popups are required.

## VEK 2.5.3 — Zero-popup Windows installer

VEK 2.5.3 fixes visible console flashes from Git operations started by the graphical Windows installer. The GUI now launches `git.exe` directly with standard Windows no-console child-process flags while continuing to capture Git progress for the 3D download bar. No `cmd.exe`, PowerShell, shell scripts, obfuscation, Defender exclusions, or antivirus bypasses are used.

The background updater uses the same no-console policy, while `vek.exe` remains a normal console program because it is the VEK command-line interface.

## VEK 2.5.2 — Fullscreen installer and background auto-update

VEK 2.5.2 refines the Windows installer into a true fullscreen experience. The 3D VEK logo is smaller and static, the verbose console-style status panel is removed, and the extruded progress bar is the primary download indicator.

Automatic update mode now performs update checks and starts updates through the GUI installer in background mode, while Manual mode waits for an explicit DOWNLOAD action. The installer continues to use the official GitHub repository at `https://github.com/defnot67kid-beep/vek.git`.

The interactive UI runtime now also exposes responsive fullscreen installer layout calculations and background-update policy helpers for reuse by native VEK hosts.


## VEK 2.3 — Secondary-Motion Physics

VEK 2.3 adds a renderer-independent spring-chain physics solver designed for
hair, cloth strips, cables, ropes and other secondary motion. It includes
substepping, damping, stiffness, inertia, wind/external acceleration,
fixed-length constraints and sphere collision. The CustomVehicleGame uses this
system for articulated hair instead of purely procedural sine-wave movement.

See `docs/PHYSICS_SYSTEM.md` and `examples/hair_physics.vek`.


**VEK (Vehicle Engineering Kernel)** is an original, embeddable programming language and gameplay SDK written in C++20.

VEK started as the secure gameplay scripting language for the Custom Vehicle Game, but the runtime is designed to be usable by other applications as well. It keeps one canonical language/runtime implementation and exposes safe host APIs instead of requiring every host language to reimplement VEK.

Current version: **3.0.2**

## What VEK is designed for

VEK is intended for configurable application and game logic where a native host should remain in control of performance-critical and security-sensitive operations.

A host can use VEK for things such as:

- gameplay rules and formulas
- vehicle-part definitions
- editor/build rules
- game modes
- health, gravity and ragdoll parameters
- GUI descriptions
- animations and interaction metadata
- proximity prompts
- garage doors and access-control rules
- camera profiles, skybox/environment policy and humanoid rig definitions
- global world/gameplay policy such as movement speeds and frame limits
- click-only / line-of-sight interaction policy
- responsive GUI text layout and auto-fit rules
- jobs, missions, economy and progression logic
- safe entity/component scripting

The native application remains responsible for rendering, low-level physics, raw memory, operating-system integration, networking, threading, cryptography and other privileged functionality.

## Language features

VEK supports:

- `.vek` source files
- numbers, strings, booleans and `nil`
- variables and assignment
- functions and parameters
- `if / else if / else`
- `while`
- `for value in collection` deterministic iteration
- `try / catch` for explicit VEK throws
- `throw value`
- `break` and `continue`
- `return`
- arithmetic, comparison and boolean operators
- comments using `#`, `//`, or `/* ... */`
- arrays
- maps/dictionaries
- nested indexing and member access
- structs/data types
- events
- safe modules/imports with configured module roots
- host-registered native functions
- configurable execution/sandbox limits
- a sealable native-function registry
- structured diagnostics and VEK call stacks
- function/line debugger breakpoints and execution trace
- CLI, evaluator, syntax checker, diagnostics runner, tracer and REPL

## SDK systems

VEK currently includes reusable native-side SDK systems for:

- Gravity
- Health
- Procedural ragdoll foundations
- Renderer-neutral GUI commands
- Survival/Sandbox game-mode policy
- Vehicle-editor/build helpers
- Dynamic VEK part registration
- Script entity/component handles
- Animation definitions and playback state
- Proximity prompts
- Garage doors
- Passlocks/access control
- screen-effect metadata and playback state
- safe audio-cue metadata
- death/respawn sequence timing
- feet-on-ground / grounding profiles
- Responsive GUI text fitting, wrapping, clipping and ellipsis
- Safe click-only interaction metadata and inside-egress garage policy
- FPS / frame-time / CPU / GPU performance tracking
- GPU capabilities and memory-budget descriptors
- Runtime profiler zone aggregation
- Generic GUI definition registry
- VEK 2.8 retained-mode GUI framework (layout, themes, focus, events, animation, accessibility, draw lists)
- Generic gameplay definition registry
- Physics Definitions v0.3 descriptors

## VEK 2.0 authority/security systems

VEK 2.0 adds deterministic server-authority helpers for multiplayer hosts. Per-action policy can require an authenticated session, capabilities, bounded payload shape/size, monotonic sequences, replay nonces and burst-tolerant token-bucket limits. Authority/action/schema/native registrations reject duplicates, and replication field types are allow-listed.

The networking layer must derive `actorId`, `sessionId` and authentication state from the trusted connection/session. These values must not be accepted from an untrusted packet and then treated as authenticated. VEK validates policy and protocol invariants; the native host still owns cryptographic authentication, encrypted transport, sockets and connection identity.

VEK deliberately does not turn heuristic anomaly scores into automatic bans. Hard rejection is based on explicit rules, which makes security behavior auditable and reduces false-positive risk.

## GUI system

VEK can describe advanced interfaces without receiving direct GPU, OS-window or renderer access. VEK 2.8 introduces `GuiFramework`, a retained-mode UI runtime that owns structure and behavior while the host owns actual drawing and native input.

The framework includes a persistent parent/child tree, 50+ widget types, row/column/grid/overlay/dock layout, fixed/content/fill/percentage sizing, min/max constraints, margin/padding/gaps, DPI scaling, safe areas, themes/classes, hover/pressed/focus/disabled states, typography and border/radius/opacity styling, hit testing, tab focus, pointer/keyboard/text/scroll input, drag/drop events, accessibility metadata, animation tweens, bounded event queues, snapshots and renderer-neutral draw lists.

VEK 2.8 keeps the older immediate command GUI and `gui_define` registry for backward compatibility. New retained-mode scripts use the `ui_*` API, such as `ui_create`, `ui_patch`, `ui_set_viewport`, `ui_layout`, `ui_next_event`, `ui_animate`, `ui_snapshot` and `ui_draw_list`.

## One runtime, multiple host languages

VEK keeps one canonical C++ runtime.

A stable C ABI is provided through:

```text
include/vek/vek_c.h
include/vek_c.h
```

The repository also contains binding foundations for:

```text
bindings/
├── csharp/
├── go/
├── java/
├── node/
├── python/
└── rust/
```

These bindings are intended to wrap the same VEK runtime rather than create separate language implementations.

## Simple VEK example

```vek
fn factorial(n) {
    let result = 1;
    let i = 2;

    while i <= n {
        result = result * i;
        i = i + 1;
    }

    return result;
}

fn main() {
    println("6! =", factorial(6));
    return 0;
}
```

Arrays and maps:

```vek
let wheels = [
    "road",
    "offroad",
    "racing"
];

let engine = {
    power: 32000,
    mass: 140,
    fuel: "petrol"
};

println(wheels[1]);
println(engine.power);
```

## Portable Windows release and GUI installer

VEK 2.2 keeps the relocatable Windows distribution and adds a native graphical installer. Normal users do not need Git, CMake or Visual Studio. Download the Windows portable ZIP and extract it. For first-time setup, double-click `VekInstaller.exe` or run `vek.exe --install` from the extracted folder. Once VEK is on PATH, `vek --install` can reopen the graphical installer.

The installer shows quick-install, custom-folder and use-this-folder choices and can add the selected VEK folder to the Windows User PATH. The `INSTALL_PATH.cmd` helper remains available as a fallback.

After setup, open a new terminal and run:

```bat
vek --version
vek info
vek doctor
```

The folder can be renamed or moved. VEK discovers its home from `vek.exe`; `VEK_HOME` is optional. `vek verify` validates package files against the release-generated local SHA-256 manifest. See `docs/PORTABLE_INSTALL.md`.

Release details are in `RELEASE_NOTES_V2.2.0.md`.

## Build

VEK uses CMake.

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Windows you can also use:

```text
BUILD_WINDOWS.bat
```

## CLI

Examples:

```bash
vek --install
vek run examples/hello.vek
vek check examples/hello.vek
vek diagnose examples/hello.vek
vek trace examples/language_v27.vek
vek eval "(10 + 5) * 2"
vek repl
vek version
```

## Embed in C++

```cpp
#include <VekScriptEngine.h>

VekScriptEngine vek;

vek.RegisterNative("speed", [](const std::vector<VekValue>&) {
    return VekValue(42.0);
});

vek.LoadFile("rules.vek");
VekValue result = vek.Call("main");
```

CMake projects can link the runtime with:

```cmake
target_link_libraries(MyProgram PRIVATE VEK::Runtime)
```

## Stable C ABI

VEK also exposes a C-compatible runtime interface for bindings and non-C++ hosts.

Conceptually:

```c
vek_runtime* runtime = vek_create();
vek_load_file(runtime, "rules.vek");
vek_value result = vek_call(runtime, "main", NULL, 0);
vek_destroy(runtime);
```

## Security model

VEK is designed around explicit host capabilities.

Scripts do not automatically receive access to:

- raw process memory
- native pointers
- arbitrary filesystem access
- shell/process execution
- operating-system APIs
- raw network sockets
- GPU memory
- DLL injection/loading
- cryptographic keys

A host application explicitly chooses which native functions and systems are exposed.

VEK also supports configurable limits for script size, execution, calls, loops and related runtime work. Applications can layer additional security on top, such as signed scripts and secure saves.

## Repository structure

```text
VEK/
├── include/
├── src/
├── cli/
├── bindings/
├── examples/
├── tests/
├── .github/
├── CMakeLists.txt
├── VERSION
├── README.md
└── Versions.md
```

## Secure server-authoritative SDK

VEK 2.0 strengthens the host-side authority/security SDK for multiplayer games. The networking transport and user authentication remain native responsibilities, while VEK can define authoritative gameplay actions and replication schemas.

The SDK includes hardened runtime security tiers, sealed capability manifests, action payload limits, sequence validation, replay-nonce detection, per-action rate limits, server-only commit checks, bounded security audit events, and replication schemas.

A hacked client must still be treated as untrusted. For meaningful multiplayer anti-cheat, authoritative economy, inventory, health, progression, jobs, spawning and build validation should run on a trusted server. VEK does not claim that an offline/client-controlled executable can be made unhackable.

The stable C ABI exposes security-tier and authority helpers so C, Rust, C#, Python, Node and future bindings can share the same canonical runtime. See `examples/server_authority.vek`.

## Version history

All release notes and feature updates are kept in **`Versions.md`**.

## License

VEK is released under the MIT License. See `LICENSE`.
## VEK 2.5 Interactive Installer Runtime

VEK 2.5 adds renderer-independent interactive UI primitives used by the Windows GitHub installer: rotating 3D presentation math, a deterministic one-button runner mini-game, semantic version status, and Auto/Manual update policy support. Trusted Git/filesystem operations remain in the native installer host.



## Part presentation assets (2.7.1)

Vehicle/editor hosts can give every VEK-defined part its own catalog icon and 3D presentation without hard-coding a new C++ enum. Example:

```vek
part_register({
  id:"seat.driver",
  display_name:"Driver Seat",
  visual:"box", // backwards-compatible fallback
  presentation:part_presentation(
    "icon:seat.bucket",
    "builtin:seat.bucket",
    "builtin:seat.bucket",
    {material:"fabric", allow_tint:false}
  ),
  mass:20, price:100, size:vec3(0.8,1.1,0.8)
});
```

Presentation strings are renderer-neutral logical IDs or host-defined asset URIs. The host decides how `builtin:` or `asset:` references are rendered/loaded.


## Installer release discovery (3.0.3)

The Windows installer discovers the newest **published GitHub Release** from GitHub release metadata. `release-version.json` is optional. GitHub asset digests are preferred for verification, `.sha256` sidecars remain supported, and a full `VekInstaller.zip` can act as the runtime bundle when a separate Windows runtime ZIP is unavailable.

## VEK 3.1 Modern UI

VEK 3.1 adds a modern token-driven dark/light GUI design system, polished widget rendering contracts, focus-visible interaction, modern scrollbars/scroll shadows, editor-grade viewport framing, and upgraded graph/timeline presentation while preserving the VEK 3.0 widget tree and scripting API. See `docs/UI_MODERN_3_1.md` and `docs/UI_MIGRATION_3_0_TO_3_1.md`.

### VEK 3.2 Camera + Physics
VEK 3.2 adds Camera Framework 1.0 and Physics Definitions v0.4. See `docs/RELEASE_NOTES_V3.2.0.md`.
