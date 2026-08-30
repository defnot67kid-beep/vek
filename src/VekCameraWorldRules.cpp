#include "VekCameraWorldRules.h"
#include "VekSecuritySystem.h"
#include "VekHostSecurity.h"

bool VekCameraWorldRules::Initialize(const std::string& p){file=p;return Load();}
bool VekCameraWorldRules::Reload(){return Load();}

bool VekCameraWorldRules::Load(){
    auto verified=VekSecuritySystem::VerifyScript(file,file+".sig");
    if(!verified.ok){error=verified.error;return false;}

    auto next=std::make_unique<VekScriptEngine>();
    ApplyGameVekSecurityPolicy(*next);
    VekRegisterStandardLibrary(*next);
    vek::VekRegisterGameplayLibrary(*next);
    cameras.Clear();skyboxes.Clear();rigs.Clear();policies.Clear();
    cameras.RegisterNatives(*next);skyboxes.RegisterNatives(*next);rigs.RegisterNatives(*next);policies.RegisterNatives(*next);
    next->SealNativeRegistry();
    if(!next->LoadSource(verified.source,file)){error=next->LastError();return false;}
    auto setup=next->Call("setup_camera_world");
    if(!next->LastError().empty()){error=next->LastError();return false;}
    if(!setup.AsBool(false)){error="camera_world.vek setup_camera_world() failed";return false;}

    if(next->HasFunction("camera_world_ids")){
        auto ids=next->Call("camera_world_ids");
        if(ids.IsMap()){
            auto s=ids.Get("camera").AsString();if(!s.empty())cameraId=s;
            s=ids.Get("skybox").AsString();if(!s.empty())skyboxId=s;
            s=ids.Get("rig").AsString();if(!s.empty())rigId=s;
            s=ids.Get("world_policy").AsString();if(!s.empty())policyId=s;
        }
    }
    if(!cameras.Find(cameraId)){error="VEK camera profile missing";return false;}
    if(!skyboxes.Find(skyboxId)){error="VEK skybox profile missing";return false;}
    if(!rigs.Find(rigId)){error="VEK humanoid rig missing";return false;}
    if(!policies.Find(policyId)){error="VEK world policy missing";return false;}
    vm=std::move(next);error.clear();return true;
}
