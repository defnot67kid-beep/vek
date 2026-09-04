#include <vek/VekGameSystems.h>
#include <vek/VekScriptEngine.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <tuple>

namespace vek {

namespace {
float ClampDt(float dt) { return std::clamp(dt, 0.0f, 0.1f); }
float Approach(float current, float target, float speed, float dt) {
    if (speed <= 0.0f) return target;
    float t = std::clamp(speed * ClampDt(dt), 0.0f, 1.0f);
    return current + (target - current) * t;
}
}

bool GravitySystem::BeginJump(GravityState& state, float jumpSpeed) {
    if (!state.grounded || jumpSpeed <= 0.0f) return false;
    state.grounded = false;
    state.airborneTime = 0.0f;
    state.lastLandingSpeed = 0.0f;
    state.verticalVelocity = jumpSpeed;
    return true;
}

bool GravitySystem::Step(GravityState& state, const GravitySettings& settings,
                         float deltaTime, float groundY, float& worldY) {
    const float dt = ClampDt(deltaTime);
    if (state.grounded) {
        worldY = groundY;
        state.verticalVelocity = settings.groundedVelocity;
        return false;
    }

    state.airborneTime += dt;
    state.verticalVelocity -= std::max(0.0f, settings.acceleration) * dt;
    state.verticalVelocity = std::max(state.verticalVelocity, -std::fabs(settings.terminalFallSpeed));
    worldY += state.verticalVelocity * dt;

    if (worldY <= groundY) {
        state.lastLandingSpeed = std::fabs(state.verticalVelocity);
        worldY = groundY;
        state.verticalVelocity = settings.groundedVelocity;
        state.grounded = true;
        state.airborneTime = 0.0f;
        return true;
    }
    return false;
}

void HealthSystem::Reset(HealthState& state, const HealthSettings& settings) {
    state.health = std::max(0.0f, settings.maxHealth);
    state.alive = state.health > 0.0f;
}

float HealthSystem::ApplyDamage(HealthState& state, const HealthSettings& settings, float amount) {
    if (amount <= 0.0f || !state.alive) return state.health;
    state.health = std::clamp(state.health - amount, 0.0f, std::max(0.0f, settings.maxHealth));
    state.alive = state.health > 0.0f;
    return state.health;
}

float HealthSystem::Heal(HealthState& state, const HealthSettings& settings, float amount) {
    if (amount <= 0.0f) return state.health;
    state.health = std::clamp(state.health + amount, 0.0f, std::max(0.0f, settings.maxHealth));
    state.alive = state.health > 0.0f;
    return state.health;
}

void HealthSystem::SetHealth(HealthState& state, const HealthSettings& settings, float value) {
    state.health = std::clamp(value, 0.0f, std::max(0.0f, settings.maxHealth));
    state.alive = state.health > 0.0f;
}

float HealthSystem::Normalized(const HealthState& state, const HealthSettings& settings) {
    return settings.maxHealth > 0.0f ? std::clamp(state.health / settings.maxHealth, 0.0f, 1.0f) : 0.0f;
}

bool HealthSystem::IsHurt(const HealthState& state, const HealthSettings& settings) {
    return state.alive && Normalized(state, settings) <= settings.hurtThreshold;
}

bool RagdollSystem::ShouldTrigger(float health, float impactSpeed, float damage,
                                  const RagdollSettings& settings) {
    if (health <= 0.0f) return true;
    if (impactSpeed >= settings.fatalImpactSpeed) return true;
    return damage > 0.0f && impactSpeed >= settings.minimumImpactSpeed;
}

void RagdollSystem::Trigger(RagdollState& state, float impactSpeed, float directionSign,
                            const RagdollSettings& settings) {
    const float sign = directionSign < 0.0f ? -1.0f : 1.0f;
    state.phase = RagdollPhase::Ragdoll;
    state.time = 0.0f;
    float normalized = std::clamp((impactSpeed - settings.minimumImpactSpeed) /
                                  std::max(1.0f, settings.fatalImpactSpeed - settings.minimumImpactSpeed), 0.0f, 1.0f);
    state.targetDuration = settings.minimumDuration + (settings.maximumDuration - settings.minimumDuration) * normalized;
    state.blend = 0.0f;
    state.pitchVelocity = (110.0f + impactSpeed * 5.0f) * sign;
    state.rollVelocity = (55.0f + impactSpeed * 2.4f) * -sign;
    state.yawVelocity = (35.0f + impactSpeed * 1.8f) * sign;
    state.bodyDrop = 0.0f;
    state.armSpread = 0.0f;
    state.legSpread = 0.0f;
}

void RagdollSystem::Update(RagdollState& state, float deltaTime, const RagdollSettings& settings) {
    const float dt = ClampDt(deltaTime);
    if (state.phase == RagdollPhase::Animated) return;
    state.time += dt;

    if (state.phase == RagdollPhase::Ragdoll) {
        state.blend = Approach(state.blend, 1.0f, 8.0f, dt);
        state.pitch += state.pitchVelocity * dt;
        state.roll += state.rollVelocity * dt;
        state.yawOffset += state.yawVelocity * dt;
        state.pitchVelocity = Approach(state.pitchVelocity, 0.0f, settings.angularDamping, dt);
        state.rollVelocity = Approach(state.rollVelocity, 0.0f, settings.angularDamping, dt);
        state.yawVelocity = Approach(state.yawVelocity, 0.0f, settings.angularDamping, dt);
        state.bodyDrop = Approach(state.bodyDrop, 0.72f, settings.linearDamping, dt);
        state.armSpread = Approach(state.armSpread, 1.0f, 6.5f, dt);
        state.legSpread = Approach(state.legSpread, 1.0f, 5.0f, dt);
        if (state.time >= state.targetDuration) BeginRecovery(state, settings);
    } else if (state.phase == RagdollPhase::Recovering) {
        state.blend = Approach(state.blend, 0.0f, 1.0f / std::max(0.05f, settings.recoveryDuration), dt);
        state.pitch = Approach(state.pitch, 0.0f, 7.0f, dt);
        state.roll = Approach(state.roll, 0.0f, 7.0f, dt);
        state.yawOffset = Approach(state.yawOffset, 0.0f, 7.0f, dt);
        state.bodyDrop = Approach(state.bodyDrop, 0.0f, 6.0f, dt);
        state.armSpread = Approach(state.armSpread, 0.0f, 7.0f, dt);
        state.legSpread = Approach(state.legSpread, 0.0f, 7.0f, dt);
        if (state.blend <= 0.01f && std::fabs(state.pitch) < 0.5f && std::fabs(state.roll) < 0.5f) {
            state = RagdollState{};
        }
    }
}

void RagdollSystem::BeginRecovery(RagdollState& state, const RagdollSettings&) {
    state.phase = RagdollPhase::Recovering;
    state.time = 0.0f;
}

bool RagdollSystem::Active(const RagdollState& state) {
    return state.phase != RagdollPhase::Animated;
}



bool AnimationLibrary::RegisterAnimation(const AnimationDefinition& d) {
    if (d.id.empty() || !std::isfinite(d.duration) || d.duration <= 0.0f || !std::isfinite(d.speed) || d.speed <= 0.0f)
        return false;
    AnimationDefinition clean=d;
    clean.duration=std::clamp(clean.duration,0.01f,3600.0f);
    clean.speed=std::clamp(clean.speed,0.01f,100.0f);
    clean.blendIn=std::clamp(clean.blendIn,0.0f,10.0f);
    clean.blendOut=std::clamp(clean.blendOut,0.0f,10.0f);
    for(auto& marker:clean.markers) marker.time=std::clamp(marker.time,0.0f,clean.duration);
    clips[clean.id]=std::move(clean);
    return true;
}
bool AnimationLibrary::RegisterAnimationValue(const VekValue& v,std::string* error) {
    if(!v.IsMap()){if(error)*error="animation_register expects a map";return false;}
    AnimationDefinition d;
    d.id=v.Get("id").AsString();
    d.duration=(float)v.Get("duration").AsNumber(1.0);
    d.speed=(float)v.Get("speed").AsNumber(1.0);
    d.blendIn=(float)v.Get("blend_in").AsNumber(0.12);
    d.blendOut=(float)v.Get("blend_out").AsNumber(0.12);
    d.loop=v.Get("loop").AsBool(false);
    if(auto tags=v.Get("tags").AsArray()) for(const auto& x:*tags) if(x.IsString()) d.tags.push_back(x.AsString());
    if(auto markers=v.Get("markers").AsArray()) for(const auto& x:*markers) if(x.IsMap()){
        AnimationMarker m; m.name=x.Get("name").AsString(); m.time=(float)x.Get("time").AsNumber(); m.data=x.Get("data");
        if(!m.name.empty()) d.markers.push_back(std::move(m));
    }
    if(!RegisterAnimation(d)){if(error)*error="invalid animation definition";return false;}
    if(error)error->clear();return true;
}
const AnimationDefinition* AnimationLibrary::Find(const std::string& id) const {auto it=clips.find(id);return it==clips.end()?nullptr:&it->second;}
void AnimationLibrary::Clear(){clips.clear();}
std::size_t AnimationLibrary::Size() const{return clips.size();}
void AnimationLibrary::RegisterNatives(VekScriptEngine& e){
    e.RegisterNative("animation_register",[this](const std::vector<VekValue>& a){std::string er;return VekValue(!a.empty()&&RegisterAnimationValue(a[0],&er));});
    e.RegisterNative("animation_exists",[this](const std::vector<VekValue>& a){return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);});
    e.RegisterNative("animation_duration",[this](const std::vector<VekValue>& a){auto*d=a.empty()?nullptr:Find(a[0].AsString());return VekValue(d?d->duration:0.0f);});
}
bool AnimationSystem::Play(AnimationPlaybackState& s,const AnimationDefinition& clip,bool restart){
    if(!restart&&s.playing&&s.clipId==clip.id)return false;
    s.clipId=clip.id;s.time=0;s.normalizedTime=0;s.loopCount=0;s.playing=true;s.finished=false;return true;
}
void AnimationSystem::Stop(AnimationPlaybackState& s){s.playing=false;s.finished=true;}
void AnimationSystem::Update(AnimationPlaybackState& s,const AnimationDefinition& clip,float dt){
    if(!s.playing||s.clipId!=clip.id)return;
    float duration=std::max(0.01f,clip.duration),step=std::clamp(dt,0.0f,0.1f)*std::max(0.01f,clip.speed);
    s.time+=step;
    if(clip.loop){
        while(s.time>=duration){s.time-=duration;++s.loopCount;}
    }else if(s.time>=duration){s.time=duration;s.playing=false;s.finished=true;}
    s.normalizedTime=std::clamp(s.time/duration,0.0f,1.0f);
}
bool AnimationSystem::PassedMarker(const AnimationPlaybackState& before,const AnimationPlaybackState& after,const AnimationDefinition& clip,const std::string& marker){
    for(const auto&m:clip.markers)if(m.name==marker){
        if(after.loopCount>before.loopCount)return before.time<=m.time||after.time>=m.time;
        return before.time<m.time&&after.time>=m.time;
    }
    return false;
}

bool ProximityPromptRegistry::RegisterPrompt(const ProximityPromptDefinition& d){
    if(d.id.empty()||d.actionText.empty()||!std::isfinite(d.maxDistance)||d.maxDistance<=0.0f)return false;
    ProximityPromptDefinition clean=d;clean.maxDistance=std::clamp(clean.maxDistance,0.1f,100.0f);clean.holdDuration=std::clamp(clean.holdDuration,0.0f,30.0f);prompts[clean.id]=std::move(clean);return true;
}
bool ProximityPromptRegistry::RegisterPromptValue(const VekValue&v,std::string*error){
    if(!v.IsMap()){if(error)*error="prompt_register expects a map";return false;}
    ProximityPromptDefinition d;d.id=v.Get("id").AsString();d.actionText=v.Get("action_text").AsString();d.objectText=v.Get("object_text").AsString();d.inputKey=v.Get("input_key").AsString();if(d.inputKey.empty())d.inputKey="E";d.maxDistance=(float)v.Get("max_distance").AsNumber(3.5);d.holdDuration=(float)v.Get("hold_duration").AsNumber(0.0);auto enabledValue=v.Get("enabled");d.enabled=enabledValue.IsNil()?true:enabledValue.AsBool();auto losValue=v.Get("requires_line_of_sight");d.requiresLineOfSight=losValue.IsNil()?false:losValue.AsBool();d.priority=(int)v.Get("priority").AsNumber(0);
    if(!RegisterPrompt(d)){if(error)*error="invalid proximity prompt definition";return false;}if(error)error->clear();return true;
}
const ProximityPromptDefinition* ProximityPromptRegistry::Find(const std::string&id)const{auto it=prompts.find(id);return it==prompts.end()?nullptr:&it->second;}
void ProximityPromptRegistry::Clear(){prompts.clear();}
std::size_t ProximityPromptRegistry::Size()const{return prompts.size();}
void ProximityPromptRegistry::RegisterNatives(VekScriptEngine&e){
    e.RegisterNative("prompt_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterPromptValue(a[0],&er));});
    e.RegisterNative("prompt_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);});
    e.RegisterNative("prompt_max_distance",[this](const std::vector<VekValue>&a){auto*d=a.empty()?nullptr:Find(a[0].AsString());return VekValue(d?d->maxDistance:0.0f);});
}
bool ProximityPromptSystem::Update(ProximityPromptState&s,const ProximityPromptDefinition&d,float distance,bool inputDown,bool inputPressed,float dt){
    s.distance=std::max(0.0f,distance);s.activated=false;s.visible=d.enabled&&s.distance<=d.maxDistance;
    if(!s.visible){s.holdProgress=0;return false;}
    if(d.holdDuration<=0.0f){if(inputPressed){s.activated=true;return true;}return false;}
    if(inputDown)s.holdProgress=std::min(d.holdDuration,s.holdProgress+std::clamp(dt,0.0f,0.1f));else s.holdProgress=0.0f;
    if(s.holdProgress>=d.holdDuration){s.activated=true;s.holdProgress=0;return true;}return false;
}

bool GarageDoorRegistry::RegisterGarage(const GarageDoorDefinition&d){
    if(d.id.empty()||!std::isfinite(d.width)||!std::isfinite(d.height)||d.width<1.0f||d.height<1.0f)return false;
    GarageDoorDefinition c=d;c.width=std::clamp(c.width,1.0f,80.0f);c.height=std::clamp(c.height,1.0f,30.0f);c.panelCount=std::clamp(c.panelCount,2,32);c.openDuration=std::clamp(c.openDuration,0.15f,30.0f);c.closeDuration=std::clamp(c.closeDuration,0.15f,30.0f);c.autoCloseDelay=std::clamp(c.autoCloseDelay,0.0f,300.0f);c.insideOpenDistance=std::clamp(c.insideOpenDistance,0.5f,30.0f);c.panelOverlap=std::clamp(c.panelOverlap,0.0f,0.15f);c.sideSealWidth=std::clamp(c.sideSealWidth,0.10f,1.0f);c.lintelHeight=std::clamp(c.lintelHeight,0.10f,2.0f);c.collisionClearFraction=std::clamp(c.collisionClearFraction,0.25f,0.95f);garages[c.id]=std::move(c);return true;
}
bool GarageDoorRegistry::RegisterGarageValue(const VekValue&v,std::string*error){
    if(!v.IsMap()){if(error)*error="garage_register expects a map";return false;}
    GarageDoorDefinition d;d.id=v.Get("id").AsString();d.displayName=v.Get("display_name").AsString();if(d.displayName.empty())d.displayName="Garage Door";d.width=(float)v.Get("width").AsNumber(24.0);d.height=(float)v.Get("height").AsNumber(8.0);d.panelCount=(int)v.Get("panel_count").AsNumber(8);d.openDuration=(float)v.Get("open_duration").AsNumber(2.6);d.closeDuration=(float)v.Get("close_duration").AsNumber(2.3);d.autoCloseDelay=(float)v.Get("auto_close_delay").AsNumber(8.0);auto sl=v.Get("starts_locked");d.startsLocked=sl.IsNil()?true:sl.AsBool();auto ac=v.Get("auto_close");d.autoClose=ac.IsNil()?true:ac.AsBool();auto ie=v.Get("allow_inside_egress");d.allowInsideEgress=ie.IsNil()?true:ie.AsBool();d.insideOpenDistance=(float)v.Get("inside_open_distance").AsNumber(6.0);auto ho=v.Get("hold_open_near_door");d.holdOpenNearDoor=ho.IsNil()?true:ho.AsBool();d.panelOverlap=(float)v.Get("panel_overlap").AsNumber(d.panelOverlap);d.sideSealWidth=(float)v.Get("side_seal_width").AsNumber(d.sideSealWidth);d.lintelHeight=(float)v.Get("lintel_height").AsNumber(d.lintelHeight);d.collisionClearFraction=(float)v.Get("collision_clear_fraction").AsNumber(d.collisionClearFraction);d.openAnimation=v.Get("open_animation").AsString();d.closeAnimation=v.Get("close_animation").AsString();d.accessId=v.Get("access_id").AsString();if(!RegisterGarage(d)){if(error)*error="invalid garage door definition";return false;}if(error)error->clear();return true;
}
const GarageDoorDefinition*GarageDoorRegistry::Find(const std::string&id)const{auto it=garages.find(id);return it==garages.end()?nullptr:&it->second;}
void GarageDoorRegistry::Clear(){garages.clear();}std::size_t GarageDoorRegistry::Size()const{return garages.size();}
void GarageDoorRegistry::RegisterNatives(VekScriptEngine&e){e.RegisterNative("garage_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterGarageValue(a[0],&er));});e.RegisterNative("garage_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);});e.RegisterNative("garage_open_duration",[this](const std::vector<VekValue>&a){auto*d=a.empty()?nullptr:Find(a[0].AsString());return VekValue(d?d->openDuration:0.0f);});}
void GarageDoorSystem::Reset(GarageDoorState&s,const GarageDoorDefinition&d){s={};s.locked=d.startsLocked;s.motion=GarageDoorMotion::Closed;}
bool GarageDoorSystem::RequestOpen(GarageDoorState&s,const GarageDoorDefinition&d,bool accessGranted){if(s.locked&&!accessGranted)return false;if(accessGranted)s.locked=false;if(s.motion==GarageDoorMotion::Open||s.motion==GarageDoorMotion::Opening)return true;s.motion=GarageDoorMotion::Opening;s.autoCloseTimer=0;s.activeAnimation=d.openAnimation;s.animationTime=0;s.animationNormalized=s.openFraction;return true;}
void GarageDoorSystem::RequestClose(GarageDoorState&s){if(s.motion!=GarageDoorMotion::Closed){s.motion=GarageDoorMotion::Closing;s.animationTime=0;}s.autoCloseTimer=0;}
void GarageDoorSystem::HoldOpen(GarageDoorState&s,const GarageDoorDefinition&d){if(s.motion==GarageDoorMotion::Open){s.autoCloseTimer=d.autoClose?d.autoCloseDelay:0.0f;}else if(s.motion==GarageDoorMotion::Closing){s.motion=GarageDoorMotion::Opening;s.activeAnimation=d.openAnimation;s.animationTime=0.0f;s.animationNormalized=s.openFraction;}}
void GarageDoorSystem::Unlock(GarageDoorState&s){s.locked=false;}void GarageDoorSystem::Lock(GarageDoorState&s){if(s.openFraction<=0.001f)s.locked=true;}
void GarageDoorSystem::Update(GarageDoorState&s,const GarageDoorDefinition&d,float dt){dt=std::clamp(dt,0.0f,0.1f);if(s.motion==GarageDoorMotion::Opening){if(s.activeAnimation.empty())s.activeAnimation=d.openAnimation;s.animationTime+=dt;s.openFraction=std::min(1.0f,s.openFraction+dt/std::max(0.15f,d.openDuration));s.animationNormalized=s.openFraction;if(s.openFraction>=1.0f){s.motion=GarageDoorMotion::Open;s.autoCloseTimer=d.autoClose?d.autoCloseDelay:0;s.animationNormalized=1.0f;}}else if(s.motion==GarageDoorMotion::Open&&d.autoClose&&s.autoCloseTimer>0){s.autoCloseTimer-=dt;if(s.autoCloseTimer<=0){s.motion=GarageDoorMotion::Closing;s.activeAnimation=d.closeAnimation;s.animationTime=0;s.animationNormalized=0;}}else if(s.motion==GarageDoorMotion::Closing){if(s.activeAnimation.empty())s.activeAnimation=d.closeAnimation;s.animationTime+=dt;s.openFraction=std::max(0.0f,s.openFraction-dt/std::max(0.15f,d.closeDuration));s.animationNormalized=1.0f-s.openFraction;if(s.openFraction<=0){s.motion=GarageDoorMotion::Closed;s.animationNormalized=1.0f;if(d.startsLocked)s.locked=true;}}}

bool PasslockRegistry::RegisterPasslock(const PasslockDefinition&d){if(d.id.empty()||d.accessCode.empty())return false;PasslockDefinition c=d;c.minDigits=std::clamp(c.minDigits,1,16);c.maxDigits=std::clamp(c.maxDigits,c.minDigits,32);c.maxAttempts=std::clamp(c.maxAttempts,1,20);c.lockoutSeconds=std::clamp(c.lockoutSeconds,0.0f,300.0f);c.maxUseDistance=std::clamp(c.maxUseDistance,0.5f,15.0f);if((int)c.accessCode.size()<c.minDigits||(int)c.accessCode.size()>c.maxDigits)return false;for(char ch:c.accessCode)if(!std::isdigit((unsigned char)ch))return false;passlocks[c.id]=std::move(c);return true;}
bool PasslockRegistry::RegisterPasslockValue(const VekValue&v,std::string*error){if(!v.IsMap()){if(error)*error="passlock_register expects a map";return false;}PasslockDefinition d;d.id=v.Get("id").AsString();d.displayName=v.Get("display_name").AsString();if(d.displayName.empty())d.displayName="Access Control";d.accessCode=v.Get("code").AsString();d.linkedGarageId=v.Get("garage_id").AsString();d.promptId=v.Get("prompt_id").AsString();d.maxUseDistance=(float)v.Get("max_use_distance").AsNumber(4.5);auto los=v.Get("requires_line_of_sight");d.requiresLineOfSight=los.IsNil()?true:los.AsBool();auto oo=v.Get("outside_only");d.outsideOnly=oo.IsNil()?true:oo.AsBool();auto swp=v.Get("show_world_prompt");d.showWorldPrompt=swp.IsNil()?false:swp.AsBool();auto co=v.Get("click_only");d.clickOnly=co.IsNil()?true:co.AsBool();d.minDigits=(int)v.Get("min_digits").AsNumber(4);d.maxDigits=(int)v.Get("max_digits").AsNumber(8);d.maxAttempts=(int)v.Get("max_attempts").AsNumber(5);d.lockoutSeconds=(float)v.Get("lockout_seconds").AsNumber(10);auto ak=v.Get("allow_keyboard");d.allowKeyboard=ak.IsNil()?true:ak.AsBool();auto am=v.Get("allow_mouse");d.allowMouse=am.IsNil()?true:am.AsBool();auto mi=v.Get("mask_input");d.maskInput=mi.IsNil()?true:mi.AsBool();if(!RegisterPasslock(d)){if(error)*error="invalid passlock definition";return false;}if(error)error->clear();return true;}
const PasslockDefinition*PasslockRegistry::Find(const std::string&id)const{auto it=passlocks.find(id);return it==passlocks.end()?nullptr:&it->second;}void PasslockRegistry::Clear(){passlocks.clear();}std::size_t PasslockRegistry::Size()const{return passlocks.size();}
void PasslockRegistry::RegisterNatives(VekScriptEngine&e){e.RegisterNative("passlock_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterPasslockValue(a[0],&er));});e.RegisterNative("passlock_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);});e.RegisterNative("passlock_max_digits",[this](const std::vector<VekValue>&a){auto*d=a.empty()?nullptr:Find(a[0].AsString());return VekValue(d?d->maxDigits:0);});}
void PasslockSystem::Reset(PasslockState&s){s={};}void PasslockSystem::Update(PasslockState&s,float dt){s.lockoutRemaining=std::max(0.0f,s.lockoutRemaining-std::clamp(dt,0.0f,0.1f));if(s.lockoutRemaining<=0&&s.failedAttempts<0)s.failedAttempts=0;}
PasslockResult PasslockSystem::Submit(PasslockState&s,const PasslockDefinition&d,const std::string&code){if(s.lockoutRemaining>0)return PasslockResult::LockedOut;if((int)code.size()<d.minDigits||(int)code.size()>d.maxDigits)return PasslockResult::InvalidInput;for(char ch:code)if(!std::isdigit((unsigned char)ch))return PasslockResult::InvalidInput;if(code==d.accessCode){s.granted=true;s.failedAttempts=0;s.lockoutRemaining=0;return PasslockResult::Granted;}s.granted=false;++s.failedAttempts;if(s.failedAttempts>=d.maxAttempts){s.failedAttempts=0;s.lockoutRemaining=d.lockoutSeconds;return PasslockResult::LockedOut;}return PasslockResult::Denied;}



// -----------------------------------------------------------------------------
// VEK 1.7 lifecycle / presentation metadata systems
// -----------------------------------------------------------------------------
bool AudioCueRegistry::RegisterCue(const AudioCueDefinition& d){
    if(d.id.empty()||d.assetId.empty())return false;
    AudioCueDefinition c=d;c.volume=std::clamp(c.volume,0.0f,1.0f);c.pitch=std::clamp(c.pitch,0.25f,4.0f);
    cues[c.id]=std::move(c);return true;
}
bool AudioCueRegistry::RegisterCueValue(const VekValue&v,std::string*error){
    if(!v.IsMap()){if(error)*error="audio_cue_register expects a map";return false;}
    AudioCueDefinition d;d.id=v.Get("id").AsString();d.assetId=v.Get("asset").AsString();d.volume=(float)v.Get("volume").AsNumber(1.0);d.pitch=(float)v.Get("pitch").AsNumber(1.0);
    if(d.assetId.find("..")!=std::string::npos||(!d.assetId.empty()&&d.assetId[0]=='/')){if(error)*error="unsafe audio asset id";return false;}
    if(!RegisterCue(d)){if(error)*error="invalid audio cue definition";return false;}if(error)error->clear();return true;
}
const AudioCueDefinition*AudioCueRegistry::Find(const std::string&id)const{auto it=cues.find(id);return it==cues.end()?nullptr:&it->second;}
void AudioCueRegistry::Clear(){cues.clear();}std::size_t AudioCueRegistry::Size()const{return cues.size();}
void AudioCueRegistry::RegisterNatives(VekScriptEngine&e){
    e.RegisterNative("audio_cue_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterCueValue(a[0],&er));});
    e.RegisterNative("audio_cue_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);});
}

bool ScreenEffectRegistry::RegisterEffect(const ScreenEffectDefinition& d){
    if(d.id.empty())return false;ScreenEffectDefinition c=d;
    c.duration=std::clamp(c.duration,0.05f,30.0f);c.fadeIn=std::clamp(c.fadeIn,0.0f,c.duration);c.hold=std::clamp(c.hold,0.0f,c.duration);c.fadeOut=std::clamp(c.fadeOut,0.0f,c.duration);
    c.tint.r=std::clamp(c.tint.r,0.0f,255.0f);c.tint.g=std::clamp(c.tint.g,0.0f,255.0f);c.tint.b=std::clamp(c.tint.b,0.0f,255.0f);c.tint.a=std::clamp(c.tint.a,0.0f,255.0f);
    c.vignette=std::clamp(c.vignette,0.0f,1.0f);c.spatter=std::clamp(c.spatter,0.0f,1.0f);c.pulse=std::clamp(c.pulse,0.0f,1.0f);effects[c.id]=std::move(c);return true;
}
bool ScreenEffectRegistry::RegisterEffectValue(const VekValue&v,std::string*error){
    if(!v.IsMap()){if(error)*error="screen_effect_register expects a map";return false;}
    ScreenEffectDefinition d;d.id=v.Get("id").AsString();d.duration=(float)v.Get("duration").AsNumber(d.duration);d.fadeIn=(float)v.Get("fade_in").AsNumber(d.fadeIn);d.hold=(float)v.Get("hold").AsNumber(d.hold);d.fadeOut=(float)v.Get("fade_out").AsNumber(d.fadeOut);d.vignette=(float)v.Get("vignette").AsNumber(d.vignette);d.spatter=(float)v.Get("spatter").AsNumber(d.spatter);d.pulse=(float)v.Get("pulse").AsNumber(d.pulse);
    auto tint=v.Get("tint");if(tint.IsMap()){d.tint.r=(float)tint.Get("r").AsNumber(d.tint.r);d.tint.g=(float)tint.Get("g").AsNumber(d.tint.g);d.tint.b=(float)tint.Get("b").AsNumber(d.tint.b);d.tint.a=(float)tint.Get("a").AsNumber(d.tint.a);}
    if(!RegisterEffect(d)){if(error)*error="invalid screen effect definition";return false;}if(error)error->clear();return true;
}
const ScreenEffectDefinition*ScreenEffectRegistry::Find(const std::string&id)const{auto it=effects.find(id);return it==effects.end()?nullptr:&it->second;}
void ScreenEffectRegistry::Clear(){effects.clear();}std::size_t ScreenEffectRegistry::Size()const{return effects.size();}
void ScreenEffectRegistry::RegisterNatives(VekScriptEngine&e){
    e.RegisterNative("screen_effect_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterEffectValue(a[0],&er));});
    e.RegisterNative("screen_effect_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);});
}
void ScreenEffectSystem::Start(ScreenEffectState&s){s={};s.active=true;}
void ScreenEffectSystem::Stop(ScreenEffectState&s){s={};}
void ScreenEffectSystem::Update(ScreenEffectState&s,const ScreenEffectDefinition&d,float dt){
    if(!s.active)return;s.time+=ClampDt(dt);float t=s.time;float alpha=1.0f;
    if(d.fadeIn>0&&t<d.fadeIn)alpha=t/d.fadeIn;
    float fadeStart=std::max(d.fadeIn,d.duration-d.fadeOut);
    if(d.fadeOut>0&&t>fadeStart)alpha=std::min(alpha,std::max(0.0f,(d.duration-t)/d.fadeOut));
    if(d.pulse>0)alpha*=1.0f-d.pulse*0.16f*(0.5f+0.5f*std::sin(t*10.0f));
    s.opacity=std::clamp(alpha,0.0f,1.0f);if(t>=d.duration){s.active=false;s.opacity=0.0f;}
}

bool DeathSequenceRegistry::RegisterSequence(const DeathSequenceDefinition&d){
    if(d.id.empty())return false;DeathSequenceDefinition c=d;c.ragdollImpact=std::clamp(c.ragdollImpact,1.0f,200.0f);c.ragdollDuration=std::clamp(c.ragdollDuration,0.1f,10.0f);c.screenDelay=std::clamp(c.screenDelay,0.0f,10.0f);c.audioDelay=std::clamp(c.audioDelay,0.0f,10.0f);c.respawnDelay=std::clamp(c.respawnDelay,0.2f,30.0f);sequences[c.id]=std::move(c);return true;
}
bool DeathSequenceRegistry::RegisterSequenceValue(const VekValue&v,std::string*error){
    if(!v.IsMap()){if(error)*error="death_sequence_register expects a map";return false;}
    DeathSequenceDefinition d;d.id=v.Get("id").AsString();d.ragdollImpact=(float)v.Get("ragdoll_impact").AsNumber(d.ragdollImpact);d.ragdollDuration=(float)v.Get("ragdoll_duration").AsNumber(d.ragdollDuration);d.screenDelay=(float)v.Get("screen_delay").AsNumber(d.screenDelay);d.audioDelay=(float)v.Get("audio_delay").AsNumber(d.audioDelay);d.respawnDelay=(float)v.Get("respawn_delay").AsNumber(d.respawnDelay);d.screenEffectId=v.Get("screen_effect").AsString();d.audioCueId=v.Get("audio_cue").AsString();
    if(!RegisterSequence(d)){if(error)*error="invalid death sequence definition";return false;}if(error)error->clear();return true;
}
const DeathSequenceDefinition*DeathSequenceRegistry::Find(const std::string&id)const{auto it=sequences.find(id);return it==sequences.end()?nullptr:&it->second;}
void DeathSequenceRegistry::Clear(){sequences.clear();}std::size_t DeathSequenceRegistry::Size()const{return sequences.size();}
void DeathSequenceRegistry::RegisterNatives(VekScriptEngine&e){
    e.RegisterNative("death_sequence_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterSequenceValue(a[0],&er));});
    e.RegisterNative("death_sequence_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);});
}
void DeathSequenceSystem::Begin(DeathSequenceState&s){s={};s.active=true;}
DeathSequenceEvents DeathSequenceSystem::Update(DeathSequenceState&s,const DeathSequenceDefinition&d,float dt){
    DeathSequenceEvents ev;if(!s.active)return ev;s.time+=ClampDt(dt);
    if(!s.screenStarted&&s.time>=d.screenDelay){s.screenStarted=true;ev.startScreen=true;}
    if(!s.audioPlayed&&s.time>=d.audioDelay){s.audioPlayed=true;ev.playAudio=true;}
    if(!s.respawnIssued&&s.time>=d.respawnDelay){s.respawnIssued=true;ev.respawn=true;s.active=false;}
    return ev;
}
void DeathSequenceSystem::Cancel(DeathSequenceState&s){s={};}

bool GroundingRegistry::RegisterProfile(const GroundingProfile&d){
    if(d.id.empty())return false;GroundingProfile c=d;c.rootOffsetPerHeight=std::clamp(c.rootOffsetPerHeight,0.0f,1.0f);c.minimumRootOffset=std::clamp(c.minimumRootOffset,0.0f,2.0f);c.maximumRootOffset=std::clamp(c.maximumRootOffset,c.minimumRootOffset,3.0f);c.snapTolerance=std::clamp(c.snapTolerance,0.0f,1.0f);profiles[c.id]=std::move(c);return true;
}
bool GroundingRegistry::RegisterProfileValue(const VekValue&v,std::string*error){
    if(!v.IsMap()){if(error)*error="grounding_register expects a map";return false;}
    GroundingProfile d;d.id=v.Get("id").AsString();d.rootOffsetPerHeight=(float)v.Get("root_offset_per_height").AsNumber(d.rootOffsetPerHeight);d.minimumRootOffset=(float)v.Get("min_root_offset").AsNumber(d.minimumRootOffset);d.maximumRootOffset=(float)v.Get("max_root_offset").AsNumber(d.maximumRootOffset);d.snapTolerance=(float)v.Get("snap_tolerance").AsNumber(d.snapTolerance);
    if(!RegisterProfile(d)){if(error)*error="invalid grounding profile";return false;}if(error)error->clear();return true;
}
const GroundingProfile*GroundingRegistry::Find(const std::string&id)const{auto it=profiles.find(id);return it==profiles.end()?nullptr:&it->second;}
void GroundingRegistry::Clear(){profiles.clear();}std::size_t GroundingRegistry::Size()const{return profiles.size();}
void GroundingRegistry::RegisterNatives(VekScriptEngine&e){
    e.RegisterNative("grounding_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterProfileValue(a[0],&er));});
    e.RegisterNative("grounding_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);});
}
float GroundingSystem::RootYForSurface(float surfaceY,float avatarHeight,const GroundingProfile&p){
    float offset=std::clamp(std::max(0.1f,avatarHeight)*p.rootOffsetPerHeight,p.minimumRootOffset,p.maximumRootOffset);return surfaceY+offset;
}

static std::vector<std::string> WrapGuiText(const std::string& text,float fontSize,float width,const GuiMeasureTextFn& measure){
    std::vector<std::string> lines;
    if(text.empty()){lines.push_back("");return lines;}
    std::string current,word;
    auto flushWord=[&](){
        if(word.empty())return;
        std::string candidate=current.empty()?word:current+" "+word;
        if(width>0.0f && !current.empty() && measure(candidate,fontSize)>width){lines.push_back(current);current=word;}
        else current=candidate;
        word.clear();
    };
    for(char ch:text){
        if(ch=='\n'){flushWord();lines.push_back(current);current.clear();}
        else if(std::isspace((unsigned char)ch)){flushWord();}
        else word.push_back(ch);
    }
    flushWord();
    if(!current.empty()||lines.empty())lines.push_back(current);
    return lines;
}
static std::string EllipsizeGuiText(std::string text,float fontSize,float width,const GuiMeasureTextFn& measure){
    if(width<=0.0f||measure(text,fontSize)<=width)return text;
    const std::string dots="...";
    while(!text.empty()&&measure(text+dots,fontSize)>width)text.pop_back();
    return text+dots;
}

// ---------------- VEK 1.8 camera / sky / rig / world policy ----------------
namespace {
static bool SafeAssetId18(const std::string& s){return s.find("..") == std::string::npos && s.find(':') == std::string::npos && (!s.empty()?s.front()!='/' : true) && (!s.empty()?s.front()!='\\' : true);}
static void ReadSkyColor18(const VekValue& map,const char* key,SkyboxDefinition::Color& c){auto v=map.Get(key);if(!v.IsMap())return;c.r=(float)v.Get("r").AsNumber(c.r);c.g=(float)v.Get("g").AsNumber(c.g);c.b=(float)v.Get("b").AsNumber(c.b);c.a=(float)v.Get("a").AsNumber(c.a);c.r=std::clamp(c.r,0.0f,255.0f);c.g=std::clamp(c.g,0.0f,255.0f);c.b=std::clamp(c.b,0.0f,255.0f);c.a=std::clamp(c.a,0.0f,255.0f);}
}

bool CameraProfileRegistry::RegisterProfile(const CameraProfileDefinition& d){
    if(d.id.empty())return false;CameraProfileDefinition c=d;
    c.thirdPersonDistance=std::clamp(c.thirdPersonDistance,1.0f,40.0f);c.closeDistance=std::clamp(c.closeDistance,0.5f,20.0f);
    c.targetHeight=std::clamp(c.targetHeight,0.0f,5.0f);c.closeTargetHeight=std::clamp(c.closeTargetHeight,0.0f,5.0f);c.firstPersonEyeHeight=std::clamp(c.firstPersonEyeHeight,0.5f,3.0f);
    c.fov=std::clamp(c.fov,25.0f,120.0f);c.pitchMin=std::clamp(c.pitchMin,-89.0f,89.0f);c.pitchMax=std::clamp(c.pitchMax,c.pitchMin,89.0f);
    c.yawSensitivity=std::clamp(c.yawSensitivity,0.005f,2.0f);c.pitchSensitivity=std::clamp(c.pitchSensitivity,0.005f,2.0f);c.alignmentStep=std::clamp(c.alignmentStep,5.0f,180.0f);
    c.maxYawSpeed=std::clamp(c.maxYawSpeed,30.0f,1440.0f);c.maxPitchSpeed=std::clamp(c.maxPitchSpeed,30.0f,1080.0f);
    c.yawAcceleration=std::clamp(c.yawAcceleration,50.0f,10000.0f);c.yawDeceleration=std::clamp(c.yawDeceleration,50.0f,12000.0f);c.pitchAcceleration=std::clamp(c.pitchAcceleration,50.0f,10000.0f);c.pitchDeceleration=std::clamp(c.pitchDeceleration,50.0f,12000.0f);
    c.rmbYawScale=std::clamp(c.rmbYawScale,0.05f,5.0f);c.rmbPitchScale=std::clamp(c.rmbPitchScale,0.05f,5.0f);
    c.editorMoveSpeed=std::clamp(c.editorMoveSpeed,1.0f,100.0f);c.editorFastMultiplier=std::clamp(c.editorFastMultiplier,1.0f,10.0f);c.editorFineMultiplier=std::clamp(c.editorFineMultiplier,0.02f,1.0f);
    c.editorYawSpeed=std::clamp(c.editorYawSpeed,10.0f,720.0f);c.editorPitchSpeed=std::clamp(c.editorPitchSpeed,10.0f,720.0f);c.editorPitchMin=std::clamp(c.editorPitchMin,-89.0f,0.0f);c.editorPitchMax=std::clamp(c.editorPitchMax,0.0f,89.0f);c.editorFov=std::clamp(c.editorFov,25.0f,120.0f);c.editorOrthoSize=std::clamp(c.editorOrthoSize,2.0f,100.0f);c.minimumWorldY=std::clamp(c.minimumWorldY,-1000.0f,10000.0f);c.minimumTargetY=std::clamp(c.minimumTargetY,-1000.0f,10000.0f);
    profiles[c.id]=std::move(c);return true;
}
bool CameraProfileRegistry::RegisterProfileValue(const VekValue&v,std::string*error){if(!v.IsMap()){if(error)*error="camera_profile_register expects a map";return false;}CameraProfileDefinition d;d.id=v.Get("id").AsString();
#define VEK_CAM_NUM(field,key) d.field=(float)v.Get(key).AsNumber(d.field)
    VEK_CAM_NUM(thirdPersonDistance,"third_person_distance");VEK_CAM_NUM(closeDistance,"close_distance");VEK_CAM_NUM(targetHeight,"target_height");VEK_CAM_NUM(closeTargetHeight,"close_target_height");VEK_CAM_NUM(firstPersonEyeHeight,"first_person_eye_height");VEK_CAM_NUM(fov,"fov");VEK_CAM_NUM(pitchMin,"pitch_min");VEK_CAM_NUM(pitchMax,"pitch_max");VEK_CAM_NUM(yawSensitivity,"yaw_sensitivity");VEK_CAM_NUM(pitchSensitivity,"pitch_sensitivity");VEK_CAM_NUM(maxYawSpeed,"max_yaw_speed");VEK_CAM_NUM(maxPitchSpeed,"max_pitch_speed");VEK_CAM_NUM(yawAcceleration,"yaw_acceleration");VEK_CAM_NUM(yawDeceleration,"yaw_deceleration");VEK_CAM_NUM(pitchAcceleration,"pitch_acceleration");VEK_CAM_NUM(pitchDeceleration,"pitch_deceleration");VEK_CAM_NUM(alignmentStep,"alignment_step");VEK_CAM_NUM(rmbYawScale,"rmb_yaw_scale");VEK_CAM_NUM(rmbPitchScale,"rmb_pitch_scale");VEK_CAM_NUM(editorMoveSpeed,"editor_move_speed");VEK_CAM_NUM(editorFastMultiplier,"editor_fast_multiplier");VEK_CAM_NUM(editorFineMultiplier,"editor_fine_multiplier");VEK_CAM_NUM(editorYawSpeed,"editor_yaw_speed");VEK_CAM_NUM(editorPitchSpeed,"editor_pitch_speed");VEK_CAM_NUM(editorPitchMin,"editor_pitch_min");VEK_CAM_NUM(editorPitchMax,"editor_pitch_max");VEK_CAM_NUM(editorFov,"editor_fov");VEK_CAM_NUM(editorOrthoSize,"editor_ortho_size");VEK_CAM_NUM(minimumWorldY,"minimum_world_y");VEK_CAM_NUM(minimumTargetY,"minimum_target_y");
#undef VEK_CAM_NUM
    auto b=v.Get("rmb_look");if(!b.IsNil())d.rmbLook=b.AsBool();b=v.Get("invert_mouse_y");if(!b.IsNil())d.invertMouseY=b.AsBool();b=v.Get("prevent_below_world");if(!b.IsNil())d.preventBelowWorld=b.AsBool();
    auto modes=v.Get("cycle_modes");if(auto a=modes.AsArray()){d.cycleModes.clear();for(const auto&x:*a)if(x.IsString()){auto m=x.AsString();if(m=="third_person"||m=="close_third_person"||m=="first_person"||m=="free_inspection")d.cycleModes.push_back(m);}if(d.cycleModes.empty())d.cycleModes={"third_person","close_third_person","first_person","free_inspection"};}
    if(!RegisterProfile(d)){if(error)*error="invalid camera profile";return false;}if(error)error->clear();return true;}
const CameraProfileDefinition*CameraProfileRegistry::Find(const std::string&id)const{auto it=profiles.find(id);return it==profiles.end()?nullptr:&it->second;}void CameraProfileRegistry::Clear(){profiles.clear();}std::size_t CameraProfileRegistry::Size()const{return profiles.size();}
void CameraProfileRegistry::RegisterNatives(VekScriptEngine&e){e.RegisterNative("camera_profile_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterProfileValue(a[0],&er));});e.RegisterNative("camera_profile_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString()));});e.RegisterNative("camera_profile_fov",[this](const std::vector<VekValue>&a){auto*d=a.empty()?nullptr:Find(a[0].AsString());return VekValue(d?d->fov:0.0f);});}

bool SkyboxRegistry::RegisterSkybox(const SkyboxDefinition&d){if(d.id.empty()||(!d.textureAsset.empty()&&!SafeAssetId18(d.textureAsset)))return false;SkyboxDefinition s=d;s.sunPitch=std::clamp(s.sunPitch,-89.0f,89.0f);s.sunSize=std::clamp(s.sunSize,0.1f,30.0f);s.ambient=std::clamp(s.ambient,0.0f,2.0f);s.fogStart=std::clamp(s.fogStart,0.0f,10000.0f);s.fogEnd=std::clamp(s.fogEnd,s.fogStart+1.0f,20000.0f);s.dayLengthSeconds=std::clamp(s.dayLengthSeconds,10.0f,86400.0f);skyboxes[s.id]=std::move(s);return true;}
bool SkyboxRegistry::RegisterSkyboxValue(const VekValue&v,std::string*error){if(!v.IsMap()){if(error)*error="skybox_register expects a map";return false;}SkyboxDefinition d;d.id=v.Get("id").AsString();ReadSkyColor18(v,"zenith",d.zenith);ReadSkyColor18(v,"horizon",d.horizon);ReadSkyColor18(v,"ground",d.ground);ReadSkyColor18(v,"sun",d.sun);d.horizonHeight=(float)v.Get("horizon_height").AsNumber(d.horizonHeight);d.sunYaw=(float)v.Get("sun_yaw").AsNumber(d.sunYaw);d.sunPitch=(float)v.Get("sun_pitch").AsNumber(d.sunPitch);d.sunSize=(float)v.Get("sun_size").AsNumber(d.sunSize);d.ambient=(float)v.Get("ambient").AsNumber(d.ambient);d.fogStart=(float)v.Get("fog_start").AsNumber(d.fogStart);d.fogEnd=(float)v.Get("fog_end").AsNumber(d.fogEnd);d.dayLengthSeconds=(float)v.Get("day_length_seconds").AsNumber(d.dayLengthSeconds);auto dyn=v.Get("dynamic_day_night");if(!dyn.IsNil())d.dynamicDayNight=dyn.AsBool();d.textureAsset=v.Get("texture_asset").AsString();if(!RegisterSkybox(d)){if(error)*error="invalid/unsafe skybox definition";return false;}if(error)error->clear();return true;}
const SkyboxDefinition*SkyboxRegistry::Find(const std::string&id)const{auto it=skyboxes.find(id);return it==skyboxes.end()?nullptr:&it->second;}void SkyboxRegistry::Clear(){skyboxes.clear();}std::size_t SkyboxRegistry::Size()const{return skyboxes.size();}
void SkyboxRegistry::RegisterNatives(VekScriptEngine&e){e.RegisterNative("skybox_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterSkyboxValue(a[0],&er));});e.RegisterNative("skybox_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString()));});}

bool HumanoidRigRegistry::RegisterRig(const HumanoidRigDefinition&d){if(d.id.empty()||d.joints.empty()||d.joints.size()>128)return false;HumanoidRigDefinition r=d;r.globalRagdollStrength=std::clamp(r.globalRagdollStrength,0.05f,5.0f);r.spineFlex=std::clamp(r.spineFlex,0.0f,3.0f);r.neckFlex=std::clamp(r.neckFlex,0.0f,3.0f);r.limbFlex=std::clamp(r.limbFlex,0.0f,3.0f);std::unordered_map<std::string,bool>seen;for(auto&j:r.joints){if(j.name.empty()||seen[j.name])return false;seen[j.name]=true;j.length=std::clamp(j.length,0.01f,3.0f);j.radius=std::clamp(j.radius,0.01f,0.5f);j.mass=std::clamp(j.mass,0.01f,100.0f);j.ragdollWeight=std::clamp(j.ragdollWeight,0.0f,5.0f);j.pitchMin=std::clamp(j.pitchMin,-180.0f,180.0f);j.pitchMax=std::clamp(j.pitchMax,j.pitchMin,180.0f);j.yawMin=std::clamp(j.yawMin,-180.0f,180.0f);j.yawMax=std::clamp(j.yawMax,j.yawMin,180.0f);j.rollMin=std::clamp(j.rollMin,-180.0f,180.0f);j.rollMax=std::clamp(j.rollMax,j.rollMin,180.0f);}rigs[r.id]=std::move(r);return true;}
bool HumanoidRigRegistry::RegisterRigValue(const VekValue&v,std::string*error){if(!v.IsMap()){if(error)*error="rig_register expects a map";return false;}HumanoidRigDefinition r;r.id=v.Get("id").AsString();r.globalRagdollStrength=(float)v.Get("ragdoll_strength").AsNumber(1.0);r.spineFlex=(float)v.Get("spine_flex").AsNumber(0.75);r.neckFlex=(float)v.Get("neck_flex").AsNumber(0.85);r.limbFlex=(float)v.Get("limb_flex").AsNumber(1.0);if(auto a=v.Get("joints").AsArray())for(const auto&x:*a)if(x.IsMap()){RigJointDefinition j;j.name=x.Get("name").AsString();j.parent=x.Get("parent").AsString();j.length=(float)x.Get("length").AsNumber(j.length);j.radius=(float)x.Get("radius").AsNumber(j.radius);j.mass=(float)x.Get("mass").AsNumber(j.mass);j.ragdollWeight=(float)x.Get("ragdoll_weight").AsNumber(j.ragdollWeight);j.pitchMin=(float)x.Get("pitch_min").AsNumber(j.pitchMin);j.pitchMax=(float)x.Get("pitch_max").AsNumber(j.pitchMax);j.yawMin=(float)x.Get("yaw_min").AsNumber(j.yawMin);j.yawMax=(float)x.Get("yaw_max").AsNumber(j.yawMax);j.rollMin=(float)x.Get("roll_min").AsNumber(j.rollMin);j.rollMax=(float)x.Get("roll_max").AsNumber(j.rollMax);r.joints.push_back(std::move(j));}if(!RegisterRig(r)){if(error)*error="invalid humanoid rig";return false;}if(error)error->clear();return true;}
const HumanoidRigDefinition*HumanoidRigRegistry::Find(const std::string&id)const{auto it=rigs.find(id);return it==rigs.end()?nullptr:&it->second;}void HumanoidRigRegistry::Clear(){rigs.clear();}std::size_t HumanoidRigRegistry::Size()const{return rigs.size();}
void HumanoidRigRegistry::RegisterNatives(VekScriptEngine&e){e.RegisterNative("rig_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterRigValue(a[0],&er));});e.RegisterNative("rig_joint_count",[this](const std::vector<VekValue>&a){auto*r=a.empty()?nullptr:Find(a[0].AsString());return VekValue(r?(double)r->joints.size():0.0);});}

bool WorldGameplayPolicyRegistry::RegisterPolicy(const WorldGameplayPolicy&d){if(d.id.empty())return false;WorldGameplayPolicy p=d;p.targetFps=std::clamp(p.targetFps,30,360);p.movementWalk=std::clamp(p.movementWalk,0.1f,20.0f);p.movementRun=std::clamp(p.movementRun,p.movementWalk,30.0f);p.movementSprint=std::clamp(p.movementSprint,p.movementRun,40.0f);p.interactionDistance=std::clamp(p.interactionDistance,0.5f,20.0f);p.vehicleEnterDistance=std::clamp(p.vehicleEnterDistance,0.5f,20.0f);p.mapMarkerDistance=std::clamp(p.mapMarkerDistance,10.0f,5000.0f);p.physicsMaxDt=std::clamp(p.physicsMaxDt,0.005f,0.1f);policies[p.id]=std::move(p);return true;}
bool WorldGameplayPolicyRegistry::RegisterPolicyValue(const VekValue&v,std::string*error){if(!v.IsMap()){if(error)*error="world_policy_register expects a map";return false;}WorldGameplayPolicy p;p.id=v.Get("id").AsString();p.targetFps=(int)v.Get("target_fps").AsNumber(p.targetFps);p.movementWalk=(float)v.Get("walk_speed").AsNumber(p.movementWalk);p.movementRun=(float)v.Get("run_speed").AsNumber(p.movementRun);p.movementSprint=(float)v.Get("sprint_speed").AsNumber(p.movementSprint);p.interactionDistance=(float)v.Get("interaction_distance").AsNumber(p.interactionDistance);p.vehicleEnterDistance=(float)v.Get("vehicle_enter_distance").AsNumber(p.vehicleEnterDistance);p.mapMarkerDistance=(float)v.Get("map_marker_distance").AsNumber(p.mapMarkerDistance);p.physicsMaxDt=(float)v.Get("physics_max_dt").AsNumber(p.physicsMaxDt);auto b=v.Get("camera_rmb_look");if(!b.IsNil())p.cameraRmbLook=b.AsBool();b=v.Get("sky_enabled");if(!b.IsNil())p.skyEnabled=b.AsBool();if(!RegisterPolicy(p)){if(error)*error="invalid world policy";return false;}if(error)error->clear();return true;}
const WorldGameplayPolicy*WorldGameplayPolicyRegistry::Find(const std::string&id)const{auto it=policies.find(id);return it==policies.end()?nullptr:&it->second;}void WorldGameplayPolicyRegistry::Clear(){policies.clear();}std::size_t WorldGameplayPolicyRegistry::Size()const{return policies.size();}
void WorldGameplayPolicyRegistry::RegisterNatives(VekScriptEngine&e){e.RegisterNative("world_policy_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterPolicyValue(a[0],&er));});e.RegisterNative("world_policy_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString()));});}

GuiTextLayoutResult GuiTextLayoutSystem::Layout(const std::string&text,float width,float height,const GuiTextPolicy&input,const GuiMeasureTextFn&measure){
    GuiTextPolicy p=input;
    p.minFontSize=std::max(4.0f,p.minFontSize);
    p.maxFontSize=std::max(p.minFontSize,p.maxFontSize);
    p.fontSize=std::clamp(p.fontSize,p.minFontSize,p.maxFontSize);
    p.lineHeight=std::clamp(p.lineHeight,0.8f,2.5f);
    p.maxLines=std::clamp(p.maxLines,1,64);
    GuiTextLayoutResult result;result.fontSize=p.fontSize;
    auto layoutAt=[&](float fs){
        std::vector<std::string> lines=p.wrap?WrapGuiText(text,fs,width,measure):std::vector<std::string>{text};
        bool truncated=false;
        int heightLines=height>0.0f?std::max(1,(int)std::floor(height/(fs*p.lineHeight))):p.maxLines;
        int allowed=std::max(1,std::min(p.maxLines,heightLines));
        if((int)lines.size()>allowed){lines.resize((std::size_t)allowed);truncated=true;}
        if(!p.wrap&&!lines.empty()&&width>0.0f&&measure(lines[0],fs)>width)truncated=true;
        if(truncated&&p.ellipsis&&!lines.empty())lines.back()=EllipsizeGuiText(lines.back(),fs,width,measure);
        else if(!p.wrap&&p.ellipsis&&!lines.empty())lines[0]=EllipsizeGuiText(lines[0],fs,width,measure);
        float maxWidth=0.0f;for(const auto&line:lines)maxWidth=std::max(maxWidth,measure(line,fs));
        float usedHeight=lines.empty()?0.0f:(float)lines.size()*fs*p.lineHeight;
        bool fits=(width<=0.0f||maxWidth<=width+0.5f)&&(height<=0.0f||usedHeight<=height+0.5f);
        return std::tuple<std::vector<std::string>,bool,bool>{std::move(lines),truncated,fits};
    };
    float fs=p.fontSize;
    auto [lines,truncated,fits]=layoutAt(fs);
    if(p.autoFit&&!fits){
        for(float candidate=fs-1.0f;candidate>=p.minFontSize;candidate-=1.0f){
            auto trial=layoutAt(candidate);
            fs=candidate;lines=std::move(std::get<0>(trial));truncated=std::get<1>(trial);fits=std::get<2>(trial);
            if(fits)break;
        }
    }
    result.fontSize=fs;result.lines=std::move(lines);result.truncated=truncated;return result;
}

void GuiSystem::BeginFrame(){commands.clear();}
void GuiSystem::EndFrame(){}
void GuiSystem::SetStyle(const GuiStyle&s){style=s;}
const GuiStyle&GuiSystem::GetStyle()const{return style;}
void GuiSystem::DefineStyle(const std::string&id,const GuiStyle&s){styles[id]=s;}
const GuiStyle*GuiSystem::FindStyle(const std::string&id)const{auto it=styles.find(id);return it==styles.end()?nullptr:&it->second;}
GuiTextPolicy GuiSystem::ResolveTextPolicy(const std::string&id)const{if(id.empty())return style.text;auto it=styles.find(id);return it==styles.end()?style.text:it->second.text;}
static GuiCommand MakeContainer(GuiCommandType t,const std::string&id,GuiRect r,const std::string&style){GuiCommand c;c.type=t;c.id=id;c.rect=r;c.styleId=style;return c;}
void GuiSystem::BeginWindow(const std::string&id,const std::string&title,GuiRect r,const std::string&st){auto c=MakeContainer(GuiCommandType::BeginWindow,id,r,st);c.text=title;c.textPolicy=ResolveTextPolicy(st);commands.push_back(std::move(c));}void GuiSystem::EndWindow(){commands.push_back(MakeContainer(GuiCommandType::EndWindow,"",{},""));}
void GuiSystem::BeginModal(const std::string&id,const std::string&title,GuiRect r,const std::string&st){auto c=MakeContainer(GuiCommandType::BeginModal,id,r,st);c.text=title;c.textPolicy=ResolveTextPolicy(st);commands.push_back(std::move(c));}void GuiSystem::EndModal(){commands.push_back(MakeContainer(GuiCommandType::EndModal,"",{},""));}
void GuiSystem::BeginPanel(const std::string&id,GuiRect r,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginPanel,id,r,st));}void GuiSystem::EndPanel(){commands.push_back(MakeContainer(GuiCommandType::EndPanel,"",{},""));}
void GuiSystem::BeginDockPanel(const std::string&id,GuiRect r,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginDockPanel,id,r,st));}void GuiSystem::EndDockPanel(){commands.push_back(MakeContainer(GuiCommandType::EndDockPanel,"",{},""));}
void GuiSystem::BeginScrollPanel(const std::string&id,GuiRect r,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginScrollPanel,id,r,st));}void GuiSystem::EndScrollPanel(){commands.push_back(MakeContainer(GuiCommandType::EndScrollPanel,"",{},""));}
void GuiSystem::BeginGrid(const std::string&id,int cols,GuiRect r,const std::string&st){auto c=MakeContainer(GuiCommandType::BeginGrid,id,r,st);c.columns=std::max(1,cols);commands.push_back(std::move(c));}void GuiSystem::EndGrid(){commands.push_back(MakeContainer(GuiCommandType::EndGrid,"",{},""));}
void GuiSystem::BeginHorizontal(const std::string&id,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginHorizontal,id,{},st));}void GuiSystem::EndHorizontal(){commands.push_back(MakeContainer(GuiCommandType::EndHorizontal,"",{},""));}
void GuiSystem::BeginVertical(const std::string&id,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginVertical,id,{},st));}void GuiSystem::EndVertical(){commands.push_back(MakeContainer(GuiCommandType::EndVertical,"",{},""));}
void GuiSystem::BeginTabs(const std::string&id,const std::vector<std::string>&items,const std::string&st){auto c=MakeContainer(GuiCommandType::BeginTabs,id,{},st);c.items=items;commands.push_back(std::move(c));}void GuiSystem::EndTabs(){commands.push_back(MakeContainer(GuiCommandType::EndTabs,"",{},""));}
void GuiSystem::BeginTree(const std::string&id,const std::string&label,const std::string&st){auto c=MakeContainer(GuiCommandType::BeginTree,id,{},st);c.text=label;commands.push_back(std::move(c));}void GuiSystem::EndTree(){commands.push_back(MakeContainer(GuiCommandType::EndTree,"",{},""));}
void GuiSystem::BeginList(const std::string&id,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginList,id,{},st));}void GuiSystem::EndList(){commands.push_back(MakeContainer(GuiCommandType::EndList,"",{},""));}
void GuiSystem::BeginContextMenu(const std::string&id,GuiRect r,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginContextMenu,id,r,st));}void GuiSystem::EndContextMenu(){commands.push_back(MakeContainer(GuiCommandType::EndContextMenu,"",{},""));}
void GuiSystem::BeginPropertyGrid(const std::string&id,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginPropertyGrid,id,{},st));}void GuiSystem::EndPropertyGrid(){commands.push_back(MakeContainer(GuiCommandType::EndPropertyGrid,"",{},""));}
void GuiSystem::Label(const std::string&t,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::Label;c.text=t;c.rect=r;c.styleId=st;c.textPolicy=ResolveTextPolicy(st);commands.push_back(std::move(c));}
bool GuiSystem::Button(const std::string&id,const std::string&t,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::Button;c.id=id;c.text=t;c.rect=r;c.styleId=st;c.textPolicy=ResolveTextPolicy(st);commands.push_back(c);auto it=pressed.find(id);bool hit=it!=pressed.end()&&it->second;if(hit)it->second=false;return hit;}
bool GuiSystem::ImageButton(const std::string&id,const std::string&img,const std::string&t,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::ImageButton;c.id=id;c.aux=img;c.text=t;c.rect=r;c.styleId=st;c.textPolicy=ResolveTextPolicy(st);commands.push_back(c);auto it=pressed.find(id);bool hit=it!=pressed.end()&&it->second;if(hit)it->second=false;return hit;}
bool GuiSystem::Checkbox(const std::string&id,const std::string&t,bool v,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::Checkbox;c.id=id;c.text=t;c.rect=r;c.checked=v;c.styleId=st;c.textPolicy=ResolveTextPolicy(st);commands.push_back(c);auto it=pressed.find(id);if(it!=pressed.end()&&it->second){it->second=false;return !v;}return v;}
float GuiSystem::Slider(const std::string&id,const std::string&t,float v,float lo,float hi,GuiRect r,const std::string&st){auto it=values.find(id);if(it!=values.end())v=it->second;v=std::clamp(v,lo,hi);GuiCommand c;c.type=GuiCommandType::Slider;c.id=id;c.text=t;c.rect=r;c.value=v;c.minValue=lo;c.maxValue=hi;c.styleId=st;c.textPolicy=ResolveTextPolicy(st);commands.push_back(c);return v;}
void GuiSystem::ProgressBar(const std::string&id,float v,float lo,float hi,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::ProgressBar;c.id=id;c.rect=r;c.value=std::clamp(v,lo,hi);c.minValue=lo;c.maxValue=hi;c.styleId=st;commands.push_back(c);}
std::string GuiSystem::TextInput(const std::string&id,const std::string&v,GuiRect r,const std::string&st){std::string cur=v;auto it=texts.find(id);if(it!=texts.end())cur=it->second;GuiCommand c;c.type=GuiCommandType::TextInput;c.id=id;c.text=cur;c.rect=r;c.styleId=st;c.textPolicy=ResolveTextPolicy(st);commands.push_back(c);return cur;}
std::string GuiSystem::PasswordInput(const std::string&id,const std::string&v,GuiRect r,const std::string&st){std::string cur=v;auto it=texts.find(id);if(it!=texts.end())cur=it->second;GuiCommand c;c.type=GuiCommandType::PasswordInput;c.id=id;c.text=cur;c.rect=r;c.styleId=st;c.textPolicy=ResolveTextPolicy(st);commands.push_back(c);return cur;}
std::string GuiSystem::SearchBox(const std::string&id,const std::string&v,GuiRect r,const std::string&st){std::string cur=v;auto it=texts.find(id);if(it!=texts.end())cur=it->second;GuiCommand c;c.type=GuiCommandType::SearchBox;c.id=id;c.text=cur;c.rect=r;c.styleId=st;c.textPolicy=ResolveTextPolicy(st);commands.push_back(c);return cur;}
int GuiSystem::ComboBox(const std::string&id,const std::vector<std::string>&items,int idx,GuiRect r,const std::string&st){auto it=indices.find(id);if(it!=indices.end())idx=it->second;if(items.empty())idx=-1;else idx=std::clamp(idx,0,(int)items.size()-1);GuiCommand c;c.type=GuiCommandType::ComboBox;c.id=id;c.items=items;c.value=(float)idx;c.rect=r;c.styleId=st;commands.push_back(c);return idx;}
int GuiSystem::Dropdown(const std::string&id,const std::vector<std::string>&items,int idx,GuiRect r,const std::string&st){auto it=indices.find(id);if(it!=indices.end())idx=it->second;if(items.empty())idx=-1;else idx=std::clamp(idx,0,(int)items.size()-1);GuiCommand c;c.type=GuiCommandType::Dropdown;c.id=id;c.items=items;c.value=(float)idx;c.rect=r;c.styleId=st;commands.push_back(c);return idx;}
void GuiSystem::StatusBadge(const std::string&id,const std::string&text,const std::string&status,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::StatusBadge;c.id=id;c.text=text;c.aux=status;c.rect=r;c.styleId=st;c.textPolicy=ResolveTextPolicy(st);commands.push_back(std::move(c));}
void GuiSystem::Keypad(const std::string&id,const std::vector<std::string>&items,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::Keypad;c.id=id;c.items=items;c.rect=r;c.styleId=st;commands.push_back(std::move(c));}
void GuiSystem::Tooltip(const std::string&id,const std::string&t){GuiCommand c;c.type=GuiCommandType::Tooltip;c.id=id;c.text=t;c.textPolicy=ResolveTextPolicy("");commands.push_back(std::move(c));}void GuiSystem::Separator(){commands.push_back(MakeContainer(GuiCommandType::Separator,"",{},""));}void GuiSystem::Spacer(float a){GuiCommand c;c.type=GuiCommandType::Spacer;c.value=a;commands.push_back(c);}void GuiSystem::SetPressed(const std::string&id,bool v){pressed[id]=v;}void GuiSystem::SetValue(const std::string&id,float v){values[id]=v;}void GuiSystem::SetText(const std::string&id,std::string v){texts[id]=std::move(v);}void GuiSystem::SetIndex(const std::string&id,int i){indices[id]=i;}const std::vector<GuiCommand>&GuiSystem::Commands()const{return commands;}
static std::vector<std::string> GuiItems(const VekValue&v){std::vector<std::string>o;if(auto*a=v.AsArray())for(auto&x:*a)o.push_back(x.AsString());return o;}
static GuiStyle StyleFromMap(const VekValue&v,GuiStyle s={}){if(!v.IsMap())return s;s.padding=(float)v.Get("padding").AsNumber(s.padding);s.cornerRadius=(float)v.Get("corner_radius").AsNumber(s.cornerRadius);s.spacing=(float)v.Get("spacing").AsNumber(s.spacing);s.opacity=(float)v.Get("opacity").AsNumber(s.opacity);s.minWidth=(float)v.Get("min_width").AsNumber(s.minWidth);s.maxWidth=(float)v.Get("max_width").AsNumber(s.maxWidth);s.minHeight=(float)v.Get("min_height").AsNumber(s.minHeight);s.maxHeight=(float)v.Get("max_height").AsNumber(s.maxHeight);auto resp=v.Get("responsive");if(!resp.IsNil())s.responsive=resp.AsBool();s.text.fontSize=(float)v.Get("font_size").AsNumber(s.text.fontSize);s.text.minFontSize=(float)v.Get("min_font_size").AsNumber(s.text.minFontSize);s.text.maxFontSize=(float)v.Get("max_font_size").AsNumber(s.text.maxFontSize);s.text.lineHeight=(float)v.Get("line_height").AsNumber(s.text.lineHeight);s.text.maxLines=(int)v.Get("max_lines").AsNumber(s.text.maxLines);auto af=v.Get("auto_fit");if(!af.IsNil())s.text.autoFit=af.AsBool();auto wr=v.Get("wrap");if(!wr.IsNil())s.text.wrap=wr.AsBool();auto el=v.Get("ellipsis");if(!el.IsNil())s.text.ellipsis=el.AsBool();auto cl=v.Get("clip_text");if(!cl.IsNil())s.text.clip=cl.AsBool();auto al=v.Get("text_align").AsString();if(al=="center")s.text.align=GuiTextAlign::Center;else if(al=="right")s.text.align=GuiTextAlign::Right;else if(al=="left")s.text.align=GuiTextAlign::Left;auto col=[&](const char*k,GuiColor&c){auto x=v.Get(k);if(x.IsMap()){c.r=(float)x.Get("r").AsNumber(c.r);c.g=(float)x.Get("g").AsNumber(c.g);c.b=(float)x.Get("b").AsNumber(c.b);c.a=(float)x.Get("a").AsNumber(c.a);}};col("background",s.background);col("foreground",s.foreground);col("accent",s.accent);col("border",s.border);return s;}
void GuiSystem::RegisterNatives(VekScriptEngine&e){
 e.RegisterNative("gui_define_style",[this](const std::vector<VekValue>&a){if(a.size()>=2)DefineStyle(a[0].AsString(),StyleFromMap(a[1],style));return VekValue();});
 e.RegisterNative("gui_begin_window",[this](const std::vector<VekValue>&a){if(a.size()>=2)BeginWindow(a[0].AsString(),a[1].AsString(),{},a.size()>2?a[2].AsString():"");return VekValue();});e.RegisterNative("gui_end_window",[this](const std::vector<VekValue>&){EndWindow();return VekValue();});
 e.RegisterNative("gui_begin_modal",[this](const std::vector<VekValue>&a){if(a.size()>=2)BeginModal(a[0].AsString(),a[1].AsString(),{},a.size()>2?a[2].AsString():"");return VekValue();});e.RegisterNative("gui_end_modal",[this](const std::vector<VekValue>&){EndModal();return VekValue();});
 e.RegisterNative("gui_begin_panel",[this](const std::vector<VekValue>&a){BeginPanel(a.empty()?"panel":a[0].AsString(),{},a.size()>1?a[1].AsString():"");return VekValue();});e.RegisterNative("gui_end_panel",[this](const std::vector<VekValue>&){EndPanel();return VekValue();});
 e.RegisterNative("gui_begin_dock",[this](const std::vector<VekValue>&a){BeginDockPanel(a.empty()?"dock":a[0].AsString());return VekValue();});e.RegisterNative("gui_end_dock",[this](const std::vector<VekValue>&){EndDockPanel();return VekValue();});
 e.RegisterNative("gui_begin_scroll",[this](const std::vector<VekValue>&a){BeginScrollPanel(a.empty()?"scroll":a[0].AsString());return VekValue();});e.RegisterNative("gui_end_scroll",[this](const std::vector<VekValue>&){EndScrollPanel();return VekValue();});
 e.RegisterNative("gui_begin_grid",[this](const std::vector<VekValue>&a){BeginGrid(a.empty()?"grid":a[0].AsString(),a.size()>1?(int)a[1].AsNumber(1):1);return VekValue();});e.RegisterNative("gui_end_grid",[this](const std::vector<VekValue>&){EndGrid();return VekValue();});
 e.RegisterNative("gui_begin_horizontal",[this](const std::vector<VekValue>&a){BeginHorizontal(a.empty()?"row":a[0].AsString());return VekValue();});e.RegisterNative("gui_end_horizontal",[this](const std::vector<VekValue>&){EndHorizontal();return VekValue();});
 e.RegisterNative("gui_begin_vertical",[this](const std::vector<VekValue>&a){BeginVertical(a.empty()?"column":a[0].AsString());return VekValue();});e.RegisterNative("gui_end_vertical",[this](const std::vector<VekValue>&){EndVertical();return VekValue();});
 e.RegisterNative("gui_begin_tabs",[this](const std::vector<VekValue>&a){BeginTabs(a.empty()?"tabs":a[0].AsString(),a.size()>1?GuiItems(a[1]):std::vector<std::string>{});return VekValue();});e.RegisterNative("gui_end_tabs",[this](const std::vector<VekValue>&){EndTabs();return VekValue();});
 e.RegisterNative("gui_begin_tree",[this](const std::vector<VekValue>&a){BeginTree(a.empty()?"tree":a[0].AsString(),a.size()>1?a[1].AsString():"");return VekValue();});e.RegisterNative("gui_end_tree",[this](const std::vector<VekValue>&){EndTree();return VekValue();});
 e.RegisterNative("gui_begin_list",[this](const std::vector<VekValue>&a){BeginList(a.empty()?"list":a[0].AsString());return VekValue();});e.RegisterNative("gui_end_list",[this](const std::vector<VekValue>&){EndList();return VekValue();});
 e.RegisterNative("gui_begin_property_grid",[this](const std::vector<VekValue>&a){BeginPropertyGrid(a.empty()?"props":a[0].AsString());return VekValue();});e.RegisterNative("gui_end_property_grid",[this](const std::vector<VekValue>&){EndPropertyGrid();return VekValue();});
 e.RegisterNative("gui_label",[this](const std::vector<VekValue>&a){if(!a.empty())Label(a[0].AsString(),{},a.size()>1?a[1].AsString():"");return VekValue();});e.RegisterNative("gui_button",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=2&&Button(a[0].AsString(),a[1].AsString(),{},a.size()>2?a[2].AsString():""));});e.RegisterNative("gui_image_button",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=3&&ImageButton(a[0].AsString(),a[1].AsString(),a[2].AsString()));});e.RegisterNative("gui_checkbox",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=3&&Checkbox(a[0].AsString(),a[1].AsString(),a[2].AsBool()));});e.RegisterNative("gui_slider",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=5?Slider(a[0].AsString(),a[1].AsString(),(float)a[2].AsNumber(),(float)a[3].AsNumber(),(float)a[4].AsNumber()):0.0);});e.RegisterNative("gui_progress",[this](const std::vector<VekValue>&a){if(a.size()>=4)ProgressBar(a[0].AsString(),(float)a[1].AsNumber(),(float)a[2].AsNumber(),(float)a[3].AsNumber());return VekValue();});e.RegisterNative("gui_text_input",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=2?TextInput(a[0].AsString(),a[1].AsString()):"");});e.RegisterNative("gui_password_input",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=2?PasswordInput(a[0].AsString(),a[1].AsString()):"");});e.RegisterNative("gui_search_box",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=2?SearchBox(a[0].AsString(),a[1].AsString()):"");});e.RegisterNative("gui_combo",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=3?ComboBox(a[0].AsString(),GuiItems(a[1]),(int)a[2].AsNumber()):-1);});e.RegisterNative("gui_dropdown",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=3?Dropdown(a[0].AsString(),GuiItems(a[1]),(int)a[2].AsNumber()):-1);});e.RegisterNative("gui_status_badge",[this](const std::vector<VekValue>&a){if(a.size()>=3)StatusBadge(a[0].AsString(),a[1].AsString(),a[2].AsString(),{},a.size()>3?a[3].AsString():"");return VekValue();});e.RegisterNative("gui_keypad",[this](const std::vector<VekValue>&a){if(a.size()>=2)Keypad(a[0].AsString(),GuiItems(a[1]),{},a.size()>2?a[2].AsString():"");return VekValue();});e.RegisterNative("gui_tooltip",[this](const std::vector<VekValue>&a){if(a.size()>=2)Tooltip(a[0].AsString(),a[1].AsString());return VekValue();});e.RegisterNative("gui_separator",[this](const std::vector<VekValue>&){Separator();return VekValue();});e.RegisterNative("gui_spacer",[this](const std::vector<VekValue>&a){Spacer(a.empty()?0.0f:(float)a[0].AsNumber());return VekValue();});
}

void VekRegisterGameplayLibrary(VekScriptEngine& e) {
    // Small safe math set commonly needed by gameplay rule scripts.
    e.RegisterNative("abs",[](const std::vector<VekValue>& a){return VekValue(a.empty()?0.0:std::fabs(a[0].AsNumber()));});
    e.RegisterNative("min",[](const std::vector<VekValue>& a){if(a.size()<2)return VekValue(0.0);return VekValue(std::min(a[0].AsNumber(),a[1].AsNumber()));});
    e.RegisterNative("max",[](const std::vector<VekValue>& a){if(a.size()<2)return VekValue(0.0);return VekValue(std::max(a[0].AsNumber(),a[1].AsNumber()));});
    e.RegisterNative("clamp",[](const std::vector<VekValue>& a){if(a.size()<3)return VekValue(0.0);return VekValue(std::clamp(a[0].AsNumber(),a[1].AsNumber(),a[2].AsNumber()));});
    e.RegisterNative("gravity_step",[](const std::vector<VekValue>& a){
        if(a.size()<5)return VekValue(0.0); float v=(float)a[0].AsNumber(); float gravity=std::max(0.0f,(float)a[1].AsNumber()); float terminal=std::fabs((float)a[2].AsNumber()); float dt=ClampDt((float)a[3].AsNumber()); bool grounded=a[4].AsBool(); if(grounded)return VekValue(0.0); return VekValue(std::max(v-gravity*dt,-terminal));
    });
    e.RegisterNative("gravity_jump",[](const std::vector<VekValue>& a){if(a.size()<2)return VekValue(0.0);return VekValue(a[0].AsBool()?std::max(0.0,a[1].AsNumber()):0.0);});
    e.RegisterNative("health_damage",[](const std::vector<VekValue>& a){if(a.size()<3)return VekValue(0.0);return VekValue(std::clamp(a[0].AsNumber()-std::max(0.0,a[2].AsNumber()),0.0,std::max(0.0,a[1].AsNumber())));});
    e.RegisterNative("health_heal",[](const std::vector<VekValue>& a){if(a.size()<3)return VekValue(0.0);return VekValue(std::clamp(a[0].AsNumber()+std::max(0.0,a[2].AsNumber()),0.0,std::max(0.0,a[1].AsNumber())));});
    e.RegisterNative("health_ratio",[](const std::vector<VekValue>& a){if(a.size()<2||a[1].AsNumber()<=0.0)return VekValue(0.0);return VekValue(std::clamp(a[0].AsNumber()/a[1].AsNumber(),0.0,1.0));});
    e.RegisterNative("health_alive",[](const std::vector<VekValue>& a){return VekValue(!a.empty()&&a[0].AsNumber()>0.0);});
    e.RegisterNative("ragdoll_should_trigger",[](const std::vector<VekValue>& a){if(a.size()<5)return VekValue(false);double health=a[0].AsNumber(),impact=a[1].AsNumber(),damage=a[2].AsNumber(),minImpact=a[3].AsNumber(),fatal=a[4].AsNumber();return VekValue(health<=0.0||impact>=fatal||(damage>0.0&&impact>=minImpact));});
    e.RegisterNative("ragdoll_duration",[](const std::vector<VekValue>& a){if(a.size()<5)return VekValue(1.0);double impact=a[0].AsNumber(),minImpact=a[1].AsNumber(),fatal=a[2].AsNumber(),minD=a[3].AsNumber(),maxD=a[4].AsNumber();double n=std::clamp((impact-minImpact)/std::max(1.0,fatal-minImpact),0.0,1.0);return VekValue(minD+(maxD-minD)*n);});
    e.RegisterNative("damp",[](const std::vector<VekValue>& a){if(a.size()<4)return VekValue(0.0);double current=a[0].AsNumber(),target=a[1].AsNumber(),speed=std::max(0.0,a[2].AsNumber()),dt=std::clamp(a[3].AsNumber(),0.0,0.1);double t=std::clamp(speed*dt,0.0,1.0);return VekValue(current+(target-current)*t);});
}

} // namespace vek
