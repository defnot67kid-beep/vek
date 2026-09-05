#include <vek/VekStdLibExtras.h>

#include <algorithm>
#include <cmath>
#include <random>

#include <vek/VekPhysicsMechanics.h>
#include <vek/VekAerodynamics.h>
#include <vek/VekWorldBuilding.h>
#include <vek/VekStyleTokens.h>
#include <vek/VekRemoteSystems.h>
#include <map>

namespace vek {

namespace {

PhysicsVec3 ReadVec3(const VekValue& v) {
    PhysicsVec3 out;
    if (!v.IsMap()) return out;
    out.x = static_cast<float>(v.Get("x").AsNumber(0.0));
    out.y = static_cast<float>(v.Get("y").AsNumber(0.0));
    out.z = static_cast<float>(v.Get("z").AsNumber(0.0));
    return out;
}

VekValue WriteVec3(PhysicsVec3 v) {
    VekMap m;
    m["x"] = static_cast<double>(v.x);
    m["y"] = static_cast<double>(v.y);
    m["z"] = static_cast<double>(v.z);
    return VekValue(std::move(m));
}

double EaseEval(const std::string& name, double t) {
    t = std::clamp(t, 0.0, 1.0);
    if (name == "linear") return t;
    if (name == "ease_in") return t * t;
    if (name == "ease_out") return 1.0 - (1.0 - t) * (1.0 - t);
    if (name == "ease_in_out") return (t < 0.5) ? 2.0 * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 2.0) / 2.0;
    if (name == "smoothstep") return t * t * (3.0 - 2.0 * t);
    if (name == "ease_out_back") {
        const double c1 = 1.70158, c3 = c1 + 1.0;
        return 1.0 + c3 * std::pow(t - 1.0, 3.0) + c1 * std::pow(t - 1.0, 2.0);
    }
    if (name == "ease_out_bounce") {
        const double n1 = 7.5625, d1 = 2.75;
        if (t < 1.0 / d1) return n1 * t * t;
        if (t < 2.0 / d1) { t -= 1.5 / d1; return n1 * t * t + 0.75; }
        if (t < 2.5 / d1) { t -= 2.25 / d1; return n1 * t * t + 0.9375; }
        t -= 2.625 / d1;
        return n1 * t * t + 0.984375;
    }
    return t;
}

std::mt19937& RngState() {
    static std::mt19937 rng{std::random_device{}()};
    return rng;
}

// --- Remote/local connection registries (VEK 3.7) ---------------------------
// Simple handle-based tables so VEK scripts can start servers/clients without
// owning raw C++ objects. Handles are small integers, never reused while the
// underlying server/client is alive.
struct RemoteRegistries {
    std::map<int, std::unique_ptr<remote::RemoteServer>> servers;
    std::map<int, std::unique_ptr<remote::RemoteClient>> clients;
    std::map<int, remote::AltitudeService> altitudeServices;
    int nextHandle = 1;
};

RemoteRegistries& Registries() {
    static RemoteRegistries registries;
    return registries;
}

} // namespace

void VekRegisterExtraStandardLibrary(VekScriptEngine& engine) {
    engine.RegisterNative("raycast_sphere", [](const std::vector<VekValue>& a) {
        if (a.size() < 3) return VekValue();
        mechanics::Ray ray;
        ray.origin = ReadVec3(a[0]);
        ray.direction = ReadVec3(a[1]);
        ray.maxDistance = a.size() > 3 ? static_cast<float>(a[3].AsNumber(1000.0)) : 1000.0f;
        mechanics::Sphere sphere;
        sphere.center = ReadVec3(a[2]);
        sphere.radius = a.size() > 4 ? static_cast<float>(a[4].AsNumber(0.5)) : 0.5f;
        auto hit = mechanics::RaycastSphere(ray, sphere);
        VekMap m;
        m["hit"] = hit.hit;
        m["distance"] = static_cast<double>(hit.distance);
        m["point"] = WriteVec3(hit.point);
        m["normal"] = WriteVec3(hit.normal);
        return VekValue(std::move(m));
    });

    engine.RegisterNative("raycast_aabb", [](const std::vector<VekValue>& a) {
        if (a.size() < 4) return VekValue();
        mechanics::Ray ray;
        ray.origin = ReadVec3(a[0]);
        ray.direction = ReadVec3(a[1]);
        mechanics::AABB box;
        box.min = ReadVec3(a[2]);
        box.max = ReadVec3(a[3]);
        auto hit = mechanics::RaycastAABB(ray, box);
        VekMap m;
        m["hit"] = hit.hit;
        m["distance"] = static_cast<double>(hit.distance);
        m["point"] = WriteVec3(hit.point);
        m["normal"] = WriteVec3(hit.normal);
        return VekValue(std::move(m));
    });

    engine.RegisterNative("sphere_sphere_resolve", [](const std::vector<VekValue>& a) {
        if (a.size() < 2) return VekValue(false);
        mechanics::RigidBodySphere sa, sb;
        sa.position = ReadVec3(a[0].Get("position"));
        sa.velocity = ReadVec3(a[0].Get("velocity"));
        sa.radius = static_cast<float>(a[0].Get("radius").AsNumber(0.5));
        sa.mass = static_cast<float>(a[0].Get("mass").AsNumber(1.0));
        sa.restitution = static_cast<float>(a[0].Get("restitution").AsNumber(0.3));
        sa.isStatic = a[0].Get("is_static").Truthy();
        sb.position = ReadVec3(a[1].Get("position"));
        sb.velocity = ReadVec3(a[1].Get("velocity"));
        sb.radius = static_cast<float>(a[1].Get("radius").AsNumber(0.5));
        sb.mass = static_cast<float>(a[1].Get("mass").AsNumber(1.0));
        sb.restitution = static_cast<float>(a[1].Get("restitution").AsNumber(0.3));
        sb.isStatic = a[1].Get("is_static").Truthy();
        bool resolved = mechanics::ResolveSphereSphere(sa, sb);
        VekMap out;
        VekMap ma; ma["position"] = WriteVec3(sa.position); ma["velocity"] = WriteVec3(sa.velocity);
        VekMap mb; mb["position"] = WriteVec3(sb.position); mb["velocity"] = WriteVec3(sb.velocity);
        out["resolved"] = resolved;
        out["a"] = VekValue(std::move(ma));
        out["b"] = VekValue(std::move(mb));
        return VekValue(std::move(out));
    });

    engine.RegisterNative("projectile_step", [](const std::vector<VekValue>& a) {
        if (a.empty()) return VekValue();
        mechanics::ProjectileState state;
        state.position = ReadVec3(a[0].Get("position"));
        state.velocity = ReadVec3(a[0].Get("velocity"));
        state.gravity = static_cast<float>(a[0].Get("gravity").AsNumber(9.81));
        state.dragCoefficient = static_cast<float>(a[0].Get("drag").AsNumber(0.0));
        float dt = a.size() > 1 ? static_cast<float>(a[1].AsNumber(0.016)) : 0.016f;
        mechanics::StepProjectile(state, dt);
        VekMap out;
        out["position"] = WriteVec3(state.position);
        out["velocity"] = WriteVec3(state.velocity);
        out["elapsed"] = static_cast<double>(state.elapsed);
        return VekValue(std::move(out));
    });

    engine.RegisterNative("predict_landing", [](const std::vector<VekValue>& a) {
        if (a.size() < 4) return VekValue();
        PhysicsVec3 origin = ReadVec3(a[0]);
        PhysicsVec3 velocity = ReadVec3(a[1]);
        float gravity = static_cast<float>(a[2].AsNumber(9.81));
        float groundY = static_cast<float>(a[3].AsNumber(0.0));
        return WriteVec3(mechanics::PredictLandingPoint(origin, velocity, gravity, groundY));
    });

    engine.RegisterNative("ease", [](const std::vector<VekValue>& a) {
        if (a.size() < 2) return VekValue(0.0);
        return VekValue(EaseEval(a[0].AsString(), a[1].AsNumber(0.0)));
    });

    engine.RegisterNative("lerp", [](const std::vector<VekValue>& a) {
        if (a.size() < 3) return VekValue(0.0);
        double x = a[0].AsNumber(0.0), y = a[1].AsNumber(0.0), t = a[2].AsNumber(0.0);
        return VekValue(x + (y - x) * t);
    });

    engine.RegisterNative("inverse_lerp", [](const std::vector<VekValue>& a) {
        if (a.size() < 3) return VekValue(0.0);
        double x = a[0].AsNumber(0.0), y = a[1].AsNumber(0.0), v = a[2].AsNumber(0.0);
        if (std::abs(y - x) < 1e-9) return VekValue(0.0);
        return VekValue(std::clamp((v - x) / (y - x), 0.0, 1.0));
    });

    engine.RegisterNative("remap", [](const std::vector<VekValue>& a) {
        if (a.size() < 5) return VekValue(0.0);
        double inMin = a[0].AsNumber(0.0), inMax = a[1].AsNumber(1.0), v = a[2].AsNumber(0.0);
        double outMin = a[3].AsNumber(0.0), outMax = a[4].AsNumber(1.0);
        double denom = (inMax - inMin);
        double t = std::abs(denom) < 1e-9 ? 0.0 : (v - inMin) / denom;
        return VekValue(outMin + (outMax - outMin) * t);
    });

    engine.RegisterNative("rand_seed", [](const std::vector<VekValue>& a) {
        RngState().seed(a.empty() ? 0u : static_cast<unsigned int>(a[0].AsNumber(0.0)));
        return VekValue();
    });

    engine.RegisterNative("rand_range", [](const std::vector<VekValue>& a) {
        double lo = a.size() > 0 ? a[0].AsNumber(0.0) : 0.0;
        double hi = a.size() > 1 ? a[1].AsNumber(1.0) : 1.0;
        std::uniform_real_distribution<double> dist(std::min(lo, hi), std::max(lo, hi));
        return VekValue(dist(RngState()));
    });

    engine.RegisterNative("palette_color", [](const std::vector<VekValue>& a) {
        double hue = a.size() > 0 ? a[0].AsNumber(0.0) : 0.0;
        double sat = a.size() > 1 ? a[1].AsNumber(0.6) : 0.6;
        int steps = a.size() > 2 ? static_cast<int>(a[2].AsNumber(10)) : 10;
        std::size_t index = a.size() > 3 ? static_cast<std::size_t>(a[3].AsNumber(5)) : 5;
        auto ramp = ui::tokens::MakePaletteRamp(hue, sat, steps);
        return VekValue(ramp.ToVekColor(index));
    });

    engine.RegisterNative("token_scale", [](const std::vector<VekValue>& a) {
        double base = a.size() > 0 ? a[0].AsNumber(4.0) : 4.0;
        double ratio = a.size() > 1 ? a[1].AsNumber(1.5) : 1.5;
        int count = a.size() > 2 ? static_cast<int>(a[2].AsNumber(8)) : 8;
        std::size_t index = a.size() > 3 ? static_cast<std::size_t>(a[3].AsNumber(0)) : 0;
        auto scale = ui::tokens::MakeGeometricScale(base, ratio, count);
        return VekValue(scale.at(index));
    });

    engine.RegisterNative("air_density", [](const std::vector<VekValue>& a) {
        mechanics::AtmosphereModel model;
        if (!a.empty()) model.seaLevelDensity = static_cast<float>(a[0].AsNumber(1.225));
        float altitude = a.size() > 1 ? static_cast<float>(a[1].AsNumber(0.0)) : 0.0f;
        if (a.size() > 2) model.scaleHeightMeters = static_cast<float>(a[2].AsNumber(8500.0));
        return VekValue(static_cast<double>(mechanics::AirDensityAtAltitude(model, altitude)));
    });

    engine.RegisterNative("sample_wind", [](const std::vector<VekValue>& a) {
        mechanics::WindField field;
        if (!a.empty() && a[0].IsMap()) {
            field.baseDirection = ReadVec3(a[0].Get("direction"));
            field.baseSpeed = static_cast<float>(a[0].Get("speed").AsNumber(5.0));
            field.gustStrength = static_cast<float>(a[0].Get("gust_strength").AsNumber(3.0));
            field.gustFrequency = static_cast<float>(a[0].Get("gust_frequency").AsNumber(0.15));
            field.seed = static_cast<unsigned int>(a[0].Get("seed").AsNumber(1.0));
        }
        PhysicsVec3 pos = a.size() > 1 ? ReadVec3(a[1]) : PhysicsVec3{};
        float t = a.size() > 2 ? static_cast<float>(a[2].AsNumber(0.0)) : 0.0f;
        return WriteVec3(mechanics::SampleWind(field, pos, t));
    });

    engine.RegisterNative("aero_force", [](const std::vector<VekValue>& a) {
        if (a.size() < 4) return VekValue();
        mechanics::AeroSurface surface;
        if (a[0].IsMap()) {
            surface.referenceArea = static_cast<float>(a[0].Get("area").AsNumber(1.0));
            surface.dragCoefficient = static_cast<float>(a[0].Get("drag_coefficient").AsNumber(0.02));
            surface.liftSlope = static_cast<float>(a[0].Get("lift_slope").AsNumber(5.5));
            surface.zeroLiftAngleDegrees = static_cast<float>(a[0].Get("zero_lift_angle").AsNumber(0.0));
            surface.stallAngleDegrees = static_cast<float>(a[0].Get("stall_angle").AsNumber(15.0));
            surface.inducedDrag = static_cast<float>(a[0].Get("induced_drag").AsNumber(0.08));
        }
        PhysicsVec3 velocity = ReadVec3(a[1]);
        PhysicsVec3 forward = ReadVec3(a[2]);
        PhysicsVec3 up = ReadVec3(a[3]);
        float density = a.size() > 4 ? static_cast<float>(a[4].AsNumber(1.225)) : 1.225f;
        auto force = mechanics::ComputeAeroForces(surface, velocity, forward, up, density);
        VekMap out;
        out["lift"] = WriteVec3(force.lift);
        out["drag"] = WriteVec3(force.drag);
        out["angle_of_attack"] = static_cast<double>(force.angleOfAttackDegrees);
        out["stalled"] = force.stalled;
        return VekValue(std::move(out));
    });

    engine.RegisterNative("terrain_height", [](const std::vector<VekValue>& a) {
        world::TerrainConfig config;
        if (a.size() > 2 && a[2].IsMap()) {
            config.baseHeight = static_cast<float>(a[2].Get("base_height").AsNumber(0.0));
            config.amplitude = static_cast<float>(a[2].Get("amplitude").AsNumber(20.0));
            config.frequency = static_cast<float>(a[2].Get("frequency").AsNumber(0.02));
            config.octaves = static_cast<int>(a[2].Get("octaves").AsNumber(4));
            config.seed = static_cast<unsigned int>(a[2].Get("seed").AsNumber(1337.0));
        }
        float x = a.size() > 0 ? static_cast<float>(a[0].AsNumber(0.0)) : 0.0f;
        float z = a.size() > 1 ? static_cast<float>(a[1].AsNumber(0.0)) : 0.0f;
        return VekValue(static_cast<double>(world::SampleTerrainHeight(config, x, z)));
    });

    engine.RegisterNative("minimap_project", [](const std::vector<VekValue>& a) {
        if (a.size() < 2) return VekValue();
        world::MinimapConfig config;
        config.worldCenter = ReadVec3(a[0].Get("center"));
        config.worldRadiusMeters = static_cast<float>(a[0].Get("world_radius").AsNumber(200.0));
        config.minimapRadiusPixels = static_cast<float>(a[0].Get("minimap_radius").AsNumber(96.0));
        config.rotateWithFacing = a[0].Get("rotate_with_facing").Truthy();
        config.facingRadians = static_cast<float>(a[0].Get("facing_radians").AsNumber(0.0));
        auto point = world::ProjectToMinimap(config, ReadVec3(a[1]));
        VekMap out;
        out["x"] = static_cast<double>(point.x);
        out["y"] = static_cast<double>(point.y);
        out["on_screen"] = point.onScreen;
        out["edge_angle"] = static_cast<double>(point.edgeAngleRadians);
        return VekValue(std::move(out));
    });

    // --- Remote/local connection + live altitude feed (VEK 3.7) ------------
    engine.RegisterNative("remote_serve_altitude", [](const std::vector<VekValue>& a) {
        std::string host = a.size() > 1 ? a[1].AsString() : std::string("0.0.0.0");
        std::uint16_t port = a.empty() ? 7777 : static_cast<std::uint16_t>(a[0].AsNumber(7777));
        auto& reg = Registries();
        int handle = reg.nextHandle++;
        auto server = std::make_unique<remote::RemoteServer>();
        std::string error;
        remote::AltitudeService& service = reg.altitudeServices[handle];
        service.Attach(*server);
        if (!server->Start(host, port, &error)) {
            reg.altitudeServices.erase(handle);
            VekMap out; out["ok"] = false; out["error"] = error;
            return VekValue(std::move(out));
        }
        reg.servers[handle] = std::move(server);
        VekMap out; out["ok"] = true; out["handle"] = static_cast<double>(handle);
        return VekValue(std::move(out));
    });

    engine.RegisterNative("remote_set_altitude", [](const std::vector<VekValue>& a) {
        if (a.size() < 2) return VekValue(false);
        auto& reg = Registries();
        int handle = static_cast<int>(a[0].AsNumber(-1));
        auto it = reg.altitudeServices.find(handle);
        if (it == reg.altitudeServices.end()) return VekValue(false);
        it->second.SetAltitudeMeters(a[1].AsNumber(0.0));
        return VekValue(true);
    });

    engine.RegisterNative("remote_stop", [](const std::vector<VekValue>& a) {
        if (a.empty()) return VekValue(false);
        auto& reg = Registries();
        int handle = static_cast<int>(a[0].AsNumber(-1));
        bool found = false;
        if (auto it = reg.servers.find(handle); it != reg.servers.end()) { it->second->Stop(); reg.servers.erase(it); found = true; }
        reg.altitudeServices.erase(handle);
        if (auto it = reg.clients.find(handle); it != reg.clients.end()) { it->second->Disconnect(); reg.clients.erase(it); found = true; }
        return VekValue(found);
    });

    engine.RegisterNative("remote_connect", [](const std::vector<VekValue>& a) {
        std::string host = a.empty() ? std::string("127.0.0.1") : a[0].AsString();
        std::uint16_t port = a.size() > 1 ? static_cast<std::uint16_t>(a[1].AsNumber(7777)) : 7777;
        auto client = std::make_unique<remote::RemoteClient>();
        std::string error;
        if (!client->Connect(host, port, &error)) {
            VekMap out; out["ok"] = false; out["error"] = error;
            return VekValue(std::move(out));
        }
        auto& reg = Registries();
        int handle = reg.nextHandle++;
        reg.clients[handle] = std::move(client);
        VekMap out; out["ok"] = true; out["handle"] = static_cast<double>(handle);
        return VekValue(std::move(out));
    });

    engine.RegisterNative("remote_request_altitude", [](const std::vector<VekValue>& a) {
        auto& reg = Registries();
        VekMap out;
        if (a.empty()) { out["ok"] = false; return VekValue(std::move(out)); }
        int handle = static_cast<int>(a[0].AsNumber(-1));
        auto it = reg.clients.find(handle);
        if (it == reg.clients.end()) { out["ok"] = false; out["error"] = std::string("invalid handle"); return VekValue(std::move(out)); }
        double fallback = a.size() > 1 ? a[1].AsNumber(0.0) : 0.0;
        double altitude = remote::RequestAltitudeMeters(*it->second, fallback);
        out["ok"] = true;
        out["altitude"] = altitude;
        return VekValue(std::move(out));
    });
}

} // namespace vek
