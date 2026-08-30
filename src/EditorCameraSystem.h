#pragma once
#include "raylib.h"
#include <VekGameSystems.h>
class EditorCameraSystem {
public:
 void Enter(Vector3 focus);void Exit();bool Active()const{return active;}
 void ConfigureWorkspaceBounds(Vector3 center,Vector3 size,float maxBuildHeight,float padding=1.0f);
 void ConfigureProfile(const vek::CameraProfileDefinition& p);
 void Update(float dt);void Apply(Camera3D& camera)const;void Focus(Vector3 point,float distance=9.0f);void FocusVehicle(Vector3 center,Vector3 size);
 void Front(Vector3 center,float distance=18);void Rear(Vector3 center,float distance=18);void Left(Vector3 center,float distance=18);void Right(Vector3 center,float distance=18);void Top(Vector3 center,float distance=22);void Bottom(Vector3 center,float distance=18);void ToggleOrthographic();
 float Speed()const{return moveSpeed;}bool Orthographic()const{return orthographic;}
private:
 bool active=false,orthographic=false,boundsEnabled=false;
 Vector3 position{15,12,-18};Vector3 boundsCenter{0,0,0},boundsSize{60,30,90};
 float boundsPadding=1.0f,boundsMaxHeight=30.0f;
 float yaw=35,pitch=-22,targetYaw=35,targetPitch=-22,moveSpeed=12,focusDistance=10;
 vek::CameraProfileDefinition profile;
 void AimAt(Vector3 target);void ClampToWorkspace();Vector3 Forward()const;Vector3 RightVec()const;
};
