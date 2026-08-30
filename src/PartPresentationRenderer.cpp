#include "PartPresentationRenderer.h"
#include "rlgl.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
std::string Id(std::string s){
    auto cut=[&](const char* p){std::string prefix=p;if(s.rfind(prefix,0)==0)s=s.substr(prefix.size());};
    cut("builtin:");cut("icon:");return s;
}
Color Mul(Color a, float f){return Color{(unsigned char)std::clamp((int)std::round(a.r*f),0,255),(unsigned char)std::clamp((int)std::round(a.g*f),0,255),(unsigned char)std::clamp((int)std::round(a.b*f),0,255),a.a};}
Color CategoryColor(const std::string& c){
    if(c=="Movement")return Color{80,116,140,255};if(c=="Mechanical")return Color{150,103,65,255};
    if(c=="Functional")return Color{83,133,105,255};if(c=="Aircraft")return Color{73,126,160,255};
    if(c=="Marine")return Color{62,121,153,255};if(c=="Experimental")return Color{137,84,154,255};
    return Color{92,118,124,255};
}
void Box(Vector3 p,Vector3 s,Color c,bool wires=false){if(wires)DrawCubeWires(p,s.x,s.y,s.z,c);else DrawCube(p,s.x,s.y,s.z,c);}
void Cyl(Vector3 a,Vector3 b,float r,Color c,int seg=16){DrawCylinderEx(a,b,r,r,seg,c);}
void Cyl2(Vector3 a,Vector3 b,float r1,float r2,Color c,int seg=16){DrawCylinderEx(a,b,r1,r2,seg,c);}
void WireBounds(Vector3 s,Color c){DrawCubeWires({0,0,0},s.x,s.y,s.z,c);}

void ModelChassis(Vector3 s,Color c,bool w){
    float rail=std::max(0.09f,s.x*0.10f);Box({-s.x*.36f,0,0},{rail,s.y*.75f,s.z},c,w);Box({s.x*.36f,0,0},{rail,s.y*.75f,s.z},c,w);
    for(float z:{-0.42f,-0.12f,0.20f,0.44f})Box({0,0,s.z*z},{s.x*.82f,s.y*.62f,std::max(.08f,s.z*.045f)},Mul(c,.88f),w);
}
void ModelBeam(Vector3 s,Color c,bool w){Box({0,0,0},{s.x*.45f,s.y,s.z},c,w);Box({0,s.y*.38f,0},{s.x,s.y*.22f,s.z},c,w);Box({0,-s.y*.38f,0},{s.x,s.y*.22f,s.z},c,w);}
void ModelPlate(Vector3 s,Color c,bool w){Box({0,0,0},s,c,w);if(!w){Box({0,s.y*.46f,0},{s.x*.92f,std::max(.018f,s.y*.08f),s.z*.92f},Mul(c,1.08f));}}
void ModelWheel(Vector3 s,Color c,bool offroad,bool w){
    float r=std::max(.12f,std::min(s.y,s.z)*.48f), width=std::max(.10f,s.x*.94f);Color tire=w?c:Color{42,45,48,c.a};
    Cyl({-width*.5f,0,0},{width*.5f,0,0},r,tire,offroad?18:24);Cyl({-width*.52f,0,0},{width*.52f,0,0},r*.45f,w?c:Color{130,137,141,c.a},16);
    if(!w&&offroad){for(int i=0;i<10;i++){float a=i*2*PI/10.0f;Vector3 p{0,std::cos(a)*r*.9f,std::sin(a)*r*.9f};Box(p,{width*1.04f,r*.18f,r*.14f},Color{28,30,32,c.a});}}
}
void ModelTrack(Vector3 s,Color c,bool w){
    Color tire=w?c:Color{45,47,49,c.a};Box({0,0,0},{s.x,s.y*.82f,s.z},tire,w);
    for(float z:{-.34f,0.0f,.34f})Cyl({-s.x*.52f,-s.y*.05f,s.z*z},{s.x*.52f,-s.y*.05f,s.z*z},s.y*.28f,w?c:Color{105,110,112,c.a},14);
}
void ModelEngine(Vector3 s,Color c,bool diesel,bool w){
    Color block=w?c:Mul(c,.84f);Box({0,-s.y*.08f,0},{s.x*.86f,s.y*.66f,s.z*.82f},block,w);Box({0,s.y*.33f,0},{s.x*.72f,s.y*.20f,s.z*.66f},w?c:Mul(c,1.05f),w);
    for(int i=-1;i<=1;i++)Cyl({s.x*.45f,s.y*.10f,s.z*i*.20f},{s.x*.58f,s.y*.10f,s.z*i*.20f},s.y*.08f,w?c:Color{92,94,96,c.a},10);
    if(diesel){Cyl({-s.x*.44f,s.y*.24f,-s.z*.25f},{-s.x*.44f,s.y*.24f,s.z*.25f},s.y*.16f,w?c:Color{70,72,74,c.a},14);Cyl2({-s.x*.48f,s.y*.24f,s.z*.25f},{-s.x*.60f,s.y*.24f,s.z*.35f},s.y*.10f,s.y*.06f,w?c:Color{105,108,110,c.a},10);}
}
void ModelMotor(Vector3 s,Color c,bool w){
    float r=std::min(s.y,s.z)*.38f;Cyl({-s.x*.38f,0,0},{s.x*.38f,0,0},r,w?c:Color{68,92,104,c.a},20);Cyl({-s.x*.48f,0,0},{-s.x*.38f,0,0},r*.75f,w?c:Mul(c,.9f),16);Cyl({s.x*.38f,0,0},{s.x*.48f,0,0},r*.75f,w?c:Mul(c,.9f),16);Cyl({s.x*.48f,0,0},{s.x*.62f,0,0},r*.16f,w?c:Color{160,165,168,c.a},12);
}
void ModelSeat(Vector3 s,Color c,bool w){
    Color fabric=w?c:Color{63,67,72,c.a};Box({0,-s.y*.34f,s.z*.08f},{s.x*.78f,s.y*.20f,s.z*.72f},fabric,w);
    rlPushMatrix();rlTranslatef(0,s.y*.10f,s.z*.30f);rlRotatef(-13,1,0,0);Box({0,0,0},{s.x*.72f,s.y*.72f,s.z*.20f},fabric,w);Box({0,s.y*.42f,0},{s.x*.48f,s.y*.20f,s.z*.18f},w?c:Mul(fabric,1.05f),w);rlPopMatrix();
    Box({-s.x*.43f,-s.y*.18f,s.z*.05f},{s.x*.10f,s.y*.38f,s.z*.65f},w?c:Mul(fabric,.82f),w);Box({s.x*.43f,-s.y*.18f,s.z*.05f},{s.x*.10f,s.y*.38f,s.z*.65f},w?c:Mul(fabric,.82f),w);
    Box({-s.x*.28f,-s.y*.51f,0},{s.x*.10f,s.y*.20f,s.z*.12f},w?c:Color{85,88,90,c.a},w);Box({s.x*.28f,-s.y*.51f,0},{s.x*.10f,s.y*.20f,s.z*.12f},w?c:Color{85,88,90,c.a},w);
}
void ModelFuelTank(Vector3 s,Color c,bool w){
    float r=std::min(s.y,s.z)*.42f;Color body=w?c:Color{105,112,112,c.a};Cyl({-s.x*.38f,0,0},{s.x*.38f,0,0},r,body,20);Cyl2({-s.x*.48f,0,0},{-s.x*.38f,0,0},r*.65f,r,body,20);Cyl2({s.x*.38f,0,0},{s.x*.48f,0,0},r,r*.65f,body,20);
    for(float x:{-.22f,.22f})Box({s.x*x,0,0},{s.x*.07f,s.y*.94f,s.z*.94f},w?c:Color{48,51,53,c.a},w);
    Cyl({s.x*.16f,r*.84f,0},{s.x*.16f,r*1.18f,0},r*.12f,w?c:Color{180,155,55,c.a},10);
}
void ModelBattery(Vector3 s,Color c,bool w){
    Color body=w?c:Color{45,52,58,c.a};Box({0,0,0},s,body,w);for(int x=-2;x<=2;x++)Box({s.x*x*.17f,s.y*.23f,0},{s.x*.13f,s.y*.36f,s.z*.86f},w?c:Color{67,76,82,c.a},w);
    if(!w){Cyl({-s.x*.32f,s.y*.56f,0},{-s.x*.32f,s.y*.72f,0},s.y*.12f,RED,10);Cyl({s.x*.32f,s.y*.56f,0},{s.x*.32f,s.y*.72f,0},s.y*.12f,Color{55,105,210,255},10);}
}
void ModelCargo(Vector3 s,Color c,bool w){Box({0,0,0},s,w?c:Mul(c,.78f),w);Box({0,s.y*.45f,0},{s.x*.96f,s.y*.12f,s.z*.96f},w?c:Mul(c,.92f),w);for(float x:{-.43f,.43f})Box({s.x*x,0,0},{s.x*.08f,s.y*.90f,s.z*.94f},w?c:Mul(c,.65f),w);}
void ModelLight(Vector3 s,Color c,bool w){Box({0,0,-s.z*.15f},{s.x,s.y,s.z*.70f},w?c:Color{55,58,60,c.a},w);Box({0,0,s.z*.31f},{s.x*.86f,s.y*.86f,s.z*.16f},w?c:Color{255,223,92,c.a},w);Cyl({0,-s.y*.55f,-s.z*.15f},{0,-s.y*.85f,-s.z*.15f},s.x*.09f,w?c:Color{90,93,95,c.a},10);}
void ModelTowHook(Vector3 s,Color c,bool w){Color metal=w?c:Color{120,125,128,c.a};Cyl({-s.x*.30f,s.y*.15f,0},{-s.x*.30f,-s.y*.28f,0},s.x*.13f,metal,12);Cyl({s.x*.30f,s.y*.15f,0},{s.x*.30f,-s.y*.28f,0},s.x*.13f,metal,12);Cyl({-s.x*.30f,-s.y*.28f,0},{s.x*.30f,-s.y*.28f,0},s.x*.13f,metal,12);Box({0,s.y*.30f,-s.z*.20f},{s.x*.70f,s.y*.18f,s.z*.35f},metal,w);}
void ModelWinch(Vector3 s,Color c,bool w){Color metal=w?c:Color{85,88,90,c.a};for(float x:{-.38f,.38f})Cyl({s.x*x,-s.y*.32f,0},{s.x*x,s.y*.32f,0},s.y*.42f,metal,14);Cyl({-s.x*.35f,0,0},{s.x*.35f,0,0},s.y*.27f,w?c:Color{52,55,57,c.a},18);Cyl({-s.x*.38f,0,0},{s.x*.38f,0,0},s.y*.06f,w?c:Color{180,182,184,c.a},10);}
void ModelSuspension(Vector3 s,Color c,bool w){Color metal=w?c:Color{130,132,134,c.a};Cyl({0,-s.y*.43f,0},{0,s.y*.43f,0},s.x*.12f,metal,12);for(int i=-3;i<=3;i++){float y=s.y*i*.11f;DrawTorus({0,y,0},s.x*.06f,s.x*.31f,8,12,w?c:Color{60,65,70,c.a});}Cyl({0,-s.y*.50f,0},{0,-s.y*.40f,0},s.x*.22f,metal,12);Cyl({0,s.y*.40f,0},{0,s.y*.50f,0},s.x*.22f,metal,12);}
void ModelSteering(Vector3 s,Color c,bool w){Color metal=w?c:Color{110,113,116,c.a};Box({0,0,0},{s.x*.58f,s.y*.82f,s.z*.82f},metal,w);Cyl({-s.x*.56f,0,0},{s.x*.56f,0,0},s.y*.12f,metal,12);for(float x:{-.58f,.58f})Cyl({s.x*x,0,0},{s.x*(x+(x<0?-0.20f:0.20f)),0,0},s.y*.06f,w?c:Color{170,173,175,c.a},10);}
void ModelWing(Vector3 s,Color c,bool w){Box({0,0,0},{s.x,s.y,s.z*.72f},w?c:Color{116,145,160,c.a},w);Box({0,-s.y*.05f,-s.z*.41f},{s.x*.82f,s.y*.65f,s.z*.18f},w?c:Mul(c,.88f),w);}
void ModelPropeller(Vector3 s,Color c,bool rotor,bool w){float blade=rotor?s.x*.46f:std::max(s.x,s.y)*.46f;Color metal=w?c:Color{145,150,153,c.a};Cyl({0,-s.z*.15f,0},{0,s.z*.15f,0},std::max(.06f,std::min(s.y,s.z)*.18f),metal,12);Box({0,0,0},{blade*2,std::max(.06f,s.y*.30f),std::max(.05f,s.z*.45f)},metal,w);Box({0,0,0},{std::max(.05f,s.x*.18f),std::max(.06f,rotor?blade*2:s.y*.18f),rotor?std::max(.05f,s.z*.45f):blade*2},Mul(metal,.9f),w);}
void ModelJet(Vector3 s,Color c,bool thruster,bool w){float r=std::min(s.x,s.y)*.43f;Color body=w?c:Color{100,106,110,c.a};Cyl({0,0,-s.z*.34f},{0,0,s.z*.28f},r,body,20);Cyl2({0,0,s.z*.28f},{0,0,s.z*.50f},r,r*.62f,w?c:Color{55,58,61,c.a},20);Cyl2({0,0,-s.z*.50f},{0,0,-s.z*.34f},r*.72f,r,body,20);if(thruster&&!w)Cyl2({0,0,s.z*.50f},{0,0,s.z*.66f},r*.55f,r*.18f,Color{230,120,45,c.a},16);}
void ModelReactor(Vector3 s,Color c,bool w){float r=std::min({s.x,s.y,s.z})*.30f;DrawSphere({0,0,0},r,w?c:Color{92,116,128,c.a});for(int axis=0;axis<3;axis++){rlPushMatrix();if(axis==1)rlRotatef(90,1,0,0);if(axis==2)rlRotatef(90,0,0,1);DrawTorus({0,0,0},r*.10f,r*1.28f,10,22,w?c:Color{140,92,175,c.a});rlPopMatrix();}}
void ModelHull(Vector3 s,Color c,bool w){Box({0,s.y*.08f,0},{s.x*.92f,s.y*.62f,s.z},w?c:Color{67,116,142,c.a},w);Box({0,-s.y*.30f,0},{s.x*.72f,s.y*.20f,s.z*.88f},w?c:Mul(c,.72f),w);Box({0,s.y*.42f,-s.z*.10f},{s.x*.58f,s.y*.18f,s.z*.62f},w?c:Mul(c,1.05f),w);}
void ModelGeneric(Vector3 s,Color c,bool w){Box({0,0,0},s,c,w);}

bool DedicatedModel(const std::string&id){static const std::array<const char*,29> ids={
"frame.chassis","frame.block","frame.beam","frame.plate","frame.roll_cage","wheel.road","wheel.offroad","track.medium","engine.petrol","engine.diesel","motor.electric","seat.bucket","fuel.tank80","battery.pack120","cargo.medium","light.work","tow.hook","winch.medium","suspension.basic","steering.rack","wing.medium","propeller.aircraft","rotor.helicopter","jet.small","thruster.vector","reactor.prototype","marine.hull","marine.propeller","generic.box"};return std::find_if(ids.begin(),ids.end(),[&](auto*s){return id==s;})!=ids.end();}

void DrawBuiltin(const std::string&raw,Vector3 s,Color c,bool w){std::string id=Id(raw);
    if(id=="frame.chassis")ModelChassis(s,c,w);else if(id=="frame.beam")ModelBeam(s,c,w);else if(id=="frame.plate")ModelPlate(s,c,w);else if(id=="frame.roll_cage"){WireBounds(s,c);}else if(id=="wheel.road")ModelWheel(s,c,false,w);else if(id=="wheel.offroad")ModelWheel(s,c,true,w);else if(id=="track.medium")ModelTrack(s,c,w);else if(id=="engine.petrol")ModelEngine(s,c,false,w);else if(id=="engine.diesel")ModelEngine(s,c,true,w);else if(id=="motor.electric")ModelMotor(s,c,w);else if(id=="seat.bucket")ModelSeat(s,c,w);else if(id=="fuel.tank80")ModelFuelTank(s,c,w);else if(id=="battery.pack120")ModelBattery(s,c,w);else if(id=="cargo.medium")ModelCargo(s,c,w);else if(id=="light.work")ModelLight(s,c,w);else if(id=="tow.hook")ModelTowHook(s,c,w);else if(id=="winch.medium")ModelWinch(s,c,w);else if(id=="suspension.basic")ModelSuspension(s,c,w);else if(id=="steering.rack")ModelSteering(s,c,w);else if(id=="wing.medium")ModelWing(s,c,w);else if(id=="propeller.aircraft")ModelPropeller(s,c,false,w);else if(id=="rotor.helicopter")ModelPropeller(s,c,true,w);else if(id=="jet.small")ModelJet(s,c,false,w);else if(id=="thruster.vector")ModelJet(s,c,true,w);else if(id=="reactor.prototype")ModelReactor(s,c,w);else if(id=="marine.hull")ModelHull(s,c,w);else if(id=="marine.propeller")ModelPropeller(s,c,false,w);else ModelGeneric(s,c,w);
}

void IconBase(Rectangle r,Color c,bool locked){DrawRectangleRounded(r,.18f,5,Color{12,20,24,255});DrawRectangleRoundedLinesEx(r,.18f,5,1,Color{51,75,82,255});if(locked)DrawRectangleRounded(r,.18f,5,Color{5,8,10,125});}
Vector2 P(Rectangle r,float x,float y){return{r.x+r.width*x,r.y+r.height*y};}
void Line(Rectangle r,float x1,float y1,float x2,float y2,float t,Color c){DrawLineEx(P(r,x1,y1),P(r,x2,y2),t,c);}
void IconWheel(Rectangle r,Color c,bool off){float rad=r.width*.28f;DrawCircleV(P(r,.5f,.5f),rad,Color{44,47,50,255});DrawCircleV(P(r,.5f,.5f),rad*.48f,c);if(off)for(int i=0;i<8;i++){float a=i*PI/4;auto a1=P(r,.5f+std::cos(a)*.31f,.5f+std::sin(a)*.31f);auto a2=P(r,.5f+std::cos(a)*.39f,.5f+std::sin(a)*.39f);DrawLineEx(a1,a2,3,Color{28,30,32,255});}}
void IconEngine(Rectangle r,Color c,bool diesel){DrawRectangleRounded({r.x+r.width*.18f,r.y+r.height*.28f,r.width*.62f,r.height*.48f},.15f,4,c);DrawRectangleRounded({r.x+r.width*.27f,r.y+r.height*.16f,r.width*.42f,r.height*.18f},.15f,4,Mul(c,1.1f));for(int i=0;i<3;i++)DrawCircleV(P(r,.23f+i*.22f,.78f),r.width*.045f,Color{115,119,122,255});if(diesel)DrawCircleV(P(r,.78f,.24f),r.width*.10f,Color{70,73,76,255});}
void IconSeat(Rectangle r,Color c){DrawRectangleRounded({r.x+r.width*.22f,r.y+r.height*.56f,r.width*.55f,r.height*.19f},.25f,5,c);DrawRectangleRounded({r.x+r.width*.43f,r.y+r.height*.18f,r.width*.30f,r.height*.48f},.22f,5,Mul(c,.88f));Line(r,.29f,.73f,.24f,.89f,3,c);Line(r,.67f,.73f,.72f,.89f,3,c);}
void IconFuel(Rectangle r,Color c){DrawRectangleRounded({r.x+r.width*.18f,r.y+r.height*.27f,r.width*.64f,r.height*.48f},.45f,8,c);Line(r,.34f,.25f,.34f,.78f,3,Color{48,51,53,255});Line(r,.66f,.25f,.66f,.78f,3,Color{48,51,53,255});DrawRectangle((int)(r.x+r.width*.56f),(int)(r.y+r.height*.16f),(int)(r.width*.12f),(int)(r.height*.13f),Color{190,161,55,255});}
void IconBattery(Rectangle r,Color c){DrawRectangleRounded({r.x+r.width*.16f,r.y+r.height*.28f,r.width*.68f,r.height*.48f},.10f,4,c);for(int i=0;i<4;i++)Line(r,.26f+i*.16f,.34f,.26f+i*.16f,.69f,2,Mul(c,1.18f));DrawRectangle((int)(r.x+r.width*.25f),(int)(r.y+r.height*.18f),(int)(r.width*.10f),(int)(r.height*.10f),RED);DrawRectangle((int)(r.x+r.width*.65f),(int)(r.y+r.height*.18f),(int)(r.width*.10f),(int)(r.height*.10f),Color{55,105,210,255});}
void IconTrack(Rectangle r,Color c){DrawRectangleRounded({r.x+r.width*.12f,r.y+r.height*.28f,r.width*.76f,r.height*.48f},.42f,8,Color{45,47,49,255});for(int i=0;i<3;i++){DrawCircleV(P(r,.30f+i*.20f,.52f),r.width*.11f,c);DrawCircleV(P(r,.30f+i*.20f,.52f),r.width*.05f,Color{45,47,49,255});}}
void IconProp(Rectangle r,Color c,bool rotor){DrawCircleV(P(r,.5f,.5f),r.width*.08f,c);Line(r,.15f,.5f,.85f,.5f,5,c);if(rotor)Line(r,.5f,.15f,.5f,.85f,5,Mul(c,.9f));else Line(r,.28f,.72f,.72f,.28f,4,Mul(c,.9f));}
void IconGeneric(Rectangle r,Color c,const std::string&id){if(id.find("beam")!=std::string::npos)DrawRectangle((int)(r.x+r.width*.20f),(int)(r.y+r.height*.43f),(int)(r.width*.60f),(int)(r.height*.15f),c);else if(id.find("plate")!=std::string::npos)DrawRectangle((int)(r.x+r.width*.20f),(int)(r.y+r.height*.32f),(int)(r.width*.60f),(int)(r.height*.42f),c);else if(id.find("light")!=std::string::npos){DrawCircleV(P(r,.5f,.43f),r.width*.23f,Color{245,210,68,255});Line(r,.5f,.66f,.5f,.86f,3,c);}else if(id.find("wing")!=std::string::npos){DrawTriangle(P(r,.10f,.58f),P(r,.88f,.34f),P(r,.78f,.68f),c);}else if(id.find("suspension")!=std::string::npos){Line(r,.5f,.14f,.5f,.86f,4,c);for(int i=0;i<5;i++)Line(r,.30f,.26f+i*.10f,.70f,.31f+i*.10f,2,Mul(c,1.12f));}else if(id.find("steering")!=std::string::npos){Line(r,.14f,.52f,.86f,.52f,4,c);DrawRectangleRounded({r.x+r.width*.34f,r.y+r.height*.35f,r.width*.32f,r.height*.34f},.18f,4,Mul(c,.86f));}else if(id.find("winch")!=std::string::npos){DrawCircleV(P(r,.5f,.5f),r.width*.24f,Mul(c,.78f));DrawCircleV(P(r,.5f,.5f),r.width*.12f,c);Line(r,.15f,.25f,.15f,.78f,4,c);Line(r,.85f,.25f,.85f,.78f,4,c);}else if(id.find("hook")!=std::string::npos){Line(r,.32f,.22f,.32f,.70f,5,c);Line(r,.68f,.22f,.68f,.70f,5,c);Line(r,.32f,.70f,.68f,.70f,5,c);}else if(id.find("cargo")!=std::string::npos){DrawRectangleRounded({r.x+r.width*.18f,r.y+r.height*.23f,r.width*.64f,r.height*.56f},.08f,4,Mul(c,.78f));Line(r,.20f,.38f,.80f,.38f,3,c);}else if(id.find("jet")!=std::string::npos||id.find("thruster")!=std::string::npos){DrawCircleV(P(r,.48f,.46f),r.width*.24f,c);DrawTriangle(P(r,.62f,.42f),P(r,.90f,.52f),P(r,.62f,.62f),Mul(c,.82f));}else if(id.find("reactor")!=std::string::npos){DrawCircleV(P(r,.5f,.5f),r.width*.17f,c);DrawRing(P(r,.5f,.5f),r.width*.25f,r.width*.29f,0,360,28,Mul(c,1.15f));}else if(id.find("hull")!=std::string::npos){DrawTriangle(P(r,.14f,.42f),P(r,.86f,.42f),P(r,.68f,.72f),c);DrawRectangle((int)(r.x+r.width*.30f),(int)(r.y+r.height*.30f),(int)(r.width*.35f),(int)(r.height*.16f),Mul(c,1.08f));}else{DrawRectangleRounded({r.x+r.width*.22f,r.y+r.height*.22f,r.width*.56f,r.height*.56f},.12f,4,c);}}
}

namespace PartPresentationRenderer {
void DrawModel(const VehiclePart&part,Vector3 position,float pitch,float yaw,float roll,Color tint,bool wires,bool worldModel){
    std::string model=worldModel?part.worldModel:part.viewModel;if(model.empty())model=part.visual;Color c=(part.allowTint||wires)?tint:Color{150,158,162,tint.a};
    Vector3 scale=part.viewScale;Vector3 s{part.size.x*scale.x,part.size.y*scale.y,part.size.z*scale.z};Vector3 off=part.viewOffset;
    rlPushMatrix();rlTranslatef(position.x,position.y,position.z);rlRotatef(yaw,0,1,0);rlRotatef(pitch,1,0,0);rlRotatef(roll,0,0,1);rlTranslatef(off.x,off.y,off.z);rlRotatef(part.viewRotation.y,0,1,0);rlRotatef(part.viewRotation.x,1,0,0);rlRotatef(part.viewRotation.z,0,0,1);DrawBuiltin(model,s,c,wires);rlPopMatrix();
}
void DrawCatalogIcon(Rectangle r,const std::string&raw,const std::string&category,bool locked){std::string id=Id(raw);Color c=CategoryColor(category);IconBase(r,c,locked);Rectangle q{r.x+3,r.y+3,r.width-6,r.height-6};
    if(id=="wheel.road")IconWheel(q,c,false);else if(id=="wheel.offroad")IconWheel(q,c,true);else if(id=="track.medium")IconTrack(q,c);else if(id=="engine.petrol")IconEngine(q,c,false);else if(id=="engine.diesel")IconEngine(q,c,true);else if(id=="motor.electric"){DrawCircleV(P(q,.46f,.5f),q.width*.25f,c);DrawCircleV(P(q,.46f,.5f),q.width*.10f,Mul(c,.65f));Line(q,.66f,.5f,.88f,.5f,4,Mul(c,1.15f));}else if(id=="seat.bucket")IconSeat(q,c);else if(id=="fuel.tank80")IconFuel(q,c);else if(id=="battery.pack120")IconBattery(q,c);else if(id=="propeller.aircraft"||id=="marine.propeller")IconProp(q,c,false);else if(id=="rotor.helicopter")IconProp(q,c,true);else IconGeneric(q,c,id);
    if(locked){DrawCircleV(P(r,.78f,.23f),r.width*.11f,Color{15,20,23,235});DrawRectangle((int)(r.x+r.width*.72f),(int)(r.y+r.height*.22f),(int)(r.width*.12f),(int)(r.height*.12f),Color{200,205,207,255});}
}
bool HasBuiltinModel(const std::string&raw){return DedicatedModel(Id(raw));}
bool HasBuiltinIcon(const std::string&raw){auto id=Id(raw);return DedicatedModel(id)||id=="frame.block"||id=="frame.chassis"||id=="frame.beam"||id=="frame.plate"||id=="frame.roll_cage";}
}
