#pragma once
// VekWorldBuilding — waypoints, minimap projection, region/zone tools, and
// procedural terrain heights for world building (VEK 3.6). Renderer-neutral:
// this produces normalized 2D positions/heights a host draws however it likes
// (a GuiFramework Canvas node, a native minimap widget, etc.).

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <vek/VekPhysicsMechanics.h>

namespace vek::world {

using vek::PhysicsVec3;

// --- Waypoints --------------------------------------------------------------
struct Waypoint {
    std::string id;
    std::string label;
    PhysicsVec3 position{};
    std::string icon;         // host-defined icon id, e.g. "flag", "objective"
    float radiusMeters = 0.0f; // 0 = point waypoint, >0 = arrival radius
    bool visited = false;
};

class WaypointSystem {
public:
    void Upsert(const Waypoint& waypoint);
    bool Remove(const std::string& id);
    const Waypoint* Find(const std::string& id) const;
    const std::vector<Waypoint>& All() const { return waypoints_; }

    // Marks any waypoint within its radiusMeters (or a fallback radius for
    // point waypoints) of `position` as visited; returns the ids that just
    // transitioned to visited this call.
    std::vector<std::string> UpdateArrivals(PhysicsVec3 position, float fallbackRadiusMeters = 3.0f);

    // Nearest not-yet-visited waypoint to a position (nullptr if none left).
    const Waypoint* NearestUnvisited(PhysicsVec3 position) const;

    // Greedy nearest-neighbor ordering starting from `start` — a simple,
    // deterministic route through all current waypoints (not optimal TSP,
    // but fine for quest routes / patrol paths).
    std::vector<std::string> GreedyRouteOrder(PhysicsVec3 start) const;

private:
    std::vector<Waypoint> waypoints_;
};

// --- Minimap projection ------------------------------------------------------
struct MinimapConfig {
    PhysicsVec3 worldCenter{};      // world position the minimap is centered on
    float worldRadiusMeters = 200.0f; // half-width of the visible world square
    float minimapRadiusPixels = 96.0f; // half-width of the minimap widget
    bool rotateWithFacing = false;
    float facingRadians = 0.0f;        // used when rotateWithFacing is true
};

struct MinimapPoint {
    float x = 0.0f;      // pixel offset from minimap center
    float y = 0.0f;
    bool onScreen = true; // false => clamped to the edge (off-map indicator)
    float edgeAngleRadians = 0.0f; // angle to the real position, for arrow icons
};

// Projects a world position onto the minimap; off-map points are clamped to
// the minimap's edge circle so hosts can draw the classic "edge arrow".
MinimapPoint ProjectToMinimap(const MinimapConfig& config, PhysicsVec3 worldPosition);

// Convenience: projects every waypoint at once.
std::vector<std::pair<std::string, MinimapPoint>> ProjectWaypoints(
    const MinimapConfig& config, const std::vector<Waypoint>& waypoints);

// --- Regions / zones ----------------------------------------------------------
struct Region {
    std::string id;
    std::string name;
    PhysicsVec3 min{};
    PhysicsVec3 max{};
    std::string biome; // host-defined tag, e.g. "forest", "desert", "vacuum"
};

class RegionMap {
public:
    void AddRegion(Region region);
    bool RemoveRegion(const std::string& id);
    const Region* RegionAt(PhysicsVec3 position) const;
    const std::vector<Region>& All() const { return regions_; }

private:
    std::vector<Region> regions_;
};

// --- Procedural terrain heights ------------------------------------------------
struct TerrainConfig {
    float baseHeight = 0.0f;
    float amplitude = 20.0f;
    float frequency = 0.02f;   // world units per noise cycle
    int octaves = 4;
    float lacunarity = 2.0f;   // frequency multiplier per octave
    float persistence = 0.5f;  // amplitude multiplier per octave
    unsigned int seed = 1337;
};

// Deterministic fractal value-noise height sample at (x, z). No external
// noise library; safe to call per-vertex when generating a heightmap grid.
float SampleTerrainHeight(const TerrainConfig& config, float x, float z);

// Generates a rowMajor (width * depth) heightmap grid, one call.
std::vector<float> GenerateHeightmapGrid(const TerrainConfig& config,
                                          int width, int depth,
                                          float worldStep);

} // namespace vek::world
