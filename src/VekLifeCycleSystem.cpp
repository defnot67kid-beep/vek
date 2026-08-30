#include "VekLifeCycleSystem.h"
#include "VekSecuritySystem.h"
#include "VekHostSecurity.h"

bool VekLifeCycleSystem::Initialize(const std::string& p){file=p;return Load();}
bool VekLifeCycleSystem::Reload(){return Load();}

float VekLifeCycleSystem::GroundRootY(float surfaceY,float avatarHeight) const{
    const auto* p=PlayerGrounding();
    if(!p) return surfaceY + 0.15f*avatarHeight;
    return vek::GroundingSystem::RootYForSurface(surfaceY,avatarHeight,*p);
}

bool VekLifeCycleSystem::Load(){
    auto verified=VekSecuritySystem::VerifyScript(file,file+".sig");
    if(!verified.ok){error=verified.error;return false;}

    auto next=std::make_unique<VekScriptEngine>();
    ApplyGameVekSecurityPolicy(*next);
    VekRegisterStandardLibrary(*next);
    vek::VekRegisterGameplayLibrary(*next);
    audio.Clear();effects.Clear();deaths.Clear();grounding.Clear();
    audio.RegisterNatives(*next);effects.RegisterNatives(*next);deaths.RegisterNatives(*next);grounding.RegisterNatives(*next);
    next->SealNativeRegistry();
    if(!next->LoadSource(verified.source,file)){error=next->LastError();return false;}
    auto setup=next->Call("setup_lifecycle");
    if(!next->LastError().empty()){error=next->LastError();return false;}
    if(!setup.AsBool(false)){error="lifecycle.vek setup_lifecycle() failed";return false;}

    if(next->HasFunction("lifecycle_ids")){
        auto ids=next->Call("lifecycle_ids");
        if(ids.IsMap()){
            auto r=ids.Get("reset_death").AsString(); if(!r.empty()) resetDeathId=r;
            auto g=ids.Get("player_grounding").AsString(); if(!g.empty()) groundingId=g;
        }
    }
    const auto* death=deaths.Find(resetDeathId);
    if(!death){error="VEK reset death sequence was not registered";return false;}
    if(!effects.Find(death->screenEffectId)){error="VEK reset death screen effect is missing";return false;}
    if(!audio.Find(death->audioCueId)){error="VEK reset death audio cue is missing";return false;}
    if(!grounding.Find(groundingId)){error="VEK player grounding profile is missing";return false;}
    vm=std::move(next);error.clear();return true;
}
