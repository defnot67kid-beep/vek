#pragma once
#include "raylib.h"
#include <VekEditorSystems.h>
#include <string>
#include <vector>

struct WorldCollisionBox{int id=0;std::string name;Vector3 center{0,0,0};Vector3 size{1,1,1};std::string tag="world";};
class World{
public:
 World();
 Vector3 workshop{0,0,0};Vector3 pickup{0,0.4f,88};Vector3 dropoff{78,0.4f,85};
 void ConfigureHangar(const vek::HangarBuildArea& area);const vek::HangarBuildArea& HangarArea()const{return hangar;}
 void ConfigurePersonnelDoor(float offsetX,float width,float height,float openAngleDegrees);
 void SetPersonnelDoorTarget(bool open);bool UpdatePersonnelDoor(float dt,float speed);
 bool SetPersonnelDoorTraversalCollisionDisabled(bool disabled);
 bool PersonnelDoorTraversalCollisionDisabled()const{return doorTraversalCollisionDisabled;}
 bool PersonnelDoorIsOpen()const{return doorOpenFraction>=0.92f;}float PersonnelDoorOpenFraction()const{return doorOpenFraction;}
 Vector3 PersonnelDoorPosition()const;Vector3 PersonnelDoorApproachPoint()const;Vector3 PersonnelDoorInsidePoint()const;
 Vector3 PersonnelDoorHandlePosition()const;Vector3 PersonnelDoorHandleSpindlePosition()const;BoundingBox PersonnelDoorHandleBounds()const;
 void SetPersonnelDoorHandleTurn(float normalized);float PersonnelDoorHandleTurn()const{return doorHandleTurn;}

 void ConfigureGarageDoor(float width,float height,int panelCount,float passlockOffsetX,float passlockHeight,float panelOverlap=0.025f,float sideSealWidth=0.34f,float lintelHeight=0.68f,float collisionClearFraction=0.60f);
 void SetGarageDoorState(float openFraction,bool locked);
 float GarageDoorOpenFraction()const{return garageOpenFraction;}bool GarageDoorIsOpen()const{return garageOpenFraction>=0.92f;}bool GarageDoorLocked()const{return garageLocked;}
 Vector3 GarageDoorPosition()const;Vector3 GarageDoorApproachPoint()const;Vector3 GarageDoorInsidePoint()const;
 Vector3 GarageControlPosition()const;BoundingBox GarageControlBounds()const;
 bool IsInsideHangar(Vector3 position,float margin=0.0f)const;
 bool IsInsideGarageSide(Vector3 position)const;
 bool IsOutsideGarageSide(Vector3 position)const;
 bool IsNearGarageOpening(Vector3 position,float distance)const;
 Vector3 PasslockPosition()const;BoundingBox PasslockBounds()const;

 void Draw()const;void DrawMap(Vector3 pos,Vector3 gps,const char*label)const;const std::vector<WorldCollisionBox>&CollisionBoxes()const;
private:
 vek::HangarBuildArea hangar;std::vector<WorldCollisionBox>collisionBoxes;
 float doorOffsetX=-21.0f,doorWidth=2.4f,doorHeight=3.0f,doorOpenAngle=95.0f,doorOpenFraction=0.0f;bool doorTargetOpen=false,doorCollisionOpen=false;
 float doorHandleTurn=0.0f; // 0..1, mapped to a visible downward handle rotation.
 bool doorTraversalCollisionDisabled=false; // temporary doorway ghosting only while scripted traversal owns the player.
 float garageWidth=24.0f,garageHeight=8.5f,garageOpenFraction=0.0f;int garagePanelCount=9;bool garageLocked=true;
 float garagePanelOverlap=0.025f,garageSideSealWidth=0.34f,garageLintelHeight=0.68f,garageCollisionClearFraction=0.60f;
 float passlockOffsetX=-18.4f,passlockHeight=1.45f;
 void RebuildCollision();Vector2 ToMap(Vector3 p,Rectangle r)const;
};
