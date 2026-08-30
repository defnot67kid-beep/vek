#include <VekPhysicsSystems.h>
#include <VekScriptEngine.h>
#include <cassert>
#include <cmath>
#include <iostream>

static float dist(vek::PhysicsVec3 a,vek::PhysicsVec3 b){float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return std::sqrt(x*x+y*y+z*z);} 

int main(){
    vek::SpringChainSettings s;
    s.segments=5;
    s.segmentLength=0.10f;
    s.motion.gravity=8.0f;
    s.motion.stiffness=0.25f;
    s.motion.damping=0.92f;
    s.motion.constraintIterations=8;
    vek::SpringChain3D chain;
    chain.Configure(s);
    vek::PhysicsVec3 root{0,0.27f,0};
    chain.Reset(root,{0,-1,0});
    for(int i=0;i<240;++i){
        float side=(i<60)?4.0f:0.0f;
        chain.Step(root,{side,0,2.0f},{0,0,0},0.255f,1.0f/120.0f);
    }
    const auto& p=chain.Particles();
    assert(p.size()==6);
    for(std::size_t i=1;i<p.size();++i){
        float d=dist(p[i].position,p[i-1].position);
        assert(std::fabs(d-0.10f)<0.012f);
        assert(dist(p[i].position,{0,0,0})>=0.255f-0.001f);
    }

    vek::SecondaryMotionProfileRegistry profiles;
    VekScriptEngine vm;
    profiles.RegisterNatives(vm);
    bool loaded=vm.LoadSource(R"(
        fn register_test_profile() {
            return secondary_motion_profile_register({
                id: "hair.curly",
                gravity: 5.5,
                stiffness: 0.42
            });
        }
    )", "<physics-test>");
    assert(loaded);
    auto ok=vm.Call("register_test_profile");
    assert(ok.AsBool());
    assert(profiles.Find("hair.curly")!=nullptr);

    vek::PhysicsDefinitionRegistry advanced;
    advanced.RegisterNatives(vm);
    bool advancedLoaded=vm.LoadSource(R"(
        fn register_advanced() {
            if (physics_v02_version() != "0.2") { return false; }
            physics_v02_definition_register("material", {id:"material.tire", static_friction:1.1});
            physics_v02_definition_register("rigid_body", {id:"body.car", mass:1250, type:"dynamic"});
            return physics_v02_definition_exists("material", "material.tire") && physics_v02_definition_count("rigid_body") == 1;
        }
    )", "<physics-v02-test>");
    assert(advancedLoaded);
    assert(vm.Call("register_advanced").AsBool());
    assert(advanced.Count("material")==1);
    assert(advanced.Find("rigid_body","body.car")!=nullptr);
    std::string error;
    assert(!advanced.RegisterDefinition("unknown",VekValue(VekMap{{"id",VekValue("x")}}),&error));
    std::cout<<"VEK secondary motion + Physics Definitions v0.2 tests passed\n";
    return 0;
}
