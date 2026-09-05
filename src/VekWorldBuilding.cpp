#include <vek/VekWorldBuilding.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace vek::world {

using vek::mechanics::Add;
using vek::mechanics::Dot;
using vek::mechanics::Length;
using vek::mechanics::Normalize;
using vek::mechanics::Scale;
using vek::mechanics::Sub;

void WaypointSystem::Upsert(const Waypoint& waypoint) {
    for (auto& w : waypoints_) {
        if (w.id == waypoint.id) { w = waypoint; return; }
    }
    waypoints_.push_back(waypoint);
}

bool WaypointSystem::Remove(const std::string& id) {
    auto it = std::remove_if(waypoints_.begin(), waypoints_.end(),
                              [&](const Waypoint& w) { return w.id == id; });
    if (it == waypoints_.end()) return false;
    waypoints_.erase(it, waypoints_.end());
    return true;
}

const Waypoint* WaypointSystem::Find(const std::string& id) const {
    for (auto& w : waypoints_) if (w.id == id) return &w;
    return nullptr;
}

std::vector<std::string> WaypointSystem::UpdateArrivals(PhysicsVec3 position, float fallbackRadiusMeters) {
    std::vector<std::string> justArrived;
    for (auto& w : waypoints_) {
        if (w.visited) continue;
        float radius = (w.radiusMeters > 0.0f) ? w.radiusMeters : fallbackRadiusMeters;
        if (Length(Sub(position, w.position)) <= radius) {
            w.visited = true;
            justArrived.push_back(w.id);
        }
    }
    return justArrived;
}

const Waypoint* WaypointSystem::NearestUnvisited(PhysicsVec3 position) const {
    const Waypoint* best = nullptr;
    float bestDist = std::numeric_limits<float>::max();
    for (auto& w : waypoints_) {
        if (w.visited) continue;
        float d = Length(Sub(position, w.position));
        if (d < bestDist) { bestDist = d; best = &w; }
    }
    return best;
}

std::vector<std::string> WaypointSystem::GreedyRouteOrder(PhysicsVec3 start) const {
    std::vector<std::string> order;
    std::vector<bool> used(waypoints_.size(), false);
    PhysicsVec3 cursor = start;
    for (std::size_t step = 0; step < waypoints_.size(); ++step) {
        int bestIdx = -1;
        float bestDist = std::numeric_limits<float>::max();
        for (std::size_t i = 0; i < waypoints_.size(); ++i) {
            if (used[i]) continue;
            float d = Length(Sub(cursor, waypoints_[i].position));
            if (d < bestDist) { bestDist = d; bestIdx = static_cast<int>(i); }
        }
        if (bestIdx < 0) break;
        used[bestIdx] = true;
        order.push_back(waypoints_[bestIdx].id);
        cursor = waypoints_[bestIdx].position;
    }
    return order;
}

MinimapPoint ProjectToMinimap(const MinimapConfig& config, PhysicsVec3 worldPosition) {
    MinimapPoint point;
    PhysicsVec3 delta = Sub(worldPosition, config.worldCenter);
    float scale = (config.worldRadiusMeters > 1e-4f)
        ? config.minimapRadiusPixels / config.worldRadiusMeters
        : 0.0f;
    float px = delta.x * scale;
    float py = delta.z * scale; // top-down: world Z maps to minimap Y

    if (config.rotateWithFacing) {
        float c = std::cos(-config.facingRadians);
        float s = std::sin(-config.facingRadians);
        float rx = px * c - py * s;
        float ry = px * s + py * c;
        px = rx;
        py = ry;
    }

    float dist = std::sqrt(px * px + py * py);
    point.edgeAngleRadians = std::atan2(py, px);
    if (dist <= config.minimapRadiusPixels || dist < 1e-5f) {
        point.x = px;
        point.y = py;
        point.onScreen = true;
    } else {
        float clampScale = config.minimapRadiusPixels / dist;
        point.x = px * clampScale;
        point.y = py * clampScale;
        point.onScreen = false;
    }
    return point;
}

std::vector<std::pair<std::string, MinimapPoint>> ProjectWaypoints(
    const MinimapConfig& config, const std::vector<Waypoint>& waypoints) {
    std::vector<std::pair<std::string, MinimapPoint>> out;
    out.reserve(waypoints.size());
    for (const auto& w : waypoints) out.emplace_back(w.id, ProjectToMinimap(config, w.position));
    return out;
}

void RegionMap::AddRegion(Region region) { regions_.push_back(std::move(region)); }

bool RegionMap::RemoveRegion(const std::string& id) {
    auto it = std::remove_if(regions_.begin(), regions_.end(),
                              [&](const Region& r) { return r.id == id; });
    if (it == regions_.end()) return false;
    regions_.erase(it, regions_.end());
    return true;
}

const Region* RegionMap::RegionAt(PhysicsVec3 position) const {
    for (auto& r : regions_) {
        if (position.x >= r.min.x && position.x <= r.max.x &&
            position.y >= r.min.y && position.y <= r.max.y &&
            position.z >= r.min.z && position.z <= r.max.z) {
            return &r;
        }
    }
    return nullptr;
}

namespace {
float Hash01(float x, float y, unsigned int seed) {
    float n = x * 127.1f + y * 311.7f + static_cast<float>(seed) * 0.618f;
    float s = std::sin(n) * 43758.5453f;
    return s - std::floor(s);
}

float ValueNoise2D(float x, float y, unsigned int seed) {
    float ix = std::floor(x), iy = std::floor(y);
    float fx = x - ix, fy = y - iy;
    float a = Hash01(ix, iy, seed);
    float b = Hash01(ix + 1.0f, iy, seed);
    float c = Hash01(ix, iy + 1.0f, seed);
    float d = Hash01(ix + 1.0f, iy + 1.0f, seed);
    float ux = fx * fx * (3.0f - 2.0f * fx);
    float uy = fy * fy * (3.0f - 2.0f * fy);
    float top = a + (b - a) * ux;
    float bottom = c + (d - c) * ux;
    return top + (bottom - top) * uy;
}
} // namespace

float SampleTerrainHeight(const TerrainConfig& config, float x, float z) {
    float amplitude = config.amplitude;
    float frequency = config.frequency;
    float sum = 0.0f;
    float maxAmplitude = 0.0f;
    int octaves = std::max(config.octaves, 1);
    for (int i = 0; i < octaves; ++i) {
        sum += ValueNoise2D(x * frequency, z * frequency, config.seed + static_cast<unsigned int>(i) * 101u) * amplitude;
        maxAmplitude += amplitude;
        amplitude *= config.persistence;
        frequency *= config.lacunarity;
    }
    float normalized = (maxAmplitude > 1e-6f) ? sum / maxAmplitude : 0.0f;
    return config.baseHeight + normalized * config.amplitude;
}

std::vector<float> GenerateHeightmapGrid(const TerrainConfig& config, int width, int depth, float worldStep) {
    std::vector<float> grid;
    if (width <= 0 || depth <= 0) return grid;
    grid.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(depth));
    for (int zi = 0; zi < depth; ++zi) {
        for (int xi = 0; xi < width; ++xi) {
            float wx = static_cast<float>(xi) * worldStep;
            float wz = static_cast<float>(zi) * worldStep;
            grid[static_cast<std::size_t>(zi) * width + xi] = SampleTerrainHeight(config, wx, wz);
        }
    }
    return grid;
}

} // namespace vek::world
