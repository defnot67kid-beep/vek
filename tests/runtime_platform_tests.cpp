#include <vek/VekRuntimeSystems.h>
#include <vek/VekPhysicsSystems.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
int main(){
    vek::RuntimePlatformPack pack;VekScriptEngine vm;VekRegisterStandardLibrary(vm);pack.RegisterNatives(vm);
    pack.performance.PushFrame(1.0/60.0,4.2,5.1);pack.performance.PushFrame(1.0/30.0,8.0,11.0);
    vek::GpuCapabilities caps;caps.adapterName="Test GPU";caps.backend=vek::GpuBackend::Vulkan;caps.computeShaders=true;caps.rayTracing=true;pack.gpu.SetCapabilities(caps);pack.gpu.SetBudget({1024,512,64});
    assert(vm.LoadSource(R"VEK(fn setup(){gui_define({id:"hud",type:"panel"});gameplay_define({id:"dash",type:"ability",cooldown:1});return perf_stats().frames+gui_count()+gameplay_count();} fn ray(){return gpu_supports("ray_tracing");})VEK"));
    assert(vm.Call("setup").AsNumber()==4);assert(vm.Call("ray").AsBool());
    vek::PhysicsDefinitionRegistry physics;std::string err;VekMap rag;rag["id"]="rag.player";assert(physics.RegisterDefinition("ragdoll",VekValue(rag),&err));assert(std::string(vek::PhysicsDefinitionRegistry::ApiVersion)=="0.4");
    std::cout<<"VEK runtime platform/physics v0.4 tests: PASS\n";
}
