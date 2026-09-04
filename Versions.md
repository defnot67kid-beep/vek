## VEK 3.0.0 — VEK UI Next (major GUI generation)
- Added the foundational systems for a modern declarative/reactive UI generation: `VekUiReactive` (signals, lazily-cached computed values, dependency-tracked effects/watchers, batched writes), `VekUiStyle` (CSS-like cascade: class/ID/descendant/pseudo-state selectors, specificity, theme design tokens with `var()`, inherited properties, a real `.vek` style/theme block parser), `VekUiCommands` (centralized command registry: id/label/icon/shortcut/enabled/checked/capability-gated execute, shared by toolbar/menu/shortcut/palette callers), and `VekUiVirtualization` (uniform, variable-height and 2D-grid visible-range computation so lists/trees/tables/grids only materialize on-screen + overscan rows regardless of total item count).
- These are new, independently-testable modules (see `tests/ui_next_tests.cpp`) layered on top of, not replacing, the VEK 2.8 retained `GuiFramework`; all existing `gui_*`/`ui_*` APIs continue to work unchanged.
- See `docs/UI_NEXT.md` for the architecture, scope, and an honest list of what in the original 31-point brief is implemented now vs. designed-but-not-yet-built in this generation.
- Version bumped to 3.0.0 (major) to reflect this is the start of a new UI generation, not an incremental 2.x update.

## VEK 2.8.0 — GUI Framework
- Added a retained-mode `GuiFramework` alongside the backward-compatible immediate GUI APIs.
- Added 50+ widget types for game HUDs, menus, windows, editor tooling, data views and advanced controls.
- Added row/column/grid/overlay/dock layout, fixed/content/fill/percentage sizing, min/max constraints, margins, padding, gaps, DPI scale and safe areas.
- Added theme/class styling plus hover, pressed, focus and disabled visual states.
- Added hit testing, pointer/keyboard/text/scroll input, tab focus navigation, bounded GUI events and drag/drop metadata.
- Added accessibility roles, labels, hints and navigation ordering.
- Added bounded tween animations with easing and renderer-neutral draw-list generation.
- Added full UI snapshot/stat APIs and built-in dark/light themes.
- Registered the `ui_*` script API through `RuntimePlatformPack` and the stable C ABI runtime so all host-language bindings share the same GUI behavior.
- Added `GUI_FRAMEWORK_V2.8.md`, a complete VEK example and a dedicated GUI regression suite.
- Preserved VEK 2.7 `gui_define` and immediate GUI command compatibility.

## VEK 2.7.1 — Part presentation assets
- Added renderer-neutral icon, view-model, world-model and material metadata to `PartDefinition`.
- Added view transform/tint presentation controls and the `part_presentation(...)` script helper.
- Kept legacy `visual` definitions fully backward compatible.

# VEK Versions

## VEK 2.7.0

- Added structured VEK diagnostics with stable domains/codes, source lines and VEK call stacks.
- Added `VekDebugger` with function and line breakpoints, pause/continue behavior and bounded trace history.
- Added append-only `VekCrashHandler`, terminate handling and Windows unhandled-exception reports.
- Added CLI `vek diagnose` and `vek trace`.
- Added `for ... in`, explicit `throw`, `try/catch`, and block comments to the VEK language.
- Expanded the standard library with collection, string, math, assertion and timing helpers.
- Added FPS/1%-low/frame-time tracking, GPU capability/budget reporting, runtime profiler, GUI definition and gameplay definition registries.
- Advanced Physics Definitions to v0.3: CCD, contacts, articulations, ragdolls, IK, particles, fluids, destruction, tires/suspension, scene queries, LOD and physics events.
- Preserved `physics_v02_*` source compatibility while adding generic `physics_*` natives.
- Extended the C ABI with diagnostics/debugger/crash APIs.
- Added Go and Java 22+ host-language bindings alongside C/C++, Python, Rust, Node and C#.

## VEK 2.6.1

- Hardened authoritative-request validation with sealed capability manifests and authenticated session-to-actor binding.
- Added bounded per-action replay nonce retention and deterministic audit decisions.
- Added `DeveloperFeatureGate` for development-tier, local, authenticated, trusted-native tooling capabilities.
- Added Advanced Physics Definitions v0.2 descriptors and a bounded script definition registry.
- Advanced v0.2 rigid-body/vehicle/soft-body definitions are API descriptors only; no solver is auto-enabled.
- Kept the deterministic secondary-motion spring-chain solver used by articulated hair.
- Updated installer/bootstrap version metadata to 2.6.1.

## VEK 2.5.4

- Redesigned Windows installer to reduce antivirus false-positive risk.
- Removed silent/background installation and self-updating.
- Removed installer self-copy/self-replacement behavior.
- AUTO mode now checks/notifies automatically; downloads require an explicit visible click.
- Installer package is intended to be extracted directly to `C:\vek`.
- Git progress remains captured in the GUI without visible console popups.


## VEK 2.5.3

### Zero-popup Windows installer

- Fixed visible console windows appearing during GitHub version checks, clone, fetch, reset, and update operations.
- Git is invoked directly as a child process; the installer does not invoke `cmd.exe` or PowerShell.
- Standard Windows `CREATE_NO_WINDOW` / hidden-window process settings are applied to GUI-owned Git child processes.
- Git stdout/stderr remain captured so download percentage still drives the 3D progress bar.
- Background automatic updates remain silent.
- `VekInstaller.exe` remains a Windows GUI subsystem executable; `vek.exe` remains a console CLI executable.
- No obfuscation, packing, Defender exclusions, or antivirus-bypass behavior was added.

---

# VEK 2.5.2

- True borderless fullscreen Windows installer.
- Smaller static 3D VEK logo; continuous logo rotation removed.
- Verbose console-like installer status/source text removed from the visible UI.
- 3D progress bar is the main download/update progress indicator.
- Geometry-Dash-style runner remains playable with Space.
- Automatic mode can update silently in background mode.
- Manual mode waits for a DOWNLOAD click.
- Update-policy and repository-management commands removed from the public CLI help.
- Added responsive fullscreen installer layout primitives to `VekInteractiveUiSystems`.
- Added `ShouldBackgroundUpdate()` policy helper.

# VEK 2.5.1

- Expanded the rotating VEK wireframe to span the full installer presentation area.
- Replaced text hash progress with a real extruded 3D loading bar.
- Git clone/fetch percentage now drives download progress during the GitHub synchronization stage.
- Added `ProgressBar3DModel` to the renderer-independent interactive UI runtime.
- Removed decorative `//` separators from visible installer labels and status messages.
- Kept the playable Space-to-jump runner and automatic/manual update controls.

# VEK 2.3.0

- Added `VEK::Runtime`-compatible secondary-motion physics.
- Added deterministic `SpringChain3D` Verlet solver.
- Added fixed-length constraint iterations and bounded substeps.
- Added sphere collision so hair/cloth chains can remain outside character geometry.
- Added inertia, damping, stiffness, air drag, gravity and external-force controls.
- Added `SecondaryMotionProfileRegistry` and VEK script registration natives.
- Added `hair_physics.vek` example and physics regression tests.
- Updated version macros and CMake runtime export.

# VEK Version History

## VEK 2.2.0

Native Windows installer and setup UX:

- `vek --install` command with VEK ASCII startup mark
- short `1 - 2 - 3` installation sequence and `Installing VEK...` console handoff
- separate `VekInstaller.exe` built as a Windows GUI application (no installer console UI)
- quick install/repair at `C:\\vek`
- graphical custom-folder picker
- use-current-folder registration mode for portable installs
- optional Windows User PATH registration, enabled by default
- Windows environment-change broadcast after PATH updates
- portable release workflow packages and validates `VekInstaller.exe`
- existing ZIP, `INSTALL_PATH.cmd`, manual PATH, and Git clone flows remain supported

## VEK 2.1.0

Portable/relocatable release foundation:

- official Windows portable ZIP generation
- installation-root discovery based on the running `vek.exe`
- no hard-coded install directory and no required `VEK_HOME`
- `vek home`, `vek info`, `vek doctor`, and `vek verify` commands
- one-click User PATH helper (`INSTALL_PATH.cmd`) and clean removal helper
- static MSVC runtime option for a more self-contained Windows CLI
- GitHub Actions release workflow that builds the portable package and SHA-256 manifest
- Git clone/source builds remain fully supported

## VEK 2.0.0

### Deterministic anti-abuse hardening with lower false-positive risk

- authenticated-session requirements can be declared per authority action
- bounded payload depth, item count and per-string size before serialization
- cyclic-container and non-finite-number rejection for network payloads
- configurable replay-nonce length/format validation
- token-bucket request limiting replaces fixed one-second windows so normal packet clumping is less likely to be rejected
- rate-limited requests do not consume sequence/nonce state, allowing safe retry after refill
- bounded server authority state prevents untrusted identities from growing memory without limit
- duplicate authority actions, replication schemas, fields and native registrations are rejected
- C ABI duplicate-native registration can no longer overwrite the callback behind a rejected duplicate name
- capability/action/schema identifiers are format-checked
- replication field types are allow-listed
- Hardened Server VM budgets are tighter than 1.9 while Development remains roomy
- standard-library container mutation helpers have hard growth caps

Security philosophy: hard denials come from deterministic protocol invariants, not opaque heuristic scores. Suspicious behavior can be audited without automatically banning a legitimate player. Trusted native networking still owns cryptographic authentication, transport security and session establishment.

---

## VEK 1.9.0

### Hardened authority, multiplayer server policy and camera-world safety

New features:

- `SecurityTier` with Development, Hardened Client and Hardened Server runtime budgets
- `SecurityPolicyFactory` for consistent VM limits
- sealed `CapabilityManifest` host permissions
- VEK-defined `AuthorityActionRegistry`
- server-authoritative action policies
- client-request allow/deny policy
- maximum request payload sizes
- monotonic action sequence validation
- replay-nonce detection with bounded nonce history
- per-actor/per-action rate limiting and burst limits
- required capability checks
- client authoritative-commit rejection
- bounded `SecurityAuditBuffer`
- VEK-defined `ReplicationSchemaRegistry`
- server-owned and owner-only replicated fields
- reliable/unreliable replication metadata and maximum update rates
- stable C ABI security-tier controls
- stable C ABI host-authority role controls
- stable C ABI request validation helpers
- safe camera minimum-world-height policy support
- server-authority example script

Security note: VEK 1.9 reduces attack surface and makes server-authoritative designs easier, but it does not promise an unhackable client or a fixed percentage of remaining vulnerabilities. Cryptography, identity/authentication, sockets, OS integration, raw memory and authoritative network transport remain native host responsibilities.

---


This file is the single version history for VEK. New release notes and feature additions should be added here instead of creating separate changelog Markdown files.

---


## VEK 1.8.0

### Camera, skybox, humanoid rig and world-policy SDK

VEK 1.8 moves another large layer of configurable engine behavior into safe script-owned policy while keeping native rendering, physics and memory access protected.

New features:

- `CameraProfileRegistry` for third-person, close, first-person and editor camera policy
- VEK-defined FOV, camera distances, target heights, pitch limits, smoothing speeds and acceleration/deceleration
- VEK-defined RMB look policy, sensitivity, inversion and 45-degree/other alignment step sizes
- VEK-defined camera cycle order/allowed modes for third-person, close, first-person and free-inspection cameras
- editor-camera move speed, fast/fine multipliers, pitch/yaw speed and orthographic size
- `SkyboxRegistry` with zenith/horizon/ground/sun colors, fog distances, sun direction/size and day-length metadata
- safe sky asset IDs with path-traversal rejection; native hosts still own GPU/texture loading
- `HumanoidRigRegistry` with named joint chains, parents, lengths, radii, mass, ragdoll weights and angular limits
- `WorldGameplayPolicyRegistry` for target FPS, walk/run/sprint speeds, interaction ranges, frame-delta safety limits and global camera/sky permissions
- camera/sky/rig/world-policy natives for embedding
- all new systems expose data and safe IDs only; scripts never receive raw cameras, renderer handles, physics pointers or OS access

---

## VEK 1.7.0

### Lifecycle effects, audio cues, death sequences and grounding

VEK 1.7 adds reusable host-safe presentation/lifecycle systems while keeping rendering, audio devices and filesystem access native.

New features:

- `AudioCueRegistry` and safe cue metadata (`asset`, volume and pitch)
- traversal-safe audio asset identifiers; VEK never opens files itself
- `ScreenEffectRegistry` / `ScreenEffectSystem` with tint, fade, vignette, pulse and spatter intensity metadata
- `DeathSequenceRegistry` / `DeathSequenceSystem` with ragdoll, screen-effect, audio and respawn timing
- one-shot lifecycle events (`startScreen`, `playAudio`, `respawn`) for native hosts
- `GroundingRegistry` / `GroundingSystem` for avatar-height-aware feet-to-surface alignment
- grounding offsets, clamps and snap policy can be authored in `.vek`
- all new registries expose only safe IDs/data, never raw pointers, GPU/audio handles or unrestricted filesystem access

---

## VEK 1.6.0

### Responsive GUI layout and physical interaction policy

VEK 1.6 upgrades the renderer-neutral UI and access-control SDK so hosts can build interfaces that remain readable across different window sizes and can enforce physical interaction rules without exposing unsafe renderer or world pointers.

New GUI features:

- `GuiTextPolicy`
- `GuiTextLayoutSystem`
- automatic font shrinking
- configurable minimum and maximum font sizes
- word wrapping
- maximum-line limits
- ellipsis for truncated content
- text clipping policy
- configurable line height
- left / centre / right text alignment metadata
- responsive style width/height constraints
- text policy inheritance through VEK GUI styles
- VEK style fields such as `font_size`, `min_font_size`, `max_font_size`, `auto_fit`, `wrap`, `ellipsis`, `clip_text`, `max_lines`, `line_height` and `text_align`

Garage/access additions:

- inside-egress policy for garage doors
- configurable inside auto-open distance
- hold-open-near-door policy
- `GarageDoorSystem::HoldOpen`
- passlock maximum use distance
- passlock line-of-sight requirement
- outside-only passlock policy
- click-only passlock policy
- optional world-prompt visibility metadata

The native host still performs actual collision queries, line-of-sight ray tests, text measurement, drawing and mouse hit testing. VEK only defines safe rules and layout policy.

---

## VEK 1.5.0

### Garage doors, passlocks and access-control UI

VEK 1.5 expands the interaction SDK with reusable garage and access-control systems.

New features:

- `GarageDoorRegistry`
- `GarageDoorSystem`
- segmented garage metadata
- configurable garage width, height and panel count
- open/close durations
- auto-close timing
- locked/unlocked policy
- garage animation IDs
- garage playback/progress state
- `PasslockRegistry`
- `PasslockSystem`
- numeric passcode validation
- maximum-attempt rules
- timed lockouts
- passlock-to-garage linking using safe string IDs
- GUI modal commands
- password-input commands
- status-badge commands
- keypad commands

New safe VEK/native integration points include garage registration/querying, passlock registration/querying and renderer-neutral access UI descriptions.

The security boundary remains unchanged: the native host owns rendering, actual input devices, raw memory, OS access, cryptography and signature verification.

---

## VEK 1.4.0

### Animations and proximity prompts

VEK 1.4 adds renderer-independent interaction primitives.

New features:

- `AnimationLibrary`
- `AnimationSystem`
- animation duration
- playback speed
- looping
- blend metadata
- animation tags
- timed animation markers
- `ProximityPromptRegistry`
- `ProximityPromptSystem`
- action/object prompt text
- input-key metadata
- activation distance
- hold duration
- prompt priority
- line-of-sight policy
- safe animation registration/query natives
- safe proximity-prompt registration/query natives

VEK scripts describe animation and interaction data while the host remains responsible for character animation execution, input and rendering.

---

## VEK 1.3.0

### Language expansion and multi-language SDK foundations

VEK 1.3 significantly expands the language and embedding architecture.

Language additions:

- arrays
- maps/dictionaries
- nested maps
- array indexing
- member access such as `engine.power`
- map indexing such as `engine["power"]`
- structs/data definitions
- events
- native host event emission
- modules/imports
- configurable module roots
- module path validation
- protection against `../` path traversal outside configured roots

SDK/embedding additions:

- stable C ABI
- `vek_create`
- `vek_destroy`
- file loading through the C API
- function calls through the C API
- event emission through the C API
- C ABI error/version helpers
- C++ runtime remains canonical
- Rust binding foundation
- C# binding foundation
- Python binding foundation
- Node/N-API binding foundation
- script entity registry using safe numeric IDs instead of native pointers
- safe component storage on script entity handles

Vehicle/editor additions:

- dynamic `PartRegistry`
- VEK-created vehicle-part definitions
- string part IDs
- component-based part metadata
- attachment metadata
- safe part registration natives

This release establishes the architecture of one VEK runtime with other host languages binding to it rather than reimplementing the interpreter.

---

## VEK 1.2.0

### Vehicle Editor and game-mode SDK

VEK 1.2 adds reusable systems for vehicle-building/editor applications.

New features:

- `GameModeSystem`
- Survival policy support
- Sandbox policy support
- `BuildPartCatalog`
- structural parts
- movement parts
- mechanical parts
- functional parts
- experimental parts
- `VehicleEditorSystem`
- grid snapping
- placement bounds
- unlock rules
- build costs
- part limits
- build validation
- `VehicleBuildCounts`
- validation/warning architecture
- safe editor helper natives
- power-to-weight helpers
- mode-aware part-cost helpers
- mode-aware unlock helpers
- renderer-neutral GUI system usable by host menus and editors

---

## VEK 1.1.0

### Gameplay systems

VEK 1.1 introduces reusable gameplay systems alongside the language runtime.

New features:

- `GravitySystem`
- frame-rate-safe gravity helpers
- jump helpers
- `HealthSystem`
- clamped damage
- healing
- alive/dead state
- health ratio helpers
- procedural `RagdollSystem`
- impact activation
- ragdoll duration
- angular collapse
- damping
- recovery state
- renderer-independent `GuiSystem`
- GUI command buffer
- windows
- labels
- buttons
- checkboxes
- sliders
- progress bars
- text input
- separators
- spacing
- gameplay helper natives
- gameplay-system tests
- GUI command-buffer tests

---

## VEK 1.0.0

### Initial VEK programming language

The first standalone VEK release established the embeddable scripting-language runtime.

Initial features:

- `.vek` source files
- numbers
- strings
- booleans
- `nil`
- local variables with `let`
- assignment
- functions
- parameters
- `if`
- `else if`
- `else`
- `while`
- `break`
- `continue`
- `return`
- arithmetic operators
- comparison operators
- boolean operators
- string concatenation
- `#` and `//` comments
- native C++ function registration
- configurable sandbox limits
- sealable native-function registry
- command-line executable
- `run`
- `check`
- `eval`
- REPL
- version command
- CMake embedding target `VEK::Runtime`

VEK began as the Vehicle Engineering Kernel for the Custom Vehicle Game while remaining independent from raylib and suitable for embedding in other C++ programs.

---

## How to update this file

For each future VEK release, add a new section at the top using:

```text
## VEK X.Y.Z

### Short release title

New features:
- feature
- feature
- feature
```

Do not create separate `CHANGELOG_*.md`, `UPDATE_GITHUB_*.md` or feature-specific Markdown files. Keep VEK documentation tidy by maintaining only:

- `README.md`
- `Versions.md`
## VEK 2.5.0

- Added renderer-independent `UiOrbit3D` projection/rotation helpers.
- Added deterministic `RunnerMiniGame` physics for interactive installer/editor backgrounds.
- Added semantic-version comparison and automatic/manual update policy primitives.
- Rebuilt the GitHub installer around a rotating 3D VEK scene, three download modes, playable jump runner, responsive 3D loading visual, and accurate always-visible update status.
- Installer can automatically update an outdated managed VEK clone when Auto Update is enabled, or wait for manual action when Manual Update is selected.

## v2.6.0
- Door manipulation and interaction examples
- Day/night cycle scripting examples
- Developer cheat-panel example flow
- Installer/runtime safety refinements
- Interactive UI/runtime iteration
- Secondary-motion and character-system groundwork for richer rigs
