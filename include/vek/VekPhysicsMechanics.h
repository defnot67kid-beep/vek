#pragma once
// VekPhysicsMechanics — concrete, renderer/backend-neutral gameplay physics
// (VEK 3.5). VekPhysicsSystems.h intentionally ships descriptors only (so
// hosts can translate them into PhysX/Jolt/Bullet); this module adds a small
// set of dependency-free mechanics VEK itself can run without a backend:
// raycasts, sphere/AABB collision + impulse resolution, trigger volumes with
// enter/exit events, and simple projectile ballistics.

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <vek/VekPhysicsSystems.h>

namespace vek::mechanics {

using vek::PhysicsVec3;

PhysicsVec3 Add(PhysicsVec3 a, PhysicsVec3 b);
PhysicsVec3 Sub(PhysicsVec3 a, PhysicsVec3 b);
PhysicsVec3 Scale(PhysicsVec3 a, float s);
float Dot(PhysicsVec3 a, PhysicsVec3 b);
float Length(PhysicsVec3 a);
PhysicsVec3 Normalize(PhysicsVec3 a);

struct AABB { PhysicsVec3 min{}; PhysicsVec3 max{}; };
struct Sphere { PhysicsVec3 center{}; float radius = 0.5f; };

struct Ray { PhysicsVec3 origin{}; PhysicsVec3 direction{0.0f, 0.0f, 1.0f}; float maxDistance = 1000.0f; };

struct RayHit {
    bool hit = false;
    float distance = 0.0f;
    PhysicsVec3 point{};
    PhysicsVec3 normal{};
    std::string bodyId;
};

// --- Raycasting -------------------------------------------------------------
RayHit RaycastSphere(const Ray& ray, const Sphere& sphere, const std::string& bodyId = {});
RayHit RaycastAABB(const Ray& ray, const AABB& box, const std::string& bodyId = {});

// Casts against every registered candidate and returns the closest hit.
struct RaycastCandidateSphere { std::string id; Sphere shape; };
struct RaycastCandidateAABB { std::string id; AABB shape; };
RayHit RaycastClosest(const Ray& ray,
                       const std::vector<RaycastCandidateSphere>& spheres,
                       const std::vector<RaycastCandidateAABB>& boxes);

// --- Collision + impulse resolution -----------------------------------------
struct RigidBodySphere {
    std::string id;
    PhysicsVec3 position{};
    PhysicsVec3 velocity{};
    float radius = 0.5f;
    float mass = 1.0f;          // ignored (treated as infinite) when isStatic
    float restitution = 0.3f;
    bool isStatic = false;
};

// Resolves overlap and applies an elastic/inelastic impulse split by
// restitution; returns true if the pair was overlapping (and was resolved).
bool ResolveSphereSphere(RigidBodySphere& a, RigidBodySphere& b);

// Simple ground/plane collision: clamps position and reflects velocity by
// restitution when the sphere is at or below a horizontal plane at planeY.
bool ResolveSphereGroundPlane(RigidBodySphere& body, float planeY, float restitutionOverride = -1.0f);

// --- Trigger volumes ---------------------------------------------------------
enum class TriggerEventKind { Enter = 0, Stay, Exit };
struct TriggerEvent { TriggerEventKind kind; std::string triggerId; std::string bodyId; };

// Tracks which bodies are currently inside which trigger volumes across
// frames so callers get discrete Enter/Exit transitions (not just overlap).
class TriggerWorld {
public:
    void SetTrigger(const std::string& triggerId, AABB bounds);
    void RemoveTrigger(const std::string& triggerId);

    // Call once per frame with the current position of every tracked body.
    // Returns the Enter/Stay/Exit transitions produced this frame.
    std::vector<TriggerEvent> Update(const std::vector<std::pair<std::string, PhysicsVec3>>& bodies);

private:
    std::unordered_map<std::string, AABB> triggers_;
    // triggerId -> set of body ids currently inside
    std::unordered_map<std::string, std::vector<std::string>> occupants_;
};

// --- Projectile ballistics ---------------------------------------------------
struct ProjectileState {
    PhysicsVec3 position{};
    PhysicsVec3 velocity{};
    float gravity = 9.81f;
    float dragCoefficient = 0.0f; // simple linear drag, 0 disables
    float elapsed = 0.0f;
};

void StepProjectile(ProjectileState& state, float dt);

// Closed-form landing point on a horizontal plane at launchY - dropHeight,
// ignoring drag; useful for aim-assist / trajectory previews.
PhysicsVec3 PredictLandingPoint(PhysicsVec3 origin, PhysicsVec3 velocity, float gravity, float groundY);

} // namespace vek::mechanics
