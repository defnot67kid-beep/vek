#include "CollisionSystem.h"
#include "World.h"
#include "Vehicle.h"
#include "HumanoidSystem.h"
#include <cmath>
#include <iostream>

#ifndef VEK_TEST_SCRIPT_PATH
#define VEK_TEST_SCRIPT_PATH "scripts/collision.vek"
#endif

static int failures = 0;
static void Check(bool condition, const char* name) {
    if (!condition) { std::cerr << "FAIL: " << name << "\n"; ++failures; }
}

int main() {
    Vector3 normal{}, point{};
    float penetration = 0.0f;
    bool hit = CollisionSystem::ResolveCircleVsBox(
        {1.2f,1.0f,0.0f}, 0.5f,
        {0.0f,1.0f,0.0f}, {2.0f,2.0f,2.0f},
        normal, penetration, point);
    Check(hit, "circle overlaps box edge");
    Check(normal.x > 0.9f, "collision normal points outward");
    Check(penetration > 0.29f && penetration < 0.31f, "penetration depth correct");

    hit = CollisionSystem::ResolveCircleVsBox(
        {5.0f,1.0f,5.0f}, 0.5f,
        {0.0f,1.0f,0.0f}, {2.0f,2.0f,2.0f},
        normal, penetration, point);
    Check(!hit, "separated circle does not collide");

    Check(!CollisionSystem::VerticalSpansOverlap(0.0f, 2.55f,
                                                  {0.0f,4.0f,0.0f}, {2.0f,2.0f,2.0f}),
          "overhead header does not overlap player height");
    Check(CollisionSystem::VerticalSpansOverlap(0.0f, 2.55f,
                                                {0.0f,1.5f,0.0f}, {2.0f,3.0f,2.0f}),
          "door slab overlaps player height");

    World world;
    auto hasGarageGate=[&](){
        for(const auto& box:world.CollisionBoxes()) if(box.id==15) return true;
        return false;
    };
    Check(hasGarageGate(), "closed garage has collision gate");
    world.SetGarageDoorState(0.59f,false);
    Check(hasGarageGate(), "garage remains solid below 60 percent clearance");
    world.SetGarageDoorState(0.61f,false);
    Check(!hasGarageGate(), "garage collision clears above 60 percent opening");
    world.SetGarageDoorState(0.0f,true);
    Check(hasGarageGate(), "closing garage restores collision gate");

    CollisionSystem system;
    Check(system.Initialize(world, VEK_TEST_SCRIPT_PATH), "collision.vek loads through VEK runtime");
    Check(system.ScriptLoaded(), "script loaded flag");

    Vector3 playerPos{0.0f,1.0f,-12.8f};
    Vector3 velocity{0.0f,0.0f,-3.0f};
    HumanoidSystem humanoid;
    system.ResolvePlayer(playerPos, velocity, humanoid, nullptr, false);
    Check(playerPos.z > -12.8f, "player is pushed out of workshop wall");
    Check(velocity.z > -0.5f, "player inward velocity removed");

    // Regression: once the personnel door is open, its overhead header remains
    // in world geometry. The old X/Z-only solver treated that header as an
    // invisible wall and blocked the scripted/dev entry sequence.
    world.SetPersonnelDoorTarget(true);
    for(int i=0;i<20;++i) world.UpdatePersonnelDoor(0.05f,2.0f);
    system.RefreshWorldGeometry(world);
    Vector3 door = world.PersonnelDoorPosition();
    Vector3 doorwayPos{door.x,0.15f,door.z};
    Vector3 doorwayVelocity{0.0f,0.0f,-2.0f};
    system.ResolvePlayer(doorwayPos, doorwayVelocity, humanoid, nullptr, false, 2.55f, 0.15f);
    Check(std::fabs(doorwayPos.z-door.z)<0.05f, "open personnel doorway is not blocked by overhead header");

    // Same regression for the main garage lintel when the garage gate itself
    // has cleared. A ground actor must be able to pass under high geometry.
    world.SetGarageDoorState(1.0f,false);
    system.RefreshWorldGeometry(world);
    Vector3 garage = world.GarageDoorPosition();
    Vector3 garagePos{garage.x,0.15f,garage.z};
    Vector3 garageVelocity{0.0f,0.0f,-2.0f};
    system.ResolvePlayer(garagePos, garageVelocity, humanoid, nullptr, false, 2.55f, 0.15f);
    Check(std::fabs(garagePos.z-garage.z)<0.05f, "open garage is not blocked by overhead header");

    if (failures == 0) {
        std::cout << "VEK collision system tests: PASS\n";
        return 0;
    }
    std::cout << "VEK collision system tests: FAIL (" << failures << ")\n";
    return 1;
}
