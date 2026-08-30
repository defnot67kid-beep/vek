// v26 note: animated door interaction, richer character rig, fuller hair, day/night cycle, and dev cheat panel groundwork.
#include "Character.h"
#include "raymath.h"
#include "rlgl.h"
#include "AvatarSaveSystem.h"
#include "ClothingSystem.h"
#include "CharacterCustomizationSystem.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

static float Approach(float current, float target, float speed, float dt) {
    float t = Clamp(speed * dt, 0.0f, 1.0f);
    return Lerp(current, target, t);
}

void CharacterAnimationSystem::SetState(CharacterAnimState next) {
    if (state == next) return;
    previous = state;
    state = next;
    stateTime = 0.0f;
    blend = 0.0f;
}


void CharacterAnimationSystem::ApplyRotationData(const CharacterRotationSystem& rotation, float speed, bool sprintingValue, bool crouchingValue, bool carryingValue, bool repairingValue, bool seatedValue) {
    movementSpeed = speed;
    movementDirection = rotation.GetMovementDirection();
    turnAngle = rotation.GetAngleDifference();
    rotationAngularVelocity = rotation.GetAngularVelocity();
    headYawOffset = rotation.GetHeadYawOffset();
    upperBodyYawOffset = rotation.GetUpperBodyYawOffset();
    isTurning = rotation.IsTurning();
    isMoving = rotation.GetMovementMagnitude() >= rotation.settings.movementDeadzone;
    isSprinting = sprintingValue;
    isCrouching = crouchingValue;
    isCarrying = carryingValue;
    isRepairing = repairingValue;
    isSeated = seatedValue;
    turnState = rotation.GetTurnState();
}

void CharacterAnimationSystem::ApplyHumanoidData(const HumanoidSystem& humanoid) {
    airborneVerticalVelocity = humanoid.GetVerticalVelocity();
    airborneTime = humanoid.GetAirborneTime();
    lastLandingSpeed = humanoid.GetLastLandingSpeed();
}

void CharacterAnimationSystem::Update(float dt, float movementAmount) {
    stateTime += dt;
    blend = Clamp(blend + dt * 7.5f, 0.0f, 1.0f);

    float cycleSpeed = 0.0f;
    float armAmp = 0.0f;
    float legAmp = 0.0f;
    float targetCrouch = 0.0f;
    float targetLean = 0.0f;
    float targetRaise = 0.0f;
    float targetKnee = 0.0f;
    float targetTorsoTwist = 0.0f;
    float targetPelvisTwist = 0.0f;
    float targetRootBob = 0.0f;
    float targetShoulderBob = 0.0f;
    float targetLeftLift = 0.0f;
    float targetRightLift = 0.0f;
    float targetJumpTuck = 0.0f;
    float targetLandingCompression = 0.0f;

    switch (state) {
        case CharacterAnimState::Walk:
            cycleSpeed=5.4f; armAmp=25; legAmp=29; break;
        case CharacterAnimState::Run:
            cycleSpeed=8.8f; armAmp=44; legAmp=49; targetLean=8; break;
        case CharacterAnimState::Sprint:
            cycleSpeed=11.4f; armAmp=58; legAmp=63; targetLean=16; break;
        case CharacterAnimState::Crouch:
            targetCrouch=0.45f; targetKnee=35; break;
        case CharacterAnimState::TurnInPlace:
            targetKnee=5; targetLean=2; break;
        case CharacterAnimState::Jump:
            // The jump pose is driven by actual vertical velocity rather than a
            // single frozen mannequin pose. Rise -> apex -> fall all read differently.
            break;
        case CharacterAnimState::Land:
            targetCrouch=0.22f; targetKnee=22; targetLean=7; break;
        case CharacterAnimState::Pickup: targetCrouch=0.35f; targetLean=28; targetRaise=-35; break;
        case CharacterAnimState::Place: targetLean=18; targetRaise=-18; break;
        case CharacterAnimState::Push: targetLean=24; targetRaise=68; break;
        case CharacterAnimState::Pull: targetLean=-10; targetRaise=72; break;
        case CharacterAnimState::RotatePart: targetRaise=48; break;
        case CharacterAnimState::Repair: targetCrouch=0.2f; targetLean=18; targetRaise=52; break;
        case CharacterAnimState::UseTool: targetRaise=60; break;
        case CharacterAnimState::Menu: targetRaise=38; break;
        case CharacterAnimState::EnterVehicle: targetCrouch=0.22f; targetLean=15; targetKnee=40; break;
        case CharacterAnimState::ExitVehicle: targetCrouch=0.18f; targetKnee=30; break;
        case CharacterAnimState::OpenDoor: targetLean=5; targetRaise=68; targetTorsoTwist=12; targetKnee=4; break;
        case CharacterAnimState::UseKeypad: targetLean=4; targetRaise=78; targetTorsoTwist=8; targetKnee=3; break;
        case CharacterAnimState::SeatedDrive: targetCrouch=0.55f; targetRaise=76; targetKnee=68; break;
        case CharacterAnimState::CarryMedium: targetRaise=52; targetLean=3; break;
        case CharacterAnimState::CarryHeavy: targetRaise=72; targetLean=18; targetKnee=12; cycleSpeed=2.8f; armAmp=3; legAmp=15; break;
        case CharacterAnimState::Inspect: targetLean=12; targetRaise=30; break;
        case CharacterAnimState::KneelRepair: targetCrouch=0.62f; targetKnee=78; targetLean=20; targetRaise=48; break;
        case CharacterAnimState::Celebrate: targetRaise=125 + sinf(stateTime*6.0f)*12; break;
        case CharacterAnimState::Hurt: targetLean=12; targetCrouch=0.12f; cycleSpeed=3.0f; armAmp=12; legAmp=18; break;
        default: break;
    }

    // Cadence follows actual movement speed, so running does not look like feet
    // are cycling at the same rate at every velocity.
    float locomotionSpeedScale = Clamp(movementSpeed / 6.0f, 0.72f, 1.45f);
    if (state == CharacterAnimState::Walk) locomotionSpeedScale = Clamp(movementSpeed / 3.3f, 0.72f, 1.25f);
    if (state == CharacterAnimState::Sprint) locomotionSpeedScale = Clamp(movementSpeed / 9.2f, 0.80f, 1.35f);
    if (cycleSpeed > 0.0f)
        gait += dt * cycleSpeed * locomotionSpeedScale * Clamp(movementAmount + 0.25f, 0.35f, 1.2f);

    float s = sinf(gait);
    float c = cosf(gait);
    // VEK 2.2 rig convention: +pitch points an arm toward the character's
    // visual forward axis (+Z). Arms must counter-swing the opposite leg; the
    // old signs made same-side arms/legs move together and read as backwards.
    float targetLA = s * armAmp;
    float targetRA = -s * armAmp;
    float targetLL = -s * legAmp;
    float targetRL = s * legAmp;

    // A real runner counter-rotates chest and pelvis. The extra turn term makes
    // the chest visibly participate in direction changes instead of remaining
    // an axis-aligned block while only the limbs rotate around it.
    float gaitTwist = 0.0f;
    if (state == CharacterAnimState::Walk) gaitTwist = -s * 3.5f;
    else if (state == CharacterAnimState::Run) gaitTwist = -s * 8.0f;
    else if (state == CharacterAnimState::Sprint) gaitTwist = -s * 11.5f;
    else if (state == CharacterAnimState::CarryHeavy) gaitTwist = -s * 2.0f;

    float turnVelocityNormalized = Clamp(rotationAngularVelocity / 450.0f, -1.0f, 1.0f);
    float turnTwist = turnVelocityNormalized * (isSprinting ? 7.0f : 5.0f);
    targetTorsoTwist = gaitTwist + turnTwist;
    targetPelvisTwist = -gaitTwist * 0.42f + turnTwist * 0.18f;

    if (state == CharacterAnimState::Walk || state == CharacterAnimState::Run || state == CharacterAnimState::Sprint) {
        float bobAmp = state == CharacterAnimState::Walk ? 0.018f : (state == CharacterAnimState::Run ? 0.035f : 0.050f);
        targetRootBob = (0.5f - 0.5f*cosf(gait*2.0f)) * bobAmp;
        targetShoulderBob = c * (state == CharacterAnimState::Sprint ? 0.035f : 0.020f);
        float liftAmp = state == CharacterAnimState::Walk ? 0.055f : (state == CharacterAnimState::Run ? 0.10f : 0.135f);
        targetLeftLift = std::max(0.0f, -s) * liftAmp;
        targetRightLift = std::max(0.0f, s) * liftAmp;
    }

    // Tight turns shorten the overall stride. The inside leg shortens slightly
    // more, which makes circular running look less like sideways skating.
    float turnStrength = Clamp(fabsf(turnAngle) / 180.0f, 0.0f, 1.0f);
    float strideScale = 1.0f - turnStrength * 0.30f;
    float turnDirection = Clamp(rotationAngularVelocity / 450.0f, -1.0f, 1.0f);
    float leftStrideScale = strideScale * (1.0f + turnDirection * 0.12f);
    float rightStrideScale = strideScale * (1.0f - turnDirection * 0.12f);
    targetLL *= leftStrideScale;
    targetRL *= rightStrideScale;
    targetLA *= strideScale;
    targetRA *= strideScale;

    float targetFootYaw = Clamp(turnAngle * 0.35f, -25.0f, 25.0f);
    leftFootYawOffset = Approach(leftFootYawOffset, targetFootYaw, 8.0f, dt);
    rightFootYawOffset = Approach(rightFootYawOffset, targetFootYaw, 8.0f, dt);

    if (state == CharacterAnimState::Jump) {
        float rise = Clamp(airborneVerticalVelocity / 6.4f, 0.0f, 1.0f);
        float fall = Clamp(-airborneVerticalVelocity / 9.0f, 0.0f, 1.0f);
        float apex = 1.0f - Clamp(fabsf(airborneVerticalVelocity) / 6.4f, 0.0f, 1.0f);

        targetJumpTuck = Clamp(apex * 0.78f + fall * 0.18f, 0.0f, 1.0f);
        targetKnee = 12.0f + targetJumpTuck * 34.0f;
        targetLean = -4.0f*rise + 7.0f*fall;
        // Keep airborne arms on the forward side of the shoulder instead of
        // folding behind the back.
        targetLA = 16.0f + apex*22.0f - fall*12.0f;
        targetRA = 16.0f + apex*22.0f - fall*12.0f;
        targetLL = 8.0f + apex*10.0f;
        targetRL = 8.0f + apex*10.0f;
        targetTorsoTwist = Approach(torsoYawTwist, 0.0f, 4.0f, dt);
        targetPelvisTwist = Approach(pelvisYawTwist, 0.0f, 4.0f, dt);
        targetLeftLift = 0.06f + targetJumpTuck*0.08f;
        targetRightLift = 0.06f + targetJumpTuck*0.08f;
    }

    if (state == CharacterAnimState::Land) {
        float impact = Clamp(lastLandingSpeed / 11.0f, 0.0f, 1.0f);
        float recovery = Clamp(1.0f - stateTime / 0.48f, 0.0f, 1.0f);
        targetLandingCompression = impact * recovery * 0.22f;
        targetCrouch += impact * recovery * 0.22f;
        targetKnee += impact * recovery * 18.0f;
        targetRootBob = -impact * recovery * 0.055f;
    }

    if (state == CharacterAnimState::SeatedDrive) {
        // Positive pitch is forward: hands reach toward the wheel/dashboard.
        targetLA = 72; targetRA = 72; targetLL = 55; targetRL = 55;
        targetTorsoTwist *= 0.2f;
        targetPelvisTwist = 0.0f;
    }
    if (state == CharacterAnimState::Celebrate) {
        targetLA = targetRaise; targetRA = targetRaise; targetLL = 0; targetRL = 0;
    }

    leftArmSwing = Approach(leftArmSwing, targetLA, 12, dt);
    rightArmSwing = Approach(rightArmSwing, targetRA, 12, dt);
    leftLegSwing = Approach(leftLegSwing, targetLL, 12, dt);
    rightLegSwing = Approach(rightLegSwing, targetRL, 12, dt);
    crouch = Approach(crouch, targetCrouch, 11, dt);
    torsoLean = Approach(torsoLean, targetLean, 10, dt);
    handRaise = Approach(handRaise, targetRaise, 10, dt);
    kneeBend = Approach(kneeBend, targetKnee, 12, dt);
    torsoYawTwist = Approach(torsoYawTwist, targetTorsoTwist, 11, dt);
    pelvisYawTwist = Approach(pelvisYawTwist, targetPelvisTwist, 10, dt);
    rootBob = Approach(rootBob, targetRootBob, 14, dt);
    shoulderBob = Approach(shoulderBob, targetShoulderBob, 14, dt);
    leftFootLift = Approach(leftFootLift, targetLeftLift, 16, dt);
    rightFootLift = Approach(rightFootLift, targetRightLift, 16, dt);
    jumpTuck = Approach(jumpTuck, targetJumpTuck, 10, dt);
    landingCompression = Approach(landingCompression, targetLandingCompression, 15, dt);
}

void EquipmentSystem::Next() {
    int v = ((int)selected + 1) % 9;
    selected = (EquipmentType)v;
}

const char* EquipmentSystem::Name() const {
    switch (selected) {
        case EquipmentType::Wrench: return "Wrench";
        case EquipmentType::Hammer: return "Hammer";
        case EquipmentType::Welder: return "Welding Tool";
        case EquipmentType::Scanner: return "Repair Scanner";
        case EquipmentType::Tablet: return "Job Tablet";
        case EquipmentType::Flashlight: return "Flashlight";
        case EquipmentType::FuelCan: return "Fuel Can";
        case EquipmentType::TowStrap: return "Tow Strap";
        case EquipmentType::GPS: return "GPS Device";
    }
    return "Tool";
}

Vector3 PlayerCharacterSystem::RotateY(Vector3 p, float yaw) const {
    float r = yaw * DEG2RAD;
    float c = cosf(r), s = sinf(r);
    return {p.x*c + p.z*s, p.y, -p.x*s + p.z*c};
}

Color PlayerCharacterSystem::SkinColor() const {
    static const Color tones[] = {
        {255,224,194,255},{244,201,164,255},{224,172,130,255},{198,134,91,255},
        {168,105,68,255},{132,78,50,255},{98,56,39,255},{67,39,29,255}
    };
    return tones[std::clamp(appearance.skinTone,0,7)];
}

Color PlayerCharacterSystem::HairColor() const {
    static const Color colors[] = {
        {28,20,18,255},{57,38,26,255},{92,58,32,255},{151,92,52,255},
        {209,171,111,255},{108,74,118,255},{45,63,72,255},{188,188,188,255}
    };
    return colors[std::clamp(appearance.hairColor,0,7)];
}

Color PlayerCharacterSystem::ClothingColor() const {
    static const Color colors[] = {
        {43,82,116,255},{63,72,76,255},{114,64,44,255},{36,104,92,255},
        {128,53,62,255},{184,120,34,255},{85,78,124,255},{196,196,190,255}
    };
    return colors[std::clamp(appearance.clothingColor,0,7)];
}

void PlayerCharacterSystem::Trigger(CharacterAnimState s) { animation.SetState(s); }
void PlayerCharacterSystem::ToggleCreator() { creatorOpen = !creatorOpen; }
void PlayerCharacterSystem::CycleCamera() { firstPerson = !firstPerson; }
void PlayerCharacterSystem::NextEquipment() { equipment.Next(); animation.SetState(CharacterAnimState::UseTool); }

void PlayerCharacterSystem::TriggerRagdoll(float impactSpeed, float directionSign, float durationOverride) {
    float strength=rigDefinition.id.empty()?1.0f:rigDefinition.globalRagdollStrength;
    vek::RagdollSystem::Trigger(ragdoll, impactSpeed*strength, directionSign, ragdollSettings);
    if (durationOverride > 0.0f) ragdoll.targetDuration = durationOverride;
    animation.SetState(CharacterAnimState::Hurt);
}

void PlayerCharacterSystem::UpdateRagdoll(float dt) {
    vek::RagdollSettings tuned=ragdollSettings;
    if(!rigDefinition.id.empty()){
        float flexibility=std::clamp((rigDefinition.spineFlex+rigDefinition.neckFlex+rigDefinition.limbFlex)/3.0f,0.2f,2.5f);
        tuned.angularDamping=std::clamp(tuned.angularDamping/(0.65f+0.35f*flexibility),1.0f,12.0f);
        tuned.recoveryDuration=std::clamp(tuned.recoveryDuration*(0.85f+0.25f*flexibility),0.25f,2.5f);
    }
    vek::RagdollSystem::Update(ragdoll, dt, tuned);
}

bool PlayerCharacterSystem::IsRagdollActive() const {
    return vek::RagdollSystem::Active(ragdoll);
}

void PlayerCharacterSystem::SetArmPhysicsTarget(bool rightArm, Vector3 targetWorld, float grip) {
    ArmPhysicsConstraint& c = rightArm ? rightArmPhysics : leftArmPhysics;
    c.desiredTarget = targetWorld;
    c.grip = Clamp(grip,0.0f,1.0f);
    c.targetWeight = 1.0f;
    if(!c.enabled){
        c.enabled=true;
        c.simulatedTarget=targetWorld;
        c.velocity={0.0f,0.0f,0.0f};
    }
}

void PlayerCharacterSystem::ReleaseArmPhysics(bool rightArm) {
    ArmPhysicsConstraint& c = rightArm ? rightArmPhysics : leftArmPhysics;
    c.targetWeight=0.0f;
    c.grip=0.0f;
}

void PlayerCharacterSystem::ReleaseAllArmPhysics() {
    ReleaseArmPhysics(false);
    ReleaseArmPhysics(true);
}

bool PlayerCharacterSystem::ArmPhysicsActive(bool rightArm) const {
    const ArmPhysicsConstraint& c = rightArm ? rightArmPhysics : leftArmPhysics;
    return c.enabled && (c.weight>0.015f || c.targetWeight>0.015f);
}

void PlayerCharacterSystem::UpdateArmPhysics(float dt) {
    dt=Clamp(dt,0.0f,0.05f);
    auto update=[](ArmPhysicsConstraint& c,float dt){
        // Critically damped-ish spring target. It gives the arm a tiny amount of
        // believable inertia while remaining stable enough for a gameplay grip.
        const float stiffness=92.0f;
        const float damping=18.5f;
        if(c.enabled){
            Vector3 error=Vector3Subtract(c.desiredTarget,c.simulatedTarget);
            Vector3 acceleration=Vector3Subtract(Vector3Scale(error,stiffness),Vector3Scale(c.velocity,damping));
            c.velocity=Vector3Add(c.velocity,Vector3Scale(acceleration,dt));
            c.simulatedTarget=Vector3Add(c.simulatedTarget,Vector3Scale(c.velocity,dt));
        }
        c.weight=Approach(c.weight,c.targetWeight,c.targetWeight>c.weight?13.0f:9.0f,dt);
        if(c.targetWeight<=0.0f && c.weight<0.01f){c.enabled=false;c.weight=0.0f;c.velocity={0,0,0};}
    };
    update(leftArmPhysics,dt);
    update(rightArmPhysics,dt);
}

void PlayerCharacterSystem::Update(float dt, float movementAmount, bool driving) {
    UpdateArmPhysics(dt);
    UpdateRagdoll(dt);
    hurt = humanoid.IsHurt();
    animation.ApplyHumanoidData(humanoid);

    if (IsRagdollActive() && !driving) {
        animation.SetState(CharacterAnimState::Hurt);
        animation.Update(dt, 0.0f);
        UpdateHairRig(dt);
        return;
    }

    if (creatorOpen) {
        animation.SetState(CharacterAnimState::Idle);
        animation.Update(dt, 0);
        UpdateHairRig(dt);
        return;
    }

    bool interactionPose = animation.state==CharacterAnimState::Repair || animation.state==CharacterAnimState::UseTool ||
        animation.state==CharacterAnimState::Inspect || animation.state==CharacterAnimState::Pickup ||
        animation.state==CharacterAnimState::Place || animation.state==CharacterAnimState::KneelRepair ||
        animation.state==CharacterAnimState::Menu || animation.state==CharacterAnimState::EnterVehicle ||
        animation.state==CharacterAnimState::ExitVehicle || animation.state==CharacterAnimState::OpenDoor || animation.state==CharacterAnimState::UseKeypad;

    if (driving) {
        animation.SetState(CharacterAnimState::SeatedDrive);
    } else if (hurt) {
        animation.SetState(CharacterAnimState::Hurt);
    } else if (carryingHeavy) {
        animation.SetState(CharacterAnimState::CarryHeavy);
    } else if (carryingMedium) {
        animation.SetState(CharacterAnimState::CarryMedium);
    } else if (!humanoid.IsGrounded()) {
        animation.SetState(CharacterAnimState::Jump);
    } else if (crouching) {
        animation.SetState(CharacterAnimState::Crouch);
    } else if (movementAmount > 0.05f) {
        if (forceWalkAnimation) animation.SetState(CharacterAnimState::Walk);
        else if (IsKeyDown(KEY_LEFT_SHIFT)) animation.SetState(CharacterAnimState::Sprint);
        else if (IsKeyDown(KEY_LEFT_ALT)) animation.SetState(CharacterAnimState::Walk);
        else animation.SetState(CharacterAnimState::Run);
    } else if (rotation.IsTurning() && humanoid.IsGrounded() && !interactionPose) {
        animation.SetState(CharacterAnimState::TurnInPlace);
    } else if (animation.stateTime > 0.55f && animation.state != CharacterAnimState::Idle) {
        // One-shot interactions naturally return to idle after a short readable pose.
        switch (animation.state) {
            case CharacterAnimState::Repair: case CharacterAnimState::UseTool:
            case CharacterAnimState::Inspect: case CharacterAnimState::Celebrate:
            case CharacterAnimState::Pickup: case CharacterAnimState::Place:
            case CharacterAnimState::KneelRepair: case CharacterAnimState::Menu:
            case CharacterAnimState::ExitVehicle: case CharacterAnimState::UseKeypad:
                if (animation.stateTime > 1.2f) animation.SetState(CharacterAnimState::Idle);
                break;
            case CharacterAnimState::OpenDoor:
                if (!ArmPhysicsActive(true) && !ArmPhysicsActive(false) && animation.stateTime > 1.2f) animation.SetState(CharacterAnimState::Idle);
                break;
            case CharacterAnimState::EnterVehicle:
                // Game owns completion of vehicle entry; keep this pose until seated.
                break;
            default: animation.SetState(CharacterAnimState::Idle); break;
        }
    }

    if (!driving) {
        if (IsKeyPressed(KEY_X)) crouching = !crouching;
        sprinting = IsKeyDown(KEY_LEFT_SHIFT) && !crouching && !carryingHeavy;
        if (IsKeyPressed(KEY_G)) animation.SetState(CharacterAnimState::Celebrate);
        if (IsKeyPressed(KEY_T)) animation.SetState(CharacterAnimState::Repair);
        if (IsKeyPressed(KEY_I)) animation.SetState(CharacterAnimState::Inspect);
        if (IsKeyPressed(KEY_N)) animation.SetState(CharacterAnimState::KneelRepair);
        if (IsKeyPressed(KEY_H)) { carryingHeavy = !carryingHeavy; carryingMedium = false; }
        if (IsKeyPressed(KEY_Y)) { carryingMedium = !carryingMedium; carryingHeavy = false; }
    }

    humanoid.UpdateVitals(dt, sprinting && movementAmount > 0.05f);
    animation.ApplyHumanoidData(humanoid);
    animation.Update(dt, movementAmount);
    UpdateHairRig(dt);
}

void PlayerCharacterSystem::DrawLimb(Vector3 a, Vector3 b, float radius, Color color) const {
    DrawCylinderEx(a, b, radius, radius*0.92f, 10, color);
}

void PlayerCharacterSystem::DrawYawedBox(Vector3 center, Vector3 size, float yawDegrees, Color color) const {
    // raylib does not provide a DrawCubePro() API. Apply the yaw to rlgl's
    // current model transform, draw a normal cube at the local origin, then
    // restore the matrix. This rotates the torso/pelvis/clothing as a single
    // rigid local shape without rotating individual skeleton pieces in world space.
    rlPushMatrix();
    rlTranslatef(center.x, center.y, center.z);
    rlRotatef(yawDegrees, 0.0f, 1.0f, 0.0f);
    DrawCube({0.0f, 0.0f, 0.0f}, size.x, size.y, size.z, color);
    rlPopMatrix();
}

void PlayerCharacterSystem::DrawOrientedBox(Vector3 center, Vector3 size, float yawDegrees, float pitchDegrees, float rollDegrees, Color color) const {
    rlPushMatrix();
    rlTranslatef(center.x, center.y, center.z);
    rlRotatef(yawDegrees, 0.0f, 1.0f, 0.0f);
    rlRotatef(pitchDegrees, 1.0f, 0.0f, 0.0f);
    rlRotatef(rollDegrees, 0.0f, 0.0f, 1.0f);
    DrawCube({0.0f,0.0f,0.0f}, size.x,size.y,size.z,color);
    rlPopMatrix();
}

void PlayerCharacterSystem::DrawTaperedBodySegment(Vector3 center, float bottomWidth, float topWidth, float height, float bottomDepth, float topDepth, float yawDegrees, float pitchDegrees, float rollDegrees, Color color, int sides) const {
    sides=std::clamp(sides,8,32);
    const float y0=-height*0.5f, y1=height*0.5f;
    rlPushMatrix();
    rlTranslatef(center.x,center.y,center.z);
    rlRotatef(yawDegrees,0.0f,1.0f,0.0f);
    rlRotatef(pitchDegrees,1.0f,0.0f,0.0f);
    rlRotatef(rollDegrees,0.0f,0.0f,1.0f);
    rlBegin(RL_TRIANGLES);
    rlColor4ub(color.r,color.g,color.b,color.a);
    for(int i=0;i<sides;++i){
        float a0=2.0f*PI*(float)i/(float)sides;
        float a1=2.0f*PI*(float)(i+1)/(float)sides;
        Vector3 b0{cosf(a0)*bottomWidth*0.5f,y0,sinf(a0)*bottomDepth*0.5f};
        Vector3 b1{cosf(a1)*bottomWidth*0.5f,y0,sinf(a1)*bottomDepth*0.5f};
        Vector3 t0{cosf(a0)*topWidth*0.5f,y1,sinf(a0)*topDepth*0.5f};
        Vector3 t1{cosf(a1)*topWidth*0.5f,y1,sinf(a1)*topDepth*0.5f};
        rlVertex3f(b0.x,b0.y,b0.z);rlVertex3f(t0.x,t0.y,t0.z);rlVertex3f(t1.x,t1.y,t1.z);
        rlVertex3f(b0.x,b0.y,b0.z);rlVertex3f(t1.x,t1.y,t1.z);rlVertex3f(b1.x,b1.y,b1.z);
        rlVertex3f(0,y1,0);rlVertex3f(t1.x,t1.y,t1.z);rlVertex3f(t0.x,t0.y,t0.z);
        rlVertex3f(0,y0,0);rlVertex3f(b0.x,b0.y,b0.z);rlVertex3f(b1.x,b1.y,b1.z);
    }
    rlEnd();
    rlPopMatrix();
}

std::vector<PlayerCharacterSystem::HairStrandSpec> PlayerCharacterSystem::BuildHairStrandSpecs() const {
    std::vector<HairStrandSpec> specs;
    constexpr float skullRadius = 0.255f;
    auto scalpRoot=[&](float x,float z,float lift){
        float radialSq=x*x+z*z;
        float y=std::sqrt(std::max(0.0f,skullRadius*skullRadius-radialSq))+lift;
        return vek::PhysicsVec3{x,y,z};
    };
    auto push=[&](vek::PhysicsVec3 root,float length,int segments,float curl,float phase,float radius,float stiff=1.0f,float grav=1.0f){
        HairStrandSpec sp;sp.root=root;sp.length=length;sp.segments=segments;sp.curl=curl;sp.phase=phase;sp.radius=radius;sp.stiffnessScale=stiff;sp.gravityScale=grav;specs.push_back(sp);
    };

    switch(appearance.hair){
        case HairStyle::Short: {
            for(int ring=0;ring<2;++ring){
                int count=ring?10:6;float rad=ring?0.145f:0.075f;
                for(int i=0;i<count;++i){float a=2.0f*PI*(float)i/(float)count;float x=cosf(a)*rad;float z=sinf(a)*rad;push(scalpRoot(x,z,0.018f),0.085f+(ring?0.01f:0.025f),3,0.10f,(float)i+ring*0.4f,0.028f,1.30f,0.55f);}
            }
        } break;
        case HairStyle::Long: {
            for(int i=0;i<16;++i){
                float u=(i-7.5f)/7.5f;float x=u*0.205f;float z=-0.075f-0.080f*(1.0f-u*u);
                float len=0.46f+0.13f*(1.0f-fabsf(u));push(scalpRoot(x,z,0.014f),len,7,0.06f,(float)i*0.65f,0.024f,0.72f,1.00f);
            }
            for(int i=0;i<6;++i){float u=(i-2.5f)/2.5f;float x=u*0.13f;push(scalpRoot(x,0.075f,0.016f),0.30f,5,0.04f,(float)i+9.0f,0.023f,0.95f,0.85f);}
        } break;
        case HairStyle::Curly: {
            for(int ring=0;ring<3;++ring){
                int count=ring==0?6:(ring==1?10:14);float rad=ring==0?0.055f:(ring==1?0.115f:0.175f);
                for(int i=0;i<count;++i){float a=2.0f*PI*((float)i+0.35f*ring)/(float)count;float x=cosf(a)*rad;float z=sinf(a)*rad;float len=0.14f+0.035f*ring;push(scalpRoot(x,z,0.020f),len,5,0.95f,(float)i*0.91f+ring,0.030f,0.92f,0.72f);}
            }
        } break;
        case HairStyle::Straight: {
            for(int i=0;i<15;++i){float u=(i-7.0f)/7.0f;float x=u*0.205f;float z=-0.065f-0.055f*(1.0f-u*u);push(scalpRoot(x,z,0.014f),0.38f+0.05f*(1.0f-fabsf(u)),6,0.0f,(float)i,0.021f,0.68f,1.0f);}
        } break;
        case HairStyle::Braids: {
            for(int i=0;i<12;++i){float u=(i-5.5f)/5.5f;float x=u*0.205f;float z=-0.065f-0.060f*(1.0f-u*u);push(scalpRoot(x,z,0.015f),0.48f+0.08f*(1.0f-fabsf(u)),8,0.28f,(float)i*1.15f,0.018f,0.82f,1.18f);}
        } break;
        case HairStyle::Ponytail: {
            // Small scalp roots lead into a physically simulated rear bundle.
            for(int i=0;i<8;++i){float u=(i-3.5f)/3.5f;push(scalpRoot(u*0.12f,-0.185f,0.018f),0.48f+0.045f*(1.0f-fabsf(u)),8,0.22f,(float)i*0.8f,0.023f,0.68f,1.05f);}
            for(int i=0;i<8;++i){float a=2.0f*PI*i/8.0f;float x=cosf(a)*0.105f,z=sinf(a)*0.085f;push(scalpRoot(x,z,0.018f),0.12f,3,0.08f,(float)i+6.0f,0.027f,1.20f,0.65f);}
        } break;
        case HairStyle::Afro: {
            for(int ring=0;ring<3;++ring){int count=ring==0?8:(ring==1?12:18);float rad=0.06f+ring*0.065f;for(int i=0;i<count;++i){float a=2.0f*PI*((float)i+ring*0.3f)/(float)count;float x=cosf(a)*rad,z=sinf(a)*rad;push(scalpRoot(x,z,0.022f),0.16f+ring*0.025f,4,0.75f,(float)i*0.77f+ring,0.034f,1.05f,0.60f);}}
        } break;
        case HairStyle::Wavy: {
            for(int i=0;i<22;++i){float u=(i-10.5f)/10.5f;float x=u*0.215f;float z=-0.055f-0.075f*(1.0f-u*u);push(scalpRoot(x,z,0.016f),0.40f+0.10f*(1.0f-fabsf(u)),7,0.42f,(float)i*0.72f,0.022f,0.76f,0.96f);}
        } break;
        case HairStyle::Locs: {
            for(int i=0;i<20;++i){float a=2.0f*PI*(float)i/20.0f;float rad=0.13f+0.045f*((i%3)/2.0f);float x=cosf(a)*rad,z=sinf(a)*rad;push(scalpRoot(x,z,0.016f),0.38f+0.12f*((i%4)/3.0f),7,0.12f,(float)i*0.93f,0.024f,0.84f,1.12f);}
        } break;
        case HairStyle::Bun: {
            for(int i=0;i<18;++i){float a=2.0f*PI*(float)i/18.0f;float rad=0.06f+0.055f*(i%2);float x=cosf(a)*rad,z=-0.10f+sinf(a)*rad;push(scalpRoot(x,z,0.024f),0.13f,4,0.55f,(float)i,0.031f,1.25f,0.55f);}
            for(int i=0;i<10;++i){float a=2.0f*PI*i/10.0f;push({cosf(a)*0.075f,0.32f,-0.18f+sinf(a)*0.045f},0.10f,3,0.65f,(float)i+4.0f,0.033f,1.35f,0.45f);}
        } break;
        case HairStyle::Bob: {
            for(int i=0;i<24;++i){float u=(i-11.5f)/11.5f;float x=u*0.22f;float z=-0.035f-0.06f*(1.0f-u*u);push(scalpRoot(x,z,0.015f),0.28f+0.04f*(1.0f-fabsf(u)),5,0.04f,(float)i*0.51f,0.024f,0.92f,0.90f);}
        } break;
        case HairStyle::Coils: {
            for(int ring=0;ring<4;++ring){int count=8+ring*6;float rad=0.035f+ring*0.052f;for(int i=0;i<count;++i){float a=2.0f*PI*((float)i+ring*0.25f)/(float)count;push(scalpRoot(cosf(a)*rad,sinf(a)*rad,0.024f),0.12f+ring*0.018f,5,1.25f,(float)i*1.11f+ring,0.027f,1.08f,0.52f);}}
        } break;
    }
    return specs;
}

vek::SecondaryMotionProfile PlayerCharacterSystem::HairPhysicsProfile() const {
    vek::SecondaryMotionProfile p;
    p.id="hair.default";
    p.maxDt=1.0f/45.0f;
    p.constraintIterations=7;
    p.maxSubsteps=4;
    p.collisionPadding=0.010f;
    p.maxSpeed=12.0f;
    p.inertia=0.95f;
    p.windInfluence=0.80f;
    switch(appearance.hair){
        case HairStyle::Short:    p.gravity=3.2f;p.damping=0.82f;p.stiffness=0.72f;p.airDrag=0.18f;break;
        case HairStyle::Long:     p.gravity=7.2f;p.damping=0.91f;p.stiffness=0.20f;p.airDrag=0.08f;break;
        case HairStyle::Curly:    p.gravity=5.0f;p.damping=0.88f;p.stiffness=0.38f;p.airDrag=0.12f;break;
        case HairStyle::Straight: p.gravity=6.8f;p.damping=0.90f;p.stiffness=0.24f;p.airDrag=0.08f;break;
        case HairStyle::Braids:   p.gravity=8.6f;p.damping=0.92f;p.stiffness=0.30f;p.airDrag=0.06f;break;
        case HairStyle::Ponytail: p.gravity=7.8f;p.damping=0.90f;p.stiffness=0.22f;p.airDrag=0.07f;p.inertia=1.12f;break;
        case HairStyle::Afro:     p.gravity=3.8f;p.damping=0.86f;p.stiffness=0.52f;p.airDrag=0.16f;break;
        case HairStyle::Wavy:     p.gravity=6.2f;p.damping=0.90f;p.stiffness=0.28f;p.airDrag=0.10f;break;
        case HairStyle::Locs:     p.gravity=8.8f;p.damping=0.93f;p.stiffness=0.34f;p.airDrag=0.06f;p.inertia=1.08f;break;
        case HairStyle::Bun:      p.gravity=3.0f;p.damping=0.88f;p.stiffness=0.72f;p.airDrag=0.14f;break;
        case HairStyle::Bob:      p.gravity=6.4f;p.damping=0.90f;p.stiffness=0.42f;p.airDrag=0.10f;break;
        case HairStyle::Coils:    p.gravity=3.6f;p.damping=0.87f;p.stiffness=0.58f;p.airDrag=0.17f;break;
    }
    return p;
}

void PlayerCharacterSystem::UpdateHairRig(float dt) {
    dt=Clamp(dt,0.0f,0.05f);
    if(dt<=0.0f)return;
    hairRig.time += dt;

    const auto specs=BuildHairStrandSpecs();
    bool topologyChanged=!hairRig.physicsInitialized || hairRig.configuredStyle!=appearance.hair || hairRig.strands.size()!=specs.size();
    if(topologyChanged){
        hairRig.strands.assign(specs.size(),{});
        hairRig.configuredStyle=appearance.hair;
        hairRig.physicsInitialized=true;
    }

    const float speed=std::max(0.0f,animation.movementSpeed);
    const float speedAccel=Clamp((speed-hairRig.previousSpeed)/std::max(dt,0.001f),-24.0f,24.0f);
    const float verticalAccel=Clamp((animation.airborneVerticalVelocity-hairRig.previousVerticalVelocity)/std::max(dt,0.001f),-35.0f,35.0f);
    const float turn=Clamp(animation.rotationAngularVelocity/360.0f,-1.5f,1.5f);
    const float gait=animation.isMoving?sinf(animation.gait*2.0f):sinf(hairRig.time*1.8f)*0.15f;

    // Forces are expressed in head-local coordinates. The chain solver itself
    // lives in VEK, so this game only supplies character acceleration/wind.
    vek::PhysicsVec3 external{
        -turn*(4.6f+0.35f*speed),
        Clamp(-verticalAccel*0.11f + gait*1.3f,-5.0f,5.0f),
        Clamp(speed*0.24f + speedAccel*0.10f,-4.5f,6.0f)
    };

    const auto base=HairPhysicsProfile();
    for(std::size_t i=0;i<specs.size();++i){
        vek::SpringChainSettings settings;
        settings.segments=specs[i].segments;
        settings.segmentLength=specs[i].length/(float)std::max(1,specs[i].segments);
        settings.motion=base;
        settings.motion.stiffness=Clamp(settings.motion.stiffness*specs[i].stiffnessScale,0.02f,0.98f);
        settings.motion.gravity=Clamp(settings.motion.gravity*specs[i].gravityScale,0.0f,20.0f);
        hairRig.strands[i].Configure(settings);
        if(topologyChanged || !hairRig.strands[i].Initialized())hairRig.strands[i].Reset(specs[i].root,{0.0f,-1.0f,0.0f});
        vek::PhysicsVec3 force=external;
        // Tiny phase variation prevents every strand from moving as a rigid sheet.
        force.x += sinf(hairRig.time*1.7f+specs[i].phase)*0.18f*base.windInfluence;
        force.z += cosf(hairRig.time*1.3f+specs[i].phase)*0.12f*base.windInfluence;
        hairRig.strands[i].Step(specs[i].root,force,{0.0f,0.0f,0.0f},0.263f,dt);
    }

    hairRig.previousSpeed=speed;
    hairRig.previousVerticalVelocity=animation.airborneVerticalVelocity;
    hairRig.sideLag=Approach(hairRig.sideLag,-turn*0.10f,6.0f,dt);
    hairRig.backLag=Approach(hairRig.backLag,speed*0.012f,5.0f,dt);
    hairRig.bounce=Approach(hairRig.bounce,gait*0.01f,8.0f,dt);
    hairRig.turbulence=Approach(hairRig.turbulence,std::min(0.04f,speed*0.004f),4.0f,dt);
}

void PlayerCharacterSystem::DrawHairStrand(std::size_t strandIndex, Vector3 head, float yaw, float scale, const HairStrandSpec& spec, Color color) const {
    Vector3 prev=Vector3Add(head,Vector3Scale(RotateY({spec.root.x,spec.root.y,spec.root.z},yaw),scale));
    const std::vector<vek::SpringChainParticle>* particles=nullptr;
    if(strandIndex<hairRig.strands.size() && hairRig.strands[strandIndex].Initialized())particles=&hairRig.strands[strandIndex].Particles();
    const int count=particles?(int)particles->size():spec.segments+1;
    for(int i=1;i<count;++i){
        float t=(float)i/(float)std::max(1,count-1);
        vek::PhysicsVec3 local;
        if(particles){local=(*particles)[(std::size_t)i].position;}
        else{local={spec.root.x,spec.root.y-spec.length*t,spec.root.z};}
        // Curl is geometry layered on the physical backbone; motion itself is
        // produced by VEK's constrained spring chain, not a fake sine sway.
        float coil=spec.phase+t*(2.0f*PI)*(1.0f+spec.curl*0.7f);
        float amp=0.018f*spec.curl*t;
        local.x += sinf(coil)*amp;
        local.z += cosf(coil)*amp;
        Vector3 point=Vector3Add(head,Vector3Scale(RotateY({local.x,local.y,local.z},yaw),scale));
        float r=spec.radius*scale*(1.0f-0.28f*t);
        DrawCylinderEx(prev,point,std::max(0.005f,r),std::max(0.004f,r*0.90f),8,color);
        DrawSphere(point,std::max(0.006f,r*0.94f),color);
        prev=point;
    }
}

void PlayerCharacterSystem::DrawHead(Vector3 center, float yaw, float scale) const {
    Color skin = SkinColor();
    float xScale = 1.0f;
    float yScale = 1.0f;
    switch (appearance.faceShape) {
        case FaceShape::Round: xScale=1.08f; yScale=0.96f; break;
        case FaceShape::Square: xScale=1.05f; yScale=1.02f; break;
        case FaceShape::Heart: xScale=1.04f; yScale=1.05f; break;
        default: break;
    }

    // Base head is deliberately simple and original. Face details are separate readable 3D marks.
    DrawSphere(center, 0.255f*scale*xScale, skin);
    Vector3 f = RotateY({0,0,1},yaw);
    Vector3 r = RotateY({1,0,0},yaw);

    Vector3 eyeL = Vector3Add(center, Vector3Add(Vector3Scale(r,-0.085f*scale), Vector3Scale(f,0.225f*scale)));
    Vector3 eyeR = Vector3Add(center, Vector3Add(Vector3Scale(r, 0.085f*scale), Vector3Scale(f,0.225f*scale)));
    eyeL.y += 0.045f*scale; eyeR.y += 0.045f*scale;
    float eyeSize = (0.022f + appearance.eyeStyle*0.0025f)*scale;
    DrawSphere(eyeL,eyeSize,DARKBROWN); DrawSphere(eyeR,eyeSize,DARKBROWN);

    Vector3 nose = Vector3Add(center, Vector3Scale(f,(0.244f + appearance.noseStyle*0.005f)*scale));
    DrawSphere(nose,(0.030f + appearance.noseStyle*0.003f)*scale,skin);

    Vector3 mouth = Vector3Add(center,Vector3Scale(f,0.239f*scale)); mouth.y -= 0.075f*scale;
    DrawSphere(mouth,(0.018f + appearance.lipStyle*0.002f)*scale,{110,56,54,255});

    // Eyebrows, ears and jaw are separate procedural features, so their creator values are visible.
    float browLift=(0.085f + appearance.eyebrowStyle*0.009f)*scale;
    Vector3 browL=Vector3Add(eyeL,{0,browLift,0}), browR=Vector3Add(eyeR,{0,browLift,0});
    DrawLine3D(Vector3Add(browL,Vector3Scale(r,-0.045f*scale)),Vector3Add(browL,Vector3Scale(r,0.045f*scale)),HairColor());
    DrawLine3D(Vector3Add(browR,Vector3Scale(r,-0.045f*scale)),Vector3Add(browR,Vector3Scale(r,0.045f*scale)),HairColor());
    Vector3 earL=Vector3Add(center,Vector3Scale(r,-0.245f*scale*xScale));
    Vector3 earR=Vector3Add(center,Vector3Scale(r, 0.245f*scale*xScale));
    float earSize=(0.035f+appearance.earStyle*0.004f)*scale;
    DrawSphere(earL,earSize,skin);DrawSphere(earR,earSize,skin);
    Vector3 jaw=Vector3Add(center,{0,-(0.20f+appearance.jawStyle*0.008f)*scale,0});
    jaw=Vector3Add(jaw,Vector3Scale(f,0.06f*scale));
    DrawSphere(jaw,(0.07f+appearance.jawStyle*0.006f)*scale,skin);

    if (appearance.freckles) {
        for (int i=-2;i<=2;i++) {
            Vector3 freckle = Vector3Add(center,Vector3Add(Vector3Scale(r,i*0.033f*scale),Vector3Scale(f,0.245f*scale)));
            freckle.y -= 0.005f*scale;
            DrawSphere(freckle,0.006f*scale,{105,65,44,255});
        }
    }
}

void PlayerCharacterSystem::DrawHair(Vector3 head, float yaw, float scale) const {
    Color hc=HairColor();
    const auto specs=BuildHairStrandSpecs();

    // Draw many small root follicles on the outside of the head instead of a
    // giant sphere intersecting the skull. This also hides tiny gaps between
    // the physical strand roots without swallowing the forehead.
    for(std::size_t i=0;i<specs.size();++i){
        const auto& sp=specs[i];
        Vector3 root=Vector3Add(head,Vector3Scale(RotateY({sp.root.x,sp.root.y,sp.root.z},yaw),scale));
        float rootRadius=std::max(0.022f,sp.radius*1.35f)*scale;
        DrawSphere(root,rootRadius,hc);
        DrawHairStrand(i,head,yaw,scale,sp,hc);
    }

    // A few small crown fillers provide continuous coverage while remaining
    // completely outside the 0.255 head sphere.
    const vek::PhysicsVec3 crownFill[]={{0.0f,0.286f,0.0f},{-0.070f,0.272f,0.010f},{0.070f,0.272f,0.010f},{0.0f,0.270f,-0.075f}};
    for(const auto& p:crownFill){
        Vector3 c=Vector3Add(head,Vector3Scale(RotateY({p.x,p.y,p.z},yaw),scale));
        DrawSphere(c,0.050f*scale,hc);
    }
}

void PlayerCharacterSystem::DrawAccessory(Vector3 head, float yaw, float scale) const {
    Vector3 f=RotateY({0,0,1},yaw), r=RotateY({1,0,0},yaw);
    switch (appearance.accessory) {
        case AccessoryStyle::Glasses: case AccessoryStyle::Goggles: {
            Color c = appearance.accessory==AccessoryStyle::Glasses ? DARKGRAY : SKYBLUE;
            Vector3 a=Vector3Add(head,Vector3Add(Vector3Scale(r,-0.09f*scale),Vector3Scale(f,0.245f*scale)));
            Vector3 b=Vector3Add(head,Vector3Add(Vector3Scale(r, 0.09f*scale),Vector3Scale(f,0.245f*scale)));
            a.y+=0.045f*scale;b.y+=0.045f*scale;
            DrawSphereWires(a,0.055f*scale,8,8,c);DrawSphereWires(b,0.055f*scale,8,8,c);DrawLine3D(a,b,c);break;
        }
        case AccessoryStyle::Cap:
            DrawCylinder(Vector3Add(head,{0,0.25f*scale,0}),0.23f*scale,0.20f*scale,0.10f*scale,16,ClothingColor());
            DrawCube(Vector3Add(head,Vector3Add(Vector3Scale(f,0.22f*scale),{0,0.24f*scale,0})),0.22f*scale,0.035f*scale,0.24f*scale,ClothingColor());break;
        case AccessoryStyle::Helmet:
            DrawSphereWires(Vector3Add(head,{0,0.09f*scale,0}),0.29f*scale,12,12,YELLOW);break;
        default: break;
    }
}

void PlayerCharacterSystem::DrawEquipment(Vector3 hand, float yaw) const {
    if (!equipment.visible) return;
    Vector3 f=RotateY({0,0,1},yaw), r=RotateY({1,0,0},yaw);
    switch (equipment.selected) {
        case EquipmentType::Wrench:
            DrawCube(Vector3Add(hand,Vector3Scale(f,0.13f)),0.045f,0.045f,0.34f,LIGHTGRAY);DrawSphere(Vector3Add(hand,Vector3Scale(f,0.31f)),0.07f,LIGHTGRAY);break;
        case EquipmentType::Hammer:
            DrawCube(Vector3Add(hand,Vector3Scale(f,0.13f)),0.05f,0.05f,0.32f,BROWN);DrawCube(Vector3Add(hand,Vector3Scale(f,0.32f)),0.20f,0.10f,0.09f,DARKGRAY);break;
        case EquipmentType::Welder:
            DrawCube(Vector3Add(hand,Vector3Scale(f,0.17f)),0.12f,0.10f,0.34f,ORANGE);break;
        case EquipmentType::Scanner:
            DrawCube(Vector3Add(hand,Vector3Scale(f,0.12f)),0.16f,0.24f,0.06f,SKYBLUE);break;
        case EquipmentType::Tablet:
            DrawCube(Vector3Add(hand,Vector3Scale(f,0.13f)),0.28f,0.36f,0.035f,DARKGRAY);break;
        case EquipmentType::Flashlight:
            DrawCylinderEx(hand,Vector3Add(hand,Vector3Scale(f,0.28f)),0.045f,0.06f,10,DARKGRAY);break;
        case EquipmentType::FuelCan:
            DrawCube(Vector3Add(hand,{0,-0.15f,0}),0.28f,0.42f,0.18f,RED);break;
        case EquipmentType::TowStrap: {
            Vector3 c=Vector3Add(hand,Vector3Scale(r,0.06f));
            for(int i=0;i<12;i++){float a=i*6.28318f/12.0f;DrawSphere(Vector3Add(c,{cosf(a)*0.10f,sinf(a)*0.10f,0}),0.025f,ORANGE);}
            break;
        }
        case EquipmentType::GPS:
            DrawCube(Vector3Add(hand,Vector3Scale(f,0.09f)),0.14f,0.18f,0.04f,GREEN);break;
    }
}

void PlayerCharacterSystem::Draw(Vector3 root, float yaw, bool seated) const {
    float H = appearance.height;
    float body = appearance.bodySize;
    float shoulders = appearance.shoulderWidth;
    float presentationShoulder = appearance.presentation==Presentation::Masculine ? 1.06f : (appearance.presentation==Presentation::Feminine ? 0.96f : 1.0f);
    float presentationHip = appearance.presentation==Presentation::Feminine ? 1.08f : (appearance.presentation==Presentation::Masculine ? 0.96f : 1.0f);
    shoulders *= presentationShoulder;
    float armScale = appearance.armSize;
    float legScale = appearance.legSize;
    float crouch = animation.crouch;

    Color skin=SkinColor(), cloth=ClothingColor();
    Color trousers = Color{48,57,61,255};
    Color boots = Color{48,39,31,255};
    Color gloves = appearance.gloves ? Color{38,42,44,255} : skin;

    // Bob/compression are visual animation offsets only. The gameplay root and
    // HumanoidSystem collision position remain stable and independent.
    float visualYOffset = animation.rootBob*H - animation.landingCompression*0.10f*H;
    float pelvisY = (0.95f - crouch*0.42f)*H + visualYOffset;
    float torsoY = pelvisY + 0.55f*H;
    float neckY = torsoY + 0.52f*H;
    float headY = neckY + 0.28f*H;
    float leanZ = sinf(animation.torsoLean*DEG2RAD)*0.30f*H;
    float shoulderX = 0.34f*shoulders*body;
    float hipX = 0.18f*body*presentationHip;

    const bool ragActive = IsRagdollActive();
    const float ragBlend = ragdoll.blend;
    const float ragPitch = ragdoll.pitch * ragBlend;
    const float ragRoll = ragdoll.roll * ragBlend;
    const float ragYawOffset = ragdoll.yawOffset * ragBlend;
    const float ragDrop = ragdoll.bodyDrop * H * ragBlend;
    const float rigStrength = rigDefinition.id.empty()?1.0f:rigDefinition.globalRagdollStrength;
    const float spineFlex = rigDefinition.id.empty()?0.75f:rigDefinition.spineFlex;
    const float neckFlex = rigDefinition.id.empty()?0.85f:rigDefinition.neckFlex;
    const float limbFlex = rigDefinition.id.empty()?1.0f:rigDefinition.limbFlex;
    auto jointWeight=[&](const char* name,float fallback){
        for(const auto& j:rigDefinition.joints) if(j.name==name) return j.ragdollWeight;
        return fallback;
    };
    const float lowerSpineRagPitch=ragPitch*0.42f*spineFlex*jointWeight("spine_lower",0.85f)*rigStrength;
    const float upperSpineRagPitch=ragPitch*0.78f*spineFlex*jointWeight("spine_upper",1.0f)*rigStrength;
    const float neckRagPitch=ragPitch*0.32f*neckFlex*jointWeight("neck",1.15f)*rigStrength;
    const float headRagRoll=ragRoll*0.44f*neckFlex*jointWeight("head",1.10f)*rigStrength;
    auto ragdollLocal=[&](Vector3 p){
        if(!ragActive) return p;
        const Vector3 pivot{0.0f,0.95f*H,0.0f};
        p=Vector3Subtract(p,pivot);
        float rx=ragPitch*DEG2RAD, rz=ragRoll*DEG2RAD;
        float cy=cosf(rx), sy=sinf(rx);
        float y1=p.y*cy-p.z*sy, z1=p.y*sy+p.z*cy; p.y=y1; p.z=z1;
        float cz=cosf(rz), sz=sinf(rz);
        float x2=p.x*cz-p.y*sz, y2=p.x*sz+p.y*cz; p.x=x2; p.y=y2;
        p=Vector3Add(p,pivot); p.y-=ragDrop;
        return p;
    };

    // Hierarchical facing layers:
    // root yaw -> pelvis counter twist -> chest/torso look+run twist -> head look.
    // The torso mesh itself now rotates, not only its child limb positions.
    float pelvisYaw = yaw + animation.pelvisYawTwist + ragYawOffset;
    float torsoYaw = yaw + animation.upperBodyYawOffset + animation.torsoYawTwist + ragYawOffset;
    float headYaw = yaw + animation.headYawOffset + animation.torsoYawTwist*0.12f + ragYawOffset;

    auto worldRoot=[&](Vector3 p){return Vector3Add(root,RotateY(ragdollLocal(p),yaw+ragYawOffset));};
    auto worldLower=[&](Vector3 p){return Vector3Add(root,RotateY(ragdollLocal(p),pelvisYaw));};
    auto worldTorso=[&](Vector3 p){return Vector3Add(root,RotateY(ragdollLocal(p),torsoYaw));};

    Vector3 torso=worldTorso({0,torsoY,leanZ*0.55f});
    Vector3 pelvis=worldLower({0,pelvisY,0});
    // VEK 1.8 rig: split the old single torso block into independently posed
    // lower-spine and upper-chest segments so ragdoll bending reads as a chain.
    Vector3 lowerSpine=worldTorso({0,torsoY-0.19f*H,leanZ*0.34f});
    Vector3 upperSpine=worldTorso({0,torsoY+0.19f*H,leanZ*0.72f});
    // VEK 2.0 organic rig shell: tapered elliptical segments replace the old
    // Minecraft/Roblox-like cubes while preserving independent spine joints.
    DrawTaperedBodySegment(lowerSpine,0.40f*body,0.52f*body*shoulders,0.44f*H,0.28f*body,0.34f*body,torsoYaw,lowerSpineRagPitch,ragRoll*0.55f,cloth,18);
    DrawTaperedBodySegment(upperSpine,0.52f*body*shoulders,0.60f*body*shoulders,0.50f*H,0.34f*body,0.31f*body,torsoYaw,upperSpineRagPitch,ragRoll*0.85f,cloth,18);
    DrawTaperedBodySegment(pelvis,0.42f*body*presentationHip,0.48f*body*presentationHip,0.31f*H,0.30f*body,0.32f*body,pelvisYaw,ragPitch*0.8f,ragRoll*0.8f,trousers,16);
    DrawLimb(worldTorso({-0.15f*body,torsoY+0.36f*H,leanZ*0.72f}),worldTorso({-shoulderX,torsoY+0.30f*H,leanZ*0.60f}),0.075f*body,cloth);
    DrawLimb(worldTorso({ 0.15f*body,torsoY+0.36f*H,leanZ*0.72f}),worldTorso({ shoulderX,torsoY+0.30f*H,leanZ*0.60f}),0.075f*body,cloth);

    // Neck/head follow the chest pivot, while facial look remains independently
    // constrained by CharacterRotationSystem's head yaw limit.
    float neckForward = sinf(neckRagPitch*DEG2RAD)*0.10f*H*ragBlend;
    Vector3 neckA=worldTorso({0,neckY-0.08f*H,leanZ*0.80f});
    Vector3 neckB=worldTorso({0,neckY+0.08f*H,leanZ+neckForward});
    DrawLimb(neckA,neckB,0.095f*body,skin);
    Vector3 head=worldTorso({sinf(headRagRoll*DEG2RAD)*0.06f*H*ragBlend,headY,leanZ+neckForward});
    DrawHead(head,headYaw,body);
    DrawHair(head,headYaw,body);
    DrawAccessory(head,headYaw,body);

    // Arms: stronger elbow drive in run/sprint, plus opposite shoulder rise/fall.
    auto arm=[&](float side,float swing,bool rightSide){
        float sideWeight=rightSide?jointWeight("upperarm_r",1.20f):jointWeight("upperarm_l",1.20f);
        float ragArmSwing=(ragRoll*side*-0.65f + ragPitch*(rightSide?0.22f:-0.18f))*limbFlex*sideWeight*rigStrength*ragBlend;
        float rad=(swing+ragArmSwing)*DEG2RAD;
        float shoulderYOffset = animation.shoulderBob * (side < 0.0f ? -1.0f : 1.0f) * H;
        Vector3 shoulder={side*(shoulderX + ragdoll.armSpread*0.08f*H*ragBlend),torsoY+0.28f*H+shoulderYOffset,leanZ*0.60f};
        Vector3 upper={side*0.035f, -0.36f*armScale*H*cosf(rad), 0.36f*armScale*H*sinf(rad)};
        Vector3 elbow=Vector3Add(shoulder,upper);
        float bend=(seated?45.0f:10.0f)+fabsf(animation.handRaise)*0.20f;
        if(ragActive) bend += (55.0f + 28.0f*side) * limbFlex * ragBlend;
        if (animation.state==CharacterAnimState::Run) bend += 20.0f;
        if (animation.state==CharacterAnimState::Sprint) bend += 34.0f;
        if (animation.state==CharacterAnimState::Jump) bend += 18.0f*animation.jumpTuck;
        // Elbow flexion must rotate the forearm toward +Z (the face/chest
        // forward direction). The old subtraction bent elbows behind the body.
        float r2=(swing+bend)*DEG2RAD;
        Vector3 fore={side*0.02f,-0.32f*armScale*H*cosf(r2),0.32f*armScale*H*sinf(r2)};
        Vector3 hand=Vector3Add(elbow,fore);
        Vector3 sW=worldTorso(shoulder), eW=worldTorso(elbow), hW=worldTorso(hand);

        const ArmPhysicsConstraint& armConstraint=rightSide?rightArmPhysics:leftArmPhysics;
        if(armConstraint.enabled && armConstraint.weight>0.001f){
            const float upperLen=0.36f*armScale*H;
            const float foreLen=0.32f*armScale*H;
            Vector3 target=Vector3Lerp(hW,armConstraint.simulatedTarget,armConstraint.weight);
            Vector3 toTarget=Vector3Subtract(target,sW);
            float rawDist=Vector3Length(toTarget);
            if(rawDist>0.0001f){
                Vector3 dir=Vector3Scale(toTarget,1.0f/rawDist);
                float d=Clamp(rawDist,std::fabs(upperLen-foreLen)+0.015f,upperLen+foreLen-0.015f);
                float along=(upperLen*upperLen-foreLen*foreLen+d*d)/(2.0f*d);
                float radiusSq=std::max(0.0f,upperLen*upperLen-along*along);
                float radius=std::sqrt(radiusSq);
                Vector3 mid=Vector3Add(sW,Vector3Scale(dir,along));
                // Preserve the natural procedural elbow side as the IK bend hint.
                Vector3 preferred=Vector3Subtract(eW,mid);
                preferred=Vector3Subtract(preferred,Vector3Scale(dir,Vector3DotProduct(preferred,dir)));
                if(Vector3Length(preferred)<0.001f){
                    Vector3 sideAxis=RotateY({rightSide?1.0f:-1.0f,-0.35f,0.0f},torsoYaw);
                    preferred=Vector3Subtract(sideAxis,Vector3Scale(dir,Vector3DotProduct(sideAxis,dir)));
                }
                if(Vector3Length(preferred)>0.001f)preferred=Vector3Normalize(preferred);
                Vector3 ikElbow=Vector3Add(mid,Vector3Scale(preferred,radius));
                eW=Vector3Lerp(eW,ikElbow,armConstraint.weight);
                hW=target;
            }
        }

        DrawSphere(sW,0.12f*body,cloth);
        DrawLimb(sW,eW,0.105f*body,cloth);
        DrawSphere(eW,0.095f*body,skin); // explicit elbow rig joint
        DrawLimb(eW,hW,0.085f*body,skin);
        DrawSphere(hW,0.105f*body,gloves); // wrist/hand joint
        if (rightSide && !(armConstraint.enabled && armConstraint.weight>0.15f)) DrawEquipment(hW,torsoYaw);
    };
    arm(-1.0f,animation.leftArmSwing,false);
    arm( 1.0f,animation.rightArmSwing,true);

    // Legs use pelvis yaw, foot lift and jump tuck. This gives the run a real
    // flight/swing phase instead of feet remaining glued to one vertical level.
    auto leg=[&](float side,float swing){
        bool leftSide=side<0.0f;
        float legWeight=leftSide?jointWeight("thigh_l",1.25f):jointWeight("thigh_r",1.25f);
        float ragLegSwing=(ragPitch*(leftSide?0.42f:-0.31f)+ragRoll*side*0.28f)*limbFlex*legWeight*rigStrength*ragBlend;
        float rad=(swing+ragLegSwing)*DEG2RAD;
        float lift = leftSide ? animation.leftFootLift : animation.rightFootLift;
        Vector3 hip={side*(hipX + ragdoll.legSpread*0.06f*H*ragBlend),pelvisY-0.12f*H,0};
        Vector3 thigh={0,-0.44f*legScale*H*cosf(rad),0.44f*legScale*H*sinf(rad)};
        Vector3 knee=Vector3Add(hip,thigh);
        float ragKnee=ragActive?(leftSide?38.0f:62.0f)*limbFlex*ragBlend:0.0f;
        float bend=(animation.kneeBend + crouch*25.0f + animation.jumpTuck*14.0f + ragKnee)*DEG2RAD;
        Vector3 shin={0,-0.43f*legScale*H*cosf(rad-bend),0.43f*legScale*H*sinf(rad-bend)};
        Vector3 ankle=Vector3Add(knee,shin);
        ankle.y += lift*H;
        knee.y += lift*H*0.28f;
        Vector3 hW=worldLower(hip), kW=worldLower(knee), aW=worldLower(ankle);
        DrawLimb(hW,kW,0.14f*body,trousers);
        DrawSphere(kW,0.125f*body,trousers); // explicit knee joint
        DrawLimb(kW,aW,0.12f*body,trousers);
        DrawSphere(aW,0.095f*body,boots); // explicit ankle joint
        float footYawOffset = side < 0.0f ? animation.leftFootYawOffset : animation.rightFootYawOffset;
        Vector3 footLocal=Vector3Add(ankle,RotateY({0,-0.04f*H,0.12f*body},footYawOffset));
        Vector3 foot=worldLower(footLocal);
        DrawYawedBox(foot,{0.24f*body,0.14f*H,0.42f*body},pelvisYaw+footYawOffset,boots);
        Vector3 toe=Vector3Add(foot,RotateY({0,-0.01f*H,0.18f*body},pelvisYaw+footYawOffset));
        DrawSphere(toe,0.075f*body,boots);
    };
    leg(-1.0f,animation.leftLegSwing);
    leg( 1.0f,animation.rightLegSwing);

    // Context cargo follows the torso, so turning the chest while carrying no
    // longer leaves the prop visually attached to the unrotated root.
    if (carryingMedium) {
        Vector3 p=worldTorso({0,torsoY-0.12f*H,0.48f*body});
        DrawYawedBox(p,{0.62f*body,0.42f*H,0.42f*body},torsoYaw,BEIGE);
        DrawCubeWires(p,0.62f*body,0.42f*H,0.42f*body,DARKBROWN);
    } else if (carryingHeavy) {
        Vector3 p=worldTorso({0,torsoY-0.28f*H,0.55f*body});
        DrawYawedBox(p,{0.86f*body,0.56f*H,0.50f*body},torsoYaw,BROWN);
        DrawCubeWires(p,0.86f*body,0.56f*H,0.50f*body,DARKBROWN);
    }

    // Clothing overlays rotate with the torso too. This was the most visible
    // source of the old "torso does not turn" look.
    auto torsoBox=[&](Vector3 local,Vector3 size,Color color){
        DrawOrientedBox(worldTorso(local),size,torsoYaw,ragPitch,ragRoll,color);
    };
    if (appearance.clothing==ClothingStyle::Overalls || appearance.clothing==ClothingStyle::SafetyVest) {
        Color overlay=appearance.clothing==ClothingStyle::SafetyVest?YELLOW:Color{40,69,93,255};
        // Narrow fabric panels/straps keep clothing readable without turning the
        // newly rounded torso back into one giant rectangle.
        torsoBox({0,torsoY-0.04f*H,0.185f*body},{0.30f*body,0.34f*H,0.028f*body},overlay);
        torsoBox({-0.13f*body,torsoY+0.22f*H,0.18f*body},{0.07f*body,0.30f*H,0.025f*body},overlay);
        torsoBox({ 0.13f*body,torsoY+0.22f*H,0.18f*body},{0.07f*body,0.30f*H,0.025f*body},overlay);
    }
    if (appearance.clothing==ClothingStyle::PilotGear) {
        torsoBox({0,torsoY+0.06f*H,0.19f*body},{0.46f*body,0.62f*H,0.04f*body},DARKBLUE);
        torsoBox({0.18f*body,torsoY+0.18f*H,0.22f*body},{0.10f*body,0.16f*H,0.04f*body},GOLD);
    } else if (appearance.clothing==ClothingStyle::MarineGear) {
        torsoBox({0,torsoY+0.02f*H,0.19f*body},{0.50f*body,0.55f*H,0.04f*body},SKYBLUE);
    } else if (appearance.clothing==ClothingStyle::IndustrialGear) {
        torsoBox({0,torsoY+0.02f*H,0.20f*body},{0.52f*body,0.62f*H,0.05f*body},ORANGE);
    } else if (appearance.clothing==ClothingStyle::CompanyUniform) {
        torsoBox({0,torsoY+0.08f*H,0.20f*body},{0.48f*body,0.58f*H,0.04f*body},ClothingColor());
        torsoBox({0,torsoY+0.02f*H,0.225f*body},{0.18f*body,0.08f*H,0.02f*body},RAYWHITE);
    }
    if (appearance.accessory==AccessoryStyle::Backpack) {
        torsoBox({0,torsoY,-0.26f*body},{0.48f*body,0.60f*H,0.20f*body},DARKGREEN);
    }
}

void PlayerCharacterSystem::DrawFirstPersonArms(const Camera3D& camera, bool driving) const {
    Vector3 f=Vector3Normalize(Vector3Subtract(camera.target,camera.position));
    Vector3 r=Vector3Normalize(Vector3CrossProduct(f,camera.up));
    Vector3 base=Vector3Add(camera.position,Vector3Scale(f,0.72f));
    Vector3 l=Vector3Add(base,Vector3Scale(r,-0.25f));
    Vector3 rr=Vector3Add(base,Vector3Scale(r,0.25f));
    l.y-=0.20f;rr.y-=0.20f;
    Vector3 lf=Vector3Add(l,Vector3Scale(f,driving?0.48f:0.32f));
    Vector3 rf=Vector3Add(rr,Vector3Scale(f,driving?0.48f:0.32f));
    if(leftArmPhysics.enabled)lf=Vector3Lerp(lf,leftArmPhysics.simulatedTarget,leftArmPhysics.weight);
    if(rightArmPhysics.enabled)rf=Vector3Lerp(rf,rightArmPhysics.simulatedTarget,rightArmPhysics.weight);
    DrawLimb(l,lf,0.075f,SkinColor());
    DrawLimb(rr,rf,0.075f,SkinColor());
    DrawSphere(lf,0.09f,appearance.gloves?DARKGRAY:SkinColor());
    DrawSphere(rf,0.09f,appearance.gloves?DARKGRAY:SkinColor());
}

const char* PlayerCharacterSystem::CreatorLabel(int row) const {
    return CharacterCustomizationSystem::Label(row);
}

void PlayerCharacterSystem::AdjustCreatorValue(int row,int d) {
    CharacterCustomizationSystem::Adjust(appearance,row,d,reputation);
}

void PlayerCharacterSystem::UpdateCreator() {
    if (!creatorOpen) return;
    bool previewRotate = IsKeyDown(KEY_LEFT_SHIFT);
    if (previewRotate) {
        if (IsKeyDown(KEY_LEFT)) avatarPreviewYaw = CharacterRotationSystem::NormalizeAngle(avatarPreviewYaw - 120.0f*GetFrameTime());
        if (IsKeyDown(KEY_RIGHT)) avatarPreviewYaw = CharacterRotationSystem::NormalizeAngle(avatarPreviewYaw + 120.0f*GetFrameTime());
    } else {
        if (IsKeyPressed(KEY_UP)) creatorRow=(creatorRow+20)%21;
        if (IsKeyPressed(KEY_DOWN)) creatorRow=(creatorRow+1)%21;
        if (IsKeyPressed(KEY_LEFT)) AdjustCreatorValue(creatorRow,-1);
        if (IsKeyPressed(KEY_RIGHT)) AdjustCreatorValue(creatorRow,1);
    }
    if (IsKeyPressed(KEY_ENTER)) SaveAppearance();
}

void PlayerCharacterSystem::DrawCreatorUI() const {
    if(!creatorOpen)return;
    int x=20,y=20,w=420,h=GetScreenHeight()-40;
    DrawRectangle(x,y,w,h,Fade(BLACK,0.90f));
    DrawRectangleLines(x,y,w,h,SKYBLUE);
    DrawText("AVATAR CREATOR",x+20,y+18,30,RAYWHITE);
    DrawText("Original procedural mechanic avatar",x+20,y+55,17,LIGHTGRAY);
    DrawText("UP/DOWN select   LEFT/RIGHT change",x+20,y+80,16,LIGHTGRAY);
    DrawText("SHIFT+LEFT/RIGHT rotate preview 360 deg",x+20,y+101,15,SKYBLUE);
    DrawText("ENTER save   C close",x+20,y+122,16,GREEN);
    int rowY=y+155;
    for(int i=0;i<21;i++){
        Color c=i==creatorRow?YELLOW:RAYWHITE;
        DrawText(i==creatorRow?">":" ",x+16,rowY+i*23,18,c);
        DrawText(CreatorLabel(i),x+38,rowY+i*23,17,c);
    }
    DrawText(TextFormat("Body %.2f  Height %.2f",appearance.bodySize,appearance.height),x+235,rowY+23,16,SKYBLUE);
    DrawText(TextFormat("Skin %d  Hair %d",appearance.skinTone+1,(int)appearance.hair+1),x+235,rowY+6*23,16,SKYBLUE);
    DrawText(TextFormat("%s (Rep %d)",ClothingSystem::Name(appearance.clothing),ClothingSystem::RequiredReputation(appearance.clothing)),x+205,rowY+17*23,14,SKYBLUE);
}

bool PlayerCharacterSystem::SaveAppearance() const {
    return AvatarSaveSystem::Save(appearance);
}

bool PlayerCharacterSystem::LoadAppearance() {
    return AvatarSaveSystem::Load(appearance);
}
