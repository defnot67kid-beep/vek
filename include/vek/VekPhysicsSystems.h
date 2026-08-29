#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <vek/VekScriptEngine.h>

namespace vek {

struct PhysicsVec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct SecondaryMotionProfile {
    std::string id = "secondary.default";
    float gravity = 7.0f;
    float damping = 0.90f;
    float stiffness = 0.35f;
    float airDrag = 0.08f;
    float inertia = 0.80f;
    float windInfluence = 0.65f;
    float maxDt = 1.0f / 30.0f;
    int constraintIterations = 5;
    int maxSubsteps = 4;
    float collisionPadding = 0.008f;
    float maxSpeed = 8.0f;
};

class SecondaryMotionProfileRegistry {
public:
    bool RegisterProfile(const SecondaryMotionProfile& profile);
    bool RegisterProfileValue(const VekValue& value, std::string* error = nullptr);
    const SecondaryMotionProfile* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string, SecondaryMotionProfile> profiles;
};

struct SpringChainSettings {
    int segments = 4;
    float segmentLength = 0.08f;
    SecondaryMotionProfile motion;
};

struct SpringChainParticle {
    PhysicsVec3 position{};
    PhysicsVec3 previousPosition{};
};

// Deterministic Verlet-style secondary-motion chain for hair, cloth strips,
// antennae, cables and other lightweight articulated pieces. The runtime owns
// no renderer and has no dependency on a game engine's vector/math types.
class SpringChain3D {
public:
    void Configure(const SpringChainSettings& settings);
    const SpringChainSettings& Settings() const { return settings_; }

    void Reset(PhysicsVec3 root, PhysicsVec3 restDirection = {0.0f, -1.0f, 0.0f});
    void Clear();
    bool Initialized() const { return initialized_; }

    // root: current attachment point in the caller's local/world space.
    // externalAcceleration: inertial/wind acceleration in the same space.
    // sphereRadius <= 0 disables sphere collision.
    void Step(PhysicsVec3 root,
              PhysicsVec3 externalAcceleration,
              PhysicsVec3 sphereCenter,
              float sphereRadius,
              float dt);

    const std::vector<SpringChainParticle>& Particles() const { return particles_; }

private:
    void StepSubstep(PhysicsVec3 root,
                     PhysicsVec3 externalAcceleration,
                     PhysicsVec3 sphereCenter,
                     float sphereRadius,
                     float dt);
    void SolveConstraints(PhysicsVec3 root, PhysicsVec3 sphereCenter, float sphereRadius);

    SpringChainSettings settings_{};
    std::vector<SpringChainParticle> particles_;
    std::vector<PhysicsVec3> restOffsets_;
    PhysicsVec3 previousRoot_{};
    bool initialized_ = false;
};

} // namespace vek
