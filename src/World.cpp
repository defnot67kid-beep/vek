// v26 note: animated door interaction, richer character rig, fuller hair, day/night cycle, and dev cheat panel groundwork.
#include "World.h"
#include "rlgl.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
World::World(){RebuildCollision();}
void World::ConfigureHangar(const vek::HangarBuildArea&a){hangar=a;workshop={a.center.x,a.center.y,a.center.z};RebuildCollision();}
void World::ConfigurePersonnelDoor(float offsetX,float width,float height,float openAngleDegrees){doorOffsetX=offsetX;doorWidth=std::clamp(width,1.2f,5.0f);doorHeight=std::clamp(height,2.0f,6.0f);doorOpenAngle=std::clamp(openAngleDegrees,45.0f,130.0f);RebuildCollision();}
void World::ConfigureGarageDoor(float width,float height,int panelCount,float lockOffsetX,float lockHeight,float panelOverlap,float sideSealWidth,float lintelHeight,float collisionClearFraction){garageWidth=std::clamp(width,8.0f,40.0f);garageHeight=std::clamp(height,4.0f,15.0f);garagePanelCount=std::clamp(panelCount,3,24);passlockOffsetX=std::clamp(lockOffsetX,-hangar.size.x*0.5f+1.0f,hangar.size.x*0.5f-1.0f);passlockHeight=std::clamp(lockHeight,0.7f,2.5f);garagePanelOverlap=std::clamp(panelOverlap,0.0f,0.15f);garageSideSealWidth=std::clamp(sideSealWidth,0.10f,1.0f);garageLintelHeight=std::clamp(lintelHeight,0.10f,2.0f);garageCollisionClearFraction=std::clamp(collisionClearFraction,0.25f,0.95f);RebuildCollision();}
void World::SetGarageDoorState(float fraction,bool locked){float f=std::clamp(fraction,0.0f,1.0f);bool changed=std::fabs(f-garageOpenFraction)>0.0005f||locked!=garageLocked;garageOpenFraction=f;garageLocked=locked;if(changed)RebuildCollision();}
Vector3 World::GarageDoorPosition()const{return{hangar.center.x,garageHeight*0.5f,hangar.center.z+hangar.size.z*0.5f};}
Vector3 World::GarageDoorApproachPoint()const{auto p=GarageDoorPosition();return{p.x,1.0f,p.z+3.6f};}
Vector3 World::GarageDoorInsidePoint()const{auto p=GarageDoorPosition();return{p.x,1.0f,p.z-5.0f};}
Vector3 World::GarageControlPosition()const{float hx=hangar.size.x*0.5f,hz=hangar.size.z*0.5f;return{hangar.center.x+std::min(hx-1.0f,garageWidth*0.5f+2.0f),1.35f,hangar.center.z+hz-0.48f};}
BoundingBox World::GarageControlBounds()const{auto p=GarageControlPosition();return{{p.x-0.36f,p.y-0.46f,p.z-0.24f},{p.x+0.36f,p.y+0.46f,p.z+0.24f}};}
bool World::IsInsideHangar(Vector3 p,float margin)const{
 float hx=std::max(0.5f,hangar.size.x*0.5f-margin),hz=std::max(0.5f,hangar.size.z*0.5f-margin);
 return p.x>=hangar.center.x-hx&&p.x<=hangar.center.x+hx&&p.z>=hangar.center.z-hz&&p.z<=hangar.center.z+hz&&p.y>=-0.5f&&p.y<=hangar.maxBuildHeight+2.0f;
}
bool World::IsInsideGarageSide(Vector3 p)const{return p.z<GarageDoorPosition().z-0.2f;}
bool World::IsOutsideGarageSide(Vector3 p)const{return p.z>GarageDoorPosition().z+0.2f;}
bool World::IsNearGarageOpening(Vector3 p,float distance)const{
 auto g=GarageDoorPosition();float half=garageWidth*0.5f+0.75f;
 return std::fabs(p.x-g.x)<=half&&std::fabs(p.z-g.z)<=std::max(0.5f,distance)&&p.y<=garageHeight+1.5f;
}
Vector3 World::PasslockPosition()const{return{hangar.center.x+passlockOffsetX,passlockHeight,hangar.center.z+hangar.size.z*0.5f+0.42f};}
BoundingBox World::PasslockBounds()const{auto p=PasslockPosition();return{{p.x-0.34f,p.y-0.52f,p.z-0.22f},{p.x+0.34f,p.y+0.52f,p.z+0.22f}};}
Vector3 World::PersonnelDoorPosition()const{return{hangar.center.x+doorOffsetX,doorHeight*0.5f,hangar.center.z+hangar.size.z*0.5f};}
Vector3 World::PersonnelDoorApproachPoint()const{auto p=PersonnelDoorPosition();return{p.x+0.30f,1.0f,p.z+0.95f};}
Vector3 World::PersonnelDoorInsidePoint()const{auto p=PersonnelDoorPosition();return{p.x+0.20f,1.0f,p.z-3.8f};}
Vector3 World::PersonnelDoorHandleSpindlePosition()const{
 float a=-doorOpenAngle*doorOpenFraction*DEG2RAD;
 Vector3 hinge{hangar.center.x+doorOffsetX-doorWidth*0.5f,1.10f,hangar.center.z+hangar.size.z*0.5f-0.12f};
 float localX=doorWidth-0.22f,localZ=0.17f;
 return{hinge.x+cosf(a)*localX+sinf(a)*localZ,hinge.y,hinge.z-sinf(a)*localX+cosf(a)*localZ};
}
Vector3 World::PersonnelDoorHandlePosition()const{
 float a=-doorOpenAngle*doorOpenFraction*DEG2RAD;
 float turn=doorHandleTurn*38.0f*DEG2RAD;
 Vector3 hinge{hangar.center.x+doorOffsetX-doorWidth*0.5f,1.10f,hangar.center.z+hangar.size.z*0.5f-0.12f};
 float spindleX=doorWidth-0.22f;
 float leverLength=0.34f;
 float localX=spindleX-leverLength*cosf(turn);
 float localY=-leverLength*sinf(turn);
 float localZ=0.19f;
 return{hinge.x+cosf(a)*localX+sinf(a)*localZ,hinge.y+localY,hinge.z-sinf(a)*localX+cosf(a)*localZ};
}
BoundingBox World::PersonnelDoorHandleBounds()const{auto p=PersonnelDoorHandlePosition();return{{p.x-0.30f,p.y-0.30f,p.z-0.30f},{p.x+0.30f,p.y+0.30f,p.z+0.30f}};}
void World::SetPersonnelDoorHandleTurn(float normalized){doorHandleTurn=std::clamp(normalized,0.0f,1.0f);}
void World::SetPersonnelDoorTarget(bool open){doorTargetOpen=open;}
bool World::SetPersonnelDoorTraversalCollisionDisabled(bool disabled){
 if(doorTraversalCollisionDisabled==disabled)return false;
 doorTraversalCollisionDisabled=disabled;
 RebuildCollision();
 return true;
}
bool World::UpdatePersonnelDoor(float dt,float speed){float target=doorTargetOpen?1.0f:0.0f;float old=doorOpenFraction;float step=std::max(0.05f,speed)*std::clamp(dt,0.0f,0.05f);if(doorOpenFraction<target)doorOpenFraction=std::min(target,doorOpenFraction+step);else if(doorOpenFraction>target)doorOpenFraction=std::max(target,doorOpenFraction-step);bool nowOpen=doorOpenFraction>=0.82f;if(nowOpen!=doorCollisionOpen){doorCollisionOpen=nowOpen;RebuildCollision();return true;}return false;}
void World::RebuildCollision(){collisionBoxes.clear();float hx=hangar.size.x*0.5f,hz=hangar.size.z*0.5f,h=hangar.size.y;Vector3 c{hangar.center.x,hangar.center.y,hangar.center.z};
 collisionBoxes.push_back({1,"Hangar Back Wall",{c.x,h*0.5f,c.z-hz},{hangar.size.x,h,0.6f},"world"});
 collisionBoxes.push_back({2,"Hangar Left Wall",{c.x-hx,h*0.5f,c.z},{0.6f,h,hangar.size.z},"world"});
 collisionBoxes.push_back({3,"Hangar Right Wall",{c.x+hx,h*0.5f,c.z},{0.6f,h,hangar.size.z},"world"});
 // Front wall keeps the large central vehicle opening, plus a separate personnel doorway.
 float mainDoor=garageWidth;float mainLeft=-mainDoor*0.5f;float leftEdge=-hx;float px=doorOffsetX;float halfDoor=doorWidth*0.5f;
 auto addFront=[&](int id,float x0,float x1,const char*name){if(x1-x0>0.1f)collisionBoxes.push_back({id,name,{c.x+(x0+x1)*0.5f,h*0.5f,c.z+hz},{x1-x0,h,0.6f},"world"});};
 addFront(4,leftEdge,px-halfDoor,"Hangar Front Left A");
 addFront(5,px+halfDoor,mainLeft,"Hangar Front Left B");
 float rightEdge=hx;addFront(12,mainDoor*0.5f,rightEdge,"Hangar Front Right");
 if(!doorCollisionOpen && !doorTraversalCollisionDisabled) collisionBoxes.push_back({13,"Personnel Door",{c.x+px,doorHeight*0.5f,c.z+hz},{doorWidth,doorHeight,0.35f},"world"});
 // Header over personnel door.
 if(h>doorHeight) collisionBoxes.push_back({14,"Personnel Door Header",{c.x+px,doorHeight+(h-doorHeight)*0.5f,c.z+hz},{doorWidth,h-doorHeight,0.6f},"world"});
 // Horizontal response uses an X/Z circle solver, but CollisionSystem now
 // performs a Y-overlap broadphase first. The garage gate still stays solid
 // until there is enough visual clearance, while the high lintel no longer
 // becomes an invisible floor-to-ceiling blocker.
 if(garageOpenFraction<garageCollisionClearFraction) collisionBoxes.push_back({15,"Main Garage Door",{c.x,garageHeight*0.5f,c.z+hz-0.05f},{garageWidth,garageHeight,0.38f},"world"});
 if(h>garageHeight) collisionBoxes.push_back({16,"Main Garage Header",{c.x,garageHeight+(h-garageHeight)*0.5f,c.z+hz},{garageWidth,h-garageHeight,0.6f},"world"});
 collisionBoxes.push_back({6,"Tool Bench",{c.x-hx+4,1,c.z-hz+10},{2.5f,2,8},"world"});
 collisionBoxes.push_back({7,"Parts Shelving",{c.x+hx-3,2,c.z-hz+14},{3,4,14},"world"});
 collisionBoxes.push_back({8,"Paint Booth",{c.x+hx-7,2.5f,c.z+hz-15},{9,5,10},"world"});
 collisionBoxes.push_back({9,"Repair Bay Bench",{c.x-hx+5,1,c.z+hz-14},{3,2,9},"world"});
 collisionBoxes.push_back({10,"Industrial Warehouse",{15,3,95},{25,6,18},"world"});collisionBoxes.push_back({11,"Delivery Building",{92,3,82},{18,6,22},"world"});
 collisionBoxes.push_back({100,"North World Limit",{0,4,150},{300,8,1},"world"});collisionBoxes.push_back({101,"South World Limit",{0,4,-150},{300,8,1},"world"});collisionBoxes.push_back({102,"East World Limit",{150,4,0},{1,8,300},"world"});collisionBoxes.push_back({103,"West World Limit",{-150,4,0},{1,8,300},"world"});}
const std::vector<WorldCollisionBox>&World::CollisionBoxes()const{return collisionBoxes;}
static void Box(Vector3 p,Vector3 s,Color c){DrawCube(p,s.x,s.y,s.z,c);}
static void DoorPanel(Vector3 hinge,float width,float height,float angle,Color color){rlPushMatrix();rlTranslatef(hinge.x,hinge.y,hinge.z);rlRotatef(angle,0,1,0);rlTranslatef(width*0.5f,0,0);DrawCube({0,0,0},width,height,0.16f,color);DrawCubeWires({0,0,0},width,height,0.16f,{28,35,38,255});rlPopMatrix();}
static void GaragePanel(Vector3 center,float width,float panelHeight,float pitch,Color color){rlPushMatrix();rlTranslatef(center.x,center.y,center.z);rlRotatef(pitch,1,0,0);DrawCube({0,0,0},width,panelHeight,0.18f,color);DrawCubeWires({0,0,0},width,panelHeight,0.18f,{31,37,43,255});rlPopMatrix();}
void World::Draw()const{DrawPlane({0,0,0},{300,300},{62,105,58,255});Vector3 c{hangar.center.x,hangar.center.y,hangar.center.z};float hx=hangar.size.x*0.5f,hz=hangar.size.z*0.5f,h=hangar.size.y;
 // Exterior apron, road, parking and service areas.
 Box({c.x,0.025f,c.z+hz+16},{hangar.size.x+24,0.05f,32},{72,74,76,255});Box({c.x,0.035f,c.z+hz+55},{12,0.07f,50},{45,45,48,255});Box({c.x-hx-10,0.04f,c.z+hz+8},{15,0.08f,24},{58,60,63,255});Box({c.x+hx+10,0.04f,c.z+hz+8},{15,0.08f,24},{58,60,63,255});
 // Hangar build floor and marked central construction zone.
 Box({c.x,0.03f,c.z},{hangar.size.x,0.06f,hangar.size.z},{47,51,55,255});Box({c.x,0.045f,c.z},{hangar.size.x-10,0.03f,hangar.size.z-14},{38,43,47,255});
 // Main shell with large vehicle opening plus animated personnel entrance.
 Box({c.x,h*0.5f,c.z-hz},{hangar.size.x,h,0.6f},{78,83,88,255});Box({c.x-hx,h*0.5f,c.z},{0.6f,h,hangar.size.z},{78,83,88,255});Box({c.x+hx,h*0.5f,c.z},{0.6f,h,hangar.size.z},{78,83,88,255});
 float mainDoor=garageWidth,px=doorOffsetX,hd=doorWidth*0.5f;auto wall=[&](float x0,float x1){if(x1>x0)Box({c.x+(x0+x1)*0.5f,h*0.5f,c.z+hz},{x1-x0,h,0.6f},{78,83,88,255});};
 wall(-hx,px-hd);wall(px+hd,-mainDoor*0.5f);wall(mainDoor*0.5f,hx);if(h>doorHeight)Box({c.x+px,doorHeight+(h-doorHeight)*0.5f,c.z+hz},{doorWidth,h-doorHeight,0.6f},{78,83,88,255});
 Vector3 hinge{c.x+px-doorWidth*0.5f,doorHeight*0.5f,c.z+hz-0.12f};DoorPanel(hinge,doorWidth,doorHeight,-doorOpenAngle*doorOpenFraction,{44,73,78,255});
 // Oversized two-sided personnel handle. The larger grip is deliberate so it
 // is readable/clickable at normal third-person camera distances.
 {
   auto spindle=PersonnelDoorHandleSpindlePosition();auto grip=PersonnelDoorHandlePosition();
   DrawCylinderEx(spindle,grip,0.060f,0.060f,14,{153,119,43,255});
   DrawSphere(spindle,0.125f,{194,153,58,255});DrawSphere(grip,0.145f,{224,184,77,255});
   float a=-doorOpenAngle*doorOpenFraction*DEG2RAD;
   Vector3 normal{sinf(a),0.0f,cosf(a)};
   Vector3 inner=Vector3Subtract(spindle,Vector3Scale(normal,0.34f));
   DrawCylinderEx(inner,spindle,0.050f,0.050f,12,{112,88,38,255});
   DrawSphere(inner,0.115f,{184,144,53,255});
 }
 // Completed front fascia and roof close the old open-top/light-leak gaps.
 Box({c.x,h+0.16f,c.z},{hangar.size.x+0.7f,0.32f,hangar.size.z+0.7f},{67,72,78,255});
 Box({c.x,h-0.38f,c.z+hz+0.03f},{hangar.size.x,0.76f,0.74f},{69,74,79,255});
 Box({c.x,garageHeight+garageLintelHeight*0.5f,c.z+hz-0.04f},{mainDoor+1.1f,garageLintelHeight,0.72f},{63,69,75,255});

 // Main segmented overhead garage door. Panels overlap by a few centimetres
 // so there are no visible daylight gaps between sections or side rails.
 Box({c.x-mainDoor*0.5f-garageSideSealWidth*0.5f,garageHeight*0.5f,c.z+hz-0.10f},{garageSideSealWidth,garageHeight+0.65f,0.38f},{39,45,51,255});
 Box({c.x+mainDoor*0.5f+garageSideSealWidth*0.5f,garageHeight*0.5f,c.z+hz-0.10f},{garageSideSealWidth,garageHeight+0.65f,0.38f},{39,45,51,255});
 float ph=garageHeight/(float)garagePanelCount;float travel=garageOpenFraction*(garageHeight+7.0f);
 for(int i=0;i<garagePanelCount;++i){
   float base=(i+0.5f)*ph;float path=base+travel;Vector3 gp{c.x,path,c.z+hz-0.16f};float pitch=0.0f;
   if(path>garageHeight-ph*0.5f){float over=path-(garageHeight-ph*0.5f);gp.y=garageHeight+0.20f;gp.z=(c.z+hz-0.16f)-std::min(over,7.0f);pitch=-90.0f;}
   GaragePanel(gp,mainDoor+garagePanelOverlap*0.8f,ph+garagePanelOverlap,pitch,{58,83,96,255});
 }
 // Access passlock mounted beside the left personnel door / garage wall.
 auto kp=PasslockPosition();Box(kp,{0.62f,0.96f,0.20f},{22,31,36,255});Box({kp.x,kp.y+0.25f,kp.z+0.12f},{0.43f,0.24f,0.05f},garageLocked?Color{160,48,48,255}:Color{45,150,92,255});for(int r=0;r<3;++r)for(int q=0;q<3;++q)Box({kp.x-0.16f+q*0.16f,kp.y+0.02f-r*0.15f,kp.z+0.13f},{0.07f,0.07f,0.04f},{105,118,124,255});
 // Interior wall control: press/click from inside to OPEN/CLOSE the main garage.
 {auto gp=GarageControlPosition();Box(gp,{0.66f,0.88f,0.20f},{24,29,32,255});Color lamp=garageOpenFraction>0.5f?Color{225,75,62,255}:Color{55,205,120,255};Box({gp.x,gp.y+0.17f,gp.z-0.12f},{0.34f,0.24f,0.07f},lamp);Box({gp.x,gp.y-0.19f,gp.z-0.12f},{0.38f,0.16f,0.07f},{95,105,110,255});}
 // Roof trusses plus a lower service layer remove the huge dead vertical gap
 // while preserving the full aircraft/large-vehicle build volume in the centre.
 for(float z=c.z-hz+5;z<c.z+hz;z+=10){
   Box({c.x,h-1,z},{hangar.size.x-1,0.35f,0.35f},{55,61,68,255});
   Box({c.x-hx*0.50f,h-4.2f,z},{0.28f,7.2f,0.28f},{61,67,73,255});
   Box({c.x+hx*0.50f,h-4.2f,z},{0.28f,7.2f,0.28f},{61,67,73,255});
 }
 // Side-wall structural columns, mezzanine/service gantries and guard rails.
 for(float z=c.z-hz+7;z<c.z+hz-7;z+=12){
   Box({c.x-hx+1.1f,7.0f,z},{0.55f,14.0f,0.55f},{56,62,68,255});
   Box({c.x+hx-1.1f,7.0f,z},{0.55f,14.0f,0.55f},{56,62,68,255});
 }
 Box({c.x-hx+3.2f,8.8f,c.z-5.0f},{4.6f,0.38f,hangar.size.z-18.0f},{66,72,78,255});
 Box({c.x+hx-3.2f,8.8f,c.z-5.0f},{4.6f,0.38f,hangar.size.z-18.0f},{66,72,78,255});
 Box({c.x-hx+5.2f,10.0f,c.z-5.0f},{0.18f,2.0f,hangar.size.z-18.0f},{112,120,124,255});
 Box({c.x+hx-5.2f,10.0f,c.z-5.0f},{0.18f,2.0f,hangar.size.z-18.0f},{112,120,124,255});
 // Back-wall catwalk, control room band and stacked storage silhouette.
 Box({c.x,9.0f,c.z-hz+2.2f},{hangar.size.x-8.0f,0.45f,3.2f},{64,71,77,255});
 Box({c.x,11.0f,c.z-hz+1.0f},{hangar.size.x-10.0f,3.2f,1.0f},{53,63,70,255});
 for(int i=-4;i<=4;++i) { unsigned char alt=(unsigned char)(i&1); Box({c.x+i*5.2f,4.0f,c.z-hz+2.0f},{3.6f,7.2f,2.2f},{(unsigned char)(68+alt*6),(unsigned char)(75+alt*5),(unsigned char)(80+alt*4),255}); }
 // Interior stations.
 Box({c.x-hx+4,1,c.z-hz+10},{2.5f,2,8},{90,75,58,255});Box({c.x+hx-3,2,c.z-hz+14},{3,4,14},{70,76,82,255});Box({c.x+hx-7,2.5f,c.z+hz-15},{9,5,10},{62,72,78,255});Box({c.x-hx+5,1,c.z+hz-14},{3,2,9},{80,70,60,255});
 // Vehicle lift, engine stands, tyre racks, welding/electronics/terminals.
 Box({c.x-15,0.18f,c.z-12},{8,0.35f,14},{180,150,45,255});Box({c.x+17,1,c.z-18},{2,2,2},{110,65,45,255});Box({c.x+hx-4,1.4f,c.z+2},{2,2.8f,10},{35,35,38,255});Box({c.x-hx+4,1,c.z+2},{3,2,4},{50,85,95,255});Box({c.x-hx+4,1,c.z+10},{3,2,4},{45,70,88,255});
 // More side storage/parts islands, leaving the central build lane clear.
 for(int i=0;i<5;++i){float z=c.z-hz+18.0f+i*12.0f;Box({c.x-hx+7.2f,2.1f,z},{4.2f,4.2f,5.0f},{58,67,72,255});Box({c.x+hx-7.2f,2.1f,z+4.0f},{4.2f,4.2f,5.0f},{62,69,74,255});}
 // Overhead crane rails, suspended service ducts and lower work lights.
 Box({c.x-10,h-3,c.z},{0.4f,0.4f,hangar.size.z-6},{230,180,35,255});Box({c.x+10,h-3,c.z},{0.4f,0.4f,hangar.size.z-6},{230,180,35,255});
 Box({c.x-18,17.5f,c.z-3},{1.15f,1.15f,hangar.size.z-16},{47,55,60,255});Box({c.x+18,17.5f,c.z-3},{1.15f,1.15f,hangar.size.z-16},{47,55,60,255});
 for(float z=c.z-hz+8;z<c.z+hz-8;z+=12){Box({c.x,18.2f,z},{15,0.18f,0.55f},{230,235,220,255});Box({c.x,24.0f,z},{0.12f,11.5f,0.12f},{72,78,82,255});}
 // Service/fuel blocks outside. The oversized teal front sign/box was removed.
 Box({c.x+hx+10,1.2f,c.z+hz+6},{2,2.4f,4},{170,70,50,255});Box({c.x+hx+13,1.2f,c.z+hz+6},{2,2.4f,4},{55,120,80,255});
 // Existing prototype world destinations/test blocks.
 Box({0,0.04f,85},{12,0.08f,30},{45,45,48,255});Box({38,0.04f,95},{76,0.08f,12},{45,45,48,255});Box({76,0.04f,63},{12,0.08f,50},{45,45,48,255});Box({32,1,25},{10,2,8},BROWN);Box({15,3,95},{25,6,18},GRAY);Box({92,3,82},{18,6,22},BROWN);DrawCylinder(pickup,3,3,0.25f,24,YELLOW);DrawCylinder(dropoff,3,3,0.25f,24,GREEN);
}
Vector2 World::ToMap(Vector3 p,Rectangle r)const{float nx=(p.x+150)/300,nz=(p.z+150)/300;return{r.x+nx*r.width,r.y+r.height-nz*r.height};}
void World::DrawMap(Vector3 pos,Vector3 gps,const char*label)const{Rectangle b{50,50,(float)GetScreenWidth()-100,(float)GetScreenHeight()-100};DrawRectangleRec(b,{18,22,28,245});DrawRectangleLinesEx(b,3,SKYBLUE);DrawText("WORLD MAP",75,70,30,RAYWHITE);Rectangle m{80,120,b.width-60,b.height-210};DrawRectangleRec(m,{35,45,45,255});DrawRectangleLinesEx(m,2,GRAY);auto pt=[&](Vector3 p,Color col,const char*s){Vector2 q=ToMap(p,m);DrawCircleV(q,6,col);DrawText(s,(int)q.x+10,(int)q.y-8,16,RAYWHITE);};pt(workshop,SKYBLUE,"Engineering Hangar");pt(pickup,YELLOW,"Cargo");pt(dropoff,GREEN,"Delivery");pt(pos,WHITE,"YOU");if(label&&label[0]){Vector2 a=ToMap(pos,m),z=ToMap(gps,m);DrawLineEx(a,z,3,YELLOW);DrawText(TextFormat("GPS: %s",label),85,GetScreenHeight()-105,20,YELLOW);}DrawText("M close map",GetScreenWidth()-200,GetScreenHeight()-105,20,RAYWHITE);}
