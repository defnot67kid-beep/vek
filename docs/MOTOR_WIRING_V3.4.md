# VEK 3.4 — Motor Binding & Part Wiring

VEK 3.4 adds `VekMotorSystems` (`include/vek/VekMotorSystems.h`,
`src/VekMotorSystems.cpp`), which answers three things the physics joint/motor
descriptors in `VekPhysicsSystems.h` don't cover on their own:

1. **What is currently driving a motor** — a keybind, an analog axis, a local
   script call, or a server-authoritative remote command.
2. **Which parts are wired/connected to which other parts**, so an editor or
   in-game HUD can draw the same kind of linkage graph a node-based rigging
   tool would show.
3. **How other languages reach this** — every operation is exposed as script
   natives, and the existing C ABI (`vek_c.h`) already forwards those natives
   to every language binding VEK ships (C#, Go, Java, Node.js, Python, Rust).
   There is no new per-language glue: bind once in `.vek`/native code, call it
   from any binding that can already load a `vek_runtime`.

## Motor bindings

`MotorBindingDefinition` describes one input source bound to one motor
(identified by the same id used in a physics `JointDefinition`'s motor). A
motor can have several bindings at once — e.g. a discrete keybind nudge and a
continuous analog axis — each with its own id so a settings screen can list,
rebind, or remove them independently.

Sources:

| `source`  | Meaning                                                             |
|-----------|----------------------------------------------------------------------|
| `keybind` | Digital key/button press. `key_code` identifies the physical input.  |
| `axis`    | Analog input (stick, trigger, mouse axis). `axis_id` names the axis. |
| `script`  | A local or remote script function drives the motor programmatically. |
| `network` | A server-authoritative remote command, routed through the existing `AuthorityActionRegistry` (`remote_action_id`), so a dedicated/listen server script remains the source of truth instead of trusting the client value. |

`script_scope` (`local` / `remote_server`) records *where* a script-sourced
binding's decision is made, independent of transport — a `local` script
binding calls a function already loaded into this engine instance; a
`remote_server` one is expected to be settled by a server script and arrive
back as a `network`-sourced command.

```
motor_binding_register({
    id: "throttle.key",
    motor_id: "engine.motor",
    source: "keybind",
    key_code: "KeyW",
    invert: 1
})
motor_binding_register({
    id: "throttle.remote",
    motor_id: "engine.motor",
    source: "network",
    remote_action_id: "vehicle.set_throttle",
    script_scope: "remote_server"
})

motor_binding_rebind("throttle.key", "KeyUp")     -- runtime rebinding
motor_binding_evaluate("throttle.key", 0, true)   -- -> { target_velocity, active, ... }
motor_bindings_for_motor("engine.motor")          -- -> array of binding maps
```

C++ natives mirror this 1:1 (`MotorBindingRegistry::Register`, `Rebind`,
`Evaluate`, `BindingsForMotor`, `BindingsForKey`, ...). `Evaluate` returns a
`MotorCommand` — the concrete target velocity/active state a physics backend
or the authority layer should apply — without touching a solver directly, the
same "engine-neutral descriptor" contract the rest of `VekPhysicsSystems.h`
uses.

## Part connection graph (wiring visualization)

`PartConnectionGraph` records edges between two part ids: a physical joint, a
weld, a "wire" (signal/logical link with no physics backing, e.g. a remote
controller wired to a motor), a socket, a motor link, or (as of VEK 3.5) a
"fuel_line" used by `VekSpacecraftSystems` resource crossfeed. Connections are
independent of the physics solver — a connection can describe pure wiring for
an editor to draw, and can also carry a `joint_id`/`motor_id` so the same
graph backs both the solver and the visualization.

```
part_connect({ id: "chassis-wheel", part_a: "chassis", part_b: "wheel.left", kind: "joint", joint_id: "wheel.left.joint" })
part_connect({ id: "remote-motor",  part_a: "remote.controller", part_b: "wheel.left.motor", kind: "wire", label: "throttle signal" })

parts_are_connected("chassis", "wheel.left")   -- true, order-independent
part_connections_for("wheel.left.motor")       -- -> array of connection maps for an editor to draw
part_connections_all()                          -- -> the whole graph, for a full wiring view
```

A host editor/HUD renders `part_connections_all()` (or a per-part subset) as
lines/edges between the corresponding part visuals; `visible=false` lets a
connection exist in the graph without being drawn.

## Multi-language reach

Both registries call `RegisterNatives(VekScriptEngine&)`, and
`VekRegisterMotorLibrary(engine, &motors, &graph)` registers both in one call
— the same pattern `VekRegisterAuthorityLibrary` uses for the authority pair.
The C ABI runtime (`vek_c.cpp`) now owns one `MotorBindingRegistry` and one
`PartConnectionGraph` per `vek_runtime` and registers them automatically, so
every existing binding under `bindings/` (C#, Go, Java, Node, Python, Rust)
gets `motor_binding_*`/`part_*` natives through `vek_call`/`vek_load_source`
with no binding-specific code changes.

## Tests

`tests/motor_systems_tests.cpp` covers the native C++ API (registration,
duplicate rejection, rebinding, evaluation, connection graph queries) and the
script-native surface end-to-end through a loaded `.vek` script, matching the
existing test style (plain `assert`, no framework, wired into `CMakeLists.txt`
as `vek_motor_wiring_v34`).
