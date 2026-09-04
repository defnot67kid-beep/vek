#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <vek/VekMotorSystems.h>
#include <vek/VekPhysicsSystems.h>
#include <vek/VekScriptEngine.h>

class VekScriptEngine;

namespace vek {

// Spacecraft Systems (VEK 3.5) -----------------------------------------------
// A Kerbal-Space-Program-flavored layer on top of the existing part/motor/
// wiring foundations: resources and tanks, engines with vacuum/atmospheric
// ISP, decouplers, a staging sequence, radial/mirror symmetry for placing
// parts, and a small set of engine-neutral orbital mechanics helpers
// (Tsiolkovsky delta-v, orbital period, apoapsis/periapsis, circularization
// and Hohmann-transfer estimates). As with the rest of VEK, these are
// descriptors and pure math a host renders/simulates with its own solver —
// VEK does not own a physics world or a renderer.

// --- Resources ---------------------------------------------------------------

enum class ResourceType { LiquidFuel = 0, Oxidizer, MonoPropellant, SolidFuel, ElectricCharge, Xenon, Ore, Custom };

struct ResourceContainerDefinition {
    std::string id;
    std::string partId;                 // owning part, e.g. "fueltank.fl-t400"
    ResourceType type = ResourceType::LiquidFuel;
    std::string customTypeName;         // used when type == Custom
    float capacity = 0.0f;
    float amount = 0.0f;                // current amount, 0..capacity
    bool flowEnabled = true;
};

class ResourceContainerRegistry {
public:
    bool Register(const ResourceContainerDefinition& definition);
    bool RegisterValue(const VekValue& value, std::string* error = nullptr);
    const ResourceContainerDefinition* Find(const std::string& id) const;
    std::vector<const ResourceContainerDefinition*> ContainersForPart(const std::string& partId) const;
    std::vector<const ResourceContainerDefinition*> ContainersOfType(ResourceType type) const;

    // Draws `amount` split proportionally across every flow-enabled container
    // of `type` reachable from `fromPartId` through `fuelLines` ("fuel_line"
    // kind PartConnectionDefinition edges) plus containers on the part itself.
    // Returns how much was actually withdrawn (may be less than requested if
    // the reachable containers run dry).
    float Withdraw(const std::string& fromPartId, ResourceType type, float amount, const PartConnectionGraph& fuelLines);
    bool Transfer(const std::string& fromContainerId, const std::string& toContainerId, float amount);
    float TotalOfType(const std::string& partId, ResourceType type, const PartConnectionGraph& fuelLines) const;

    void Clear();
    std::size_t Size() const { return containers.size(); }
    void RegisterNatives(VekScriptEngine& engine);

private:
    std::vector<std::string> ReachablePartsWithFuel(const std::string& fromPartId, ResourceType type,
                                                      const PartConnectionGraph& fuelLines) const;
    std::unordered_map<std::string, ResourceContainerDefinition> containers;
    std::unordered_map<std::string, std::vector<std::string>> partIndex; // partId -> container ids
};

// --- Engines -------------------------------------------------------------

struct EngineDefinition {
    std::string id;
    std::string partId;
    float thrustVacuumKn = 0.0f;
    float thrustAtmosphereKn = 0.0f;   // at 1 atm; interpolated by Pressure()
    float ispVacuumSeconds = 0.0f;
    float ispAtmosphereSeconds = 0.0f;
    ResourceType primaryResource = ResourceType::LiquidFuel;
    ResourceType secondaryResource = ResourceType::Oxidizer; // ResourceType::Custom == "no secondary"
    float mixtureRatio = 0.9f;         // fraction of mass flow drawn from primary resource
    float gimbalRangeDegrees = 0.0f;
    bool throttleable = true;
    float minThrottle = 0.0f;          // 0..1, e.g. 1.0 for solid boosters
    bool ignited = false;
    float currentThrottle = 0.0f;      // 0..1
};

// Result of stepping an engine for one tick: what to withdraw from resources
// and what thrust/consumption a physics backend should apply.
struct EngineOutput {
    bool valid = false;
    float thrustKn = 0.0f;
    float massFlowKgPerSecond = 0.0f;
    float primaryResourceUnits = 0.0f;   // amount to withdraw this tick
    float secondaryResourceUnits = 0.0f;
};

class EngineRegistry {
public:
    bool Register(const EngineDefinition& definition);
    bool RegisterValue(const VekValue& value, std::string* error = nullptr);
    const EngineDefinition* Find(const std::string& id) const;
    EngineDefinition* Mutable(const std::string& id);

    bool SetThrottle(const std::string& id, float throttle01);
    bool SetIgnited(const std::string& id, bool ignited);

    // atmospheres: 0 = vacuum, 1 = sea level. Interpolates thrust/ISP linearly
    // between the vacuum and atmosphere-rated figures, which is the same
    // simplification stock KSP engines use for their thrust curve display.
    float ThrustAt(const std::string& id, float atmospheres) const;
    float IspAt(const std::string& id, float atmospheres) const;
    EngineOutput Step(const std::string& id, float atmospheres, float dt) const;

    void Clear();
    std::size_t Size() const { return engines.size(); }
    void RegisterNatives(VekScriptEngine& engine);

private:
    std::unordered_map<std::string, EngineDefinition> engines;
};

// --- Decouplers & staging --------------------------------------------------

struct DecouplerDefinition {
    std::string id;
    std::string partId;
    float ejectionForce = 250.0f;      // Newtons applied to the separated parts
    bool staged = true;                // fires automatically when its stage activates
    bool fired = false;
};

class DecouplerRegistry {
public:
    bool Register(const DecouplerDefinition& definition);
    bool RegisterValue(const VekValue& value, std::string* error = nullptr);
    const DecouplerDefinition* Find(const std::string& id) const;
    bool Fire(const std::string& id); // marks fired=true; false if unknown or already fired
    void Clear();
    std::size_t Size() const { return decouplers.size(); }
    void RegisterNatives(VekScriptEngine& engine);

private:
    std::unordered_map<std::string, DecouplerDefinition> decouplers;
};

// A stage groups engine ids to ignite and decoupler ids to fire together,
// same as a KSP stage: activating stage N ignites its engines, fires its
// staged decouplers, then the sequencer advances so the next Activate() call
// hits stage N-1 (stages count down, as in the game).
struct StageDefinition {
    int index = 0;
    std::vector<std::string> engineIds;
    std::vector<std::string> decouplerIds;
};

struct StageActivationResult {
    bool valid = false;
    int stageIndex = 0;
    std::vector<std::string> ignitedEngineIds;
    std::vector<std::string> firedDecouplerIds;
};

class StagingSequencer {
public:
    bool AddStage(const StageDefinition& stage);
    bool AddStageValue(const VekValue& value, std::string* error = nullptr);
    int CurrentStage() const { return currentIndex; }
    int StageCount() const { return (int)stages.size(); }
    const StageDefinition* Find(int index) const;

    // Ignites every engine and fires every staged decoupler in the current
    // stage via the supplied registries, then decrements to the next stage.
    // Returns what actually happened so a host can play effects/sound.
    StageActivationResult Activate(EngineRegistry& engines, DecouplerRegistry& decouplers);
    void Reset(); // rewinds to the highest stage index, as in "revert to launch"

    void Clear();
    void RegisterNatives(VekScriptEngine& engine, EngineRegistry* engineRegistry, DecouplerRegistry* decouplerRegistry);

private:
    std::unordered_map<int, StageDefinition> stages;
    int currentIndex = -1; // -1 until stages are added; set to the highest index
    bool started = false;
};

// --- Symmetry ---------------------------------------------------------------

enum class SymmetryMode { Mirror = 0, Radial = 1 };

struct SymmetryGroupDefinition {
    std::string id;
    std::string originPartId;          // the part the group is built around (e.g. a tank)
    SymmetryMode mode = SymmetryMode::Radial;
    int count = 2;                     // radial: total instances around the ring; mirror: always 2
    PhysicsVec3 axis{0.0f, 1.0f, 0.0f}; // radial symmetry axis / mirror plane normal
    std::vector<std::string> memberPartIds; // populated as members are placed
};

class SymmetryGroupRegistry {
public:
    bool CreateGroup(const SymmetryGroupDefinition& definition);
    bool CreateGroupValue(const VekValue& value, std::string* error = nullptr);
    const SymmetryGroupDefinition* Find(const std::string& id) const;
    bool AddMember(const std::string& groupId, const std::string& partId);

    // Computes the placement transforms for every symmetry instance given the
    // origin part's local offset from the group's origin. Radial symmetry
    // rotates `localOffset` evenly around `axis`; mirror symmetry reflects it
    // across the plane through the origin with normal `axis`. Returns one
    // PhysicsVec3 per instance (including the original at index 0).
    std::vector<PhysicsVec3> ComputePlacements(const std::string& groupId, PhysicsVec3 localOffset) const;

    void Clear();
    std::size_t Size() const { return groups.size(); }
    void RegisterNatives(VekScriptEngine& engine);

private:
    std::unordered_map<std::string, SymmetryGroupDefinition> groups;
};

// --- Orbital mechanics (pure math, no state) --------------------------------

struct OrbitalState {
    double radiusMeters = 0.0;        // distance from the body's center
    double speedMetersPerSecond = 0.0;
    double flightPathAngleRadians = 0.0; // angle between velocity and local horizontal
};

struct OrbitDescription {
    bool valid = false;
    double semiMajorAxisMeters = 0.0;
    double eccentricity = 0.0;
    double apoapsisMeters = 0.0;      // altitude-from-center at farthest point
    double periapsisMeters = 0.0;     // altitude-from-center at closest point
    double periodSeconds = 0.0;       // orbital period; 0 for hyperbolic/parabolic orbits
    bool hyperbolic = false;
};

class OrbitalMechanics {
public:
    // mu = standard gravitational parameter of the body (G * mass), m^3/s^2.
    static OrbitDescription DescribeOrbit(const OrbitalState& state, double mu);

    // Circular orbital velocity at a given radius.
    static double CircularVelocity(double radiusMeters, double mu);

    // Kepler's third law: T = 2*pi*sqrt(a^3/mu).
    static double OrbitalPeriod(double semiMajorAxisMeters, double mu);

    // Instantaneous orbital speed at radius r for an orbit with semi-major
    // axis a (vis-viva equation).
    static double VisViva(double radiusMeters, double semiMajorAxisMeters, double mu);

    // Delta-v to circularize an orbit whose apoapsis/periapsis are known,
    // burning at the apoapsis (the standard "circularize at Ap" maneuver).
    static double CircularizationDeltaV(double periapsisMeters, double apoapsisMeters, double mu);

    // Two-burn Hohmann transfer delta-v between two circular orbits.
    static double HohmannTransferDeltaV(double radiusFromMeters, double radiusToMeters, double mu);

    // Tsiolkovsky rocket equation: dv = isp * g0 * ln(m0/m1). g0 = 9.80665.
    static double TsiolkovskyDeltaV(double ispSeconds, double wetMassKg, double dryMassKg);

    // Mass of propellant needed for a target delta-v, inverse of the above.
    static double PropellantMassForDeltaV(double ispSeconds, double deltaVMetersPerSecond, double dryMassKg);

    // Total vacuum delta-v for a multi-stage vehicle: sums per-stage
    // Tsiolkovsky delta-v using each stage's own ISP and wet/dry mass. Stage
    // 0 is the first stage burned (bottom of the rocket, highest wet mass).
    struct StageMassSpec { double ispSeconds = 0.0; double wetMassKg = 0.0; double dryMassKg = 0.0; };
    static double TotalStagedDeltaV(const std::vector<StageMassSpec>& stages);
};

void VekRegisterSpacecraftLibrary(VekScriptEngine& engine, ResourceContainerRegistry* resources, EngineRegistry* engines,
                                   DecouplerRegistry* decouplers, StagingSequencer* staging, SymmetryGroupRegistry* symmetry);

} // namespace vek
