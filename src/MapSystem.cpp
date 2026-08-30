#include "MapSystem.h"
#include "World.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace {
float LerpF(float a, float b, float t) { return a + (b-a)*t; }
Vector2 LerpV2(Vector2 a, Vector2 b, float t) { return {LerpF(a.x,b.x,t),LerpF(a.y,b.y,t)}; }
Rectangle LerpRect(Rectangle a, Rectangle b, float t) {
    return {LerpF(a.x,b.x,t),LerpF(a.y,b.y,t),LerpF(a.width,b.width,t),LerpF(a.height,b.height,t)};
}
float SmoothStep01(float t) { t=Clamp(t,0.0f,1.0f); return t*t*(3.0f-2.0f*t); }
float LerpAngle(float a,float b,float t) {
    float delta=std::fmod((b-a)+540.0f,360.0f)-180.0f;
    float v=a+delta*t;
    v=std::fmod(v,360.0f);if(v<0)v+=360.0f;return v;
}
Color WithAlpha(Color c, float opacity) {
    c.a=(unsigned char)Clamp((float)c.a*opacity,0.0f,255.0f);
    return c;
}
Vector2 RectCenter(Rectangle r) { return {r.x+r.width*0.5f,r.y+r.height*0.5f}; }
bool PointInRect(Vector2 p, Rectangle r) { return CheckCollisionPointRec(p,r); }
void DrawQuad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color color) {
    DrawTriangle(a,b,c,color);
    DrawTriangle(a,c,d,color);
}
}

float MapSystem::NormalizeDegrees(float degrees) {
    float r=std::fmod(degrees,360.0f);
    if(r<0.0f)r+=360.0f;
    return r;
}

float MapSystem::SmoothAlpha(float response, float deltaTime) {
    if(deltaTime<=0.0f || response<=0.0f)return 0.0f;
    return 1.0f-std::exp(-response*deltaTime);
}

float MapSystem::StepTransition(float current, float target, float deltaTime, float duration) {
    if(duration<=0.0001f)return target;
    float step=deltaTime/duration;
    if(current<target)return std::min(target,current+step);
    return std::max(target,current-step);
}

Vector2 MapSystem::RotateMapDelta(Vector3 d, float yawDegrees) {
    float r=yawDegrees*DEG2RAD;
    float c=std::cos(r),s=std::sin(r);
    // Positive world Z is north/up. The requested game compass convention
    // mirrors east/west: world +X (E) is LEFT, world -X (W) is RIGHT.
    // Apply the mirror after yaw rotation so PlayerUp still keeps the gameplay
    // heading at the top of the minimap.
    float localX=d.x*c-d.z*s;
    float localZ=d.x*s+d.z*c;
    return {-localX,-localZ};
}

void MapSystem::Initialize(const World& world) {
    roads.clear(); areas.clear(); buildings.clear(); regions.clear(); markers.clear();
    roads.reserve(16); areas.reserve(16); buildings.reserve(16); regions.reserve(12); markers.reserve(64);
    route.points.clear(); route.points.reserve(32);
    nextMarkerId=1;
    BuildWorldData(world);
    currentMapPan=targetMapPan={world.workshop.x,world.workshop.z};
    currentWorldMapZoom=targetWorldMapZoom=1.0f;
    currentMinimapZoom=targetMinimapZoom=1.0f;
}

void MapSystem::BuildWorldData(const World& world) {
    // Existing prototype roads and test surfaces. These are cached once rather
    // than rendered from the 3D scene every minimap frame.
    roads.push_back({{0,0,0},{0,0,90},12.0f,true});
    roads.push_back({{0,0,85},{76,0,85},12.0f,true});
    roads.push_back({{76,0,38},{76,0,88},12.0f,true});
    roads.push_back({{-25,0,5},{-25,0,85},8.0f,false});

    areas.push_back({{-13,-13,26,26},{62,65,70,255},false});                // workshop pad
    areas.push_back({{39,-21,18,22},{82,52,31,255},true});                 // mud test
    areas.push_back({{61,-21,18,22},{159,139,83,255},true});               // sand test
    areas.push_back({{27,21,10,8},{101,80,55,255},true});                  // ramp/test area

    buildings.push_back({{0,0,-13},{28,1.0f},"Workshop",{91,98,105,255}});
    buildings.push_back({{-14,0,-5},{1.0f,16},"Workshop",{91,98,105,255}});
    buildings.push_back({{14,0,-5},{1.0f,16},"Workshop",{91,98,105,255}});
    buildings.push_back({{15,0,95},{25,18},"Industrial Warehouse",{101,108,115,255}});
    buildings.push_back({{92,0,82},{18,22},"Delivery Company",{116,91,64,255}});

    regions.push_back({MapRegionID::StarterTown,"Starter Town",{-60,-45,120,110},{48,73,66,255},true});
    regions.push_back({MapRegionID::IndustrialDistrict,"Industrial District",{-15,60,125,70},{65,68,73,255},false});
    regions.push_back({MapRegionID::TestGrounds,"Vehicle Test Grounds",{20,-30,70,65},{82,69,54,255},false});

    MapMarker workshop;
    workshop.type=MapIconType::Workshop; workshop.worldPosition=world.workshop; workshop.name="Starter Workshop";
    workshop.description="Your mechanic workshop, vehicle builder and home garage.";
    workshop.services="Build • Repair • Blueprints • Upgrades"; workshop.region=MapRegionID::StarterTown;
    workshop.discovered=true; workshop.fastTravel=true; workshop.iconScale=1.15f;
    AddMarker(workshop);

    MapMarker job;
    job.type=MapIconType::Job; job.worldPosition={-10,0,-10}; job.name="Workshop Job Board";
    job.description="Local contracts for the starter workshop."; job.services="Delivery contracts";
    job.region=MapRegionID::StarterTown; job.discovered=true; job.jobReward=500; job.jobDifficulty=2; job.availableJobs=1; job.jobType=MapJobType::Delivery;
    AddMarker(job);

    MapMarker warehouse;
    warehouse.type=MapIconType::Pickup; warehouse.worldPosition=world.pickup; warehouse.name="Industrial Cargo Warehouse";
    warehouse.description="Cargo collection point for industrial deliveries."; warehouse.services="Cargo pickup";
    warehouse.region=MapRegionID::IndustrialDistrict; warehouse.discoveryRadius=32.0f; warehouse.showWhenUndiscovered=true;
    AddMarker(warehouse);

    MapMarker delivery;
    delivery.type=MapIconType::Delivery; delivery.worldPosition=world.dropoff; delivery.name="Delivery Company Yard";
    delivery.description="Receiving yard for the starter delivery contract."; delivery.services="Cargo delivery";
    delivery.region=MapRegionID::IndustrialDistrict; delivery.discoveryRadius=32.0f; delivery.showWhenUndiscovered=true;
    AddMarker(delivery);

    MapMarker test;
    test.type=MapIconType::TestTrack; test.worldPosition={32,0,25}; test.name="Engineering Test Grounds";
    test.description="Ramp, mud and sand areas for testing experimental vehicles."; test.services="Vehicle testing";
    test.region=MapRegionID::TestGrounds; test.discoveryRadius=35.0f; test.showWhenUndiscovered=true;
    AddMarker(test);
}

int MapSystem::AddMarker(const MapMarker& source) {
    MapMarker m=source;
    if(m.id<=0)m.id=nextMarkerId++;
    else nextMarkerId=std::max(nextMarkerId,m.id+1);
    markers.push_back(m);
    return m.id;
}

void MapSystem::RemoveMarker(int id) {
    markers.erase(std::remove_if(markers.begin(),markers.end(),[&](const MapMarker&m){return m.id==id;}),markers.end());
    if(selectedMarkerId==id)selectedMarkerId=0;
    if(trackedMarkerId==id)trackedMarkerId=0;
}

void MapSystem::SetVehicleState(Vector3 position, float heading, bool exists, bool occupied, float speedKph) {
    vehiclePosition=position; vehicleHeading=NormalizeDegrees(heading); vehicleExists=exists; vehicleOccupied=occupied; vehicleSpeedKph=speedKph;
}

void MapSystem::SetMovementContext(float speedMetersPerSecond, MapActorMode mode) {
    movementSpeed=std::max(0.0f,speedMetersPerSecond); actorMode=mode;
}

void MapSystem::SetGPSDestination(Vector3 position, const std::string& name, MapIconType type) {
    gpsActive=true; gpsDestination=position; gpsDestinationName=name; gpsDestinationType=type;
}

void MapSystem::ClearGPSDestination() {
    gpsActive=false; gpsDestinationName.clear(); route.active=false;
}

Vector3 MapSystem::GetGPSDestination() const {
    if(gpsActive)return gpsDestination;
    if(waypointActive)return waypointPosition;
    return GameplayMapSource();
}

const std::string& MapSystem::GetGPSDestinationName() const {
    if(gpsActive)return gpsDestinationName;
    static const std::string waypointName="Custom Waypoint";
    static const std::string none="";
    return waypointActive?waypointName:none;
}

void MapSystem::SetWaypoint(Vector3 p) {
    p.x=Clamp(p.x,worldBoundsXZ.x,worldBoundsXZ.x+worldBoundsXZ.width);
    p.z=Clamp(p.z,worldBoundsXZ.y,worldBoundsXZ.y+worldBoundsXZ.height);
    p.y=0.0f;
    waypointPosition=p; waypointActive=true;
}

void MapSystem::ClearWaypoint() { waypointActive=false; }

Vector3 MapSystem::GameplayMapSource() const { return vehicleOccupied?vehiclePosition:playerPosition; }
float MapSystem::GameplayHeading() const { return vehicleOccupied?vehicleHeading:playerHeading; }
float MapSystem::DistanceTo(Vector3 target) const { return Vector3Distance(GameplayMapSource(),target); }

void MapSystem::ToggleMap() {
    SetDisplayMode(displayMode==MapDisplayMode::Minimap?MapDisplayMode::Fullscreen:MapDisplayMode::Minimap);
}

void MapSystem::SetDisplayMode(MapDisplayMode mode) {
    displayMode=mode;
    if(mode==MapDisplayMode::Fullscreen && transitionProgress<0.05f) {
        Vector3 src=GameplayMapSource();
        targetMapPan=currentMapPan={src.x,src.z};
    }
}

bool MapSystem::BlocksGameplayInput() const {
    return displayMode==MapDisplayMode::Fullscreen || transitionProgress>0.015f;
}

bool MapSystem::NeedsCursor() const { return BlocksGameplayInput(); }

void MapSystem::Update(float dt) {
    if(IsKeyPressed(KEY_F9))debug=!debug;

    float targetTransition=displayMode==MapDisplayMode::Fullscreen?1.0f:0.0f;
    transitionProgress=StepTransition(transitionProgress,targetTransition,dt,transitionDuration);

    UpdateDiscovery();
    UpdateMinimapZoom(dt);
    UpdateRoute();

    if(BlocksGameplayInput()) UpdateFullscreenInput(dt);

    float panAlpha=SmoothAlpha(11.0f,dt);
    currentMapPan=LerpV2(currentMapPan,targetMapPan,panAlpha);
    float zoomAlpha=SmoothAlpha(10.0f,dt);
    currentWorldMapZoom=LerpF(currentWorldMapZoom,targetWorldMapZoom,zoomAlpha);
    ClampMapPan();
}

void MapSystem::UpdateMinimapZoom(float dt) {
    if(actorMode==MapActorMode::Aircraft) targetMinimapZoom=0.25f;
    else if(actorMode==MapActorMode::Boat) targetMinimapZoom=0.52f;
    else if(actorMode==MapActorMode::RoadVehicle) {
        float t=Clamp(vehicleSpeedKph/110.0f,0.0f,1.0f);
        targetMinimapZoom=LerpF(0.78f,0.48f,t);
    } else {
        if(movementSpeed<3.8f)targetMinimapZoom=1.25f;
        else if(movementSpeed<7.2f)targetMinimapZoom=1.0f;
        else targetMinimapZoom=0.85f;
    }
    currentMinimapZoom=LerpF(currentMinimapZoom,targetMinimapZoom,SmoothAlpha(5.5f,dt));
}

void MapSystem::UpdateDiscovery() {
    for(auto& region:regions) {
        if(CheckCollisionPointRec({playerPosition.x,playerPosition.z},region.boundsXZ))region.discovered=true;
    }
    for(auto& marker:markers) {
        if(marker.discovered)continue;
        if(Vector3Distance(playerPosition,marker.worldPosition)<=marker.discoveryRadius)marker.discovered=true;
    }
}

void MapSystem::UpdateRoute() {
    bool active=gpsActive||waypointActive;
    route.active=active;
    if(!active){route.points.clear();route.destinationName.clear();return;}
    route.points.clear();
    route.points.push_back(GameplayMapSource());
    route.points.push_back(GetGPSDestination());
    route.destinationName=GetGPSDestinationName();
    route.destinationType=gpsActive?gpsDestinationType:MapIconType::Waypoint;
    route.mode=actorMode==MapActorMode::Aircraft?MapRouteMode::Air:(actorMode==MapActorMode::Boat?MapRouteMode::Water:(actorMode==MapActorMode::RoadVehicle?MapRouteMode::Road:MapRouteMode::Direct));
}

void MapSystem::ClampMapPan() {
    targetMapPan.x=Clamp(targetMapPan.x,worldBoundsXZ.x,worldBoundsXZ.x+worldBoundsXZ.width);
    targetMapPan.y=Clamp(targetMapPan.y,worldBoundsXZ.y,worldBoundsXZ.y+worldBoundsXZ.height);
    currentMapPan.x=Clamp(currentMapPan.x,worldBoundsXZ.x,worldBoundsXZ.x+worldBoundsXZ.width);
    currentMapPan.y=Clamp(currentMapPan.y,worldBoundsXZ.y,worldBoundsXZ.y+worldBoundsXZ.height);
}

Rectangle MapSystem::MinimapRect() const {
    float sw=(float)GetScreenWidth(),sh=(float)GetScreenHeight();
    float margin=Clamp(sw*0.015f,10.0f,24.0f);
    float maxSize=buildMode?184.0f:220.0f;
    float reservedTop=sh<600.0f?std::min(150.0f,sh*0.27f):190.0f;
    float availableHeight=std::max(110.0f,sh-reservedTop-margin-44.0f);
    float size=std::min(maxSize,std::min(sw*0.30f,std::min(sh*0.31f,availableHeight)));
    size=std::max(110.0f,size);
    if(reservedTop+size+margin>sh)reservedTop=std::max(margin,sh-size-margin);
    return {sw-size-margin,reservedTop,size,size};
}

Rectangle MapSystem::FullscreenRect() const {
    float sw=(float)GetScreenWidth(),sh=(float)GetScreenHeight();
    float margin=Clamp(std::min(sw,sh)*0.032f,16.0f,32.0f);
    return {margin,margin,sw-margin*2.0f,sh-margin*2.0f};
}

Rectangle MapSystem::FullscreenViewport(Rectangle outer) const {
    float side=outer.width>=950.0f?300.0f:220.0f;
    return {outer.x+22.0f,outer.y+62.0f,std::max(200.0f,outer.width-side-62.0f),std::max(180.0f,outer.height-120.0f)};
}

Rectangle MapSystem::SidePanelRect(Rectangle outer) const {
    float side=outer.width>=950.0f?300.0f:220.0f;
    return {outer.x+outer.width-side-20.0f,outer.y+62.0f,side,outer.height-120.0f};
}

void MapSystem::UpdateFullscreenInput(float dt) {
    if(displayMode!=MapDisplayMode::Fullscreen || transitionProgress<0.72f)return;

    Rectangle outer=FullscreenRect();
    Rectangle viewport=FullscreenViewport(outer);
    Vector2 mouse=GetMousePosition();

    if(IsKeyPressed(KEY_O)) orientation=orientation==MinimapOrientation::PlayerUp?MinimapOrientation::NorthUp:MinimapOrientation::PlayerUp;
    if(IsKeyPressed(KEY_ONE))filters.jobs=!filters.jobs;
    if(IsKeyPressed(KEY_TWO))filters.services=!filters.services;
    if(IsKeyPressed(KEY_THREE))filters.vehicles=!filters.vehicles;
    if(IsKeyPressed(KEY_FOUR))filters.events=!filters.events;
    if(IsKeyPressed(KEY_DELETE)||IsKeyPressed(KEY_BACKSPACE))ClearWaypoint();

    if(IsKeyPressed(KEY_HOME)) { Vector3 p=GameplayMapSource(); targetMapPan={p.x,p.z}; }
    if(IsKeyPressed(KEY_G) && (gpsActive||waypointActive)) { Vector3 p=GetGPSDestination(); targetMapPan={p.x,p.z}; }

    float panSpeed=125.0f/std::max(0.35f,currentWorldMapZoom);
    Vector2 panInput{0,0};
    if(IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT))panInput.x+=1;
    if(IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT))panInput.x-=1;
    if(IsKeyDown(KEY_W)||IsKeyDown(KEY_UP))panInput.y+=1;
    if(IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN))panInput.y-=1;
    if(Vector2Length(panInput)>0.1f) {
        panInput=Vector2Normalize(panInput);
        targetMapPan.x+=panInput.x*panSpeed*dt;
        targetMapPan.y+=panInput.y*panSpeed*dt;
    }

    // Mouse wheel zooms toward cursor when it is over the actual map viewport.
    float wheel=GetMouseWheelMove();
    if(std::fabs(wheel)>0.001f && PointInRect(mouse,viewport)) {
        float baseScale=std::min(viewport.width/worldBoundsXZ.width,viewport.height/worldBoundsXZ.height);
        Vector3 before=ViewToWorld(mouse,viewport,currentMapPan,baseScale*currentWorldMapZoom,0.0f);
        targetWorldMapZoom=Clamp(targetWorldMapZoom*std::pow(1.18f,wheel),minWorldMapZoom,maxWorldMapZoom);
        Vector3 after=ViewToWorld(mouse,viewport,currentMapPan,baseScale*targetWorldMapZoom,0.0f);
        targetMapPan.x+=before.x-after.x;
        targetMapPan.y+=before.z-after.z;
    }

    if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && PointInRect(mouse,viewport)) {
        rightDragActive=true;rightDragged=false;rightDragStart=rightDragLast=mouse;
    }
    if(rightDragActive && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta=Vector2Subtract(mouse,rightDragLast);
        if(Vector2Distance(mouse,rightDragStart)>5.0f)rightDragged=true;
        float baseScale=std::min(viewport.width/worldBoundsXZ.width,viewport.height/worldBoundsXZ.height);
        float scale=std::max(0.001f,baseScale*currentWorldMapZoom);
        if(rightDragged) {
            targetMapPan.x+=delta.x/scale;
            targetMapPan.y+=delta.y/scale;
        }
        rightDragLast=mouse;
    }
    if(rightDragActive && IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
        if(!rightDragged && PointInRect(mouse,viewport)) {
            float baseScale=std::min(viewport.width/worldBoundsXZ.width,viewport.height/worldBoundsXZ.height);
            SetWaypoint(ViewToWorld(mouse,viewport,currentMapPan,baseScale*currentWorldMapZoom,0.0f));
        }
        rightDragActive=false;
    }

    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && PointInRect(mouse,viewport)) {
        float baseScale=std::min(viewport.width/worldBoundsXZ.width,viewport.height/worldBoundsXZ.height);
        float scale=baseScale*currentWorldMapZoom;
        int closest=0; float best=18.0f;
        for(const auto& marker:markers) {
            if(!MarkerShouldDraw(marker)||!MarkerPassesFilter(marker))continue;
            Vector2 p=WorldToView(marker.worldPosition,viewport,currentMapPan,scale,0.0f);
            float d=Vector2Distance(mouse,p);
            if(d<best){best=d;closest=marker.id;}
        }
        selectedMarkerId=closest;
    }

    Rectangle panel=SidePanelRect(outer);
    Rectangle waypointButton{panel.x+16,panel.y+panel.height-92,panel.width-32,32};
    Rectangle clearButton{panel.x+16,panel.y+panel.height-52,panel.width-32,28};
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if(selectedMarkerId!=0 && PointInRect(mouse,waypointButton)) {
            const MapMarker* marker=FindMarker(selectedMarkerId);
            if(marker) { SetWaypoint(marker->worldPosition); trackedMarkerId=marker->id; }
        }
        if(waypointActive && PointInRect(mouse,clearButton))ClearWaypoint();
    }

    ClampMapPan();
}

Vector2 MapSystem::WorldToView(Vector3 world, Rectangle clip, Vector2 centerWorldXZ,
                               float pixelsPerWorldUnit, float rotationYaw) const {
    Vector3 d{world.x-centerWorldXZ.x,0,world.z-centerWorldXZ.y};
    Vector2 r=RotateMapDelta(d,rotationYaw);
    Vector2 c=RectCenter(clip);
    return {c.x+r.x*pixelsPerWorldUnit,c.y+r.y*pixelsPerWorldUnit};
}

Vector3 MapSystem::ViewToWorld(Vector2 screen, Rectangle clip, Vector2 centerWorldXZ,
                               float pixelsPerWorldUnit, float rotationYaw) const {
    Vector2 c=RectCenter(clip);
    Vector2 r={(screen.x-c.x)/pixelsPerWorldUnit,(screen.y-c.y)/pixelsPerWorldUnit};
    // Inverse RotateMapDelta: screen X is mirrored and screen Y stores
    // negative local Z.
    float localX=-r.x,localZ=-r.y;
    float a=rotationYaw*DEG2RAD,cosa=std::cos(a),sina=std::sin(a);
    float dx=localX*cosa+localZ*sina;
    float dz=-localX*sina+localZ*cosa;
    return {centerWorldXZ.x+dx,0,centerWorldXZ.y+dz};
}

bool MapSystem::MarkerPassesFilter(const MapMarker& m) const {
    if(m.tracked || m.type==MapIconType::Objective || m.type==MapIconType::Waypoint)return true;
    switch(m.type) {
        case MapIconType::Job: case MapIconType::Pickup: case MapIconType::Delivery: return filters.jobs;
        case MapIconType::Fuel: case MapIconType::Repair: case MapIconType::Shop: case MapIconType::Airport:
        case MapIconType::Harbour: case MapIconType::Hospital: case MapIconType::Company: case MapIconType::FastTravel:
            return filters.services;
        case MapIconType::Vehicle: return filters.vehicles;
        case MapIconType::Event: case MapIconType::Recovery: return filters.events;
        default: return true;
    }
}

bool MapSystem::MarkerShouldDraw(const MapMarker& m) const {
    return m.visible && (m.discovered || m.showWhenUndiscovered || m.tracked);
}

const MapMarker* MapSystem::FindMarker(int id) const { for(const auto&m:markers)if(m.id==id)return &m;return nullptr; }
MapMarker* MapSystem::FindMarker(int id) { for(auto&m:markers)if(m.id==id)return &m;return nullptr; }
const MapRegion* MapSystem::FindRegion(MapRegionID id) const { for(const auto&r:regions)if(r.id==id)return &r;return nullptr; }
const char* MapSystem::RegionName(MapRegionID id) const { const MapRegion*r=FindRegion(id);return r?r->name.c_str():"Future Region"; }

Color MapSystem::IconColor(MapIconType t) const {
    switch(t) {
        case MapIconType::Player:return RAYWHITE;
        case MapIconType::Vehicle:return SKYBLUE;
        case MapIconType::Workshop:return {90,205,235,255};
        case MapIconType::Garage:return {98,178,226,255};
        case MapIconType::Job:return {245,191,65,255};
        case MapIconType::Objective:return {255,215,60,255};
        case MapIconType::Pickup:return {250,190,55,255};
        case MapIconType::Delivery:return {74,218,116,255};
        case MapIconType::Fuel:return {237,151,55,255};
        case MapIconType::Repair:return {102,222,180,255};
        case MapIconType::Shop:return {190,136,235,255};
        case MapIconType::Airport:return {130,185,238,255};
        case MapIconType::Harbour:return {67,162,225,255};
        case MapIconType::Hospital:return {236,104,104,255};
        case MapIconType::Company:return {164,196,108,255};
        case MapIconType::TestTrack:return {226,152,78,255};
        case MapIconType::Waypoint:return {233,98,230,255};
        case MapIconType::Event:return {255,131,72,255};
        case MapIconType::Recovery:return {217,116,92,255};
        case MapIconType::FastTravel:return {88,217,208,255};
        default:return {145,151,158,255};
    }
}

const char* MapSystem::IconShortName(MapIconType t) const {
    switch(t) {
        case MapIconType::Workshop:return "W"; case MapIconType::Garage:return "G"; case MapIconType::Job:return "J";
        case MapIconType::Objective:return "!"; case MapIconType::Pickup:return "P"; case MapIconType::Delivery:return "D";
        case MapIconType::Fuel:return "F"; case MapIconType::Repair:return "+"; case MapIconType::Shop:return "$";
        case MapIconType::Airport:return "A"; case MapIconType::Harbour:return "H"; case MapIconType::Hospital:return "+";
        case MapIconType::Company:return "C"; case MapIconType::TestTrack:return "T"; case MapIconType::Waypoint:return "*";
        case MapIconType::Event:return "!"; case MapIconType::Recovery:return "R"; case MapIconType::FastTravel:return ">>";
        case MapIconType::Vehicle:return "V"; default:return "?";
    }
}

void MapSystem::DrawMarkerIcon(Vector2 p, MapIconType type, float scale, float heading, Color tint, bool pulse) const {
    float r=6.0f*scale;
    if(pulse)r*=1.0f+0.10f*std::sin((float)GetTime()*4.0f);
    Color c=WithAlpha(IconColor(type),(float)tint.a/255.0f);

    if(type==MapIconType::Waypoint) {
        DrawCircleLines((int)p.x,(int)p.y,r+3,c);
        DrawLineEx({p.x-r,p.y},{p.x+r,p.y},2,c); DrawLineEx({p.x,p.y-r},{p.x,p.y+r},2,c);
        return;
    }
    if(type==MapIconType::Objective) {
        DrawCircleV(p,r+4,WithAlpha(BLACK,0.70f));
        DrawCircleV(p,r,c);
        DrawCircleLines((int)p.x,(int)p.y,r+5,WithAlpha(c,0.55f));
    } else if(type==MapIconType::Workshop || type==MapIconType::Garage || type==MapIconType::Company || type==MapIconType::TestTrack) {
        DrawRectangle((int)(p.x-r),(int)(p.y-r),(int)(r*2),(int)(r*2),WithAlpha(BLACK,0.65f));
        DrawRectangle((int)(p.x-r+2),(int)(p.y-r+2),(int)(r*2-4),(int)(r*2-4),c);
    } else {
        DrawCircleV(p,r+2,WithAlpha(BLACK,0.65f)); DrawCircleV(p,r,c);
    }
    const char* s=IconShortName(type);
    int fs=(int)std::max(10.0f,10.0f*scale);
    int tw=MeasureText(s,fs); DrawText(s,(int)(p.x-tw*0.5f),(int)(p.y-fs*0.5f),fs,WithAlpha(BLACK,(float)c.a/255.0f));
    (void)heading;
}

void MapSystem::DrawPlayerOrVehicleMarker(Vector2 p, float headingOnMap, float opacity) const {
    float r=10.0f;
    float a=headingOnMap*DEG2RAD;
    Vector2 dir{std::sin(a),-std::cos(a)};
    Vector2 right{dir.y,-dir.x};
    Vector2 tip=Vector2Add(p,Vector2Scale(dir,r+5));
    Vector2 back=Vector2Subtract(p,Vector2Scale(dir,r-1));
    Vector2 l=Vector2Add(back,Vector2Scale(right,7));
    Vector2 rr=Vector2Subtract(back,Vector2Scale(right,7));
    Color c=WithAlpha(vehicleOccupied?SKYBLUE:RAYWHITE,opacity);
    DrawTriangle(tip,l,rr,WithAlpha(BLACK,0.85f*opacity));
    Vector2 tip2=Vector2Subtract(tip,Vector2Scale(dir,2));
    DrawTriangle(tip2,Vector2Lerp(l,p,0.20f),Vector2Lerp(rr,p,0.20f),c);
}

void MapSystem::DrawMapContents(Rectangle clip, Vector2 centerWorldXZ, float scale,
                                float rotationYaw, bool fullMap, float opacity) {
    visibleMarkerCount=0;
    DrawRectangleRec(clip,WithAlpha({37,56,52,255},opacity));

    if(fullMap) {
        for(const auto& region:regions) {
            Vector3 a{region.boundsXZ.x,0,region.boundsXZ.y};
            Vector3 b{region.boundsXZ.x+region.boundsXZ.width,0,region.boundsXZ.y};
            Vector3 c{region.boundsXZ.x+region.boundsXZ.width,0,region.boundsXZ.y+region.boundsXZ.height};
            Vector3 d{region.boundsXZ.x,0,region.boundsXZ.y+region.boundsXZ.height};
            Vector2 pa=WorldToView(a,clip,centerWorldXZ,scale,rotationYaw),pb=WorldToView(b,clip,centerWorldXZ,scale,rotationYaw);
            Vector2 pc=WorldToView(c,clip,centerWorldXZ,scale,rotationYaw),pd=WorldToView(d,clip,centerWorldXZ,scale,rotationYaw);
            Color rc=region.discovered?WithAlpha(region.mapColor,0.46f*opacity):WithAlpha({24,29,31,255},0.70f*opacity);
            DrawQuad(pa,pb,pc,pd,rc);
            if(region.discovered && currentWorldMapZoom>0.55f) {
                Vector3 regionCenter{region.boundsXZ.x+region.boundsXZ.width*0.5f,0,region.boundsXZ.y+region.boundsXZ.height*0.5f};
                Vector2 rp=WorldToView(regionCenter,clip,centerWorldXZ,scale,rotationYaw);
                int fs=currentWorldMapZoom>1.2f?16:13;
                int tw=MeasureText(region.name.c_str(),fs);
                DrawText(region.name.c_str(),(int)rp.x-tw/2,(int)rp.y-fs/2,fs,WithAlpha({180,194,190,255},0.55f*opacity));
            }
            if(debug){DrawLineEx(pa,pb,1,ORANGE);DrawLineEx(pb,pc,1,ORANGE);DrawLineEx(pc,pd,1,ORANGE);DrawLineEx(pd,pa,1,ORANGE);}
        }
    }

    for(const auto& area:areas) {
        Vector3 a{area.boundsXZ.x,0,area.boundsXZ.y};
        Vector3 b{area.boundsXZ.x+area.boundsXZ.width,0,area.boundsXZ.y};
        Vector3 c{area.boundsXZ.x+area.boundsXZ.width,0,area.boundsXZ.y+area.boundsXZ.height};
        Vector3 d{area.boundsXZ.x,0,area.boundsXZ.y+area.boundsXZ.height};
        Vector2 pa=WorldToView(a,clip,centerWorldXZ,scale,rotationYaw),pb=WorldToView(b,clip,centerWorldXZ,scale,rotationYaw);
        Vector2 pc=WorldToView(c,clip,centerWorldXZ,scale,rotationYaw),pd=WorldToView(d,clip,centerWorldXZ,scale,rotationYaw);
        DrawQuad(pa,pb,pc,pd,WithAlpha(area.color,0.78f*opacity));
        if(area.outline){DrawLineEx(pa,pb,1,WithAlpha(LIGHTGRAY,0.35f*opacity));DrawLineEx(pb,pc,1,WithAlpha(LIGHTGRAY,0.35f*opacity));DrawLineEx(pc,pd,1,WithAlpha(LIGHTGRAY,0.35f*opacity));DrawLineEx(pd,pa,1,WithAlpha(LIGHTGRAY,0.35f*opacity));}
    }

    for(const auto& road:roads) {
        Vector2 a=WorldToView(road.start,clip,centerWorldXZ,scale,rotationYaw);
        Vector2 b=WorldToView(road.end,clip,centerWorldXZ,scale,rotationYaw);
        float w=Clamp(road.width*scale,fullMap?2.0f:2.0f,fullMap?9.0f:7.0f);
        DrawLineEx(a,b,w+2.0f,WithAlpha({22,28,30,255},opacity));
        DrawLineEx(a,b,w,WithAlpha(road.major?Color{116,126,128,255}:Color{95,105,107,255},opacity));
    }

    for(const auto& b:buildings) {
        Vector3 a{b.center.x-b.size.x*0.5f,0,b.center.z-b.size.y*0.5f};
        Vector3 bb{b.center.x+b.size.x*0.5f,0,b.center.z-b.size.y*0.5f};
        Vector3 c{b.center.x+b.size.x*0.5f,0,b.center.z+b.size.y*0.5f};
        Vector3 d{b.center.x-b.size.x*0.5f,0,b.center.z+b.size.y*0.5f};
        DrawQuad(WorldToView(a,clip,centerWorldXZ,scale,rotationYaw),WorldToView(bb,clip,centerWorldXZ,scale,rotationYaw),
                 WorldToView(c,clip,centerWorldXZ,scale,rotationYaw),WorldToView(d,clip,centerWorldXZ,scale,rotationYaw),WithAlpha(b.color,opacity));
    }

    if(route.active && route.points.size()>=2) {
        for(size_t i=1;i<route.points.size();++i) {
            Vector2 a=WorldToView(route.points[i-1],clip,centerWorldXZ,scale,rotationYaw);
            Vector2 b=WorldToView(route.points[i],clip,centerWorldXZ,scale,rotationYaw);
            DrawLineEx(a,b,fullMap?5.5f:4.0f,WithAlpha({45,209,233,255},0.95f*opacity));
            DrawLineEx(a,b,fullMap?1.5f:1.0f,WithAlpha(RAYWHITE,0.45f*opacity));
        }
    }

    for(const auto& marker:markers) {
        if(!MarkerShouldDraw(marker)||!MarkerPassesFilter(marker))continue;
        Vector2 p=WorldToView(marker.worldPosition,clip,centerWorldXZ,scale,rotationYaw);
        if(p.x<clip.x-18||p.x>clip.x+clip.width+18||p.y<clip.y-18||p.y>clip.y+clip.height+18)continue;
        bool unknown=!marker.discovered;
        DrawMarkerIcon(p,unknown?MapIconType::Unknown:marker.type,marker.iconScale,0,WithAlpha(WHITE,opacity),marker.tracked);
        visibleMarkerCount++;
        if(fullMap && marker.discovered && currentWorldMapZoom>0.72f) {
            DrawText(marker.name.c_str(),(int)p.x+10,(int)p.y-7,13,WithAlpha(RAYWHITE,0.88f*opacity));
        }
    }

    if(waypointActive && MarkerPassesFilter(MapMarker{0,MapIconType::Waypoint})) {
        Vector2 p=WorldToView(waypointPosition,clip,centerWorldXZ,scale,rotationYaw);
        DrawMarkerIcon(p,MapIconType::Waypoint,1.15f,0,WithAlpha(WHITE,opacity),false);
        visibleMarkerCount++;
        if(fullMap)DrawText("Custom Waypoint",(int)p.x+12,(int)p.y-7,13,WithAlpha(RAYWHITE,opacity));
    }

    if(gpsActive) {
        Vector2 p=WorldToView(gpsDestination,clip,centerWorldXZ,scale,rotationYaw);
        DrawMarkerIcon(p,MapIconType::Objective,1.30f,0,WithAlpha(WHITE,opacity),true);
        visibleMarkerCount++;
    }

    if(vehicleExists && !vehicleOccupied && filters.vehicles) {
        Vector2 p=WorldToView(vehiclePosition,clip,centerWorldXZ,scale,rotationYaw);
        DrawMarkerIcon(p,MapIconType::Vehicle,1.0f,0,WithAlpha(WHITE,opacity),false);
        visibleMarkerCount++;
    }

    Vector2 source=WorldToView(GameplayMapSource(),clip,centerWorldXZ,scale,rotationYaw);
    float markerHeading=NormalizeDegrees(rotationYaw-GameplayHeading());
    DrawPlayerOrVehicleMarker(source,markerHeading,opacity);
}

Vector2 MapSystem::ClampToMinimapEdge(Vector2 offset, float half) const {
    float m=std::max(std::fabs(offset.x),std::fabs(offset.y));
    if(m<=half)return offset;
    return Vector2Scale(offset,half/m);
}

void MapSystem::DrawCompass(Rectangle outer, Rectangle inner, float rotationYaw, float opacity) const {
    struct Cardinal{Vector3 dir;const char*label;};
    const Cardinal cards[4]={{{0,0,1},"N"},{{1,0,0},"E"},{{0,0,-1},"S"},{{-1,0,0},"W"}};
    Vector2 center=RectCenter(inner);
    float edge=inner.width*0.46f;
    for(const auto& card:cards) {
        Vector2 d=RotateMapDelta(card.dir,rotationYaw);
        float len=std::sqrt(d.x*d.x+d.y*d.y);if(len>0.001f){d.x/=len;d.y/=len;}
        Vector2 p={center.x+d.x*edge,center.y+d.y*edge};
        int tw=MeasureText(card.label,12);
        DrawText(card.label,(int)p.x-tw/2,(int)p.y-6,12,WithAlpha(card.label[0]=='N'?RAYWHITE:LIGHTGRAY,opacity));
    }
    (void)outer;
}

void MapSystem::DrawMinimap() {
    if(hidden)return;
    Rectangle outer=MinimapRect();
    float opacity=buildMode?0.58f:1.0f;
    float round=0.12f;
    DrawRectangleRounded(outer,round,8,WithAlpha({12,18,20,235},opacity));
    Rectangle inner{outer.x+9,outer.y+9,outer.width-18,outer.height-18};

    BeginScissorMode((int)inner.x,(int)inner.y,(int)inner.width,(int)inner.height);
    float range=60.0f/std::max(0.1f,currentMinimapZoom);
    float scale=(inner.width*0.5f)/range;
    float rotationYaw=orientation==MinimapOrientation::PlayerUp?GameplayHeading():0.0f;
    Vector3 src=GameplayMapSource();
    DrawMapContents(inner,{src.x,src.z},scale,rotationYaw,false,opacity);
    EndScissorMode();

    DrawRectangleRoundedLinesEx(outer,round,8,2.0f,WithAlpha({114,151,157,255},0.85f*opacity));
    DrawCompass(outer,inner,rotationYaw,opacity);

    // Active destination remains visible at the edge when it lies outside range.
    if(route.active) {
        Vector3 delta=Vector3Subtract(GetGPSDestination(),src);
        Vector2 local=RotateMapDelta(delta,rotationYaw);
        Vector2 px=Vector2Scale(local,scale);
        float edge=inner.width*0.5f-18.0f;
        if(std::max(std::fabs(px.x),std::fabs(px.y))>edge) {
            Vector2 edgeOffset=ClampToMinimapEdge(px,edge);
            Vector2 p=Vector2Add(RectCenter(inner),edgeOffset);
            DrawMarkerIcon(p,gpsActive?MapIconType::Objective:MapIconType::Waypoint,1.0f,0,WithAlpha(WHITE,opacity),gpsActive);
            float d=DistanceTo(GetGPSDestination());
            const char* dist=d>=1000.0f?TextFormat("%.1f km",d/1000.0f):TextFormat("%.0f m",d);
            int tw=MeasureText(dist,12);
            DrawText(dist,(int)Clamp(p.x-tw*0.5f,inner.x+3,inner.x+inner.width-tw-3),(int)Clamp(p.y+10,inner.y+2,inner.y+inner.height-15),12,WithAlpha(RAYWHITE,opacity));
        }
    }

    if(route.active && !buildMode) {
        float d=DistanceTo(GetGPSDestination());
        const std::string& name=GetGPSDestinationName();
        int fs=14;
        int maxW=(int)outer.width;
        DrawText(name.c_str(),(int)outer.x,(int)(outer.y+outer.height+8),fs,WithAlpha(RAYWHITE,opacity));
        const char* dist=d>=1000.0f?TextFormat("%.1f km",d/1000.0f):TextFormat("%.0f m",d);
        int tw=MeasureText(dist,fs); DrawText(dist,(int)(outer.x+maxW-tw),(int)(outer.y+outer.height+8),fs,WithAlpha(SKYBLUE,opacity));
        if(!objectiveText.empty())DrawText(objectiveText.c_str(),(int)outer.x,(int)(outer.y+outer.height+27),12,WithAlpha(LIGHTGRAY,opacity));
    }
}

void MapSystem::DrawFullscreenTransition() {
    float t=SmoothStep01(transitionProgress);
    Rectangle mini=MinimapRect(),full=FullscreenRect();
    Rectangle outer=LerpRect(mini,full,t);
    DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),WithAlpha({4,7,9,220},t));
    DrawRectangleRounded(outer,0.035f,10,WithAlpha({13,19,22,248},0.85f+0.15f*t));

    Rectangle finalView=FullscreenViewport(full);
    Rectangle miniInner{mini.x+9,mini.y+9,mini.width-18,mini.height-18};
    Rectangle view=LerpRect(miniInner,finalView,t);
    float miniRange=60.0f/std::max(0.1f,currentMinimapZoom);
    float miniScale=(miniInner.width*0.5f)/miniRange;
    float fullBase=std::min(finalView.width/worldBoundsXZ.width,finalView.height/worldBoundsXZ.height);
    float fullScale=fullBase*currentWorldMapZoom;
    float scale=LerpF(miniScale,fullScale,t);
    Vector3 src=GameplayMapSource();
    Vector2 center=LerpV2({src.x,src.z},currentMapPan,t);
    float startRot=orientation==MinimapOrientation::PlayerUp?GameplayHeading():0.0f;
    float rotation=LerpAngle(startRot,0.0f,t);

    BeginScissorMode((int)view.x,(int)view.y,(int)view.width,(int)view.height);
    DrawMapContents(view,center,scale,rotation,t>0.75f,1.0f);
    EndScissorMode();
    DrawRectangleLinesEx(view,2.0f,WithAlpha({102,135,142,255},0.85f));

    if(t>0.72f)DrawFullscreenChrome(full,finalView,(t-0.72f)/0.28f);
}

void MapSystem::DrawFullscreenChrome(Rectangle outer, Rectangle viewport, float opacity) {
    DrawText("WORLD MAP",(int)outer.x+24,(int)outer.y+18,28,WithAlpha(RAYWHITE,opacity));
    const char* orientationText=orientation==MinimapOrientation::PlayerUp?"Minimap: PLAYER-UP":"Minimap: NORTH-UP";
    int ow=MeasureText(orientationText,14);
    DrawText(orientationText,(int)(outer.x+outer.width-ow-24),(int)outer.y+24,14,WithAlpha(SKYBLUE,opacity));

    Rectangle panel=SidePanelRect(outer);
    DrawRectangleRounded(panel,0.04f,6,WithAlpha({20,27,30,235},opacity));
    DrawRectangleRoundedLinesEx(panel,0.04f,6,1.0f,WithAlpha({88,111,116,255},opacity));
    DrawSelectionPanel(panel,opacity);

    if(showInstructionBar){
        int by=(int)(outer.y+outer.height-40);
        DrawText("M Close | Wheel Zoom | RMB Waypoint | HOME Player | G GPS | O Orientation",(int)outer.x+24,by,14,WithAlpha(LIGHTGRAY,opacity));
        DrawText("Filters: [1] Jobs  [2] Services  [3] Vehicles  [4] Events",(int)viewport.x,(int)(viewport.y+viewport.height+10),13,WithAlpha(SKYBLUE,opacity));
    }

    // Compact legend, deliberately original engineering-style symbols.
    int lx=(int)panel.x+16,ly=(int)panel.y+panel.height-180;
    DrawText("LEGEND",lx,ly,14,WithAlpha(LIGHTGRAY,opacity));
    DrawText("W Workshop   J Job   ! Objective",lx,ly+20,12,WithAlpha(RAYWHITE,opacity));
    DrawText("V Vehicle    + Repair   * Waypoint",lx,ly+38,12,WithAlpha(RAYWHITE,opacity));
}

void MapSystem::DrawSelectionPanel(Rectangle panel, float opacity) {
    int x=(int)panel.x+16,y=(int)panel.y+16;
    const MapMarker* m=FindMarker(selectedMarkerId);
    if(!m) {
        DrawText("SELECTED LOCATION",x,y,16,WithAlpha(SKYBLUE,opacity));
        DrawText("Click a discovered marker",x,y+28,14,WithAlpha(LIGHTGRAY,opacity));
        DrawText("to inspect its details.",x,y+48,14,WithAlpha(LIGHTGRAY,opacity));
        if(waypointActive) {
            float d=DistanceTo(waypointPosition);
            DrawText("CUSTOM WAYPOINT",x,y+84,16,WithAlpha(IconColor(MapIconType::Waypoint),opacity));
            DrawText(d>=1000?TextFormat("Distance: %.1f km",d/1000):TextFormat("Distance: %.0f m",d),x,y+108,14,WithAlpha(RAYWHITE,opacity));
        }
    } else {
        DrawText(m->discovered?m->name.c_str():"Undiscovered Location",x,y,17,WithAlpha(RAYWHITE,opacity));
        DrawText(TextFormat("Region: %s",RegionName(m->region)),x,y+30,13,WithAlpha(LIGHTGRAY,opacity));
        float d=DistanceTo(m->worldPosition);
        DrawText(d>=1000?TextFormat("Distance: %.1f km",d/1000):TextFormat("Distance: %.0f m",d),x,y+51,13,WithAlpha(LIGHTGRAY,opacity));
        if(m->discovered) {
            if(!m->services.empty())DrawText(TextFormat("Services: %s",m->services.c_str()),x,y+78,12,WithAlpha(SKYBLUE,opacity));
            if(m->availableJobs>0)DrawText(TextFormat("Available Jobs: %d",m->availableJobs),x,y+101,13,WithAlpha(RAYWHITE,opacity));
            if(m->jobReward>0)DrawText(TextFormat("Example Reward: $%d",m->jobReward),x,y+122,13,WithAlpha(GREEN,opacity));
            if(m->jobDifficulty>0)DrawText(TextFormat("Difficulty: %d / 5",m->jobDifficulty),x,y+143,13,WithAlpha(YELLOW,opacity));
            if(!m->description.empty()) {
                std::string line=m->description.substr(0,std::min<size_t>(m->description.size(),38));
                DrawText(line.c_str(),x,y+170,12,WithAlpha(LIGHTGRAY,opacity));
            }
        }
    }

    Rectangle setBtn{panel.x+16,panel.y+panel.height-92,panel.width-32,32};
    Rectangle clrBtn{panel.x+16,panel.y+panel.height-52,panel.width-32,28};
    Color bc=selectedMarkerId?Color{46,107,119,255}:Color{48,56,59,255};
    DrawRectangleRounded(setBtn,0.15f,4,WithAlpha(bc,opacity));
    DrawText("SET WAYPOINT",(int)setBtn.x+12,(int)setBtn.y+8,14,WithAlpha(RAYWHITE,selectedMarkerId?opacity:0.35f*opacity));
    if(waypointActive) {
        DrawRectangleRounded(clrBtn,0.15f,4,WithAlpha({83,50,67,255},opacity));
        DrawText("REMOVE WAYPOINT",(int)clrBtn.x+12,(int)clrBtn.y+6,13,WithAlpha(RAYWHITE,opacity));
    }
}

void MapSystem::DrawDebugOverlay() const {
    if(!debug)return;
    int x=16,y=GetScreenHeight()-230;
    DrawRectangle(x,y,390,210,Fade(BLACK,0.86f));
    DrawText("MAP DEBUG [F9]",x+12,y+10,18,SKYBLUE);
    DrawText(TextFormat("Player world: %.1f, %.1f",playerPosition.x,playerPosition.z),x+12,y+38,15,RAYWHITE);
    float nx=(playerPosition.x-worldBoundsXZ.x)/worldBoundsXZ.width;
    float nz=(playerPosition.z-worldBoundsXZ.y)/worldBoundsXZ.height;
    DrawText(TextFormat("Player map: %.3f, %.3f",nx,nz),x+12,y+59,15,RAYWHITE);
    DrawText(TextFormat("Heading: %.1f deg",GameplayHeading()),x+12,y+80,15,RAYWHITE);
    DrawText(TextFormat("Minimap zoom: %.3f",currentMinimapZoom),x+12,y+101,15,RAYWHITE);
    DrawText(TextFormat("World zoom: %.3f",currentWorldMapZoom),x+12,y+122,15,RAYWHITE);
    DrawText(TextFormat("Visible markers: %d",visibleMarkerCount),x+12,y+143,15,RAYWHITE);
    DrawText(TextFormat("GPS: %s",route.active?GetGPSDestinationName().c_str():"None"),x+12,y+164,15,RAYWHITE);
    DrawText(TextFormat("Waypoint: %s",waypointActive?"Set":"None"),x+12,y+185,15,RAYWHITE);
}

void MapSystem::Draw() {
    if(hidden)return;
    if(transitionProgress<=0.001f && displayMode==MapDisplayMode::Minimap)DrawMinimap();
    else DrawFullscreenTransition();
    DrawDebugOverlay();
}

void MapSystem::DrawWorldMarkers3D() const {
    if(waypointActive) {
        Vector3 p=waypointPosition;p.y=0.08f;
        DrawCylinder(p,0.65f,0.65f,0.12f,20,WithAlpha(IconColor(MapIconType::Waypoint),0.75f));
        DrawLine3D(Vector3Add(p,{0,0.1f,0}),Vector3Add(p,{0,5.0f,0}),WithAlpha(IconColor(MapIconType::Waypoint),0.55f));
    }
}

MapSaveData MapSystem::ExportSaveData() const {
    MapSaveData d;
    d.orientation=orientation;d.hasWaypoint=waypointActive;d.waypoint=waypointPosition;
    d.worldMapZoom=currentWorldMapZoom;d.worldMapPan=currentMapPan;d.filters=filters;d.trackedMarkerId=trackedMarkerId;
    for(const auto&m:markers){if(m.discovered)d.discoveredMarkerIds.push_back(m.id);if(m.fastTravel&&m.discovered)d.fastTravelMarkerIds.push_back(m.id);}
    for(const auto&r:regions)if(r.discovered)d.discoveredRegionIds.push_back((int)r.id);
    return d;
}

void MapSystem::ApplySaveData(const MapSaveData& d) {
    orientation=d.orientation;waypointActive=d.hasWaypoint;waypointPosition=d.waypoint;
    currentWorldMapZoom=targetWorldMapZoom=Clamp(d.worldMapZoom,minWorldMapZoom,maxWorldMapZoom);
    currentMapPan=targetMapPan=d.worldMapPan;filters=d.filters;trackedMarkerId=d.trackedMarkerId;
    for(int id:d.discoveredMarkerIds){MapMarker*m=FindMarker(id);if(m)m->discovered=true;}
    for(int id:d.fastTravelMarkerIds){MapMarker*m=FindMarker(id);if(m){m->fastTravel=true;m->discovered=true;}}
    for(int id:d.discoveredRegionIds){for(auto&r:regions)if((int)r.id==id)r.discovered=true;}
    ClampMapPan();
}
