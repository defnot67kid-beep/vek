#include <VekScriptEngine.h>
#include <VekSpacecraftSystems.h>
#include <cassert>
#include <cmath>

static bool near(double a, double b, double eps) { return std::fabs(a - b) <= eps; }

int main() {
    // --- Resources & fuel lines ------------------------------------------------
    vek::ResourceContainerRegistry resources;
    vek::PartConnectionGraph fuelLines;

    vek::ResourceContainerDefinition tank1;
    tank1.id = "tank1.lf"; tank1.partId = "tank1"; tank1.type = vek::ResourceType::LiquidFuel;
    tank1.capacity = 100; tank1.amount = 100;
    assert(resources.Register(tank1));

    vek::ResourceContainerDefinition tank2;
    tank2.id = "tank2.lf"; tank2.partId = "tank2"; tank2.type = vek::ResourceType::LiquidFuel;
    tank2.capacity = 100; tank2.amount = 100;
    assert(resources.Register(tank2));

    vek::ResourceContainerDefinition engineTank;
    engineTank.id = "engine.lf"; engineTank.partId = "engine1"; engineTank.type = vek::ResourceType::LiquidFuel;
    engineTank.capacity = 0; engineTank.amount = 0;
    assert(resources.Register(engineTank));

    // Without a fuel line, only the engine's own part is reachable.
    assert(resources.Withdraw("engine1", vek::ResourceType::LiquidFuel, 50.0f, fuelLines) == 0.0f);

    vek::PartConnectionDefinition line1;
    line1.id = "line.engine-tank1"; line1.partA = "engine1"; line1.partB = "tank1"; line1.kind = "fuel_line";
    assert(fuelLines.Connect(line1));
    vek::PartConnectionDefinition line2;
    line2.id = "line.tank1-tank2"; line2.partA = "tank1"; line2.partB = "tank2"; line2.kind = "fuel_line";
    assert(fuelLines.Connect(line2));

    float drawn = resources.Withdraw("engine1", vek::ResourceType::LiquidFuel, 150.0f, fuelLines);
    assert(near(drawn, 150.0f, 1e-3));
    assert(near(resources.TotalOfType("engine1", vek::ResourceType::LiquidFuel, fuelLines), 50.0f, 1e-2));

    assert(resources.Transfer("tank2.lf", "tank1.lf", 10.0f));
    assert(resources.Register(tank1) == false); // duplicate id rejected

    // --- Engines ---------------------------------------------------------------
    vek::EngineRegistry engines;
    vek::EngineDefinition eng;
    eng.id = "engine1.def"; eng.partId = "engine1";
    eng.thrustVacuumKn = 200.0f; eng.thrustAtmosphereKn = 170.0f;
    eng.ispVacuumSeconds = 320.0f; eng.ispAtmosphereSeconds = 280.0f;
    eng.primaryResource = vek::ResourceType::LiquidFuel; eng.secondaryResource = vek::ResourceType::Oxidizer;
    eng.mixtureRatio = 0.45f;
    assert(engines.Register(eng));

    assert(engines.SetIgnited("engine1.def", true));
    assert(engines.SetThrottle("engine1.def", 1.0f));
    assert(near(engines.ThrustAt("engine1.def", 0.0f), 200.0f, 1e-3));
    assert(near(engines.ThrustAt("engine1.def", 1.0f), 170.0f, 1e-3));

    auto out = engines.Step("engine1.def", 0.0f, 1.0f);
    assert(out.valid);
    // massFlow = thrust(N) / (isp*g0) = 200000 / (320*9.80665)
    double expectedFlow = 200000.0 / (320.0 * 9.80665);
    assert(near(out.massFlowKgPerSecond, expectedFlow, 0.5));
    assert(near(out.primaryResourceUnits + out.secondaryResourceUnits, expectedFlow, 0.5));

    assert(engines.SetIgnited("engine1.def", false));
    assert(near(engines.ThrustAt("engine1.def", 0.0f), 0.0, 1e-6)); // unignited engine makes no thrust

    // --- Decouplers & staging ---------------------------------------------------
    vek::DecouplerRegistry decouplers;
    vek::DecouplerDefinition dec;
    dec.id = "decoupler1"; dec.partId = "stack1"; dec.ejectionForce = 300.0f; dec.staged = true;
    assert(decouplers.Register(dec));

    vek::StagingSequencer staging;
    vek::StageDefinition stage1; stage1.index = 1; stage1.engineIds = {"engine1.def"};
    assert(staging.AddStage(stage1));
    vek::StageDefinition stage0; stage0.index = 0; stage0.decouplerIds = {"decoupler1"};
    assert(staging.AddStage(stage0));
    assert(staging.CurrentStage() == 1); // highest index activates first, like KSP

    auto result1 = staging.Activate(engines, decouplers);
    assert(result1.valid && result1.stageIndex == 1);
    assert(result1.ignitedEngineIds.size() == 1);
    assert(engines.Find("engine1.def")->ignited);
    assert(staging.CurrentStage() == 0);

    auto result0 = staging.Activate(engines, decouplers);
    assert(result0.valid && result0.stageIndex == 0);
    assert(result0.firedDecouplerIds.size() == 1);
    assert(decouplers.Find("decoupler1")->fired);
    assert(staging.CurrentStage() == -1);

    auto resultPastEnd = staging.Activate(engines, decouplers);
    assert(!resultPastEnd.valid);

    // --- Symmetry ----------------------------------------------------------------
    vek::SymmetryGroupRegistry symmetry;
    vek::SymmetryGroupDefinition radial;
    radial.id = "boosters"; radial.originPartId = "core"; radial.mode = vek::SymmetryMode::Radial;
    radial.count = 4; radial.axis = {0, 1, 0};
    assert(symmetry.CreateGroup(radial));
    auto placements = symmetry.ComputePlacements("boosters", {2.0f, 0.0f, 0.0f});
    assert(placements.size() == 4);
    assert(near(placements[0].x, 2.0, 1e-4));
    assert(near(placements[2].x, -2.0, 1e-3)); // opposite side of the ring at 180 degrees
    assert(near(placements[1].z, -2.0, 1e-3) || near(placements[1].z, 2.0, 1e-3));

    vek::SymmetryGroupDefinition mirror;
    mirror.id = "wings"; mirror.originPartId = "fuselage"; mirror.mode = vek::SymmetryMode::Mirror; mirror.axis = {1, 0, 0};
    assert(symmetry.CreateGroup(mirror));
    auto mirrored = symmetry.ComputePlacements("wings", {3.0f, 1.0f, 0.0f});
    assert(mirrored.size() == 2);
    assert(near(mirrored[1].x, -3.0, 1e-3));
    assert(near(mirrored[1].y, 1.0, 1e-3));

    // --- Orbital mechanics ---------------------------------------------------------
    const double muKerbin = 3.5316e12; // Kerbin's mu, m^3/s^2 (KSP's homeworld)
    double circularV = vek::OrbitalMechanics::CircularVelocity(700000.0, muKerbin); // 700km radius circular orbit
    assert(circularV > 2000.0 && circularV < 2400.0);

    vek::OrbitalState state;
    state.radiusMeters = 700000.0;
    state.speedMetersPerSecond = circularV;
    state.flightPathAngleRadians = 0.0;
    auto orbit = vek::OrbitalMechanics::DescribeOrbit(state, muKerbin);
    assert(orbit.valid && !orbit.hyperbolic);
    assert(near(orbit.eccentricity, 0.0, 1e-3)); // circular orbit -> eccentricity ~0
    assert(near(orbit.apoapsisMeters, 700000.0, 1000.0));
    assert(near(orbit.periapsisMeters, 700000.0, 1000.0));

    double period = vek::OrbitalMechanics::OrbitalPeriod(700000.0, muKerbin);
    assert(period > 0.0 && near(orbit.periodSeconds, period, 1.0));

    double hohmann = vek::OrbitalMechanics::HohmannTransferDeltaV(700000.0, 3500000.0, muKerbin);
    assert(hohmann > 0.0);

    // --- Rocket equation --------------------------------------------------------
    double dv = vek::OrbitalMechanics::TsiolkovskyDeltaV(320.0, 10000.0, 4000.0);
    double expectedDv = 320.0 * 9.80665 * std::log(10000.0 / 4000.0);
    assert(near(dv, expectedDv, 1e-6));

    double propMass = vek::OrbitalMechanics::PropellantMassForDeltaV(320.0, dv, 4000.0);
    assert(near(propMass, 6000.0, 1.0)); // round-trips back to the original propellant mass

    std::vector<vek::OrbitalMechanics::StageMassSpec> stages = {
        {300.0, 20000.0, 8000.0},
        {320.0, 8000.0, 3000.0},
    };
    double staged = vek::OrbitalMechanics::TotalStagedDeltaV(stages);
    double expectedStaged = vek::OrbitalMechanics::TsiolkovskyDeltaV(300.0, 20000.0, 8000.0)
                           + vek::OrbitalMechanics::TsiolkovskyDeltaV(320.0, 8000.0, 3000.0);
    assert(near(staged, expectedStaged, 1e-6));

    // --- Script-facing natives, wired through the C ABI runtime pattern ----------
    VekScriptEngine vm;
    VekRegisterStandardLibrary(vm);
    vek::ResourceContainerRegistry vmResources;
    vek::EngineRegistry vmEngines;
    vek::DecouplerRegistry vmDecouplers;
    vek::StagingSequencer vmStaging;
    vek::SymmetryGroupRegistry vmSymmetry;
    vek::VekRegisterSpacecraftLibrary(vm, &vmResources, &vmEngines, &vmDecouplers, &vmStaging, &vmSymmetry);

    bool loaded = vm.LoadSource(R"(
        fn setup() {
            engine_register({
                id: "main.engine",
                part_id: "core",
                thrust_vacuum_kn: 200,
                isp_vacuum_seconds: 320
            });
            engine_set_ignited("main.engine", true);
            engine_set_throttle("main.engine", 1);
            return rocket_delta_v(320, 10000, 4000);
        }
    )");
    assert(loaded);
    auto scriptResult = vm.Call("setup");
    assert(near(scriptResult.AsNumber(), expectedDv, 1e-3));
    assert(vmEngines.Find("main.engine") != nullptr);

    bool loadedThrust = vm.LoadSource(R"(
        fn check_thrust() { return engine_thrust_at("main.engine", 0); }
    )");
    assert(loadedThrust);
    assert(near(vm.Call("check_thrust").AsNumber(), 200.0, 1e-3));

    return 0;
}
