#include "VehicleBuilder.h"
#include "VekVehicleEditorRules.h"
#include "VekVehicleEditorGuiSystem.h"
#include "VekGuiTextRenderer.h"
#include "PartPresentationRenderer.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cfloat>
#include <limits>
#include <sstream>

namespace {
float Clamp01(float v){return std::clamp(v,0.0f,1.0f);} 
std::string Lower(std::string s){for(char&c:s)c=(char)std::tolower((unsigned char)c);return s;}
Vector3 ToVector3(const vek::VekVec3&v){return{v.x,v.y,v.z};}
Vector3 RotateEuler(Vector3 p,float pitch,float yaw,float roll){
    Matrix m=MatrixRotateXYZ({pitch*DEG2RAD,yaw*DEG2RAD,roll*DEG2RAD});
    return Vector3Transform(p,m);
}
Color PlacementColor(vek::PlacementResult r){
    if(r==vek::PlacementResult::Allowed)return Color{66,225,132,205};
    if(r==vek::PlacementResult::Warning)return Color{245,177,55,210};
    return Color{238,68,75,210};
}
PartType LegacyTypeFor(const std::string&id){
    if(id=="frame.chassis_basic")return PartType::Chassis;
    if(id=="wheel.road")return PartType::Wheel;
    if(id=="engine.small_petrol")return PartType::Engine;
    if(id=="seat.driver")return PartType::Seat;
    if(id=="frame.block")return PartType::Frame;
    if(id=="frame.beam_2m")return PartType::Beam;
    if(id=="frame.plate")return PartType::Plate;
    if(id=="frame.roll_cage")return PartType::RollCage;
    if(id=="wing.medium")return PartType::Wing;
    if(id=="wheel.offroad_large")return PartType::OffroadWheel;
    if(id=="track.medium")return PartType::Track;
    if(id=="propeller.medium")return PartType::Propeller;
    if(id=="rotor.helicopter")return PartType::Rotor;
    if(id=="experimental.thruster")return PartType::Thruster;
    if(id=="engine.turbo_diesel")return PartType::DieselEngine;
    if(id=="motor.electric_medium")return PartType::ElectricMotor;
    if(id=="tank.petrol_80")return PartType::FuelTank;
    if(id=="battery.120")return PartType::Battery;
    if(id=="suspension.basic")return PartType::Suspension;
    if(id=="steering.rack")return PartType::Steering;
    if(id=="light.work")return PartType::Light;
    if(id=="cargo.box_medium")return PartType::CargoBox;
    if(id=="tow.hook")return PartType::TowHook;
    if(id=="winch.medium")return PartType::Winch;
    if(id=="engine.jet_small")return PartType::JetEngine;
    if(id=="experimental.reactor")return PartType::SandboxReactor;
    return PartType::Dynamic;
}
float PropNumber(const vek::PartComponentDefinition*c,const std::string&k,float fallback=0){
    if(!c)return fallback;auto it=c->properties.find(k);return it==c->properties.end()?fallback:(float)it->second.AsNumber(fallback);
}
bool PropBool(const vek::PartComponentDefinition*c,const std::string&k,bool fallback=false){
    if(!c)return fallback;auto it=c->properties.find(k);return it==c->properties.end()?fallback:it->second.AsBool(fallback);
}
BoundingBox PartBounds(const VehiclePart&p){
    // Conservative AABB of the oriented part. This keeps intersection checks fast
    // while the attachment system handles precise connection intent.
    Vector3 h=Vector3Scale(p.size,0.5f);Vector3 mn{FLT_MAX,FLT_MAX,FLT_MAX},mx{-FLT_MAX,-FLT_MAX,-FLT_MAX};
    for(int xi=-1;xi<=1;xi+=2)for(int yi=-1;yi<=1;yi+=2)for(int zi=-1;zi<=1;zi+=2){
        Vector3 q{h.x*xi,h.y*yi,h.z*zi};q=RotateEuler(q,p.pitch,p.yaw,p.roll);q=Vector3Add(q,p.localPosition);
        mn.x=std::min(mn.x,q.x);mn.y=std::min(mn.y,q.y);mn.z=std::min(mn.z,q.z);
        mx.x=std::max(mx.x,q.x);mx.y=std::max(mx.y,q.y);mx.z=std::max(mx.z,q.z);
    }return{mn,mx};
}
bool AabbOverlap(const BoundingBox&a,const BoundingBox&b,float margin=0.015f){
    return a.min.x<b.max.x-margin&&a.max.x>b.min.x+margin&&a.min.y<b.max.y-margin&&a.max.y>b.min.y+margin&&a.min.z<b.max.z-margin&&a.max.z>b.min.z+margin;
}
void DrawRotatedBox(Vector3 p,Vector3 s,float pitch,float yaw,float roll,Color color,bool wires){
    rlPushMatrix();rlTranslatef(p.x,p.y,p.z);rlRotatef(yaw,0,1,0);rlRotatef(pitch,1,0,0);rlRotatef(roll,0,0,1);
    if(wires)DrawCubeWires({0,0,0},s.x,s.y,s.z,color);else DrawCube({0,0,0},s.x,s.y,s.z,color);rlPopMatrix();
}
}

void VehicleBuilder::Configure(vek::GameMode m,VekVehicleEditorRules*r,EconomyState*e,const vek::PartRegistry*pr,const vek::HangarBuildArea*a,VekVehicleEditorGuiSystem*g){
    mode=m;rules=r;economy=e;registry=pr;area=a;gui=g;gridIndex=2;freePlacement=false;symmetryX=false;symmetryZ=false;SelectCategory("Structural");
    auto steps=rules?rules->GridSteps():std::vector<float>{1,0.5f,0.25f,0.1f,0.05f};if(!steps.empty())gridIndex=std::min<std::size_t>(2,steps.size()-1);
}
void VehicleBuilder::Toggle(){active=!active;status=active?"Engineering editor active. Select or place a VEK-defined component.":"Vehicle editor closed.";if(!active){selectedPlaced=-1;pendingCommand.clear();}}
int VehicleBuilder::ProgressionLevel()const{return mode==vek::GameMode::Sandbox?999:std::max(1,1+(economy?economy->reputation.Get():0));}
const vek::PartDefinition*VehicleBuilder::SelectedDefinition()const{return registry?registry->FindPart(selectedPartId):nullptr;}
std::vector<const vek::PartDefinition*>VehicleBuilder::VisibleParts()const{
    if(!registry)return{};
    if(category=="Favorites"){auto all=registry->Search(search);std::vector<const vek::PartDefinition*> out;for(auto*p:all)if(favorites.count(p->id))out.push_back(p);return out;}
    if(category=="Recent"){std::vector<const vek::PartDefinition*> out;for(auto&id:recent){auto*p=registry->FindPart(id);if(!p)continue;if(!search.empty()){std::string hay=Lower(p->displayName+" "+p->id+" "+p->subcategory);if(hay.find(Lower(search))==std::string::npos)continue;}out.push_back(p);}return out;}
    return registry->Search(search,category);
}
bool VehicleBuilder::IsUnlocked(const vek::PartDefinition&d)const{return rules?rules->PartUnlocked(mode,ProgressionLevel(),d.unlockLevel,d.sandboxOnly):mode==vek::GameMode::Sandbox||(!d.sandboxOnly&&ProgressionLevel()>=d.unlockLevel);}
void VehicleBuilder::SelectCategory(const std::string&c){category=c;auto v=VisibleParts();if(v.empty())return;auto it=std::find_if(v.begin(),v.end(),[&](auto*p){return IsUnlocked(*p);});selectedPartId=(it!=v.end()?(*it)->id:v.front()->id);}
void VehicleBuilder::CyclePart(int direction){auto v=VisibleParts();if(v.empty())return;int index=0;for(int i=0;i<(int)v.size();++i)if(v[i]->id==selectedPartId){index=i;break;}for(int n=0;n<(int)v.size();++n){index=(index+direction+(int)v.size())%(int)v.size();if(IsUnlocked(*v[index])){selectedPartId=v[index]->id;return;}}}
float VehicleBuilder::GridStep()const{auto s=rules?rules->GridSteps():std::vector<float>{1,0.5f,0.25f,0.1f,0.05f};if(s.empty())return 0.25f;return s[std::min(gridIndex,s.size()-1)];}
bool VehicleBuilder::PointInEditorUI(Vector2 m)const{return gui?gui->MouseOverUI(m):(m.x<460.0f);}

VehiclePart VehicleBuilder::MakePart(const vek::PartDefinition&d,Vector3 pos)const{
    VehiclePart p;p.type=LegacyTypeFor(d.id);p.legacyType=p.type==PartType::Dynamic?-1:(int)p.type;p.partId=d.id;p.displayName=d.displayName;p.category=d.category;p.visual=d.visual;p.icon=d.presentation.icon;p.viewModel=d.presentation.viewModel;p.worldModel=d.presentation.worldModel;p.material=d.presentation.material;p.viewScale=ToVector3(d.presentation.viewScale);p.viewRotation=ToVector3(d.presentation.viewRotation);p.viewOffset=ToVector3(d.presentation.viewOffset);p.allowTint=d.presentation.allowTint;p.localPosition=pos;p.size=ToVector3(d.size);p.yaw=rotation;p.pitch=pitch;p.roll=roll;p.mass=d.mass;p.cost=rules?rules->PartCost(mode,d.price):(mode==vek::GameMode::Sandbox?0:d.price);p.durability=d.durability;
    auto engine=d.Component("Engine"),motor=d.Component("Motor"),wheel=d.Component("Wheel"),fuel=d.Component("FuelTank"),battery=d.Component("Battery"),cargo=d.Component("Cargo"),wing=d.Component("Wing"),rotor=d.Component("Rotor"),thruster=d.Component("Thruster"),buoy=d.Component("Buoyancy"),seat=d.Component("Seat"),prop=d.Component("Propeller");
    p.power=PropNumber(engine,"power",PropNumber(motor,"power",0));p.torque=PropNumber(engine,"torque",PropNumber(motor,"torque",0));p.traction=PropNumber(wheel,"traction",0);p.fuelCapacity=PropNumber(fuel,"capacity",0);p.batteryCapacity=PropNumber(battery,"capacity",0);p.cargoCapacity=PropNumber(cargo,"capacity",0);p.wingArea=PropNumber(wing,"area",0);p.liftEstimate=PropNumber(rotor,"lift",p.wingArea*9.81f*120.0f);p.drag=PropNumber(wing,"drag",PropNumber(rotor,"drag",PropNumber(buoy,"drag",0)));p.thrust=PropNumber(thruster,"thrust",PropNumber(prop,"thrust",0));p.buoyancy=PropNumber(buoy,"volume",0)*1000.0f*9.81f;
    p.isWheel=wheel!=nullptr||d.HasTag("wheel")||d.HasTag("track");p.isStructural=d.HasTag("structural")||d.Component("Mass")!=nullptr&&d.category=="Structural";p.isPowered=engine||motor||thruster||d.HasTag("powered");p.isSeat=seat!=nullptr||d.HasTag("seat");p.isMovement=p.isWheel||rotor||prop||thruster||d.HasTag("movement");
    if(d.id=="experimental.reactor")p.power=std::max(p.power,150000.0f);
    return p;
}
vek::VehicleBuildCounts VehicleBuilder::Counts()const{vek::VehicleBuildCounts c;for(auto&p:loose){c.totalParts++;c.mass+=p.mass;c.power+=p.power+p.thrust;c.estimatedCost+=p.cost;if(PartIsStructural(p))c.structuralParts++;if(PartIsWheel(p))c.wheels++;if(PartIsPowered(p))c.engines++;if(p.isSeat)c.seats++;if(PartIsMovement(p))c.movementParts++;}return c;}
VehicleEngineeringStats VehicleBuilder::Stats()const{
    VehicleEngineeringStats s;if(loose.empty())return s;Vector3 weighted{0,0,0};float minx=FLT_MAX,miny=FLT_MAX,minz=FLT_MAX,maxx=-FLT_MAX,maxy=-FLT_MAX,maxz=-FLT_MAX;float traction=0;int tractionCount=0;
    for(auto&p:loose){s.partCount++;s.mass+=p.mass;s.power+=p.power;s.torque+=p.torque;s.fuelCapacity+=p.fuelCapacity;s.batteryCapacity+=p.batteryCapacity;s.cargoCapacity+=p.cargoCapacity;s.wingArea+=p.wingArea;s.liftEstimate+=p.liftEstimate;s.thrust+=p.thrust;s.buoyancy+=p.buoyancy;s.buildCost+=p.cost;if(PartIsWheel(p)){s.wheelCount++;if(p.traction>0){traction+=p.traction;tractionCount++;}}weighted=Vector3Add(weighted,Vector3Scale(p.localPosition,std::max(1.0f,p.mass)));auto b=PartBounds(p);minx=std::min(minx,b.min.x);miny=std::min(miny,b.min.y);minz=std::min(minz,b.min.z);maxx=std::max(maxx,b.max.x);maxy=std::max(maxy,b.max.y);maxz=std::max(maxz,b.max.z);}
    s.centerOfMass=Vector3Scale(weighted,1.0f/std::max(1.0f,s.mass));s.size=Vector3{maxx-minx,maxy-miny,maxz-minz};s.powerToWeight=(s.power+s.thrust)/std::max(1.0f,s.mass);float avgTraction=tractionCount?traction/tractionCount:0.7f;s.estimatedAcceleration=std::clamp(s.powerToWeight*0.055f*avgTraction,0.0f,20.0f);s.estimatedTopSpeed=std::clamp(35.0f+std::sqrt(std::max(0.0f,s.power+s.thrust))*0.42f,25.0f,420.0f);float energy=s.fuelCapacity*8.9f+s.batteryCapacity;s.estimatedRange=energy>0?std::clamp(energy*2.8f/std::max(0.4f,(s.power/22000.0f)+0.3f),2.0f,1200.0f):0;float footprint=std::max(0.1f,s.size.x*s.size.z);s.stability=Clamp01((footprint/(std::max(0.5f,s.size.y)*std::max(1.0f,s.mass/300.0f)))*0.16f);return s;
}
Vector3 VehicleBuilder::VehicleCenter()const{if(loose.empty())return area?ToVector3(area->center):Vector3{0,0,0};Vector3 c{0,0,0};for(auto&p:loose)c=Vector3Add(c,p.localPosition);return Vector3Scale(c,1.0f/(float)loose.size());}
Vector3 VehicleBuilder::VehicleSize()const{return Stats().size;}
Vector3 VehicleBuilder::SelectedOrVehicleCenter()const{return selectedPlaced>=0&&selectedPlaced<(int)loose.size()?loose[selectedPlaced].localPosition:VehicleCenter();}
bool VehicleBuilder::ConsumeConstructRequested(){bool r=constructRequested;constructRequested=false;return r;}
std::string VehicleBuilder::ConsumeCameraCommand(){if(pendingCommand.rfind("cam_",0)!=0)return{};std::string r=pendingCommand;pendingCommand.clear();return r;}

void VehicleBuilder::Record(EditorAction a){if(historyCursor<history.size())history.erase(history.begin()+historyCursor,history.end());history.push_back(std::move(a));historyCursor=history.size();if(history.size()>512){history.erase(history.begin());historyCursor=history.size();}}
void VehicleBuilder::ApplyUndo(const EditorAction&a){if(a.type==ActionType::Place){if(a.index>=0&&a.index<(int)loose.size())loose.erase(loose.begin()+a.index);}else if(a.type==ActionType::Delete){int idx=std::clamp(a.index,0,(int)loose.size());loose.insert(loose.begin()+idx,a.before);}else if(a.index>=0&&a.index<(int)loose.size())loose[a.index]=a.before;selectedPlaced=-1;}
void VehicleBuilder::ApplyRedo(const EditorAction&a){if(a.type==ActionType::Place){int idx=std::clamp(a.index,0,(int)loose.size());loose.insert(loose.begin()+idx,a.after);}else if(a.type==ActionType::Delete){if(a.index>=0&&a.index<(int)loose.size())loose.erase(loose.begin()+a.index);}else if(a.index>=0&&a.index<(int)loose.size())loose[a.index]=a.after;selectedPlaced=-1;}
void VehicleBuilder::Undo(){if(historyCursor==0)return;historyCursor--;ApplyUndo(history[historyCursor]);status="Undo.";}
void VehicleBuilder::Redo(){if(historyCursor>=history.size())return;ApplyRedo(history[historyCursor]);historyCursor++;status="Redo.";}
void VehicleBuilder::DeleteSelected(){if(selectedPlaced<0||selectedPlaced>=(int)loose.size())return;EditorAction a{ActionType::Delete,selectedPlaced,loose[selectedPlaced],{}};Record(a);loose.erase(loose.begin()+selectedPlaced);selectedPlaced=-1;status="Part deleted.";}
void VehicleBuilder::DuplicateSelected(){if(selectedPlaced<0||selectedPlaced>=(int)loose.size())return;VehiclePart p=loose[selectedPlaced];p.localPosition.x+=GridStep();int idx=(int)loose.size();loose.push_back(p);Record({ActionType::Place,idx,{},p});selectedPlaced=idx;status="Part duplicated.";}
void VehicleBuilder::MirrorSelected(){if(selectedPlaced<0||selectedPlaced>=(int)loose.size())return;VehiclePart p=loose[selectedPlaced];p.localPosition.x=-p.localPosition.x;p.yaw=-p.yaw;int idx=(int)loose.size();loose.push_back(p);Record({ActionType::Place,idx,{},p});selectedPlaced=idx;status="Mirrored copy created across X axis.";}
void VehicleBuilder::MoveSelected(Vector3 d){if(selectedPlaced<0||selectedPlaced>=(int)loose.size())return;VehiclePart before=loose[selectedPlaced];loose[selectedPlaced].localPosition=Vector3Add(loose[selectedPlaced].localPosition,d);Record({ActionType::Modify,selectedPlaced,before,loose[selectedPlaced]});}
void VehicleBuilder::RotateSelected(float d){if(selectedPlaced<0||selectedPlaced>=(int)loose.size())return;VehiclePart before=loose[selectedPlaced];loose[selectedPlaced].yaw=fmodf(loose[selectedPlaced].yaw+d+360.0f,360.0f);Record({ActionType::Modify,selectedPlaced,before,loose[selectedPlaced]});}
void VehicleBuilder::PaintSelected(){if(selectedPlaced<0||selectedPlaced>=(int)loose.size())return;VehiclePart before=loose[selectedPlaced];static const unsigned char palette[][3]={{150,160,165},{42,135,205},{230,75,55},{235,185,45},{72,176,112},{172,92,205},{235,235,235},{45,45,48}};int best=0;for(int i=0;i<8;i++)if(loose[selectedPlaced].paintR==palette[i][0]&&loose[selectedPlaced].paintG==palette[i][1]&&loose[selectedPlaced].paintB==palette[i][2]){best=i;break;}best=(best+1)%8;loose[selectedPlaced].paintR=palette[best][0];loose[selectedPlaced].paintG=palette[best][1];loose[selectedPlaced].paintB=palette[best][2];Record({ActionType::Modify,selectedPlaced,before,loose[selectedPlaced]});status="Paint changed.";}

void VehicleBuilder::SelectPlacedFromRay(Ray ray){float best=std::numeric_limits<float>::max();int hit=-1;for(int i=0;i<(int)loose.size();++i){RayCollision c=GetRayCollisionBox(ray,PartBounds(loose[i]));if(c.hit&&c.distance<best){best=c.distance;hit=i;}}if(hit>=0){selectedPlaced=hit;selectedPartId=loose[hit].partId;category=loose[hit].category;status="Selected: "+loose[hit].displayName;}}

bool VehicleBuilder::FindAttachmentSnap(const vek::PartDefinition&ghost,Vector3&position,bool&compatible,float&supportDistance)const{
    supportDistance=std::numeric_limits<float>::max();compatible=false;if(loose.empty()||ghost.attachments.empty()||!registry)return false;bool found=false;Vector3 best=position;float snapRadius=std::max(0.45f,GridStep()*2.5f);
    for(const auto&g:ghost.attachments){Vector3 gLocal=RotateEuler(ToVector3(g.position),pitch,rotation,roll);Vector3 gWorld=Vector3Add(position,gLocal);for(const auto&p:loose){auto*pd=registry->FindPart(p.partId);if(!pd)continue;for(const auto&n:pd->attachments){Vector3 nWorld=Vector3Add(p.localPosition,RotateEuler(ToVector3(n.position),p.pitch,p.yaw,p.roll));float dist=Vector3Distance(gWorld,nWorld);supportDistance=std::min(supportDistance,dist);bool ok=rules?rules->AttachmentCompatible(g.type,n.type):(g.type==n.type);if(!ok){for(auto&s:g.compatibleTypes)if(s==n.type)ok=true;for(auto&s:n.compatibleTypes)if(s==g.type)ok=true;}if(ok&&dist<snapRadius){compatible=true;if(!found||dist<Vector3Distance(Vector3Add(best,gLocal),nWorld)){best=Vector3Add(position,Vector3Subtract(nWorld,gWorld));found=true;}}}}}
    if(supportDistance==std::numeric_limits<float>::max())supportDistance=999;position=best;return found;
}
bool VehicleBuilder::GhostIntersects(const vek::PartDefinition&d,Vector3 pos)const{VehiclePart g=MakePart(d,pos);BoundingBox gb=PartBounds(g);for(auto&p:loose)if(AabbOverlap(gb,PartBounds(p),0.025f))return true;return false;}
void VehicleBuilder::UpdateGhost(const Camera3D&camera){const auto*d=SelectedDefinition();if(!d){placement=vek::PlacementResult::Invalid;return;}Vector2 mouse=GetMousePosition();Ray ray=GetMouseRay(mouse,camera);float floorY=area?area->center.y:0.0f;if(fabsf(ray.direction.y)<0.0001f)return;float t=(floorY-ray.position.y)/ray.direction.y;if(t<=0)return;Vector3 h=Vector3Add(ray.position,Vector3Scale(ray.direction,t));if(!freePlacement){float step=GridStep();h.x=roundf(h.x/step)*step;h.y=roundf(h.y/step)*step;h.z=roundf(h.z/step)*step;}h.y+=std::max(0.05f,d->size.y*0.5f);bool compatible=false;float support=999;FindAttachmentSnap(*d,h,compatible,support);cursor=h;bool intersects=GhostIntersects(*d,h);int maxParts=rules?rules->MaxParts(mode):(area?area->maxParts:800);bool inside=true;if(area){Vector3 c=ToVector3(area->center);Vector3 s=ToVector3(area->size);inside=std::fabs(h.x-c.x)<=s.x*0.5f&&std::fabs(h.z-c.z)<=s.z*0.5f&&h.y>=c.y&&h.y<=c.y+area->maxBuildHeight;}if(rules)inside=inside&&rules->PlacementValid(mode,h.x-(area?area->center.x:0),h.z-(area?area->center.z:0),(int)loose.size());if(!inside||(int)loose.size()>=maxParts||!IsUnlocked(*d)){placement=vek::PlacementResult::Invalid;return;}placement=rules?rules->PlacementStatus(intersects,compatible,support):(intersects?vek::PlacementResult::Invalid:vek::PlacementResult::Allowed);}

void VehicleBuilder::ApplyPendingGuiActions(){if(!pendingSearch.empty()||search.empty())search=pendingSearch;if(!pendingCategory.empty())SelectCategory(pendingCategory);if(!pendingPart.empty()&&registry&&registry->FindPart(pendingPart)){selectedPartId=pendingPart;recent.erase(std::remove(recent.begin(),recent.end(),pendingPart),recent.end());recent.push_front(pendingPart);while(recent.size()>12)recent.pop_back();}if(!pendingCommand.empty()){if(pendingCommand=="toggle_com")showCOM=!showCOM;else if(pendingCommand=="construct_vehicle")constructRequested=true;else if(pendingCommand=="cam_front"||pendingCommand=="cam_side"||pendingCommand=="cam_top"||pendingCommand=="cam_focus"){ /* consumed by Game/editor camera */ } }pendingCategory.clear();pendingPart.clear();if(pendingCommand.rfind("cam_",0)!=0)pendingCommand.clear();}

void VehicleBuilder::Update(const Camera3D&camera){if(!active)return;ApplyPendingGuiActions();
    auto steps=rules?rules->GridSteps():std::vector<float>{1,0.5f,0.25f,0.1f,0.05f};
    if(IsKeyPressed(KEY_G)&&!steps.empty()){gridIndex=(gridIndex+1)%steps.size();status="Grid: "+std::to_string(GridStep())+" m";}
    if(IsKeyPressed(KEY_H)){bool allowed=rules?rules->FreePlacementAllowed(mode):mode==vek::GameMode::Sandbox;if(allowed){freePlacement=!freePlacement;status=freePlacement?"Free placement enabled.":"Grid snapping enabled.";}else status="Free placement is locked by Survival VEK policy.";}
    if(IsKeyPressed(KEY_X)){if(rules&&!rules->SymmetryAllowed(mode))status="Symmetry disabled by VEK policy.";else{symmetryX=!symmetryX;status=symmetryX?"X symmetry enabled.":"X symmetry disabled.";}}
    if(IsKeyPressed(KEY_Z))symmetryZ=!symmetryZ;
    if(IsKeyPressed(KEY_T)){localTransform=!localTransform;status=localTransform?"Transform gizmo: LOCAL":"Transform gizmo: WORLD";}
    if(IsKeyPressed(KEY_Y)){if(favorites.count(selectedPartId))favorites.erase(selectedPartId);else favorites.insert(selectedPartId);status=favorites.count(selectedPartId)?"Part added to favorites.":"Part removed from favorites.";}
    if(IsKeyPressed(KEY_R)&&selectedPlaced<0)rotation=fmodf(rotation+15.0f,360.0f);
    if((IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL))&&IsKeyPressed(KEY_Q))CyclePart(-1);if((IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL))&&IsKeyPressed(KEY_E))CyclePart(1);
    bool ctrl=IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL),shift=IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT),alt=IsKeyDown(KEY_LEFT_ALT)||IsKeyDown(KEY_RIGHT_ALT);
    if(ctrl&&IsKeyPressed(KEY_Z))Undo();if(ctrl&&IsKeyPressed(KEY_Y))Redo();if(ctrl&&IsKeyPressed(KEY_D))DuplicateSelected();if(IsKeyPressed(KEY_DELETE))DeleteSelected();if(IsKeyPressed(KEY_M))MirrorSelected();if(IsKeyPressed(KEY_P))PaintSelected();
    if(selectedPlaced>=0){float step=GridStep();auto moveAxis=[&](Vector3 d){if(localTransform)d=RotateEuler(d,0,loose[selectedPlaced].yaw,0);MoveSelected(d);};if(alt&&IsKeyPressed(KEY_LEFT))moveAxis({-step,0,0});if(alt&&IsKeyPressed(KEY_RIGHT))moveAxis({step,0,0});if(alt&&IsKeyPressed(KEY_UP))moveAxis({0,0,step});if(alt&&IsKeyPressed(KEY_DOWN))moveAxis({0,0,-step});if(IsKeyPressed(KEY_PAGE_UP))moveAxis({0,step,0});if(IsKeyPressed(KEY_PAGE_DOWN))moveAxis({0,-step,0});if(IsKeyPressed(KEY_R)&&!shift&&!ctrl)RotateSelected(15);if(shift&&IsKeyPressed(KEY_R)){VehiclePart before=loose[selectedPlaced];loose[selectedPlaced].pitch=fmodf(loose[selectedPlaced].pitch+15,360);Record({ActionType::Modify,selectedPlaced,before,loose[selectedPlaced]});}if(ctrl&&IsKeyPressed(KEY_R)){VehiclePart before=loose[selectedPlaced];loose[selectedPlaced].roll=fmodf(loose[selectedPlaced].roll+15,360);Record({ActionType::Modify,selectedPlaced,before,loose[selectedPlaced]});}}
    if(shift&&IsKeyPressed(KEY_F)||IsKeyPressed(KEY_ENTER))constructRequested=true;
    UpdateGhost(camera);
    Vector2 mouse=GetMousePosition();if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&!PointInEditorUI(mouse)){Ray r=GetMouseRay(mouse,camera);if(!shift){float best=std::numeric_limits<float>::max();int hit=-1;for(int i=0;i<(int)loose.size();++i){auto c=GetRayCollisionBox(r,PartBounds(loose[i]));if(c.hit&&c.distance<best){best=c.distance;hit=i;}}if(hit>=0){selectedPlaced=hit;selectedPartId=loose[hit].partId;category=loose[hit].category;status="Selected: "+loose[hit].displayName;return;}}
      if(placement!=vek::PlacementResult::Invalid){const auto*d=SelectedDefinition();if(d){VehiclePart p=MakePart(*d,cursor);int idx=(int)loose.size();loose.push_back(p);Record({ActionType::Place,idx,{},p});selectedPlaced=-1;if(symmetryX&&fabsf(cursor.x-(area?area->center.x:0))>0.05f){VehiclePart q=p;q.localPosition.x=2*(area?area->center.x:0)-q.localPosition.x;q.yaw=-q.yaw;int qi=(int)loose.size();loose.push_back(q);Record({ActionType::Place,qi,{},q});}if(symmetryZ&&fabsf(cursor.z-(area?area->center.z:0))>0.05f){VehiclePart q=p;q.localPosition.z=2*(area?area->center.z:0)-q.localPosition.z;q.yaw=180-q.yaw;int qi=(int)loose.size();loose.push_back(q);Record({ActionType::Place,qi,{},q});}recent.erase(std::remove(recent.begin(),recent.end(),d->id),recent.end());recent.push_front(d->id);while(recent.size()>12)recent.pop_back();status=placement==vek::PlacementResult::Warning?"Part placed with engineering warning.":"Part placed.";}}}
}

void VehicleBuilder::DrawPart(const VehiclePart&p,Color color,bool wires)const{
    Color tint=wires?color:Color{p.paintR,p.paintG,p.paintB,color.a};
    PartPresentationRenderer::DrawModel(p,p.localPosition,p.pitch,p.yaw,p.roll,tint,wires,false);
}

void VehicleBuilder::DrawGrid()const{if(!area)return;Vector3 c=ToVector3(area->center),s=ToVector3(area->size);float selected=GridStep();float visualStep=std::max(selected,0.25f);int nx=(int)(s.x/visualStep),nz=(int)(s.z/visualStep);for(int i=-nx/2;i<=nx/2;i++){float x=c.x+i*visualStep;bool major=std::fabs(std::fmod(std::fabs(x-c.x)+0.001f,1.0f))<visualStep*0.3f;DrawLine3D({x,c.y+0.055f,c.z-s.z*0.5f},{x,c.y+0.055f,c.z+s.z*0.5f},major?Color{80,125,135,120}:Color{55,76,82,80});}for(int i=-nz/2;i<=nz/2;i++){float z=c.z+i*visualStep;bool major=std::fabs(std::fmod(std::fabs(z-c.z)+0.001f,1.0f))<visualStep*0.3f;DrawLine3D({c.x-s.x*0.5f,c.y+0.055f,z},{c.x+s.x*0.5f,c.y+0.055f,z},major?Color{80,125,135,120}:Color{55,76,82,80});}}
void VehicleBuilder::DrawNodes()const{if(!registry)return;for(int i=0;i<(int)loose.size();++i){if(selectedPlaced>=0&&i!=selectedPlaced)continue;auto*d=registry->FindPart(loose[i].partId);if(!d)continue;for(auto&n:d->attachments){Vector3 p=Vector3Add(loose[i].localPosition,RotateEuler(ToVector3(n.position),loose[i].pitch,loose[i].yaw,loose[i].roll));DrawSphere(p,0.11f,selectedPlaced==i?YELLOW:SKYBLUE);}}}
void VehicleBuilder::DrawWorld(){if(!active)return;DrawGrid();for(int i=0;i<(int)loose.size();++i){DrawPart(loose[i],selectedPlaced==i?YELLOW:WHITE,false);if(selectedPlaced==i)DrawPart(loose[i],YELLOW,true);}const auto*d=SelectedDefinition();if(d){VehiclePart g=MakePart(*d,cursor);Color pc=PlacementColor(placement);DrawPart(g,pc,true);}DrawNodes();if(selectedPlaced>=0&&selectedPlaced<(int)loose.size()){Vector3 p=loose[selectedPlaced].localPosition;float l=1.6f;DrawLine3D(p,Vector3Add(p,{l,0,0}),RED);DrawLine3D(p,Vector3Add(p,{0,l,0}),GREEN);DrawLine3D(p,Vector3Add(p,{0,0,l}),BLUE);/* axis labels are shown in the 2D editor HUD */}if(showCOM&&!loose.empty()){Vector3 c=Stats().centerOfMass;DrawSphere(c,0.28f,MAGENTA);DrawLine3D(Vector3Add(c,{-0.65f,0,0}),Vector3Add(c,{0.65f,0,0}),MAGENTA);DrawLine3D(Vector3Add(c,{0,-0.65f,0}),Vector3Add(c,{0,0.65f,0}),MAGENTA);DrawLine3D(Vector3Add(c,{0,0,-0.65f}),Vector3Add(c,{0,0,0.65f}),MAGENTA);}}

void VehicleBuilder::DrawUI(){
    if(!active)return;
    auto parts=VisibleParts();
    if(gui){
        auto a=gui->UpdateAndDraw(mode,search,category,selectedPartId,parts,ProgressionLevel());
        pendingSearch=a.search;pendingCategory=a.category;pendingPart=a.selectedPartId;pendingCommand=a.command;
    }

    VehicleEngineeringStats stats=Stats();
    int sw=GetScreenWidth(),sh=GetScreenHeight();
    float panelW=std::clamp(sw*0.34f,250.0f,340.0f);
    float panelX=sw-panelW-12.0f,panelY=12.0f,panelH=312.0f;
    Rectangle analysis{panelX,panelY,panelW,panelH};
    DrawRectangleRounded(analysis,0.035f,8,Color{9,16,20,235});
    DrawRectangleRoundedLinesEx(analysis,0.035f,8,1.2f,Color{61,104,115,255});

    vek::GuiTextPolicy title;title.fontSize=20;title.minFontSize=12;title.maxFontSize=20;title.maxLines=1;title.wrap=false;
    VekGuiTextRenderer::DrawTextAuto("ENGINEERING ANALYSIS",{analysis.x+12,analysis.y+8,analysis.width-24,28},title,SKYBLUE,true);

    float yy=analysis.y+42.0f;
    auto line=[&](const std::string& label,const std::string& value,Color valueColor=RAYWHITE){
        float labelW=analysis.width*0.43f;
        vek::GuiTextPolicy lp;lp.fontSize=14;lp.minFontSize=9;lp.maxFontSize=14;lp.maxLines=1;lp.wrap=false;
        vek::GuiTextPolicy vp=lp;vp.align=vek::GuiTextAlign::Right;
        VekGuiTextRenderer::DrawTextAuto(label,{analysis.x+12,yy,labelW-6,19},lp,LIGHTGRAY,true);
        VekGuiTextRenderer::DrawTextAuto(value,{analysis.x+labelW,yy,analysis.width-labelW-12,19},vp,valueColor,true);
        yy+=21.0f;
    };
    line("Parts",TextFormat("%d / %d",stats.partCount,rules?rules->MaxParts(mode):(area?area->maxParts:800)));
    line("Mass",TextFormat("%.0f kg",stats.mass));
    line("Power",TextFormat("%.0f W",stats.power));
    line("Power / mass",TextFormat("%.1f W/kg",stats.powerToWeight));
    line("Est. top speed",TextFormat("%.0f km/h",stats.estimatedTopSpeed));
    line("Est. accel",TextFormat("%.1f m/s2",stats.estimatedAcceleration));
    line("Fuel / battery",TextFormat("%.0f L / %.0f kWh",stats.fuelCapacity,stats.batteryCapacity));
    line("Cargo",TextFormat("%.0f kg",stats.cargoCapacity));
    line("Size",TextFormat("%.1f x %.1f x %.1f m",stats.size.x,stats.size.y,stats.size.z));
    line("Stability",TextFormat("%.0f%%",stats.stability*100));
    line("Build cost",mode==vek::GameMode::Sandbox?"FREE":TextFormat("$%.0f",stats.buildCost),GREEN);

    std::string warning=rules?rules->PowerWarning(stats.mass,stats.power+stats.thrust):"";
    if(!warning.empty()){
        vek::GuiTextPolicy wp;wp.fontSize=13;wp.minFontSize=9;wp.maxFontSize=13;wp.maxLines=2;wp.wrap=true;wp.ellipsis=true;
        VekGuiTextRenderer::DrawTextAuto(warning,{analysis.x+12,analysis.y+268,analysis.width-24,34},wp,ORANGE,false);
    }

    // v26.8.5: Build Mode intentionally keeps the viewport clean.
    // The old always-on command guide was removed; editor controls remain
    // available through input and the engineering analysis/part UI.

}

bool VehicleBuilder::Finalize(Vehicle&v){auto c=Counts();int money=economy?economy->money.Get():0;float cost=c.estimatedCost;bool ok=rules?rules->BuildValid(mode,c.structuralParts,c.engines,c.seats,c.wheels,c.movementParts,money,cost):(c.structuralParts>=1&&c.engines>=1&&c.seats>=1&&(c.wheels>=3||c.movementParts>=1));status=rules?rules->BuildMessage(mode,c.structuralParts,c.engines,c.seats,c.wheels,c.movementParts,money,cost):(ok?"Build ready.":"Build requirements not met.");if(!ok)return false;if(loose.empty())return false;if(mode==vek::GameMode::Survival&&cost>0){int charge=(int)ceilf(cost);if(!economy||economy->money.Get()<charge){status="Not enough money to construct this design.";return false;}economy->money=economy->money.Get()-charge;}
    v.parts=loose;Vector3 center=VehicleCenter();center.y=0.7f;for(auto&p:v.parts)p.localPosition=Vector3Subtract(p.localPosition,center);v.Reset();v.finalized=v.ValidBuild();if(!v.finalized){status="Native safety validation rejected the design.";return false;}loose.clear();history.clear();historyCursor=0;selectedPlaced=-1;status="Vehicle constructed from VEK part definitions.";return true;}
void VehicleBuilder::Clear(){loose.clear();history.clear();historyCursor=0;selectedPlaced=-1;status="Editor cleared.";}
