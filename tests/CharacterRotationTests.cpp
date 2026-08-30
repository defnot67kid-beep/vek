#include "CharacterRotationSystem.h"
#include "CameraRotationSystem.h"
#include <cassert>
#include <cmath>
#include <iostream>

static bool Near(float a,float b,float eps=0.75f){return std::fabs(CharacterRotationSystem::DeltaAngle(a,b))<=eps;}

static CharacterRotationSystem SimulateTurn(float fps, CharacterMovementState state, Vector3 direction, float seconds) {
    CharacterRotationSystem r;
    r.SnapBodyYaw(0.0f);
    const float dt=1.0f/fps;
    int steps=(int)std::round(seconds*fps);
    for(int i=0;i<steps;i++) r.Update(dt,direction,1.0f,0.0f,state);
    return r;
}

int main(){
    // Angle wrapping / shortest path.
    assert(Near(CharacterRotationSystem::NormalizeAngle(-1.0f),359.0f,0.001f));
    assert(std::fabs(CharacterRotationSystem::DeltaAngle(359.0f,0.0f)-1.0f)<0.001f);
    assert(std::fabs(CharacterRotationSystem::DeltaAngle(350.0f,10.0f)-20.0f)<0.001f);
    assert(std::fabs(CharacterRotationSystem::DeltaAngle(10.0f,350.0f)+20.0f)<0.001f);
    assert(Near(CharacterRotationSystem::MoveTowardsAngle(350.0f,10.0f,5.0f),355.0f,0.001f));

    // Deadzone: tiny input must not change facing target.
    CharacterRotationSystem deadzone;
    deadzone.SnapBodyYaw(0.0f);
    deadzone.Update(1.0f/60.0f,{1,0,0},0.05f,0.0f,CharacterMovementState::Run);
    assert(Near(deadzone.GetTargetYaw(),0.0f,0.001f));

    // Normal direction changes accelerate instead of snapping.
    CharacterRotationSystem responsive;
    responsive.SnapBodyYaw(0.0f);
    responsive.Update(1.0f/60.0f,{1,0,0},1.0f,0.0f,CharacterMovementState::Run);
    assert(responsive.GetBodyYaw()>0.0f && responsive.GetBodyYaw()<12.0f);
    assert(std::fabs(responsive.GetAngularVelocity())<=responsive.settings.runRotationSpeed+0.01f);

    // Heavy carry must visibly rotate slower than a run.
    auto runTurn=SimulateTurn(60.0f,CharacterMovementState::Run,{1,0,0},0.25f);
    auto heavyTurn=SimulateTurn(60.0f,CharacterMovementState::HeavyCarry,{1,0,0},0.25f);
    assert(runTurn.GetBodyYaw()>heavyTurn.GetBodyYaw()+5.0f);

    // Stationary camera orbit has a safe range before turn-in-place begins.
    CharacterRotationSystem turnInPlace;
    turnInPlace.SnapBodyYaw(0.0f);
    turnInPlace.Update(1.0f/60.0f,{0,0,0},0.0f,50.0f,CharacterMovementState::Idle);
    assert(Near(turnInPlace.GetTargetYaw(),0.0f,0.001f));
    turnInPlace.Update(1.0f/60.0f,{0,0,0},0.0f,85.0f,CharacterMovementState::Idle);
    assert(Near(turnInPlace.GetTargetYaw(),85.0f,0.001f));

    // First-person body follows only after the smaller first-person threshold.
    CharacterRotationSystem firstPerson;
    firstPerson.SnapBodyYaw(0.0f);
    firstPerson.SetFirstPerson(true);
    firstPerson.Update(1.0f/60.0f,{0,0,0},0.0f,45.0f,CharacterMovementState::Idle);
    assert(Near(firstPerson.GetTargetYaw(),0.0f,0.001f));
    firstPerson.Update(1.0f/60.0f,{0,0,0},0.0f,65.0f,CharacterMovementState::Idle);
    assert(Near(firstPerson.GetTargetYaw(),65.0f,0.001f));

    // Interaction target creates a smooth locked facing request.
    CharacterRotationSystem interaction;
    interaction.SnapBodyYaw(0.0f);
    interaction.FaceTarget({0,0,0},{-10,0,0});
    assert(interaction.GetRotationMode()==CharacterMovementRotationMode::LockedDirection);
    interaction.Update(1.0f/60.0f,{0,0,0},0.0f,0.0f,CharacterMovementState::Repairing);
    assert(interaction.GetBodyYaw()>340.0f || interaction.GetBodyYaw()<0.01f); // starts turning shortest-way left toward 270.

    // Seated body is owned by the vehicle heading.
    CharacterRotationSystem seated;
    seated.SnapBodyYaw(15.0f);
    seated.SetSeatedYaw(123.0f);
    seated.Update(1.0f/60.0f,{0,0,0},0.0f,160.0f,CharacterMovementState::Seated);
    assert(Near(seated.GetBodyYaw(),123.0f,0.001f));

    // Free-camera style camera influence must not rotate a stationary body.
    CharacterRotationSystem freeCam;
    freeCam.SnapBodyYaw(33.0f);
    freeCam.SetCameraInfluenceEnabled(false);
    for(int i=0;i<120;i++) freeCam.Update(1.0f/60.0f,{0,0,0},0.0f,220.0f,CharacterMovementState::Idle);
    assert(Near(freeCam.GetBodyYaw(),33.0f,0.001f));

    // Frame-rate consistency: same one-second turn at common frame rates.
    auto r30=SimulateTurn(30.0f,CharacterMovementState::Run,{1,0,0},1.0f);
    auto r60=SimulateTurn(60.0f,CharacterMovementState::Run,{1,0,0},1.0f);
    auto r120=SimulateTurn(120.0f,CharacterMovementState::Run,{1,0,0},1.0f);
    auto r144=SimulateTurn(144.0f,CharacterMovementState::Run,{1,0,0},1.0f);
    assert(Near(r30.GetBodyYaw(),90.0f));
    assert(Near(r60.GetBodyYaw(),90.0f));
    assert(Near(r120.GetBodyYaw(),90.0f));
    assert(Near(r144.GetBodyYaw(),90.0f));


    // Camera target/current separation: one frame must not snap to a large target change.
    CameraRotationSystem cameraSmooth;
    cameraSmooth.Snap(0.0f, 18.0f);
    cameraSmooth.SetTarget(90.0f, 35.0f);
    cameraSmooth.Update(1.0f/60.0f);
    assert(cameraSmooth.GetYaw()>0.0f && cameraSmooth.GetYaw()<20.0f);
    assert(cameraSmooth.GetPitch()>18.0f && cameraSmooth.GetPitch()<35.0f);

    // Camera yaw also uses the shortest path across 359/0.
    CameraRotationSystem cameraWrap;
    cameraWrap.Snap(359.0f, 0.0f);
    cameraWrap.SetTarget(1.0f, 0.0f);
    for(int i=0;i<20;i++) cameraWrap.Update(1.0f/60.0f);
    assert(std::fabs(CameraRotationSystem::DeltaAngle(cameraWrap.GetYaw(),1.0f))<0.25f);

    // Pitch limits and cockpit-style yaw clamps are respected.
    CameraRotationSystem cameraClamp;
    cameraClamp.SetYawClamp(true,-90.0f,90.0f);
    cameraClamp.SetPitchLimits(-45.0f,55.0f);
    cameraClamp.Snap(0.0f,0.0f);
    cameraClamp.SetTarget(200.0f,100.0f);
    for(int i=0;i<120;i++) cameraClamp.Update(1.0f/60.0f);
    assert(std::fabs(cameraClamp.GetYaw()-90.0f)<0.1f);
    assert(std::fabs(cameraClamp.GetPitch()-55.0f)<0.1f);

    // User clarification: alignment means the PHYSICAL arrow keys, not
    // the < and > characters. Mapping remains intentionally inverted.
    assert(CameraRotationSystem::AlignmentDirectionForArrowKey(KEY_LEFT)==+1);
    assert(CameraRotationSystem::AlignmentDirectionForArrowKey(KEY_RIGHT)==-1);
    assert(CameraRotationSystem::AlignmentDirectionForArrowKey(KEY_COMMA)==0);
    assert(CameraRotationSystem::AlignmentDirectionForArrowKey(KEY_PERIOD)==0);
    assert(CameraRotationSystem::AlignmentDirectionForArrowKey(KEY_LEFT_BRACKET)==0);
    assert(CameraRotationSystem::AlignmentDirectionForArrowKey(KEY_RIGHT_BRACKET)==0);

    // Camera alignment keys move to clean 45-degree world brackets while
    // retaining the normal smooth target/current camera motion.
    CameraRotationSystem alignKeys;
    alignKeys.Snap(10.0f,18.0f);
    alignKeys.AlignYawStep(+1,45.0f);
    assert(std::fabs(CameraRotationSystem::DeltaAngle(alignKeys.GetTargetYaw(),45.0f))<0.001f);
    alignKeys.AlignYawStep(+1,45.0f);
    assert(std::fabs(CameraRotationSystem::DeltaAngle(alignKeys.GetTargetYaw(),90.0f))<0.001f);
    alignKeys.AlignYawStep(-1,45.0f);
    assert(std::fabs(CameraRotationSystem::DeltaAngle(alignKeys.GetTargetYaw(),45.0f))<0.001f);

    CameraRotationSystem alignWrap;
    alignWrap.Snap(350.0f,18.0f);
    alignWrap.AlignYawStep(+1,45.0f);
    assert(std::fabs(CameraRotationSystem::DeltaAngle(alignWrap.GetTargetYaw(),0.0f))<0.001f);
    alignWrap.AlignYawStep(-1,45.0f);
    assert(std::fabs(CameraRotationSystem::DeltaAngle(alignWrap.GetTargetYaw(),315.0f))<0.001f);

    // Camera smoothing is approximately frame-rate independent.
    auto simulateCamera=[](float fps){
        CameraRotationSystem c;
        c.Snap(0.0f,18.0f);
        c.SetTarget(135.0f,40.0f);
        float dt=1.0f/fps;
        int steps=(int)std::round(fps*1.0f);
        for(int i=0;i<steps;i++) c.Update(dt);
        return c;
    };
    auto c30=simulateCamera(30.0f);
    auto c60=simulateCamera(60.0f);
    auto c120=simulateCamera(120.0f);
    auto c144=simulateCamera(144.0f);
    assert(std::fabs(CameraRotationSystem::DeltaAngle(c30.GetYaw(),135.0f))<0.5f);
    assert(std::fabs(CameraRotationSystem::DeltaAngle(c60.GetYaw(),135.0f))<0.5f);
    assert(std::fabs(CameraRotationSystem::DeltaAngle(c120.GetYaw(),135.0f))<0.5f);
    assert(std::fabs(CameraRotationSystem::DeltaAngle(c144.GetYaw(),135.0f))<0.5f);

    std::cout << "Character + Camera rotation tests: PASS\n";
    return 0;
}
