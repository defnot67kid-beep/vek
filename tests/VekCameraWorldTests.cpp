#include <VekGameSystems.h>
#include <VekScriptEngine.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#ifndef VEK_CAMERA_WORLD_SCRIPT_PATH
#define VEK_CAMERA_WORLD_SCRIPT_PATH "scripts/camera_world.vek"
#endif
int main(){
    std::ifstream f(VEK_CAMERA_WORLD_SCRIPT_PATH,std::ios::binary);assert(f.good());std::ostringstream ss;ss<<f.rdbuf();
    VekScriptEngine vm;VekRegisterStandardLibrary(vm);vek::VekRegisterGameplayLibrary(vm);
    vek::CameraProfileRegistry cameras;vek::SkyboxRegistry skies;vek::HumanoidRigRegistry rigs;vek::WorldGameplayPolicyRegistry policies;
    cameras.RegisterNatives(vm);skies.RegisterNatives(vm);rigs.RegisterNatives(vm);policies.RegisterNatives(vm);vm.SealNativeRegistry();
    assert(vm.LoadSource(ss.str(),VEK_CAMERA_WORLD_SCRIPT_PATH));assert(vm.Call("setup_camera_world").AsBool(false));assert(vm.LastError().empty());
    auto* camera=cameras.Find("camera.player");assert(camera&&camera->rmbLook&&camera->cycleModes.size()>=3);
    auto* sky=skies.Find("sky.engineering_day");assert(sky&&sky->fogEnd>sky->fogStart);
    auto* rig=rigs.Find("rig.player_mechanic_v2");assert(rig&&rig->joints.size()>=20);
    auto* policy=policies.Find("game.default");assert(policy&&policy->targetFps>=60&&policy->movementSprint>policy->movementRun);
    std::cout<<"VEK camera/world/sky/rig script tests: PASS\n";
}
