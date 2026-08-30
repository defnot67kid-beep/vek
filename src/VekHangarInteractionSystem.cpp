// v26 note: animated door interaction, richer character rig, fuller hair, day/night cycle, and dev cheat panel groundwork.
#include "VekHangarInteractionSystem.h"
#include "VekSecuritySystem.h"
#include "VekHostSecurity.h"

bool VekHangarInteractionSystem::Initialize(const std::string& p){file=p;return Load();}
bool VekHangarInteractionSystem::Reload(){return Load();}
float VekHangarInteractionSystem::AnimationDuration(const std::string&id,float fallback)const{auto*a=animations.Find(id);return a?a->duration:fallback;}

bool VekHangarInteractionSystem::BuildPasslockGui(const std::string& maskedInput,const std::string& status){
    if(!vm||!vm->HasFunction("build_passlock_ui"))return false;
    gui.BeginFrame();
    auto result=vm->Call("build_passlock_ui",{VekValue(maskedInput),VekValue(status)});
    gui.EndFrame();
    if(!vm->LastError().empty()){error=vm->LastError();return false;}
    return result.IsNil()?true:result.AsBool(true);
}

bool VekHangarInteractionSystem::Load(){
    auto verified=VekSecuritySystem::VerifyScript(file,file+".sig");
    if(!verified.ok){error=verified.error;return false;}

    auto nextVm=std::make_unique<VekScriptEngine>();
    ApplyGameVekSecurityPolicy(*nextVm);
    VekRegisterStandardLibrary(*nextVm);
    vek::VekRegisterGameplayLibrary(*nextVm);
    animations.Clear();garages.Clear();passlocks.Clear();gui.BeginFrame();
    animations.RegisterNatives(*nextVm);
    garages.RegisterNatives(*nextVm);
    passlocks.RegisterNatives(*nextVm);
    gui.RegisterNatives(*nextVm);
    nextVm->SealNativeRegistry();
    if(!nextVm->LoadSource(verified.source,file)){error=nextVm->LastError();return false;}
    auto setup=nextVm->Call("setup_hangar_interactions");
    if(!setup.AsBool(false)){error="hangar_interactions.vek setup_hangar_interactions() failed";return false;}

    HangarDoorSettings nextDoor=door;
    if(nextVm->HasFunction("door_config")){
        auto config=nextVm->Call("door_config");
        if(config.IsMap()){
            nextDoor.offsetX=(float)config.Get("offset_x").AsNumber(nextDoor.offsetX);
            nextDoor.width=(float)config.Get("width").AsNumber(nextDoor.width);
            nextDoor.height=(float)config.Get("height").AsNumber(nextDoor.height);
            nextDoor.openAngle=(float)config.Get("open_angle").AsNumber(nextDoor.openAngle);
            nextDoor.openSpeed=(float)config.Get("open_speed").AsNumber(nextDoor.openSpeed);
            nextDoor.autoCloseDelay=(float)config.Get("auto_close_delay").AsNumber(nextDoor.autoCloseDelay);
        }
    }

    if(!nextVm->HasFunction("access_config")){error="access_config() is required";return false;}
    auto accessConfig=nextVm->Call("access_config");
    if(!accessConfig.IsMap()){error="access_config() must return a map";return false;}
    HangarAccessSettings nextAccess;
    nextAccess.passlockOffsetX=(float)accessConfig.Get("passlock_offset_x").AsNumber(nextAccess.passlockOffsetX);
    nextAccess.passlockHeight=(float)accessConfig.Get("passlock_height").AsNumber(nextAccess.passlockHeight);
    auto read=[&](const char*k,std::string&out){auto v=accessConfig.Get(k);if(v.IsString()&&!v.AsString().empty())out=v.AsString();};
    read("garage_id",nextAccess.garageId);read("passlock_id",nextAccess.passlockId);read("keypad_clip",nextAccess.keypadClip);read("garage_open_clip",nextAccess.garageOpenClip);read("garage_close_clip",nextAccess.garageCloseClip);

    if(nextDoor.width<1.2f||nextDoor.width>5.0f||nextDoor.height<2.0f||nextDoor.height>6.0f){error="VEK personnel-door settings rejected by native safety limits";return false;}
    const auto* garage=garages.Find(nextAccess.garageId);
    const auto* passlock=passlocks.Find(nextAccess.passlockId);
    if(!garage){error="VEK main garage was not registered";return false;}
    if(!passlock){error="VEK passlock was not registered";return false;}
    if(passlock->linkedGarageId!=garage->id){error="VEK passlock garage link is invalid";return false;}
    if(garage->width<8.0f||garage->width>40.0f||garage->height<4.0f||garage->height>15.0f){error="VEK garage dimensions rejected by native safety limits";return false;}
    if(nextAccess.passlockHeight<0.7f||nextAccess.passlockHeight>2.5f){error="VEK passlock height rejected by native safety limits";return false;}
    if(!nextVm->HasFunction("build_passlock_ui")){error="build_passlock_ui() is required";return false;}

    door=nextDoor;access=nextAccess;vm=std::move(nextVm);error.clear();return true;
}
