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
