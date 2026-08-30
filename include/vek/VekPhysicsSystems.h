#pragma once

#include <cstddef>
#include <cstdint>
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


// Advanced Physics Definitions v0.2 -----------------------------------------
// These are engine-independent descriptors and a bounded script registry. They
// intentionally do not implement a rigid-body solver: hosts may translate the
// definitions into PhysX/Jolt/Bullet/custom backends without coupling VEK to one.
struct PhysicsQuat { float x=0.0f,y=0.0f,z=0.0f,w=1.0f; };
struct PhysicsTransform { PhysicsVec3 position{}; PhysicsQuat rotation{}; PhysicsVec3 scale{1.0f,1.0f,1.0f}; };
enum class PhysicsBodyType { Static=0, Kinematic=1, Dynamic=2 };
enum class PhysicsShapeType { Box=0, Sphere, Capsule, Cylinder, ConvexHull, TriangleMesh, HeightField, Compound };
enum class PhysicsJointType { Fixed=0, Hinge, BallSocket, Distance, Prismatic, ConeTwist, Gear, SixDof, Spring };
enum class PhysicsForceMode { Force=0, Acceleration, Impulse, VelocityChange };
enum class PhysicsQueryMode { Closest=0, Any, All };

struct PhysicsMaterialDefinition {
    std::string id="physics.material.default";
    float staticFriction=0.65f,dynamicFriction=0.55f,restitution=0.05f,density=1000.0f;
    float rollingFriction=0.0f,spinningFriction=0.0f;
};
struct ColliderDefinition {
    std::string id; PhysicsShapeType shape=PhysicsShapeType::Box; PhysicsTransform local{};
    PhysicsVec3 halfExtents{0.5f,0.5f,0.5f}; float radius=0.5f,height=1.0f;
    std::string materialId="physics.material.default"; std::uint32_t layer=1,mask=0xffffffffu;
    bool trigger=false,continuous=false;
};
struct RigidBodyDefinition {
    std::string id; PhysicsBodyType type=PhysicsBodyType::Dynamic; float mass=1.0f;
    PhysicsVec3 centerOfMass{},linearVelocity{},angularVelocity{};
    float linearDamping=0.05f,angularDamping=0.05f,maxLinearSpeed=250.0f,maxAngularSpeed=100.0f;
    bool gravity=true,continuousCollision=false,allowSleep=true;
    std::vector<std::string> colliderIds;
};
struct ConstraintLimitDefinition { bool enabled=false; float lower=0.0f,upper=0.0f,stiffness=0.0f,damping=0.0f; };
struct MotorDefinition { bool enabled=false; float targetVelocity=0.0f,targetPosition=0.0f,maxForce=0.0f; };
struct JointDefinition {
    std::string id,bodyA,bodyB; PhysicsJointType type=PhysicsJointType::Fixed;
    PhysicsTransform frameA{},frameB{}; ConstraintLimitDefinition linearLimit{},angularLimit{}; MotorDefinition motor{};
    float breakForce=0.0f,breakTorque=0.0f; bool collideConnected=false;
};
struct SolverDefinition {
    std::string id="physics.solver.default"; int velocityIterations=8,positionIterations=3,substeps=1;
    float fixedDt=1.0f/60.0f,maxDt=1.0f/20.0f; bool warmStart=true,deterministicOrdering=true;
};
struct PhysicsWorldDefinition {
    std::string id="physics.world.default"; PhysicsVec3 gravity{0.0f,-9.81f,0.0f};
    std::string solverId="physics.solver.default"; float sleepLinearThreshold=0.05f,sleepAngularThreshold=0.05f;
    float broadphaseCellSize=8.0f; std::size_t maxBodies=65536,maxColliders=131072,maxPairs=524288;
};
struct CharacterControllerDefinition {
    std::string id; float radius=0.35f,height=1.8f,stepHeight=0.35f,slopeLimitDegrees=48.0f,skinWidth=0.03f;
    float gravityScale=1.0f,maxFallSpeed=55.0f,pushForce=20.0f; bool enableGroundSnap=true;
};
struct VehicleWheelDefinition {
    std::string id; PhysicsVec3 localPosition{}; float radius=0.34f,width=0.22f,mass=18.0f;
    float suspensionRestLength=0.30f,suspensionStiffness=32000.0f,suspensionDamping=4200.0f;
    float longitudinalGrip=1.0f,lateralGrip=1.0f; bool steer=false,drive=false,brake=true;
};
struct VehiclePhysicsDefinition {
    std::string id; std::string chassisBodyId; std::vector<VehicleWheelDefinition> wheels;
    float engineForce=8000.0f,brakeForce=12000.0f,handbrakeForce=16000.0f,maxSteerDegrees=36.0f;
    float downforce=0.0f,dragCoefficient=0.32f;
};
struct SoftBodyDefinition { std::string id; float particleMass=0.05f,stiffness=0.7f,damping=0.05f,pressure=0.0f; int solverIterations=6; bool selfCollision=false; };
struct ClothDefinition { std::string id; int widthSegments=16,heightSegments=16; float particleMass=0.02f,stretchStiffness=0.85f,bendStiffness=0.25f,damping=0.04f,windInfluence=1.0f; bool selfCollision=false; };
struct RopeDefinition { std::string id; int segments=24; float length=4.0f,radius=0.025f,particleMass=0.04f,stretchStiffness=0.95f,bendStiffness=0.08f,damping=0.03f; };
struct AerodynamicsDefinition { std::string id; float dragCoefficient=0.30f,liftCoefficient=0.0f,referenceArea=1.0f,airDensity=1.225f; PhysicsVec3 centerOfPressure{}; };
struct BuoyancyDefinition { std::string id; float fluidDensity=1000.0f,linearDrag=1.0f,angularDrag=1.0f; PhysicsVec3 sampleHalfExtents{0.5f,0.5f,0.5f}; int sampleCount=8; };
struct BreakableDefinition { std::string id; float impulseThreshold=1000.0f,energyThreshold=0.0f; int maxFragments=64; bool propagateDamage=true; };
struct ForceFieldDefinition { std::string id; PhysicsVec3 direction{0.0f,1.0f,0.0f}; float strength=0.0f,radius=1.0f,falloff=1.0f; bool radial=false; };
struct PhysicsQueryFilterDefinition { std::string id; std::uint32_t layerMask=0xffffffffu; bool includeTriggers=false,includeStatic=true,includeKinematic=true,includeDynamic=true; PhysicsQueryMode mode=PhysicsQueryMode::Closest; };

class PhysicsDefinitionRegistry {
public:
    static constexpr const char* ApiVersion="0.2";
    bool RegisterDefinition(const std::string& category,const VekValue& definition,std::string* error=nullptr);
    const VekValue* Find(const std::string& category,const std::string& id) const;
    bool Exists(const std::string& category,const std::string& id) const { return Find(category,id)!=nullptr; }
    std::size_t Count(const std::string& category) const;
    void Clear();
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,std::unordered_map<std::string,VekValue>> definitions;
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
