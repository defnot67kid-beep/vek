#pragma once
// VekAerodynamics — runnable wind/aero/rocket-ascent mechanics (VEK 3.6).
// VekPhysicsSystems.h ships AerodynamicsDefinition/WingDefinition as inert
// descriptors for hosts with their own solver. This module adds a small,
// dependency-free simulation layer VEK itself can run: a procedural 3D wind
// field, lift/drag/angle-of-attack force computation, an altitude-based air
// density model, and a simple multi-stage rocket ascent integrator (thrust,
// mass flow, gravity, drag, wind) built on the existing Tsiolkovsky/orbital
// math in VekSpacecraftSystems.h.

#include <string>
#include <vector>

#include <vek/VekPhysicsMechanics.h>

namespace vek::mechanics {

// --- Atmosphere ---------------------------------------------------------
// Simple exponential ("barometric") atmosphere model: density and pressure
// fall off with altitude toward zero past a scale height.
struct AtmosphereModel {
    float seaLevelDensity = 1.225f;      // kg/m^3
    float scaleHeightMeters = 8500.0f;   // e-folding altitude
    float topOfAtmosphereMeters = 70000.0f;
};

float AirDensityAtAltitude(const AtmosphereModel& model, float altitudeMeters);

// --- Procedural wind field ------------------------------------------------
// Layered sine/cosine "value noise" — deterministic, dependency-free, no
// lookup tables. Good enough for gameplay wind gusts/turbulence, not a
// physically simulated fluid.
struct WindField {
    PhysicsVec3 baseDirection{1.0f, 0.0f, 0.0f}; // normalized prevailing direction
    float baseSpeed = 5.0f;                       // m/s steady wind
    float gustStrength = 3.0f;                    // m/s added by turbulence
    float gustFrequency = 0.15f;                   // turbulence speed (Hz-ish)
    float verticalTurbulence = 0.5f;               // fraction of gust applied to Y
    unsigned int seed = 1;
};

// Samples the wind velocity at a world position/time. Deterministic for a
// given (position, time, seed) — safe to call every frame without state.
PhysicsVec3 SampleWind(const WindField& field, PhysicsVec3 position, float timeSeconds);

// --- Lift/drag ------------------------------------------------------------
struct AeroSurface {
    float referenceArea = 1.0f;
    float dragCoefficient = 0.02f;
    float liftSlope = 5.5f;             // lift coefficient per radian of AoA
    float zeroLiftAngleDegrees = 0.0f;
    float stallAngleDegrees = 15.0f;
    float inducedDrag = 0.08f;
};

struct AeroForceResult {
    PhysicsVec3 lift{};
    PhysicsVec3 drag{};
    float angleOfAttackDegrees = 0.0f;
    bool stalled = false;
};

// velocity: airspeed vector (world velocity minus wind). forward/up: the
// body's orientation basis vectors (should be unit length, orthogonal).
AeroForceResult ComputeAeroForces(const AeroSurface& surface,
                                   PhysicsVec3 velocity,
                                   PhysicsVec3 forward,
                                   PhysicsVec3 up,
                                   float airDensity);

// --- Rocket ascent ---------------------------------------------------------
struct RocketStageConfig {
    std::string id;
    float ispSeconds = 300.0f;
    float thrustNewtons = 0.0f;
    float wetMassKg = 1000.0f;
    float dryMassKg = 400.0f;
    float burnedFuelKg = 0.0f;      // running total, mutated by StepRocketAscent
};

struct RocketAscentState {
    PhysicsVec3 position{0.0f, 0.0f, 0.0f}; // y = altitude
    PhysicsVec3 velocity{};
    PhysicsVec3 facing{0.0f, 1.0f, 0.0f};   // unit vector, current thrust direction
    float throttle = 1.0f;                  // 0..1
    int activeStage = 0;
    float elapsed = 0.0f;
    bool stagedOut = false;                 // true once the last stage runs dry
};

// Advances one stage's Tsiolkovsky burn plus gravity/drag/wind for dt
// seconds. Consumes propellant from stages[state.activeStage]; when that
// stage's wet mass is exhausted it auto-advances to the next stage (a
// gameplay-friendly staging sequence, distinct from the descriptor-only
// StagingSequencer in VekSpacecraftSystems).
void StepRocketAscent(RocketAscentState& state,
                       std::vector<RocketStageConfig>& stages,
                       const AtmosphereModel& atmosphere,
                       const WindField& wind,
                       const AeroSurface& airframe,
                       float gravity,
                       float dt);

} // namespace vek::mechanics
