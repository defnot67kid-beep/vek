#include "MapSystem.h"
#include "World.h"
#include <cassert>
#include <cmath>
#include <iostream>

static bool Near(float a,float b,float eps=0.02f){return std::fabs(a-b)<=eps;}

static float SimSmooth(float fps) {
    float value=1.25f,target=0.48f;
    float dt=1.0f/fps;
    int frames=(int)std::round(fps*2.0f);
    for(int i=0;i<frames;i++) value += (target-value)*MapSystem::SmoothAlpha(5.5f,dt);
    return value;
}

static float SimTransition(float fps) {
    float t=0.0f,dt=1.0f/fps;
    int frames=(int)std::ceil(0.30f*fps);
    for(int i=0;i<frames;i++) t=MapSystem::StepTransition(t,1.0f,dt,0.24f);
    return t;
}

int main() {
    assert(Near(MapSystem::NormalizeDegrees(360.0f),0.0f,0.001f));
    assert(Near(MapSystem::NormalizeDegrees(-1.0f),359.0f,0.001f));

    Vector2 north=MapSystem::RotateMapDelta({0,0,10},0.0f);
    assert(std::fabs(north.x)<0.001f && north.y<0.0f);

    // Requested game compass: East is left and West is right.
    Vector2 east=MapSystem::RotateMapDelta({10,0,0},0.0f);
    Vector2 west=MapSystem::RotateMapDelta({-10,0,0},0.0f);
    assert(east.x<0.0f && std::fabs(east.y)<0.001f);
    assert(west.x>0.0f && std::fabs(west.y)<0.001f);

    // At 90 degrees, world +X is the player's forward direction and therefore
    // must appear toward the top of a PlayerUp minimap.
    Vector2 eastAsForward=MapSystem::RotateMapDelta({10,0,0},90.0f);
    assert(std::fabs(eastAsForward.x)<0.01f && eastAsForward.y<0.0f);

    float v30=SimSmooth(30),v60=SimSmooth(60),v120=SimSmooth(120),v144=SimSmooth(144);
    assert(Near(v30,v60,0.002f));
    assert(Near(v60,v120,0.002f));
    assert(Near(v120,v144,0.002f));

    assert(Near(SimTransition(30),1.0f,0.001f));
    assert(Near(SimTransition(60),1.0f,0.001f));
    assert(Near(SimTransition(120),1.0f,0.001f));
    assert(Near(SimTransition(144),1.0f,0.001f));

    // Shared minimap/world-map state and persistence behavior.
    World world;
    MapSystem map;
    map.Initialize(world);
    map.SetPlayerPosition({0,1,-8});
    map.SetPlayerHeading(42.0f);
    map.SetWaypoint({999,0,-999});
    Vector3 clamped=map.GetWaypoint();
    assert(clamped.x<=150.0f && clamped.z>=-150.0f);

    map.SetGPSDestination(world.pickup,"Cargo Pickup",MapIconType::Pickup);
    assert(map.HasGPSDestination());
    assert(std::fabs(map.GetGPSDestination().z-world.pickup.z)<0.001f);
    map.ClearGPSDestination();
    assert(!map.HasGPSDestination());
    assert(map.HasWaypoint());
    assert(map.GetGPSDestinationName()=="Custom Waypoint");

    MapSaveData saved=map.ExportSaveData();
    saved.orientation=MinimapOrientation::NorthUp;
    saved.worldMapZoom=2.25f;
    MapSystem restored;
    restored.Initialize(world);
    restored.ApplySaveData(saved);
    assert(restored.GetOrientation()==MinimapOrientation::NorthUp);
    assert(restored.HasWaypoint());
    assert(Near(restored.GetWorldMapZoom(),2.25f,0.001f));

    std::cout << "MapSystem math/state/transition tests: PASS\n";
    return 0;
}
