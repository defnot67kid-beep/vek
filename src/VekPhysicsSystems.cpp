#include <vek/VekPhysicsSystems.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <unordered_set>
#include <limits>

namespace vek {
namespace {

PhysicsVec3 Add(PhysicsVec3 a, PhysicsVec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
PhysicsVec3 Sub(PhysicsVec3 a, PhysicsVec3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
PhysicsVec3 Mul(PhysicsVec3 a, float s) { return {a.x*s,a.y*s,a.z*s}; }
float Dot(PhysicsVec3 a, PhysicsVec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
float LengthSq(PhysicsVec3 v) { return Dot(v,v); }
float Length(PhysicsVec3 v) { return std::sqrt(std::max(0.0f,LengthSq(v))); }
PhysicsVec3 NormalizeOr(PhysicsVec3 v, PhysicsVec3 fallback) {
    float n=Length(v);
    if(n<=1e-6f || !std::isfinite(n)) return fallback;
    return Mul(v,1.0f/n);
}
PhysicsVec3 ClampMagnitude(PhysicsVec3 v,float maxLen){
    if(maxLen<=0.0f)return {0,0,0};
    float n=Length(v);
    if(n<=maxLen || n<=1e-6f)return v;
    return Mul(v,maxLen/n);
}
float FiniteOr(float value,float fallback){return std::isfinite(value)?value:fallback;}

bool SafePhysicsToken(const std::string& s,std::size_t maxLen){
    if(s.empty()||s.size()>maxLen)return false;
    for(unsigned char c:s) if(!(std::isalnum(c)||c=='_'||c=='-'||c=='.'||c==':'||c=='/')) return false;
    return true;
}
bool ValidPhysicsCategory(const std::string& c){
    static const std::unordered_set<std::string> allowed={
        "material","collider","rigid_body","joint","solver","world","character_controller","vehicle",
        "soft_body","cloth","rope","aerodynamics","buoyancy","breakable","force_field","query_filter",
        "contact_material","ccd","articulation","ragdoll","ik_chain","particle_emitter","fluid_volume",
        "destruction_cluster","tire_friction","suspension","scene_query","physics_lod","physics_event",
        "mass_properties","advanced_material","contact_solver","constraint_graph","sensor","drivetrain",
        "differential","aero_surface","wheel_contact","scene_settings","debug_draw","snapshot","network_sync"};
    return allowed.count(c)!=0;
}

SecondaryMotionProfile Sanitize(SecondaryMotionProfile p){
    if(p.id.empty()) p.id="secondary.default";
    p.gravity=std::clamp(FiniteOr(p.gravity,7.0f),-40.0f,40.0f);
    p.damping=std::clamp(FiniteOr(p.damping,0.90f),0.0f,0.9995f);
    p.stiffness=std::clamp(FiniteOr(p.stiffness,0.35f),0.0f,1.0f);
    p.airDrag=std::clamp(FiniteOr(p.airDrag,0.08f),0.0f,1.0f);
    p.inertia=std::clamp(FiniteOr(p.inertia,0.80f),0.0f,3.0f);
    p.windInfluence=std::clamp(FiniteOr(p.windInfluence,0.65f),0.0f,3.0f);
    p.maxDt=std::clamp(FiniteOr(p.maxDt,1.0f/30.0f),1.0f/240.0f,0.05f);
    p.constraintIterations=std::clamp(p.constraintIterations,1,16);
    p.maxSubsteps=std::clamp(p.maxSubsteps,1,8);
    p.collisionPadding=std::clamp(FiniteOr(p.collisionPadding,0.008f),0.0f,0.10f);
    p.maxSpeed=std::clamp(FiniteOr(p.maxSpeed,8.0f),0.1f,100.0f);
    return p;
}

}

bool PhysicsDefinitionRegistry::RegisterDefinition(const std::string& category,const VekValue& definition,std::string* error){
    if(!ValidPhysicsCategory(category)){if(error)*error="unknown physics v0.3 definition category";return false;}
    if(!definition.IsMap()){if(error)*error="physics definition register expects a map definition";return false;}
    const std::string id=definition.Get("id").AsString();
    if(!SafePhysicsToken(id,128)){if(error)*error="physics v0.3 definition requires a safe id";return false;}
    std::string serialized;
    try{serialized=definition.ToJson();}catch(...){if(error)*error="physics v0.3 definition could not be serialized";return false;}
    if(serialized.size()>64u*1024u){if(error)*error="physics v0.3 definition exceeds 64 KiB";return false;}
    auto& bucket=definitions[category];
    if(bucket.size()>=4096 && !bucket.count(id)){if(error)*error="physics v0.3 category capacity reached";return false;}
    bucket[id]=definition;
    if(error)error->clear();
    return true;
}
const VekValue* PhysicsDefinitionRegistry::Find(const std::string& category,const std::string& id)const{
    auto c=definitions.find(category);if(c==definitions.end())return nullptr;
    auto d=c->second.find(id);return d==c->second.end()?nullptr:&d->second;
}
std::size_t PhysicsDefinitionRegistry::Count(const std::string& category)const{auto it=definitions.find(category);return it==definitions.end()?0:it->second.size();}
void PhysicsDefinitionRegistry::Clear(){definitions.clear();}
void PhysicsDefinitionRegistry::RegisterNatives(VekScriptEngine& engine){
    // Stable generic names for 2.7+, plus v0.2 aliases for source compatibility.
    auto version=[](const std::vector<VekValue>&){return VekValue(std::string(ApiVersion));};
    auto reg=[this](const std::vector<VekValue>& a){if(a.size()<2)return VekValue(false);std::string error;return VekValue(RegisterDefinition(a[0].AsString(),a[1],&error));};
    auto exists=[this](const std::vector<VekValue>& a){return VekValue(a.size()>=2&&Exists(a[0].AsString(),a[1].AsString()));};
    auto count=[this](const std::vector<VekValue>& a){return VekValue((double)(a.empty()?0:Count(a[0].AsString())));};
    engine.RegisterNative("physics_version",version);
    engine.RegisterNative("physics_definition_register",reg);
    engine.RegisterNative("physics_definition_exists",exists);
    engine.RegisterNative("physics_definition_count",count);
    engine.RegisterNative("physics_v03_version",[](const std::vector<VekValue>&){return VekValue("0.3");});
    engine.RegisterNative("physics_v03_definition_register",reg);
    engine.RegisterNative("physics_v03_definition_exists",exists);
    engine.RegisterNative("physics_v03_definition_count",count);
    engine.RegisterNative("physics_v02_version",[](const std::vector<VekValue>&){return VekValue("0.2");});
    engine.RegisterNative("physics_v02_definition_register",reg);
    engine.RegisterNative("physics_v02_definition_exists",exists);
    engine.RegisterNative("physics_v02_definition_count",count);
}

bool SecondaryMotionProfileRegistry::RegisterProfile(const SecondaryMotionProfile& profile){
    if(profile.id.empty()) return false;
    auto p=Sanitize(profile);
    profiles[p.id]=std::move(p);
    return true;
}

bool SecondaryMotionProfileRegistry::RegisterProfileValue(const VekValue& value,std::string* error){
    if(!value.IsMap()){
        if(error)*error="secondary_motion_profile_register expects a map";
        return false;
    }
    SecondaryMotionProfile p;
    p.id=value.Get("id").AsString();
    p.gravity=(float)value.Get("gravity").AsNumber(p.gravity);
    p.damping=(float)value.Get("damping").AsNumber(p.damping);
    p.stiffness=(float)value.Get("stiffness").AsNumber(p.stiffness);
    p.airDrag=(float)value.Get("air_drag").AsNumber(p.airDrag);
    p.inertia=(float)value.Get("inertia").AsNumber(p.inertia);
    p.windInfluence=(float)value.Get("wind_influence").AsNumber(p.windInfluence);
    p.maxDt=(float)value.Get("max_dt").AsNumber(p.maxDt);
    p.constraintIterations=(int)value.Get("constraint_iterations").AsNumber(p.constraintIterations);
    p.maxSubsteps=(int)value.Get("max_substeps").AsNumber(p.maxSubsteps);
    p.collisionPadding=(float)value.Get("collision_padding").AsNumber(p.collisionPadding);
    p.maxSpeed=(float)value.Get("max_speed").AsNumber(p.maxSpeed);
    if(p.id.empty()){
        if(error)*error="secondary-motion profile id is required";
        return false;
    }
    bool ok=RegisterProfile(p);
    if(error) error->clear();
    return ok;
}

const SecondaryMotionProfile* SecondaryMotionProfileRegistry::Find(const std::string& id) const{
    auto it=profiles.find(id);
    return it==profiles.end()?nullptr:&it->second;
}
void SecondaryMotionProfileRegistry::Clear(){profiles.clear();}
std::size_t SecondaryMotionProfileRegistry::Size()const{return profiles.size();}
void SecondaryMotionProfileRegistry::RegisterNatives(VekScriptEngine& engine){
    engine.RegisterNative("secondary_motion_profile_register",[this](const std::vector<VekValue>& a){
        std::string error; return VekValue(!a.empty()&&RegisterProfileValue(a[0],&error));
    });
    engine.RegisterNative("secondary_motion_profile_exists",[this](const std::vector<VekValue>& a){
        return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);
    });
    engine.RegisterNative("secondary_motion_profile_count",[this](const std::vector<VekValue>&){
        return VekValue((double)Size());
    });
}

void SpringChain3D::Configure(const SpringChainSettings& settings){
    const int oldSegments=settings_.segments;
    const float oldLength=settings_.segmentLength;
    settings_=settings;
    settings_.segments=std::clamp(settings_.segments,1,32);
    settings_.segmentLength=std::clamp(FiniteOr(settings_.segmentLength,0.08f),0.002f,4.0f);
    settings_.motion=Sanitize(settings_.motion);
    if(initialized_ && ((int)particles_.size()!=settings_.segments+1 || oldSegments!=settings_.segments || std::fabs(oldLength-settings_.segmentLength)>1e-5f)) Clear();
}

void SpringChain3D::Reset(PhysicsVec3 root,PhysicsVec3 restDirection){
    PhysicsVec3 dir=NormalizeOr(restDirection,{0,-1,0});
    particles_.assign((std::size_t)settings_.segments+1,{});
    restOffsets_.assign((std::size_t)settings_.segments+1,{});
    for(int i=0;i<=settings_.segments;++i){
        PhysicsVec3 offset=Mul(dir,settings_.segmentLength*(float)i);
        restOffsets_[(std::size_t)i]=offset;
        PhysicsVec3 p=Add(root,offset);
        particles_[(std::size_t)i].position=p;
        particles_[(std::size_t)i].previousPosition=p;
    }
    previousRoot_=root;
    initialized_=true;
}

void SpringChain3D::Clear(){
    particles_.clear();restOffsets_.clear();initialized_=false;previousRoot_={};
}

void SpringChain3D::Step(PhysicsVec3 root,PhysicsVec3 externalAcceleration,PhysicsVec3 sphereCenter,float sphereRadius,float dt){
    if(!std::isfinite(dt)||dt<=0.0f)return;
    if(!initialized_)Reset(root);
    if((int)particles_.size()!=settings_.segments+1)Reset(root);
    float remaining=std::clamp(dt,0.0f,settings_.motion.maxDt*(float)settings_.motion.maxSubsteps);
    int steps=std::max(1,std::min(settings_.motion.maxSubsteps,(int)std::ceil(remaining/settings_.motion.maxDt)));
    float h=remaining/(float)steps;
    const PhysicsVec3 startRoot=previousRoot_;
    for(int i=0;i<steps;++i){
        const float t=(float)(i+1)/(float)steps;
        PhysicsVec3 subRoot=Add(startRoot,Mul(Sub(root,startRoot),t));
        StepSubstep(subRoot,externalAcceleration,sphereCenter,sphereRadius,h);
        previousRoot_=subRoot;
    }
    previousRoot_=root;
}

void SpringChain3D::StepSubstep(PhysicsVec3 root,PhysicsVec3 externalAcceleration,PhysicsVec3 sphereCenter,float sphereRadius,float dt){
    const auto& m=settings_.motion;
    PhysicsVec3 rootDelta=Sub(root,previousRoot_);
    PhysicsVec3 inertialAccel = Mul(rootDelta, -m.inertia/std::max(dt*dt,1e-6f));
    inertialAccel=ClampMagnitude(inertialAccel,m.maxSpeed/std::max(dt,1e-4f));
    PhysicsVec3 accel=Add(externalAcceleration,inertialAccel);
    accel.y-=m.gravity;

    float retained=std::pow(std::clamp(m.damping*(1.0f-m.airDrag),0.0f,0.9995f),std::max(0.01f,dt*60.0f));
    particles_[0].position=root;
    particles_[0].previousPosition=root;

    for(std::size_t i=1;i<particles_.size();++i){
        auto& p=particles_[i];
        PhysicsVec3 velocity=Mul(Sub(p.position,p.previousPosition),retained);
        velocity=ClampMagnitude(velocity,m.maxSpeed*dt);
        p.previousPosition=p.position;

        PhysicsVec3 target=Add(root,restOffsets_[i]);
        PhysicsVec3 spring=Mul(Sub(target,p.position),m.stiffness*60.0f);
        PhysicsVec3 total=Add(accel,spring);
        p.position=Add(p.position,Add(velocity,Mul(total,dt*dt)));
    }
    SolveConstraints(root,sphereCenter,sphereRadius);
}

void SpringChain3D::SolveConstraints(PhysicsVec3 root,PhysicsVec3 sphereCenter,float sphereRadius){
    const float wanted=settings_.segmentLength;
    const float collisionRadius=std::max(0.0f,sphereRadius+settings_.motion.collisionPadding);
    for(int iter=0;iter<settings_.motion.constraintIterations;++iter){
        particles_[0].position=root;
        for(std::size_t i=1;i<particles_.size();++i){
            PhysicsVec3 delta=Sub(particles_[i].position,particles_[i-1].position);
            float dist=Length(delta);
            PhysicsVec3 dir=dist>1e-6f?Mul(delta,1.0f/dist):NormalizeOr(restOffsets_[i],{0,-1,0});
            PhysicsVec3 correction=Mul(dir,dist-wanted);
            if(i==1){
                particles_[i].position=Sub(particles_[i].position,correction);
            }else{
                particles_[i-1].position=Add(particles_[i-1].position,Mul(correction,0.5f));
                particles_[i].position=Sub(particles_[i].position,Mul(correction,0.5f));
            }

            if(collisionRadius>0.0f){
                PhysicsVec3 fromCenter=Sub(particles_[i].position,sphereCenter);
                float d=Length(fromCenter);
                if(d<collisionRadius){
                    PhysicsVec3 normal=NormalizeOr(fromCenter,NormalizeOr(restOffsets_[i],{0,1,0}));
                    particles_[i].position=Add(sphereCenter,Mul(normal,collisionRadius));
                }
            }
        }
    }
    particles_[0].position=root;
}


VehicleDynamicsOutput VehicleDynamicsModel::Step(VehicleDynamicsState& state,const VehicleDynamicsConfig& c0,const VehicleDynamicsInput& i0,float dt){
    VehicleDynamicsConfig c=c0;VehicleDynamicsInput in=i0;VehicleDynamicsOutput out;
    dt=std::clamp(FiniteOr(dt,0.0f),0.0f,0.05f);if(dt<=0.0f)return out;
    c.mass=std::clamp(FiniteOr(c.mass,1200.0f),50.0f,100000.0f);
    c.wheelbase=std::clamp(FiniteOr(c.wheelbase,2.6f),0.4f,20.0f);c.yawInertia=std::clamp(FiniteOr(c.yawInertia,1800.0f),10.0f,1.0e7f);
    c.engineForce=std::max(0.0f,FiniteOr(c.engineForce,8500.0f));c.brakeForce=std::max(0.0f,FiniteOr(c.brakeForce,14000.0f));
    c.handbrakeForce=std::max(0.0f,FiniteOr(c.handbrakeForce,18000.0f));c.maxSteerDegrees=std::clamp(FiniteOr(c.maxSteerDegrees,35.0f),1.0f,75.0f);
    c.tireGrip=std::clamp(FiniteOr(c.tireGrip,1.0f),0.05f,4.0f);c.corneringStiffness=std::clamp(FiniteOr(c.corneringStiffness,7.5f),0.1f,50.0f);
    c.rollingResistance=std::clamp(FiniteOr(c.rollingResistance,0.015f),0.0f,0.25f);c.dragCoefficient=std::clamp(FiniteOr(c.dragCoefficient,0.32f),0.0f,3.0f);
    c.frontalArea=std::clamp(FiniteOr(c.frontalArea,2.2f),0.05f,100.0f);c.maxSpeed=std::clamp(FiniteOr(c.maxSpeed,80.0f),1.0f,400.0f);
    in.throttle=std::clamp(FiniteOr(in.throttle,0.0f),-1.0f,1.0f);in.brake=std::clamp(FiniteOr(in.brake,0.0f),0.0f,1.0f);
    in.steer=std::clamp(FiniteOr(in.steer,0.0f),-1.0f,1.0f);in.handbrake=std::clamp(FiniteOr(in.handbrake,0.0f),0.0f,1.0f);
    const float speed=std::fabs(state.longitudinalSpeed);const float steerTarget=in.steer*c.maxSteerDegrees;
    const float steerAlpha=1.0f-std::exp(-std::max(0.1f,c.steeringResponse)*dt);state.steerAngleDegrees+=(steerTarget-state.steerAngleDegrees)*steerAlpha;
    const float normalLoad=c.mass*9.81f;const float aeroDown=0.5f*1.225f*std::max(0.0f,c.downforceCoefficient)*c.frontalArea*speed*speed;
    out.downforce=aeroDown;out.tractionLimit=(normalLoad+aeroDown)*c.tireGrip;
    const float driveForce=in.throttle*c.engineForce;const float brakeSign=state.longitudinalSpeed>=0.0f?1.0f:-1.0f;
    float braking=in.brake*c.brakeForce*brakeSign+in.handbrake*c.handbrakeForce*brakeSign;
    const float drag=0.5f*1.225f*c.dragCoefficient*c.frontalArea*speed*speed*brakeSign;
    const float rolling=c.rollingResistance*normalLoad*brakeSign;
    float longitudinalForce=driveForce-braking-drag-rolling;longitudinalForce=std::clamp(longitudinalForce,-out.tractionLimit,out.tractionLimit);
    out.longitudinalAcceleration=longitudinalForce/c.mass;state.longitudinalSpeed+=out.longitudinalAcceleration*dt;
    if(std::fabs(in.throttle)<0.01f&&in.brake<0.01f&&std::fabs(state.longitudinalSpeed)<0.03f)state.longitudinalSpeed=0.0f;
    state.longitudinalSpeed=std::clamp(state.longitudinalSpeed,-c.maxSpeed*0.35f,c.maxSpeed);
    const float steerRad=state.steerAngleDegrees*3.14159265358979323846f/180.0f;
    const float desiredYaw=(std::fabs(c.wheelbase)>1e-4f)?(state.longitudinalSpeed/c.wheelbase)*std::tan(steerRad):0.0f;
    const float lateralDamping=std::min(out.tractionLimit/c.mass,std::max(0.0f,c.corneringStiffness)*std::fabs(state.longitudinalSpeed));
    out.lateralAcceleration=-state.lateralSpeed*lateralDamping;state.lateralSpeed+=out.lateralAcceleration*dt;
    const float yawTorque=(desiredYaw-state.yawRate)*c.yawInertia*4.0f-state.yawRate*c.yawInertia*0.8f;out.yawAcceleration=yawTorque/c.yawInertia;state.yawRate+=out.yawAcceleration*dt;
    const float maxYaw=std::max(0.5f,std::fabs(state.longitudinalSpeed)/std::max(0.4f,c.wheelbase)*1.5f);state.yawRate=std::clamp(state.yawRate,-maxYaw,maxYaw);
    out.longitudinalSlip=std::clamp(std::fabs(driveForce)/std::max(1.0f,out.tractionLimit),0.0f,2.0f);
    out.lateralSlip=std::clamp(std::fabs(state.lateralSpeed)/std::max(1.0f,std::fabs(state.longitudinalSpeed)),0.0f,2.0f);
    const float pitchTarget=std::clamp(-out.longitudinalAcceleration*0.035f,-0.12f,0.12f);const float rollTarget=std::clamp(-state.yawRate*state.longitudinalSpeed*0.012f,-0.18f,0.18f);
    state.bodyPitch+=(pitchTarget-state.bodyPitch)*(1.0f-std::exp(-7.0f*dt));state.bodyRoll+=(rollTarget-state.bodyRoll)*(1.0f-std::exp(-7.0f*dt));
    state.suspensionCompression+=(std::clamp((std::fabs(out.longitudinalAcceleration)+std::fabs(out.lateralAcceleration))*0.02f,0.0f,0.18f)-state.suspensionCompression)*(1.0f-std::exp(-9.0f*dt));
    state.engineRpm=std::clamp(850.0f+std::fabs(state.longitudinalSpeed)*110.0f+std::fabs(in.throttle)*2200.0f,800.0f,7800.0f);
    return out;
}

} // namespace vek
