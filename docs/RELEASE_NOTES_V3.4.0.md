# VEK 3.4.0 — Motor Binding & Part Wiring

VEK 3.4.0 adds a new engine-neutral module, `VekMotorSystems`, for driving
physics motors from input and for describing how parts are wired/connected to
each other. See `docs/MOTOR_WIRING_V3.4.md` for the full writeup.

## Motor binding
- New `MotorBindingDefinition` / `MotorBindingRegistry`: bind a motor to a
  keybind, an analog axis, a local script function, or a network/remote
  server-authoritative command (via the existing `AuthorityActionRegistry`).
- Runtime rebinding (`Rebind` / `motor_binding_rebind`) for "press any key"
  settings screens.
- `Evaluate` / `motor_binding_evaluate` turns a raw input sample into a
  concrete `MotorCommand` (target velocity, active state, source, script
  scope) without touching a physics backend directly.
- Script natives: `motor_binding_register`, `motor_binding_unregister`,
  `motor_binding_exists`, `motor_binding_get`, `motor_binding_rebind`,
  `motor_bindings_for_motor`, `motor_bindings_for_key`,
  `motor_binding_evaluate`, `motor_binding_count`.

## Part connection graph
- New `PartConnectionDefinition` / `PartConnectionGraph` for describing
  joint/weld/wire/socket/motor links between parts, independent of the
  physics solver, so an editor or HUD can draw the linkage/wiring graph.
- Order-independent connection queries (`AreConnected`,
  `ConnectionsForPart`, `All`) for host-side visualization.
- Script natives: `part_connect`, `part_disconnect`,
  `part_disconnect_parts`, `part_connections_for`, `part_connections_all`,
  `parts_are_connected`, `part_connection_count`.

## Multi-language reach
- `VekRegisterMotorLibrary(engine, &motors, &graph)` registers both
  registries' natives in one call, mirroring `VekRegisterAuthorityLibrary`.
- The C ABI runtime (`vek_c.cpp`) now owns a `MotorBindingRegistry` and a
  `PartConnectionGraph` per `vek_runtime` and wires them in automatically, so
  every existing language binding (C#, Go, Java, Node.js, Python, Rust) gets
  the new natives with no per-language code changes.

## Tests
- Added `tests/motor_systems_tests.cpp`, wired into `CMakeLists.txt` as the
  `vek_motor_wiring_v34` test target.

## Compatibility
- Purely additive: no existing header, native, or `.vek` language semantics
  changed. Existing `JointDefinition`/`MotorDefinition` physics descriptors
  are unchanged; `PartConnectionDefinition` optionally references their ids
  but does not require a physics joint to exist.
