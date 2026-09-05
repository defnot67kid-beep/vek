#include <vek/VekAerodynamics.h>

#include <algorithm>
#include <cmath>

namespace vek::mechanics {

float AirDensityAtAltitude(const AtmosphereModel& model, float altitudeMeters) {
    if (altitudeMeters <= 0.0f) return model.seaLevelDensity;
    if (altitudeMeters >= model.topOfAtmosphereMeters) return 0.0f;
    return model.seaLevelDensity * std::exp(-altitudeMeters / std::max(model.scaleHeightMeters, 1.0f));
}

namespace {
// Cheap deterministic hash -> [0,1). Not cryptographic, just decorrelated
// enough for turbulence octaves.
float Hash01(float x) {
    float s = std::sin(x * 127.1f) * 43758.5453f;
    return s - std::floor(s);
}

float ValueNoise1D(float t) {
    float i = std::floor(t);
    float f = t - i;
    float a = Hash01(i);
    float b = Hash01(i + 1.0f);
    float u = f * f * (3.0f - 2.0f * f); // smoothstep
    return a + (b - a) * u;
}
} // namespace

PhysicsVec3 SampleWind(const WindField& field, PhysicsVec3 position, float timeSeconds) {
    PhysicsVec3 dir = Normalize(field.baseDirection);
    PhysicsVec3 steady = Scale(dir, field.baseSpeed);

    float phase = static_cast<float>(field.seed) * 17.0f +
                  timeSeconds * field.gustFrequency +
                  (position.x + position.z) * 0.001f;
    float gustX = (ValueNoise1D(phase) - 0.5f) * 2.0f;
    float gustZ = (ValueNoise1D(phase + 91.7f) - 0.5f) * 2.0f;
    float gustY = (ValueNoise1D(phase + 233.3f) - 0.5f) * 2.0f * field.verticalTurbulence;

    PhysicsVec3 gust{gustX * field.gustStrength, gustY * field.gustStrength, gustZ * field.gustStrength};
    return Add(steady, gust);
}

AeroForceResult ComputeAeroForces(const AeroSurface& surface,
                                   PhysicsVec3 velocity,
                                   PhysicsVec3 forward,
                                   PhysicsVec3 up,
                                   float airDensity) {
    AeroForceResult result;
    float speed = Length(velocity);
    if (speed < 1e-4f || airDensity <= 0.0f) return result;

    PhysicsVec3 flowDir = Scale(velocity, -1.0f / speed); // direction air comes from
    float cosAoA = std::clamp(Dot(Normalize(forward), Normalize(velocity)) * -1.0f, -1.0f, 1.0f);
    float angleRad = std::acos(cosAoA) - static_cast<float>(M_PI) / 2.0f;
    // Signed angle of attack via the up axis component of velocity relative to forward.
    float verticalComponent = Dot(velocity, up);
    float forwardComponent = Dot(velocity, forward);
    float aoaRad = std::atan2(-verticalComponent, std::max(std::abs(forwardComponent), 1e-4f));
    float aoaDeg = aoaRad * 180.0f / static_cast<float>(M_PI);

    float zeroLiftRad = surface.zeroLiftAngleDegrees * static_cast<float>(M_PI) / 180.0f;
    float stallDeg = surface.stallAngleDegrees;
    result.stalled = std::abs(aoaDeg) > stallDeg;

    float effectiveAoaRad = aoaRad - zeroLiftRad;
    float liftCoefficient = result.stalled
        ? surface.liftSlope * (stallDeg * static_cast<float>(M_PI) / 180.0f) *
              (aoaDeg > 0.0f ? 1.0f : -1.0f) * 0.4f // post-stall lift collapse
        : surface.liftSlope * effectiveAoaRad;

    float dynamicPressure = 0.5f * airDensity * speed * speed;
    float liftMag = dynamicPressure * surface.referenceArea * liftCoefficient;
    float dragCoefficient = surface.dragCoefficient + surface.inducedDrag * liftCoefficient * liftCoefficient;
    float dragMag = dynamicPressure * surface.referenceArea * dragCoefficient;

    PhysicsVec3 dragDir = Scale(velocity, -1.0f / speed);
    // Lift acts perpendicular to velocity, in the plane containing "up".
    PhysicsVec3 velNorm = Scale(velocity, 1.0f / speed);
    PhysicsVec3 liftDir = Sub(up, Scale(velNorm, Dot(up, velNorm)));
    float liftDirLen = Length(liftDir);
    liftDir = (liftDirLen > 1e-5f) ? Scale(liftDir, 1.0f / liftDirLen) : PhysicsVec3{0.0f, 1.0f, 0.0f};

    result.drag = Scale(dragDir, dragMag);
    result.lift = Scale(liftDir, liftMag);
    result.angleOfAttackDegrees = aoaDeg;
    (void)flowDir;
    (void)cosAoA;
    (void)angleRad;
    return result;
}

void StepRocketAscent(RocketAscentState& state,
                       std::vector<RocketStageConfig>& stages,
                       const AtmosphereModel& atmosphere,
                       const WindField& wind,
                       const AeroSurface& airframe,
                       float gravity,
                       float dt) {
    if (dt <= 0.0f || state.stagedOut || state.activeStage >= static_cast<int>(stages.size())) return;

    RocketStageConfig& stage = stages[state.activeStage];
    float propellantMass = stage.wetMassKg - stage.dryMassKg;
    float propellantRemaining = std::max(propellantMass - stage.burnedFuelKg, 0.0f);

    float throttle = std::clamp(state.throttle, 0.0f, 1.0f);
    float g0 = 9.80665f;
    float massFlowRate = (stage.ispSeconds > 1e-3f)
        ? (stage.thrustNewtons * throttle) / (stage.ispSeconds * g0)
        : 0.0f;
    float fuelThisStep = std::min(massFlowRate * dt, propellantRemaining);
    float actualThrust = (massFlowRate > 1e-6f) ? stage.thrustNewtons * throttle * (fuelThisStep / (massFlowRate * dt)) : 0.0f;
    if (fuelThisStep <= 0.0f) actualThrust = 0.0f;

    float currentMass = stage.wetMassKg - stage.burnedFuelKg;
    currentMass = std::max(currentMass, stage.dryMassKg);

    PhysicsVec3 thrustForce = Scale(Normalize(state.facing), actualThrust);

    PhysicsVec3 windVelocity = SampleWind(wind, state.position, state.elapsed);
    PhysicsVec3 airVelocity = Sub(state.velocity, windVelocity);
    float altitude = std::max(state.position.y, 0.0f);
    float density = AirDensityAtAltitude(atmosphere, altitude);
    PhysicsVec3 up = Normalize(state.facing);
    AeroForceResult aero = ComputeAeroForces(airframe, airVelocity, up, up, density);

    PhysicsVec3 gravityForce{0.0f, -gravity * currentMass, 0.0f};
    PhysicsVec3 totalForce = Add(Add(thrustForce, gravityForce), Add(aero.lift, aero.drag));
    PhysicsVec3 acceleration = Scale(totalForce, 1.0f / std::max(currentMass, 1.0f));

    state.velocity = Add(state.velocity, Scale(acceleration, dt));
    state.position = Add(state.position, Scale(state.velocity, dt));
    state.elapsed += dt;
    stage.burnedFuelKg += fuelThisStep;

    if (stage.burnedFuelKg >= propellantMass - 1e-6f) {
        state.activeStage += 1;
        if (state.activeStage >= static_cast<int>(stages.size())) state.stagedOut = true;
    }
}

} // namespace vek::mechanics
