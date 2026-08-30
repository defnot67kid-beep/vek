#pragma once

#include "raylib.h"
#include "VekScriptEngine.h"
#include <string>
#include <vector>

class World;
class Vehicle;
class HumanoidSystem;

struct CollisionResponse {
    bool solid = true;
    float friction = 0.65f;
    float bounce = 0.0f;
    float damage = 0.0f;
};

struct CollisionContact {
    std::string actorA;
    std::string actorB;
    std::string objectName;
    Vector3 point{0,0,0};
    Vector3 normal{0,0,0};
    float penetration = 0.0f;
    float impactSpeed = 0.0f;
    CollisionResponse response;
};

class CollisionSystem {
public:
    bool Initialize(const World& world, const std::string& scriptPath = "scripts/collision.vek");
    void RefreshWorldGeometry(const World& world);
    bool ReloadScript();
    bool CanHotReload() const;
    bool SignatureVerified() const;
    const char* VekSecurityMode() const;

    void ResolvePlayer(Vector3& position,
                       Vector3& horizontalVelocity,
                       HumanoidSystem& humanoid,
                       const Vehicle* parkedVehicle,
                       bool ignoreParkedVehicle,
                       float standingHeight = 2.55f,
                       float rootToFeet = 0.15f);

    void ResolveVehicle(Vehicle& vehicle, float deltaTime);

    void ToggleDebug();
    bool DebugEnabled() const;
    void DrawDebug3D(const Vector3& playerPosition, const Vehicle* vehicle) const;
    void DrawDebugHUD() const;

    bool ScriptLoaded() const;
    const std::string& ScriptError() const;
    const std::string& ScriptPath() const;
    int ContactsThisFrame() const;
    const CollisionContact& LastContact() const;

    // Geometry helper exposed for verification tests and future NPC reuse.
    static bool ResolveCircleVsBox(Vector3 position,
                                   float radius,
                                   Vector3 boxCenter,
                                   Vector3 boxSize,
                                   Vector3& outNormal,
                                   float& outPenetration,
                                   Vector3& outPoint);

    // Horizontal collision response is still circle-vs-box, but contacts are
    // first filtered by real vertical overlap. This prevents overhead headers,
    // beams and raised geometry from becoming invisible floor-to-ceiling walls.
    static bool VerticalSpansOverlap(float actorMinY,
                                     float actorMaxY,
                                     Vector3 boxCenter,
                                     Vector3 boxSize);

private:
    struct StaticBox {
        int id = 0;
        std::string name;
        Vector3 center{0,0,0};
        Vector3 size{1,1,1};
        std::string tag = "world";
    };

    std::vector<StaticBox> staticBoxes;
    VekScriptEngine vek;
    std::string scriptFile;
    std::string scriptError;
    bool scriptLoaded = false;
    bool signatureVerified = false;
    bool debug = false;
    int contactsThisFrame = 0;
    CollisionContact lastContact;
    float vehicleDamageCooldown = 0.0f;

    bool LoadScriptInternal();

    CollisionResponse EvaluateRule(const std::string& actorA,
                                   const std::string& actorB,
                                   float impactSpeed);

    void RegisterVekNatives();
    void ResolveVelocity(Vector3& velocity, Vector3 normal, const CollisionResponse& response) const;
};
