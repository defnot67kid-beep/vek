#include <vek/VekGameSystems.h>
#include <vek/VekScriptEngine.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>

int main(){
    vek::AnimationLibrary animations;
    VekValue clip=VekValue::Map();
    clip.Set("id","door.open");
    clip.Set("duration",1.2);
    clip.Set("loop",false);
    VekValue markers=VekValue::Array();
    VekValue marker=VekValue::Map(); marker.Set("name","latch"); marker.Set("time",0.25);
    markers.Push(marker); clip.Set("markers",markers);
    std::string error;
    assert(animations.RegisterAnimationValue(clip,&error));
    auto* found=animations.Find("door.open"); assert(found&&found->duration>1.19f);

    vek::AnimationPlaybackState state,before;
    assert(vek::AnimationSystem::Play(state,*found));
    for(int i=0;i<2;++i) vek::AnimationSystem::Update(state,*found,0.1f);
    before=state; vek::AnimationSystem::Update(state,*found,0.1f);
    assert(vek::AnimationSystem::PassedMarker(before,state,*found,"latch"));

    vek::ProximityPromptRegistry prompts;
    VekValue prompt=VekValue::Map();
    prompt.Set("id","hangar.enter"); prompt.Set("action_text","Enter Workspace");
    prompt.Set("object_text","Engineering Hangar"); prompt.Set("input_key","E"); prompt.Set("max_distance",3.5);
    assert(prompts.RegisterPromptValue(prompt,&error));
    auto* pd=prompts.Find("hangar.enter"); assert(pd);
    vek::ProximityPromptState ps;
    assert(!vek::ProximityPromptSystem::Update(ps,*pd,5.0f,false,false,0.016f));
    assert(vek::ProximityPromptSystem::Update(ps,*pd,2.0f,true,true,0.016f));
    assert(ps.activated);

    vek::GarageDoorRegistry garages;
    VekValue garage=VekValue::Map();
    garage.Set("id","hangar.main"); garage.Set("width",24.0); garage.Set("height",8.0);
    garage.Set("panel_count",8); garage.Set("starts_locked",true); garage.Set("open_duration",2.0); garage.Set("allow_inside_egress",true); garage.Set("inside_open_distance",6.0); garage.Set("panel_overlap",0.03); garage.Set("collision_clear_fraction",0.70);
    assert(garages.RegisterGarageValue(garage,&error));
    const auto* gd=garages.Find("hangar.main"); assert(gd); assert(gd->panelOverlap>0.02f&&gd->collisionClearFraction<0.8f);
    vek::GarageDoorState gs; vek::GarageDoorSystem::Reset(gs,*gd);
    assert(!vek::GarageDoorSystem::RequestOpen(gs,*gd,false));
    assert(vek::GarageDoorSystem::RequestOpen(gs,*gd,true));
    for(int i=0;i<25;++i) vek::GarageDoorSystem::Update(gs,*gd,0.1f);
    assert(gs.openFraction>0.99f); vek::GarageDoorSystem::HoldOpen(gs,*gd); assert(gs.autoCloseTimer>0.0f);

    vek::PasslockRegistry locks;
    VekValue lock=VekValue::Map();
    lock.Set("id","hangar.access"); lock.Set("display_name","Engineering Access");
    lock.Set("code","2580"); lock.Set("garage_id","hangar.main"); lock.Set("max_attempts",3); lock.Set("max_use_distance",4.5); lock.Set("outside_only",true); lock.Set("show_world_prompt",false); lock.Set("click_only",true);
    assert(locks.RegisterPasslockValue(lock,&error));
    const auto* ld=locks.Find("hangar.access"); assert(ld); assert(ld->clickOnly&&ld->outsideOnly&&!ld->showWorldPrompt&&ld->requiresLineOfSight);
    vek::PasslockState ls;
    assert(vek::PasslockSystem::Submit(ls,*ld,"1111")==vek::PasslockResult::Denied);
    assert(vek::PasslockSystem::Submit(ls,*ld,"2580")==vek::PasslockResult::Granted);

    // VEK 1.7 lifecycle/presentation metadata remains host-safe: scripts only
    // describe cues, screen effects, death timing and feet-on-ground alignment.
    vek::AudioCueRegistry audio;
    VekValue cue=VekValue::Map(); cue.Set("id","death.reset"); cue.Set("asset","audio/death_reset.wav"); cue.Set("volume",0.8); cue.Set("pitch",0.9);
    assert(audio.RegisterCueValue(cue,&error)); assert(audio.Find("death.reset"));
    VekValue unsafeCue=VekValue::Map(); unsafeCue.Set("id","bad"); unsafeCue.Set("asset","../../secret.wav");
    assert(!audio.RegisterCueValue(unsafeCue,&error));

    vek::ScreenEffectRegistry effects;
    VekValue fx=VekValue::Map(); fx.Set("id","death.blood"); fx.Set("duration",2.4); fx.Set("fade_in",0.1); fx.Set("fade_out",0.7); fx.Set("vignette",0.85); fx.Set("spatter",0.65);
    VekValue tint=VekValue::Map(); tint.Set("r",145); tint.Set("g",0); tint.Set("b",0); tint.Set("a",190); fx.Set("tint",tint);
    assert(effects.RegisterEffectValue(fx,&error)); auto* fxd=effects.Find("death.blood"); assert(fxd&&fxd->vignette>0.8f);
    vek::ScreenEffectState fxs; vek::ScreenEffectSystem::Start(fxs); vek::ScreenEffectSystem::Update(fxs,*fxd,0.2f); assert(fxs.active&&fxs.opacity>0.0f);

    vek::DeathSequenceRegistry deaths;
    VekValue death=VekValue::Map(); death.Set("id","player.reset"); death.Set("ragdoll_impact",28); death.Set("ragdoll_duration",2.2); death.Set("screen_delay",0.05); death.Set("audio_delay",0.12); death.Set("respawn_delay",0.4); death.Set("screen_effect","death.blood"); death.Set("audio_cue","death.reset");
    assert(deaths.RegisterSequenceValue(death,&error)); auto* dd=deaths.Find("player.reset"); assert(dd);
    vek::DeathSequenceState ds; vek::DeathSequenceSystem::Begin(ds); bool screen=false,sound=false,respawn=false;
    for(int i=0;i<8;++i){auto e=vek::DeathSequenceSystem::Update(ds,*dd,0.1f);screen|=e.startScreen;sound|=e.playAudio;respawn|=e.respawn;}
    assert(screen&&sound&&respawn&&!ds.active);

    vek::GroundingRegistry grounding;
    VekValue gp=VekValue::Map(); gp.Set("id","player.default"); gp.Set("root_offset_per_height",0.15); gp.Set("min_root_offset",0.08); gp.Set("max_root_offset",0.30);
    assert(grounding.RegisterProfileValue(gp,&error)); auto* gpd=grounding.Find("player.default"); assert(gpd);
    float rootY=vek::GroundingSystem::RootYForSurface(0.0f,1.0f,*gpd); assert(rootY>0.149f&&rootY<0.151f);

    vek::GuiSystem gui; gui.BeginFrame(); gui.BeginModal("access","GARAGE ACCESS");
    gui.PasswordInput("pin","****"); gui.StatusBadge("status","READY","neutral");
    gui.Keypad("pad",{"1","2","3","4","5","6","7","8","9","CLEAR","0","ENTER"}); gui.EndModal(); gui.EndFrame();
    bool sawModal=false,sawPassword=false,sawKeypad=false;
    for(const auto&c:gui.Commands()){sawModal|=c.type==vek::GuiCommandType::BeginModal;sawPassword|=c.type==vek::GuiCommandType::PasswordInput;sawKeypad|=c.type==vek::GuiCommandType::Keypad;}
    assert(sawModal&&sawPassword&&sawKeypad);

    vek::GuiTextPolicy textPolicy;
    textPolicy.fontSize=24.0f; textPolicy.minFontSize=10.0f; textPolicy.maxLines=2;
    textPolicy.autoFit=true; textPolicy.wrap=true; textPolicy.ellipsis=true; textPolicy.clip=true;
    auto layout=vek::GuiTextLayoutSystem::Layout(
        "Engineering garage access status text that must stay inside its panel",
        180.0f, 44.0f, textPolicy,
        [](const std::string& t,float fs){ return (float)t.size()*fs*0.52f; });
    assert(layout.fontSize<=24.0f&&layout.fontSize>=10.0f);
    assert(!layout.lines.empty()&&layout.lines.size()<=2);
    vek::GuiStyle fittedStyle; fittedStyle.text=textPolicy; gui.DefineStyle("fit",fittedStyle);
    gui.BeginFrame(); gui.Label("A long label that must fit",{0,0,120,30},"fit"); gui.EndFrame();
    assert(!gui.Commands().empty()&&gui.Commands()[0].textPolicy.autoFit&&gui.Commands()[0].textPolicy.maxLines==2);

    // VEK 1.8 camera/sky/rig/world-policy registries.
    vek::CameraProfileRegistry cameras;
    VekValue camera=VekValue::Map();camera.Set("id","camera.test");camera.Set("fov",67);camera.Set("rmb_look",true);camera.Set("alignment_step",30);camera.Set("editor_move_speed",18);
    assert(cameras.RegisterProfileValue(camera,&error));auto* cam=cameras.Find("camera.test");assert(cam&&cam->fov==67.0f&&cam->rmbLook&&cam->alignmentStep==30.0f);

    vek::SkyboxRegistry skies;
    VekValue sky=VekValue::Map();sky.Set("id","sky.test");sky.Set("sun_pitch",42);sky.Set("fog_start",100);sky.Set("fog_end",250);
    VekValue zen=VekValue::Map();zen.Set("r",80);zen.Set("g",140);zen.Set("b",210);sky.Set("zenith",zen);
    assert(skies.RegisterSkyboxValue(sky,&error));assert(skies.Find("sky.test"));
    VekValue badSky=VekValue::Map();badSky.Set("id","sky.bad");badSky.Set("texture_asset","../../evil.dds");assert(!skies.RegisterSkyboxValue(badSky,&error));

    vek::HumanoidRigRegistry rigs;
    VekValue rig=VekValue::Map();rig.Set("id","rig.test");VekValue joints=VekValue::Array();
    VekValue root=VekValue::Map();root.Set("name","root");root.Set("length",0.2);root.Set("ragdoll_weight",0.5);joints.Push(root);
    VekValue spine=VekValue::Map();spine.Set("name","spine");spine.Set("parent","root");spine.Set("length",0.4);spine.Set("ragdoll_weight",1.1);joints.Push(spine);rig.Set("joints",joints);
    assert(rigs.RegisterRigValue(rig,&error));assert(rigs.Find("rig.test")&&rigs.Find("rig.test")->joints.size()==2);

    vek::WorldGameplayPolicyRegistry worldPolicies;
    VekValue worldPolicy=VekValue::Map();worldPolicy.Set("id","game.test");worldPolicy.Set("target_fps",144);worldPolicy.Set("run_speed",6.5);worldPolicy.Set("sprint_speed",9.8);worldPolicy.Set("camera_rmb_look",true);
    assert(worldPolicies.RegisterPolicyValue(worldPolicy,&error));auto* wp=worldPolicies.Find("game.test");assert(wp&&wp->targetFps==144&&wp->cameraRmbLook);

    VekScriptEngine vm; VekRegisterStandardLibrary(vm);
    animations.RegisterNatives(vm); prompts.RegisterNatives(vm); garages.RegisterNatives(vm); locks.RegisterNatives(vm); audio.RegisterNatives(vm); effects.RegisterNatives(vm); deaths.RegisterNatives(vm); grounding.RegisterNatives(vm); cameras.RegisterNatives(vm); skies.RegisterNatives(vm); rigs.RegisterNatives(vm); worldPolicies.RegisterNatives(vm); gui.RegisterNatives(vm);
    assert(vm.LoadSource(R"(
      fn setup(){
        animation_register({id:"walk.to.door",duration:2.0,loop:false});
        prompt_register({id:"door.prompt",action_text:"Open",object_text:"Door",input_key:"E",max_distance:4});
        garage_register({id:"garage.script",width:18,height:7,panel_count:7,starts_locked:true});
        passlock_register({id:"lock.script",code:"2468",garage_id:"garage.script"});
        audio_cue_register({id:"death.script",asset:"audio/death.wav",volume:0.8,pitch:1.0});
        screen_effect_register({id:"blood.script",duration:2.0,vignette:0.8,spatter:0.5});
        death_sequence_register({id:"reset.script",ragdoll_impact:25,respawn_delay:2.5,screen_effect:"blood.script",audio_cue:"death.script"});
        grounding_register({id:"ground.script",root_offset_per_height:0.15});
        camera_profile_register({id:"camera.script",fov:62,rmb_look:true,alignment_step:45});
        skybox_register({id:"sky.script",sun_pitch:50});
        rig_register({id:"rig.script",joints:[{name:"root",length:0.2},{name:"spine",parent:"root",length:0.4}]});
        world_policy_register({id:"game.script",target_fps:120,run_speed:6,sprint_speed:9});
        gui_begin_modal("lock","ACCESS"); gui_password_input("pin","****"); gui_status_badge("state","LOCKED","warning"); gui_end_modal();
        return animation_duration("walk.to.door") + prompt_max_distance("door.prompt") + garage_open_duration("garage.script") + passlock_max_digits("lock.script") + camera_profile_fov("camera.script") + rig_joint_count("rig.script");
      }
    )","interaction_test.vek"));
    auto value=vm.Call("setup");
    assert(value.AsNumber()>69.0);
    std::cout<<"VEK interaction + responsive GUI + camera/sky/rig tests: PASS\n";
    return 0;
}
