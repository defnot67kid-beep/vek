#include "VekAuthorityRules.h"
#include "VekSecuritySystem.h"
#include <algorithm>

bool VekAuthorityRules::Initialize(const std::string& p){file=p;return Load();}
bool VekAuthorityRules::Reload(){return CanHotReload()?Load():false;}
bool VekAuthorityRules::CanHotReload()const{return VekSecuritySystem::IsDevelopmentMode();}
bool VekAuthorityRules::AllowsLocalCommit(const std::string& id)const{return authority.ValidateAuthoritativeCommit(id,actions).allowed;}
bool VekAuthorityRules::AllowsDeveloperFeature(const std::string& feature,bool developerUnlocked)const{
    vek::DeveloperAccessContext ctx;
    ctx.tier=VekSecuritySystem::IsDevelopmentMode()?vek::SecurityTier::Development:vek::SecurityTier::HardenedClient;
    ctx.localSession=true;
    ctx.authenticatedDeveloper=developerUnlocked;
    // The cheat implementation is compiled into the trusted native host. It is
    // not supplied by a downloaded player script, so it satisfies signed-tooling
    // policy for this local development host.
    ctx.signedTooling=true;
    return developerGate.Allows(feature,ctx);
}

bool VekAuthorityRules::Load(){
    developerGate=vek::DeveloperFeatureGate{};
    if(VekSecuritySystem::IsDevelopmentMode()){
        for(const char* feature:{"dev.panel","dev.garage.unlock","dev.performance_hud","dev.fly","dev.noclip","dev.god_mode","dev.teleport","dev.movement_speed","dev.garage.toggle"})
            developerGate.Grant(feature);
    }
    developerGate.Seal();
    auto verified=VekSecuritySystem::VerifyScript(file,file+".sig");
    if(!verified.ok){error=verified.error;return false;}
    auto next=std::make_unique<VekScriptEngine>();
    VekRegisterStandardLibrary(*next);
    vek::SecurityPolicyFactory::Apply(*next,VekSecuritySystem::IsDevelopmentMode()?vek::SecurityTier::Development:vek::SecurityTier::HardenedClient);
    actions.Clear();replication.Clear();
    vek::VekRegisterAuthorityLibrary(*next,&actions,&replication);
    next->SealNativeRegistry();
    if(!next->LoadSource(verified.source,file)){error=next->LastError();return false;}
    if(!next->Call("setup_security_authority").AsBool(false)){error=next->LastError().empty()?"security_authority.vek setup failed":next->LastError();return false;}
    auto v=next->Call("game_shell_policy");
    if(v.IsMap()){
        auto b=v.Get("show_help_overlay");if(!b.IsNil())shell.showHelpOverlay=b.AsBool();
        b=v.Get("show_map_instruction_bar");if(!b.IsNil())shell.showMapInstructionBar=b.AsBool();
        auto s=v.Get("performance_toggle_key").AsString();if(!s.empty())shell.performanceToggleKey=s;
        b=v.Get("performance_default_visible");if(!b.IsNil())shell.performanceDefaultVisible=b.AsBool();
        b=v.Get("performance_show_fps");if(!b.IsNil())shell.performanceShowFps=b.AsBool();
        b=v.Get("performance_show_frame_ms");if(!b.IsNil())shell.performanceShowFrameMs=b.AsBool();
        b=v.Get("performance_show_vek_mode");if(!b.IsNil())shell.performanceShowVekMode=b.AsBool();
        b=v.Get("performance_show_authority");if(!b.IsNil())shell.performanceShowAuthority=b.AsBool();
        shell.cameraFloorClearance=(float)v.Get("camera_floor_clearance").AsNumber(shell.cameraFloorClearance);
        shell.cameraTargetFloorClearance=(float)v.Get("camera_target_floor_clearance").AsNumber(shell.cameraTargetFloorClearance);
        shell.cameraFloorClearance=std::clamp(shell.cameraFloorClearance,0.0f,10.0f);
        shell.cameraTargetFloorClearance=std::clamp(shell.cameraTargetFloorClearance,0.0f,10.0f);
    }
    vm=std::move(next);error.clear();return true;
}
