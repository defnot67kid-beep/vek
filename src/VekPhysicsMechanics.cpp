#include <vek/VekPhysicsMechanics.h>

#include <algorithm>
#include <cmath>

namespace vek::mechanics {

PhysicsVec3 Add(PhysicsVec3 a, PhysicsVec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
PhysicsVec3 Sub(PhysicsVec3 a, PhysicsVec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
PhysicsVec3 Scale(PhysicsVec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
float Dot(PhysicsVec3 a, PhysicsVec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float Length(PhysicsVec3 a) { return std::sqrt(Dot(a, a)); }
PhysicsVec3 Normalize(PhysicsVec3 a) {
    float len = Length(a);
    if (len < 1e-8f) return {0.0f, 0.0f, 0.0f};
    return Scale(a, 1.0f / len);
}

RayHit RaycastSphere(const Ray& ray, const Sphere& sphere, const std::string& bodyId) {
    RayHit result;
    PhysicsVec3 dir = Normalize(ray.direction);
    PhysicsVec3 originToCenter = Sub(sphere.center, ray.origin);
    float tca = Dot(originToCenter, dir);
    if (tca < 0.0f) return result;
    float d2 = Dot(originToCenter, originToCenter) - tca * tca;
    float r2 = sphere.radius * sphere.radius;
    if (d2 > r2) return result;
    float thc = std::sqrt(r2 - d2);
    float t0 = tca - thc;
    float t = (t0 >= 0.0f) ? t0 : (tca + thc);
    if (t < 0.0f || t > ray.maxDistance) return result;
    result.hit = true;
    result.distance = t;
    result.point = Add(ray.origin, Scale(dir, t));
    result.normal = Normalize(Sub(result.point, sphere.center));
    result.bodyId = bodyId;
    return result;
}

RayHit RaycastAABB(const Ray& ray, const AABB& box, const std::string& bodyId) {
    RayHit result;
    PhysicsVec3 dir = Normalize(ray.direction);
    float tmin = 0.0f;
    float tmax = ray.maxDistance;
    const float origin[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
    const float d[3] = {dir.x, dir.y, dir.z};
    const float bmin[3] = {box.min.x, box.min.y, box.min.z};
    const float bmax[3] = {box.max.x, box.max.y, box.max.z};
    int hitAxis = -1;
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(d[axis]) < 1e-8f) {
            if (origin[axis] < bmin[axis] || origin[axis] > bmax[axis]) return result;
            continue;
        }
        float inv = 1.0f / d[axis];
        float t1 = (bmin[axis] - origin[axis]) * inv;
        float t2 = (bmax[axis] - origin[axis]) * inv;
        bool swapped = false;
        if (t1 > t2) { std::swap(t1, t2); swapped = true; }
        if (t1 > tmin) { tmin = t1; hitAxis = swapped ? axis + 3 : axis; }
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return result;
    }
    result.hit = true;
    result.distance = tmin;
    result.point = Add(ray.origin, Scale(dir, tmin));
    PhysicsVec3 normal{0.0f, 0.0f, 0.0f};
    if (hitAxis >= 0) {
        int axis = hitAxis % 3;
        float sign = (hitAxis >= 3) ? 1.0f : -1.0f;
        if (axis == 0) normal.x = sign;
        else if (axis == 1) normal.y = sign;
        else normal.z = sign;
    }
    result.normal = normal;
    result.bodyId = bodyId;
    return result;
}

RayHit RaycastClosest(const Ray& ray,
                       const std::vector<RaycastCandidateSphere>& spheres,
                       const std::vector<RaycastCandidateAABB>& boxes) {
    RayHit best;
    for (const auto& s : spheres) {
        RayHit h = RaycastSphere(ray, s.shape, s.id);
        if (h.hit && (!best.hit || h.distance < best.distance)) best = h;
    }
    for (const auto& b : boxes) {
        RayHit h = RaycastAABB(ray, b.shape, b.id);
        if (h.hit && (!best.hit || h.distance < best.distance)) best = h;
    }
    return best;
}

bool ResolveSphereSphere(RigidBodySphere& a, RigidBodySphere& b) {
    PhysicsVec3 delta = Sub(b.position, a.position);
    float dist = Length(delta);
    float overlap = (a.radius + b.radius) - dist;
    if (overlap <= 0.0f) return false;

    PhysicsVec3 normal = (dist > 1e-6f) ? Scale(delta, 1.0f / dist) : PhysicsVec3{0.0f, 1.0f, 0.0f};

    // Positional correction, weighted by inverse mass (static = infinite mass).
    float invMassA = a.isStatic ? 0.0f : 1.0f / std::max(a.mass, 1e-6f);
    float invMassB = b.isStatic ? 0.0f : 1.0f / std::max(b.mass, 1e-6f);
    float invMassSum = invMassA + invMassB;
    if (invMassSum > 0.0f) {
        PhysicsVec3 correction = Scale(normal, overlap / invMassSum);
        if (!a.isStatic) a.position = Sub(a.position, Scale(correction, invMassA));
        if (!b.isStatic) b.position = Add(b.position, Scale(correction, invMassB));
    }

    // Velocity impulse along the collision normal.
    PhysicsVec3 relativeVelocity = Sub(b.velocity, a.velocity);
    float velAlongNormal = Dot(relativeVelocity, normal);
    if (velAlongNormal > 0.0f) return true; // already separating

    float restitution = std::min(a.restitution, b.restitution);
    float j = -(1.0f + restitution) * velAlongNormal;
    if (invMassSum > 0.0f) j /= invMassSum;
    PhysicsVec3 impulse = Scale(normal, j);
    if (!a.isStatic) a.velocity = Sub(a.velocity, Scale(impulse, invMassA));
    if (!b.isStatic) b.velocity = Add(b.velocity, Scale(impulse, invMassB));
    return true;
}

bool ResolveSphereGroundPlane(RigidBodySphere& body, float planeY, float restitutionOverride) {
    float bottom = body.position.y - body.radius;
    if (bottom > planeY) return false;
    body.position.y += (planeY - bottom);
    if (body.velocity.y < 0.0f) {
        float restitution = (restitutionOverride >= 0.0f) ? restitutionOverride : body.restitution;
        body.velocity.y = -body.velocity.y * restitution;
    }
    return true;
}

void TriggerWorld::SetTrigger(const std::string& triggerId, AABB bounds) {
    triggers_[triggerId] = bounds;
    occupants_.try_emplace(triggerId);
}

void TriggerWorld::RemoveTrigger(const std::string& triggerId) {
    triggers_.erase(triggerId);
    occupants_.erase(triggerId);
}

namespace {
bool PointInAABB(const AABB& box, PhysicsVec3 p) {
    return p.x >= box.min.x && p.x <= box.max.x &&
           p.y >= box.min.y && p.y <= box.max.y &&
           p.z >= box.min.z && p.z <= box.max.z;
}
} // namespace

std::vector<TriggerEvent> TriggerWorld::Update(
    const std::vector<std::pair<std::string, PhysicsVec3>>& bodies) {
    std::vector<TriggerEvent> events;
    for (auto& [triggerId, bounds] : triggers_) {
        auto& previous = occupants_[triggerId];
        std::vector<std::string> current;
        for (const auto& [bodyId, pos] : bodies) {
            if (!PointInAABB(bounds, pos)) continue;
            current.push_back(bodyId);
            bool wasInside = std::find(previous.begin(), previous.end(), bodyId) != previous.end();
            events.push_back({wasInside ? TriggerEventKind::Stay : TriggerEventKind::Enter, triggerId, bodyId});
        }
        for (const auto& bodyId : previous) {
            bool stillInside = std::find(current.begin(), current.end(), bodyId) != current.end();
            if (!stillInside) events.push_back({TriggerEventKind::Exit, triggerId, bodyId});
        }
        previous = std::move(current);
    }
    return events;
}

void StepProjectile(ProjectileState& state, float dt) {
    if (dt <= 0.0f) return;
    if (state.dragCoefficient > 0.0f) {
        PhysicsVec3 drag = Scale(state.velocity, -state.dragCoefficient * Length(state.velocity));
        state.velocity = Add(state.velocity, Scale(drag, dt));
    }
    state.velocity.y -= state.gravity * dt;
    state.position = Add(state.position, Scale(state.velocity, dt));
    state.elapsed += dt;
}

PhysicsVec3 PredictLandingPoint(PhysicsVec3 origin, PhysicsVec3 velocity, float gravity, float groundY) {
    // Solve origin.y + vy*t - 0.5*g*t^2 == groundY for the positive root.
    float a = -0.5f * gravity;
    float b = velocity.y;
    float c = origin.y - groundY;
    if (std::abs(a) < 1e-8f) {
        float t = (std::abs(b) < 1e-8f) ? 0.0f : -c / b;
        return Add(origin, Scale(velocity, std::max(t, 0.0f)));
    }
    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) return origin; // never reaches groundY
    float sqrtD = std::sqrt(discriminant);
    float t1 = (-b + sqrtD) / (2.0f * a);
    float t2 = (-b - sqrtD) / (2.0f * a);
    float t = std::max(t1, t2);
    if (t < 0.0f) t = std::min(t1, t2);
    t = std::max(t, 0.0f);
    return {origin.x + velocity.x * t, groundY, origin.z + velocity.z * t};
}

} // namespace vek::mechanics
