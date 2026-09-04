#include <vek/VekGameSystems.h>
#include <vek/VekPhysicsSystems.h>
#include <vek/VekScriptEngine.h>
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    using namespace vek;
    assert(std::string(PhysicsDefinitionRegistry::ApiVersion)=="0.4");
    PhysicsDefinitionRegistry physics;
    std::string err;
    VekMap scene; scene["id"]="scene.vehicle_shop";
    assert(physics.RegisterDefinition("scene_settings",VekValue(scene),&err));
    VekMap drive; drive["id"]="drive.awd";
    assert(physics.RegisterDefinition("drivetrain",VekValue(drive),&err));

    VehicleDynamicsConfig cfg; cfg.mass=1350.0f; cfg.engineForce=9500.0f; cfg.tireGrip=1.15f;
    VehicleDynamicsState state; VehicleDynamicsInput input; input.throttle=1.0f; input.steer=0.35f;
    VehicleDynamicsOutput out{};
    for(int i=0;i<120;++i) out=VehicleDynamicsModel::Step(state,cfg,input,1.0f/60.0f);
    assert(state.longitudinalSpeed>0.5f);
    assert(std::isfinite(state.yawRate));
    assert(out.tractionLimit>1000.0f);

    CameraDefinitionRegistry cameras;
    VekMap lens; lens["id"]="lens.cinematic";
    assert(cameras.RegisterDefinition("lens",VekValue(lens),&err));
    VekMap shot; shot["id"]="shot.orbit";
    assert(cameras.RegisterDefinition("shot",VekValue(shot),&err));
    assert(cameras.Count("shot")==1);

    CameraPose a,b; a.position={0,2,-10}; a.target={0,1,0}; a.fovDegrees=55;
    b.position={10,4,0}; b.target={0,1,0}; b.fovDegrees=45;
    auto mid=CameraBlendSystem::Blend(a,b,0.5f,CameraBlendCurve::EaseInOut);
    assert(mid.position.x>4.9f && mid.position.x<5.1f);
    assert(mid.fovDegrees>49.0f && mid.fovDegrees<51.0f);

    VekScriptEngine vm; VekRegisterStandardLibrary(vm); cameras.RegisterNatives(vm); physics.RegisterNatives(vm);
    assert(vm.LoadSource("fn test(){ return camera_version() + \":\" + physics_version(); }","v32.vek"));
    assert(vm.Call("test").AsString()=="1.0:0.4");
    std::cout << "VEK 3.2 camera + physics v0.4 tests passed\n";
    return 0;
}
