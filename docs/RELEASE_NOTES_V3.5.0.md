# VEK 3.5.0 — Spacecraft Systems (KSP-inspired)

VEK 3.5.0 adds `VekSpacecraftSystems`, a Kerbal-Space-Program-flavored layer
of resources, engines, staging, symmetry and orbital mechanics on top of the
existing part/motor/wiring foundations. See
`docs/SPACECRAFT_SYSTEMS_V3.5.md` for the full writeup.

## Resources & fuel lines
- `ResourceContainerDefinition` / `ResourceContainerRegistry`: typed tanks
  (`liquid_fuel`, `oxidizer`, `mono_propellant`, `solid_fuel`,
  `electric_charge`, `xenon`, `ore`, custom) with capacity/amount.
- `Withdraw` draws proportionally across every reachable, flow-enabled tank
  of the right type, walking `"fuel_line"`-kind edges in the 3.4
  `PartConnectionGraph` — crossfeed only happens across an explicit fuel
  line (or within the same part), matching KSP's model.
- Direct tank-to-tank `Transfer`.

## Engines
- `EngineDefinition`: vacuum/atmosphere thrust and ISP, primary/secondary
  resource mixture ratio, gimbal range, throttle limits (including
  non-throttleable solids via `min_throttle`).
- `ThrustAt` / `IspAt` linearly interpolate by atmospheric pressure; `Step`
  produces a per-tick `EngineOutput` (thrust, mass flow, resource units) via
  the rocket mass-flow relation `F = isp * g0 * mdot`.

## Decouplers & staging
- `DecouplerDefinition` / `DecouplerRegistry::Fire` for one-shot separation.
- `StagingSequencer`: indexed stages of engine/decoupler ids, activating
  from the highest index down — the same order KSP's staging stack uses.

## Symmetry
- `SymmetryGroupRegistry`: radial (N-way) and mirror (2-way) part symmetry.
  `ComputePlacements` returns the offset for every instance in the group
  using a proper axis-angle rotation (Rodrigues' formula) or plane
  reflection.

## Orbital mechanics
- `OrbitalMechanics`: `DescribeOrbit` (semi-major axis, eccentricity,
  apoapsis/periapsis/period from radius+speed+flight-path-angle),
  `CircularVelocity`, `OrbitalPeriod`, `VisViva`,
  `CircularizationDeltaV`, `HohmannTransferDeltaV`.
- `TsiolkovskyDeltaV` / `PropellantMassForDeltaV`: the rocket equation, both
  directions, plus `TotalStagedDeltaV` for a multi-stage vehicle's total
  delta-v budget (the number KSP's stock delta-v readout shows).

## Multi-language reach
- `VekRegisterSpacecraftLibrary(...)` registers every native in one call.
  The C ABI runtime (`vek_c.cpp`) now owns one of each registry per
  `vek_runtime`, so every existing language binding (C#, Go, Java, Node.js,
  Python, Rust) gets the new natives automatically — no per-language code
  changes, same pattern as the 3.4 motor/wiring release.

## Tests
- Added `tests/spacecraft_systems_tests.cpp`, wired into `CMakeLists.txt` as
  the `vek_spacecraft_v35` test target.

## Compatibility
- Purely additive: no existing header, native, or `.vek` language semantics
  changed. `PartConnectionGraph::Connect` now also accepts a `"fuel_line"`
  connection kind alongside the existing `joint`/`weld`/`wire`/`socket`/
  `motor` kinds introduced in 3.4; all previously-accepted kinds are
  unaffected.
