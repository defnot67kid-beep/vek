#pragma once
#include "raylib.h"
#include <string>
#include <vector>

class World;

enum class MapDisplayMode { Minimap, Fullscreen };
enum class MinimapOrientation { PlayerUp, NorthUp };
enum class MapActorMode { OnFoot, RoadVehicle, Boat, Aircraft };
enum class MapRouteMode { Direct, Road, Water, Air };
enum class MapAreaType { Terrain, Workshop, Mud, Sand, Water, TestArea };
enum class MapJobType { None, Delivery, Taxi, HeavyCargo, Rescue, Construction, Aircraft, Boat, Recovery, Emergency };

enum class MapIconType {
    Player,
    Vehicle,
    Workshop,
    Garage,
    Job,
    Objective,
    Pickup,
    Delivery,
    Fuel,
    Repair,
    Shop,
    Airport,
    Harbour,
    Hospital,
    Company,
    TestTrack,
    Waypoint,
    Event,
    Recovery,
    FastTravel,
    Unknown
};

enum class MapRegionID {
    StarterTown,
    MainCity,
    IndustrialDistrict,
    Countryside,
    Mountains,
    Airport,
    Harbour,
    Desert,
    Forest,
    Islands,
    TestGrounds
};

struct MapRoad {
    Vector3 start{0,0,0};
    Vector3 end{0,0,0};
    float width = 6.0f;
    bool major = false;
};

struct MapArea {
    Rectangle boundsXZ{0,0,0,0};
    Color color{50,70,62,255};
    bool outline = false;
    MapAreaType type = MapAreaType::Terrain;
};

struct MapBuilding {
    Vector3 center{0,0,0};
    Vector2 size{1,1};
    std::string name;
    Color color{90,96,102,255};
};

struct MapRegion {
    MapRegionID id = MapRegionID::StarterTown;
    std::string name;
    Rectangle boundsXZ{0,0,0,0};
    Color mapColor{52,68,65,255};
    bool discovered = false;
};

struct MapMarker {
    int id = 0;
    MapIconType type = MapIconType::Unknown;
    Vector3 worldPosition{0,0,0};
    std::string name;
    std::string description;
    std::string services;
    MapRegionID region = MapRegionID::StarterTown;

    bool discovered = false;
    bool visible = true;
    bool tracked = false;
    bool showWhenUndiscovered = false;
    bool fastTravel = false;

    float iconScale = 1.0f;
    float discoveryRadius = 24.0f;
    int jobReward = 0;
    int jobDifficulty = 0;
    int availableJobs = 0;
    MapJobType jobType = MapJobType::None;
};

struct MapFilterSettings {
    bool jobs = true;
    bool services = true;
    bool vehicles = true;
    bool events = true;
};

struct MapRoute {
    std::vector<Vector3> points;
    bool active = false;
    std::string destinationName;
    MapIconType destinationType = MapIconType::Objective;
    MapRouteMode mode = MapRouteMode::Direct;
};

struct MapSaveData {
    int version = 1;
    MinimapOrientation orientation = MinimapOrientation::PlayerUp;
    bool hasWaypoint = false;
    Vector3 waypoint{0,0,0};
    float worldMapZoom = 1.0f;
    Vector2 worldMapPan{0,0};
    MapFilterSettings filters;
    int trackedMarkerId = 0;
    std::vector<int> discoveredMarkerIds;
    std::vector<int> discoveredRegionIds;
    std::vector<int> fastTravelMarkerIds;
};

class MapSystem {
public:
    void Initialize(const World& world);
    void Update(float deltaTime);
    void Draw();
    void DrawWorldMarkers3D() const;

    void ToggleMap();
    void SetDisplayMode(MapDisplayMode mode);
    MapDisplayMode GetDisplayMode() const { return displayMode; }
    bool BlocksGameplayInput() const;
    bool NeedsCursor() const;
    bool IsFullscreenVisible() const { return transitionProgress > 0.001f; }
    float GetTransitionProgress() const { return transitionProgress; }

    void SetPlayerPosition(Vector3 position) { playerPosition = position; }
    void SetPlayerHeading(float yaw) { playerHeading = NormalizeDegrees(yaw); }
    void SetVehicleState(Vector3 position, float heading, bool exists, bool occupied, float speedKph);
    void SetMovementContext(float speedMetersPerSecond, MapActorMode mode);
    void SetBuildMode(bool enabled) { buildMode = enabled; }
    void SetHidden(bool value) { hidden = value; }
    void SetCurrentObjective(const std::string& text) { objectiveText = text; }
    void SetInstructionBarVisible(bool value) { showInstructionBar = value; }

    void SetGPSDestination(Vector3 position, const std::string& name, MapIconType type = MapIconType::Objective);
    void ClearGPSDestination();
    bool HasGPSDestination() const { return gpsActive; }
    Vector3 GetGPSDestination() const;
    const std::string& GetGPSDestinationName() const;

    void SetWaypoint(Vector3 position);
    void ClearWaypoint();
    bool HasWaypoint() const { return waypointActive; }
    Vector3 GetWaypoint() const { return waypointPosition; }

    int AddMarker(const MapMarker& marker);
    void RemoveMarker(int id);

    MapSaveData ExportSaveData() const;
    void ApplySaveData(const MapSaveData& data);

    MinimapOrientation GetOrientation() const { return orientation; }
    float GetMinimapZoom() const { return currentMinimapZoom; }
    float GetWorldMapZoom() const { return currentWorldMapZoom; }
    int GetVisibleMarkerCount() const { return visibleMarkerCount; }

    // Public math helpers are used by the verification tests and keep angle/
    // transition behavior deterministic at different frame rates.
    static float NormalizeDegrees(float degrees);
    static float SmoothAlpha(float response, float deltaTime);
    static float StepTransition(float current, float target, float deltaTime, float duration);
    static Vector2 RotateMapDelta(Vector3 worldDelta, float yawDegrees);

private:
    MapDisplayMode displayMode = MapDisplayMode::Minimap;
    MinimapOrientation orientation = MinimapOrientation::PlayerUp;
    MapActorMode actorMode = MapActorMode::OnFoot;

    std::vector<MapRoad> roads;
    std::vector<MapArea> areas;
    std::vector<MapBuilding> buildings;
    std::vector<MapRegion> regions;
    std::vector<MapMarker> markers;
    MapRoute route;
    MapFilterSettings filters;

    Rectangle worldBoundsXZ{-150,-150,300,300};
    int nextMarkerId = 1;

    Vector3 playerPosition{0,0,0};
    float playerHeading = 0.0f;
    Vector3 vehiclePosition{0,0,0};
    float vehicleHeading = 0.0f;
    float vehicleSpeedKph = 0.0f;
    bool vehicleExists = false;
    bool vehicleOccupied = false;
    float movementSpeed = 0.0f;

    bool gpsActive = false;
    Vector3 gpsDestination{0,0,0};
    std::string gpsDestinationName;
    MapIconType gpsDestinationType = MapIconType::Objective;
    std::string objectiveText;

    bool waypointActive = false;
    Vector3 waypointPosition{0,0,0};

    bool hidden = false;
    bool buildMode = false;
    bool debug = false;
    bool showInstructionBar = false;

    float currentMinimapZoom = 1.0f;
    float targetMinimapZoom = 1.0f;
    float currentWorldMapZoom = 1.0f;
    float targetWorldMapZoom = 1.0f;
    const float minWorldMapZoom = 0.35f;
    const float maxWorldMapZoom = 5.0f;

    Vector2 currentMapPan{0,0}; // x = world X, y = world Z
    Vector2 targetMapPan{0,0};

    float transitionProgress = 0.0f;
    float transitionDuration = 0.24f;

    int selectedMarkerId = 0;
    int trackedMarkerId = 0;
    int visibleMarkerCount = 0;

    bool rightDragActive = false;
    bool rightDragged = false;
    Vector2 rightDragStart{0,0};
    Vector2 rightDragLast{0,0};

    void BuildWorldData(const World& world);
    void UpdateMinimapZoom(float dt);
    void UpdateDiscovery();
    void UpdateFullscreenInput(float dt);
    void UpdateRoute();
    void ClampMapPan();

    Rectangle MinimapRect() const;
    Rectangle FullscreenRect() const;
    Rectangle FullscreenViewport(Rectangle outer) const;
    Rectangle SidePanelRect(Rectangle outer) const;

    Vector3 GameplayMapSource() const;
    float GameplayHeading() const;

    void DrawMinimap();
    void DrawFullscreenTransition();
    void DrawMapContents(Rectangle clip, Vector2 centerWorldXZ, float pixelsPerWorldUnit,
                         float rotationYaw, bool fullMap, float opacity);
    void DrawCompass(Rectangle outer, Rectangle inner, float rotationYaw, float opacity) const;
    void DrawMarkerIcon(Vector2 p, MapIconType type, float scale, float heading, Color tint, bool pulse=false) const;
    void DrawPlayerOrVehicleMarker(Vector2 p, float headingOnMap, float opacity) const;
    void DrawFullscreenChrome(Rectangle outer, Rectangle viewport, float opacity);
    void DrawSelectionPanel(Rectangle panel, float opacity);
    void DrawDebugOverlay() const;

    Vector2 WorldToView(Vector3 world, Rectangle clip, Vector2 centerWorldXZ,
                        float pixelsPerWorldUnit, float rotationYaw) const;
    Vector3 ViewToWorld(Vector2 screen, Rectangle clip, Vector2 centerWorldXZ,
                        float pixelsPerWorldUnit, float rotationYaw) const;
    bool MarkerPassesFilter(const MapMarker& marker) const;
    bool MarkerShouldDraw(const MapMarker& marker) const;
    const MapMarker* FindMarker(int id) const;
    MapMarker* FindMarker(int id);
    const MapRegion* FindRegion(MapRegionID id) const;
    const char* RegionName(MapRegionID id) const;
    const char* IconShortName(MapIconType type) const;
    Color IconColor(MapIconType type) const;
    float DistanceTo(Vector3 target) const;
    Vector2 ClampToMinimapEdge(Vector2 offset, float halfExtent) const;
};
