#include "CollisionSystem.h"
#include "VekHostSecurity.h"

#include "World.h"
#include "Vehicle.h"
#include "HumanoidSystem.h"
#include "VekSecuritySystem.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
float ClampFloat(float v, float lo, float hi) { return std::max(lo, std::min(v, hi)); }

bool PairMatches(const std::string& a, const std::string& b,
                 const std::string& wantedA, const std::string& wantedB) {
    return (a == wantedA && b == wantedB) || (a == wantedB && b == wantedA);
}
}

bool CollisionSystem::Initialize(const World& world, const std::string& scriptPath) {
    RefreshWorldGeometry(world);

    scriptFile = scriptPath;
    RegisterVekNatives();
    return LoadScriptInternal();
}

void CollisionSystem::RefreshWorldGeometry(const World& world) {
    staticBoxes.clear();
    staticBoxes.reserve(world.CollisionBoxes().size());
    for (const auto& box : world.CollisionBoxes())
        staticBoxes.push_back({box.id, box.name, box.center, box.size, box.tag});
}

void CollisionSystem::RegisterVekNatives() {
    // Native functions are registered once and operate against the active rule
    // context stored by EvaluateRule(). See the thread-local state below.
}

namespace {
thread_local CollisionResponse* gResponse = nullptr;
thread_local std::string gActorA;
thread_local std::string gActorB;
}

bool CollisionSystem::LoadScriptInternal() {
    // Recreate the engine so stale functions from a previous successful script
    // cannot survive a failed hot reload.
    VekScriptEngine fresh;
    ApplyGameVekSecurityPolicy(fresh);

    fresh.RegisterNative("pair", [](const std::vector<VekValue>& args) -> VekValue {
        if (args.size() != 4) return false;
        return PairMatches(args[0].AsString(), args[1].AsString(), args[2].AsString(), args[3].AsString());
    });
    fresh.RegisterNative("has_tag", [](const std::vector<VekValue>& args) -> VekValue {
        if (args.size() != 2) return false;
        return args[0].AsString() == args[1].AsString();
    });
    fresh.RegisterNative("solid", [](const std::vector<VekValue>& args) -> VekValue {
        if (gResponse && !args.empty()) gResponse->solid = args[0].AsBool(true);
        return {};
    });
    fresh.RegisterNative("friction", [](const std::vector<VekValue>& args) -> VekValue {
        if (gResponse && !args.empty()) gResponse->friction = ClampFloat((float)args[0].AsNumber(), 0.0f, 1.0f);
        return {};
    });
    fresh.RegisterNative("bounce", [](const std::vector<VekValue>& args) -> VekValue {
        if (gResponse && !args.empty()) gResponse->bounce = ClampFloat((float)args[0].AsNumber(), 0.0f, 1.5f);
        return {};
    });
    fresh.RegisterNative("damage", [](const std::vector<VekValue>& args) -> VekValue {
        if (gResponse && !args.empty()) gResponse->damage = std::max(0.0f, (float)args[0].AsNumber());
        return {};
    });
    fresh.RegisterNative("min", [](const std::vector<VekValue>& args) -> VekValue {
        if (args.size() != 2) return 0.0;
        return std::min(args[0].AsNumber(), args[1].AsNumber());
    });
    fresh.RegisterNative("max", [](const std::vector<VekValue>& args) -> VekValue {
        if (args.size() != 2) return 0.0;
        return std::max(args[0].AsNumber(), args[1].AsNumber());
    });
    fresh.RegisterNative("clamp", [](const std::vector<VekValue>& args) -> VekValue {
        if (args.size() != 3) return 0.0;
        return std::clamp(args[0].AsNumber(), args[1].AsNumber(), args[2].AsNumber());
    });

    fresh.SealNativeRegistry();

    const std::string signaturePath = scriptFile + ".sig";
    VekVerifiedScript verified = VekSecuritySystem::VerifyScript(scriptFile, signaturePath);
    if (!verified.ok) {
        scriptLoaded = false;
        signatureVerified = false;
        scriptError = verified.error;
        return false;
    }
    signatureVerified = verified.signatureVerified;
    if (!fresh.LoadSource(verified.source, scriptFile)) {
        scriptLoaded = false;
        scriptError = fresh.LastError();
        return false;
    }
    if (!fresh.HasFunction("on_collision")) {
        scriptLoaded = false;
        scriptError = "VEK: collision script must define fn on_collision(a, b, speed)";
        return false;
    }

    vek = std::move(fresh);
    scriptLoaded = true;
    scriptError.clear();
    return true;
}

bool CollisionSystem::ReloadScript() {
    if (!CanHotReload()) {
        scriptError = "VEK Guard: F11 hot reload is disabled in SECURE RELEASE builds";
        return false;
    }
    return LoadScriptInternal();
}

bool CollisionSystem::CanHotReload() const { return VekSecuritySystem::IsDevelopmentMode(); }
bool CollisionSystem::SignatureVerified() const { return signatureVerified; }
const char* CollisionSystem::VekSecurityMode() const { return VekSecuritySystem::SecurityModeName(); }

CollisionResponse CollisionSystem::EvaluateRule(const std::string& actorA,
                                                const std::string& actorB,
                                                float impactSpeed) {
    CollisionResponse response;

    if (!scriptLoaded) {
        // Safe fallback keeps collision protection active even if a user edits
        // collision.vek incorrectly.
        if (PairMatches(actorA, actorB, "vehicle", "world")) {
            response.friction = 0.55f;
            response.bounce = 0.10f;
            if (impactSpeed > 8.0f) response.damage = (impactSpeed - 8.0f) * 0.7f;
        }
        return response;
    }

    gResponse = &response;
    gActorA = actorA;
    gActorB = actorB;
    vek.Call("on_collision", {actorA, actorB, impactSpeed});
    gResponse = nullptr;

    if (!vek.LastError().empty()) {
        scriptError = vek.LastError();
        CollisionResponse safe;
        if (PairMatches(actorA, actorB, "vehicle", "world")) {
            safe.friction = 0.55f;
            safe.bounce = 0.10f;
            if (impactSpeed > 8.0f) safe.damage = std::min(35.0f, (impactSpeed - 8.0f) * 0.7f);
        }
        return safe;
    }
    if (!std::isfinite(response.friction) || !std::isfinite(response.bounce) || !std::isfinite(response.damage)) {
        scriptError = "VEK Guard: non-finite collision output rejected";
        return CollisionResponse{};
    }
    response.friction = ClampFloat(response.friction, 0.0f, 1.0f);
    response.bounce = ClampFloat(response.bounce, 0.0f, 1.5f);
    response.damage = ClampFloat(response.damage, 0.0f, 50.0f);
    return response;
}

void CollisionSystem::ResolveVelocity(Vector3& velocity, Vector3 normal, const CollisionResponse& response) const {
    float normalSpeed = Vector3DotProduct(velocity, normal);
    if (normalSpeed < 0.0f) {
        velocity = Vector3Subtract(velocity, Vector3Scale(normal, (1.0f + response.bounce) * normalSpeed));
    }

    // Friction only damps tangential speed a little per contact. It should stop
    // wall-skating without freezing the character while they slide alongside it.
    float vn = Vector3DotProduct(velocity, normal);
    Vector3 normalPart = Vector3Scale(normal, vn);
    Vector3 tangent = Vector3Subtract(velocity, normalPart);
    float tangentKeep = 1.0f - response.friction * 0.08f;
    velocity = Vector3Add(normalPart, Vector3Scale(tangent, tangentKeep));
}


bool CollisionSystem::VerticalSpansOverlap(float actorMinY,
                                           float actorMaxY,
                                           Vector3 boxCenter,
                                           Vector3 boxSize) {
    if (!std::isfinite(actorMinY) || !std::isfinite(actorMaxY) ||
        !std::isfinite(boxCenter.y) || !std::isfinite(boxSize.y)) return false;
    if (actorMinY > actorMaxY) std::swap(actorMinY, actorMaxY);
    const float halfHeight = std::max(0.0f, boxSize.y) * 0.5f;
    const float boxMinY = boxCenter.y - halfHeight;
    const float boxMaxY = boxCenter.y + halfHeight;
    constexpr float epsilon = 0.001f;
    return actorMaxY > boxMinY + epsilon && actorMinY < boxMaxY - epsilon;
}

bool CollisionSystem::ResolveCircleVsBox(Vector3 position,
                                         float radius,
                                         Vector3 boxCenter,
                                         Vector3 boxSize,
                                         Vector3& outNormal,
                                         float& outPenetration,
                                         Vector3& outPoint) {
    float minX = boxCenter.x - boxSize.x * 0.5f;
    float maxX = boxCenter.x + boxSize.x * 0.5f;
    float minZ = boxCenter.z - boxSize.z * 0.5f;
    float maxZ = boxCenter.z + boxSize.z * 0.5f;

    float closestX = ClampFloat(position.x, minX, maxX);
    float closestZ = ClampFloat(position.z, minZ, maxZ);
    float dx = position.x - closestX;
    float dz = position.z - closestZ;
    float distSq = dx * dx + dz * dz;

    if (distSq >= radius * radius) return false;

    if (distSq > 1e-8f) {
        float dist = std::sqrt(distSq);
        outNormal = {dx / dist, 0.0f, dz / dist};
        outPenetration = radius - dist;
        outPoint = {closestX, position.y, closestZ};
        return true;
    }

    // Circle center lies inside the box in XZ. Push out through the nearest side.
    float toLeft = position.x - minX;
    float toRight = maxX - position.x;
    float toSouth = position.z - minZ;
    float toNorth = maxZ - position.z;
    float best = toLeft;
    outNormal = {-1,0,0};
    outPoint = {minX, position.y, position.z};
    if (toRight < best) { best = toRight; outNormal = {1,0,0}; outPoint = {maxX, position.y, position.z}; }
    if (toSouth < best) { best = toSouth; outNormal = {0,0,-1}; outPoint = {position.x, position.y, minZ}; }
    if (toNorth < best) { best = toNorth; outNormal = {0,0,1}; outPoint = {position.x, position.y, maxZ}; }
    outPenetration = radius + best;
    return true;
}

void CollisionSystem::ResolvePlayer(Vector3& position,
                                    Vector3& horizontalVelocity,
                                    HumanoidSystem& humanoid,
                                    const Vehicle* parkedVehicle,
                                    bool ignoreParkedVehicle,
                                    float standingHeight,
                                    float rootToFeet) {
    contactsThisFrame = 0;
    constexpr float playerRadius = 0.46f;
    standingHeight = ClampFloat(std::isfinite(standingHeight) ? standingHeight : 2.55f, 0.75f, 4.5f);
    rootToFeet = ClampFloat(std::isfinite(rootToFeet) ? rootToFeet : 0.15f, 0.0f, standingHeight * 0.45f);
    const float playerMinY = position.y - rootToFeet;
    const float playerMaxY = playerMinY + standingHeight;

    // A few passes are enough for corners where two walls overlap. World
    // contacts are filtered by Y before the X/Z solver runs. The old 2D-only
    // broadphase made the personnel-door header (and garage lintel) behave like
    // invisible floor-to-ceiling walls even when the actual door was open.
    for (int pass = 0; pass < 3; ++pass) {
        bool resolvedAny = false;
        for (const auto& box : staticBoxes) {
            if (!VerticalSpansOverlap(playerMinY, playerMaxY, box.center, box.size)) continue;
            Vector3 normal, point;
            float penetration = 0.0f;
            if (!ResolveCircleVsBox(position, playerRadius, box.center, box.size, normal, penetration, point)) continue;

            float approach = std::max(0.0f, -Vector3DotProduct(horizontalVelocity, normal));
            CollisionResponse response = EvaluateRule("player", box.tag, approach);
            if (!response.solid) continue;

            position = Vector3Add(position, Vector3Scale(normal, penetration + 0.001f));
            ResolveVelocity(horizontalVelocity, normal, response);
            if (response.damage > 0.0f) humanoid.ApplyDamage(response.damage);

            lastContact = {"player", box.tag, box.name, point, normal, penetration, approach, response};
            ++contactsThisFrame;
            resolvedAny = true;
        }
        if (!resolvedAny) break;
    }

    if (parkedVehicle && parkedVehicle->finalized && !ignoreParkedVehicle) {
        constexpr float vehicleRadius = 1.75f;
        Vector3 delta = Vector3Subtract(position, parkedVehicle->position);
        delta.y = 0.0f;
        float distance = Vector3Length(delta);
        float required = playerRadius + vehicleRadius;
        if (distance < required) {
            Vector3 normal = distance > 0.0001f ? Vector3Scale(delta, 1.0f / distance) : Vector3{1,0,0};
            float penetration = required - distance;
            float approach = std::max(0.0f, -Vector3DotProduct(horizontalVelocity, normal));
            CollisionResponse response = EvaluateRule("player", "vehicle", approach);
            if (response.solid) {
                position = Vector3Add(position, Vector3Scale(normal, penetration + 0.001f));
                ResolveVelocity(horizontalVelocity, normal, response);
                if (response.damage > 0.0f) humanoid.ApplyDamage(response.damage);
                lastContact = {"player", "vehicle", "Parked Custom Vehicle", parkedVehicle->position, normal, penetration, approach, response};
                ++contactsThisFrame;
            }
        }
    }
}

void CollisionSystem::ResolveVehicle(Vehicle& vehicle, float deltaTime) {
    contactsThisFrame = 0;
    if (!vehicle.finalized) return;
    vehicleDamageCooldown = std::max(0.0f, vehicleDamageCooldown - deltaTime);

    constexpr float vehicleRadius = 1.85f;
    float vehicleMinY = vehicle.position.y - 0.75f;
    float vehicleMaxY = vehicle.position.y + 1.75f;
    if (!vehicle.parts.empty()) {
        vehicleMinY = 1.0e9f;
        vehicleMaxY = -1.0e9f;
        for (const auto& part : vehicle.parts) {
            // Conservative vertical extent: pitch/roll may rotate X/Z size into Y,
            // so use the largest half-dimension rather than under-estimating.
            const float halfExtent = 0.5f * std::max({std::fabs(part.size.x), std::fabs(part.size.y), std::fabs(part.size.z), 0.05f});
            const float centerY = vehicle.position.y + part.localPosition.y;
            vehicleMinY = std::min(vehicleMinY, centerY - halfExtent);
            vehicleMaxY = std::max(vehicleMaxY, centerY + halfExtent);
        }
    }

    for (int pass = 0; pass < 3; ++pass) {
        bool resolvedAny = false;
        for (const auto& box : staticBoxes) {
            if (!VerticalSpansOverlap(vehicleMinY, vehicleMaxY, box.center, box.size)) continue;
            Vector3 normal, point;
            float penetration = 0.0f;
            if (!ResolveCircleVsBox(vehicle.position, vehicleRadius, box.center, box.size, normal, penetration, point)) continue;

            float approach = std::max(0.0f, -Vector3DotProduct(vehicle.velocity, normal));
            CollisionResponse response = EvaluateRule("vehicle", box.tag, approach);
            if (!response.solid) continue;

            vehicle.position = Vector3Add(vehicle.position, Vector3Scale(normal, penetration + 0.002f));
            ResolveVelocity(vehicle.velocity, normal, response);

            if (response.damage > 0.0f && vehicleDamageCooldown <= 0.0f) {
                vehicle.health = ClampFloat(vehicle.health - response.damage, 0.0f, 100.0f);
                vehicleDamageCooldown = 0.22f;
            }

            lastContact = {"vehicle", box.tag, box.name, point, normal, penetration, approach, response};
            ++contactsThisFrame;
            resolvedAny = true;
        }
        if (!resolvedAny) break;
    }
}

void CollisionSystem::ToggleDebug() { debug = !debug; }
bool CollisionSystem::DebugEnabled() const { return debug; }

void CollisionSystem::DrawDebug3D(const Vector3& playerPosition, const Vehicle* vehicle) const {
    if (!debug) return;
    for (const auto& box : staticBoxes) {
        DrawCubeWires(box.center, box.size.x, box.size.y, box.size.z, Fade(MAGENTA, 0.75f));
    }
    DrawCylinder({playerPosition.x, 0.05f, playerPosition.z}, 0.46f, 0.46f, 0.05f, 24, Fade(SKYBLUE, 0.7f));
    if (vehicle && vehicle->finalized)
        DrawCylinder({vehicle->position.x,0.05f,vehicle->position.z},1.85f,1.85f,0.05f,32,Fade(ORANGE,0.65f));
    DrawLine3D(lastContact.point, Vector3Add(lastContact.point, Vector3Scale(lastContact.normal, 2.0f)), LIME);
}

void CollisionSystem::DrawDebugHUD() const {
    if (!debug) return;
    int x = 18;
    int y = GetScreenHeight() - 238;
    DrawRectangle(x, y-28, 500, 243, Fade(BLACK, 0.82f));
    DrawText("VEK COLLISION + GUARD DEBUG [F10]", x + 14, y - 16, 20, RAYWHITE);
    DrawText(TextFormat("Mode: %s | signed: %s", VekSecurityMode(), signatureVerified ? "yes" : (CanHotReload()?"dev bypass":"no")), x + 14, y + 8, 16, SKYBLUE);
    DrawText(TextFormat("Script: %s", scriptLoaded ? "LOADED" : "FALLBACK"), x + 14, y + 36, 18, scriptLoaded ? LIME : ORANGE);
    DrawText(TextFormat("Contacts this frame: %d", contactsThisFrame), x + 14, y + 60, 18, RAYWHITE);
    DrawText(TextFormat("Last: %s <-> %s", lastContact.actorA.c_str(), lastContact.actorB.c_str()), x + 14, y + 84, 18, RAYWHITE);
    DrawText(TextFormat("Object: %s", lastContact.objectName.c_str()), x + 14, y + 108, 18, RAYWHITE);
    DrawText(TextFormat("Impact: %.2f m/s | pen %.3f", lastContact.impactSpeed, lastContact.penetration), x + 14, y + 132, 18, RAYWHITE);
    DrawText(TextFormat("solid %s  friction %.2f  bounce %.2f  damage %.1f",
                        lastContact.response.solid ? "yes" : "no",
                        lastContact.response.friction,
                        lastContact.response.bounce,
                        lastContact.response.damage), x + 14, y + 156, 17, RAYWHITE);
    DrawText(CanHotReload()?"F11 hot reload enabled (DEV build)":"F11 hot reload LOCKED in secure release", x + 14, y + 183, 16, CanHotReload()?SKYBLUE:ORANGE);
}

bool CollisionSystem::ScriptLoaded() const { return scriptLoaded; }
const std::string& CollisionSystem::ScriptError() const { return scriptError; }
const std::string& CollisionSystem::ScriptPath() const { return scriptFile; }
int CollisionSystem::ContactsThisFrame() const { return contactsThisFrame; }
const CollisionContact& CollisionSystem::LastContact() const { return lastContact; }
