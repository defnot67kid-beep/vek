# VEK 3.5 — Spacecraft Systems (KSP-inspired)

VEK 3.5 adds `VekSpacecraftSystems` (`include/vek/VekSpacecraftSystems.h`,
`src/VekSpacecraftSystems.cpp`): resources/fuel tanks, engines with
vacuum/atmosphere ISP, decouplers, staging, radial/mirror symmetry, and a
small set of pure-math orbital mechanics helpers — the parts of a
Kerbal-Space-Program-style build/fly loop that sit above VEK's existing
part/motor/physics descriptors. Everything here is an engine-neutral
descriptor or pure math, same contract as the rest of VEK: a host renders and
simulates it with its own physics/graphics backend.

## Resources & fuel lines

`ResourceContainerDefinition` is a tank on a part (`liquid_fuel`, `oxidizer`,
`mono_propellant`, `solid_fuel`, `electric_charge`, `xenon`, `ore`, or a
custom type). `ResourceContainerRegistry::Withdraw` draws resources from
every reachable, flow-enabled tank of the right type, walking `"fuel_line"`
edges in a [[VekMotorSystems]] `PartConnectionGraph` — the same graph used for
motor wiring and part linkage — so crossfeed matches KSP's model: tanks only
share fuel across an explicit fuel line (or being the same part), not just by
touching.

```
resource_container_register({ id: "tank1.lf", part_id: "tank1", type: "liquid_fuel", capacity: 100, amount: 100 })
part_connect({ id: "line1", part_a: "engine1", part_b: "tank1", kind: "fuel_line" })
resource_transfer("tank2.lf", "tank1.lf", 10)   -- direct tank-to-tank transfer
```

## Engines

`EngineDefinition` carries vacuum and atmosphere thrust/ISP figures, a
primary/secondary resource pair (e.g. liquid fuel + oxidizer) split by
`mixture_ratio`, throttle limits (`min_throttle=1.0` for a solid booster that
can't be throttled down), and gimbal range. `EngineRegistry::ThrustAt` /
`IspAt` linearly interpolate between the vacuum and atmosphere ratings by an
`atmospheres` parameter (0 = vacuum, 1 = sea level) — the same simplification
stock KSP uses for its thrust display. `Step()` turns that into a per-tick
`EngineOutput` (thrust, mass flow, resource units to withdraw) without
touching a physics backend directly.

```
engine_register({ id: "mainsail", part_id: "core", thrust_vacuum_kn: 1379, thrust_atmosphere_kn: 1181, isp_vacuum_seconds: 310, isp_atmosphere_seconds: 280 })
engine_set_ignited("mainsail", true)
engine_set_throttle("mainsail", 1.0)
engine_step("mainsail", 0.0, dt)   -- -> { thrust_kn, mass_flow_kg_per_second, primary_resource_units, secondary_resource_units }
```

## Decouplers & staging

`DecouplerDefinition` + `DecouplerRegistry::Fire` model a one-shot
separation. `StagingSequencer` groups engine ids to ignite and decoupler ids
to fire into indexed `StageDefinition`s and counts *down* from the highest
stage index on `Activate()`, exactly like KSP's staging stack — the first
`Activate()` call fires the topmost stage, and each call afterward moves to
the next one down until there are none left.

```
stage_add({ index: 1, engine_ids: ["mainsail"] })
stage_add({ index: 0, decoupler_ids: ["decoupler1"] })
stage_activate()   -- fires stage 1 first (ignites the engine), then stage 0 (decouples)
```

## Symmetry

`SymmetryGroupRegistry` computes placement offsets for radial (N-way, around
an axis) or mirror (2-way, across a plane) part symmetry — the same "place
one, get N" workflow as KSP's editor symmetry tool. `ComputePlacements`
returns one `PhysicsVec3` per instance (including the original) so a host
editor can spawn/move the mirrored or radial copies.

```
symmetry_group_create({ id: "boosters", origin_part_id: "core", mode: "radial", count: 4, axis: {x:0,y:1,z:0} })
symmetry_group_placements("boosters", {x:2,y:0,z:0})   -- -> 4 positions evenly spaced around the ring
```

## Orbital mechanics

`OrbitalMechanics` is a stateless set of static functions:

- `DescribeOrbit(state, mu)` — from radius/speed/flight-path-angle, returns
  semi-major axis, eccentricity, apoapsis, periapsis and period.
- `CircularVelocity`, `OrbitalPeriod` (Kepler's third law), `VisViva`.
- `CircularizationDeltaV` — burn at apoapsis to circularize.
- `HohmannTransferDeltaV` — two-burn transfer between circular orbits.
- `TsiolkovskyDeltaV(isp, wetMass, dryMass)` / `PropellantMassForDeltaV` —
  the rocket equation, both directions.
- `TotalStagedDeltaV(stages)` — sums per-stage delta-v for a multi-stage
  vehicle, the number KSP's stock delta-v readout shows per stage and total.

```
rocket_delta_v(320, 10000, 4000)                 -- Tsiolkovsky, isp/wet/dry
orbit_circularization_delta_v(700000, 3500000, mu)
orbit_hohmann_delta_v(700000, 3500000, mu)
rocket_total_staged_delta_v([{ isp_seconds: 300, wet_mass_kg: 20000, dry_mass_kg: 8000 }, ...])
```

## Multi-language reach

`VekRegisterSpacecraftLibrary(engine, &resources, &engines, &decouplers,
&staging, &symmetry)` registers every native in one call, and the C ABI
runtime (`vek_c.cpp`) now owns one of each registry per `vek_runtime` and
wires them in automatically — so every existing language binding (C#, Go,
Java, Node.js, Python, Rust) gets `engine_*`, `resource_*`, `decoupler_*`,
`stage_*`, `symmetry_group_*`, `orbit_*` and `rocket_*` natives with no
per-language code changes, the same pattern used for motors/wiring in 3.4.

## Tests

`tests/spacecraft_systems_tests.cpp` covers fuel-line crossfeed, engine
thrust/ISP interpolation and mass flow, staging order, symmetry placement
math, orbital mechanics (including a round-trip through the rocket equation),
and the script-native surface end-to-end. Wired into `CMakeLists.txt` as
`vek_spacecraft_v35`.
