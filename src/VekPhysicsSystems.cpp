#include <vek/VekPhysicsSystems.h>

#include <algorithm>
#include <cmath>
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

} // namespace vek
