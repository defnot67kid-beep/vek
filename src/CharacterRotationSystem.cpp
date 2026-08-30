#include "CharacterRotationSystem.h"
#include <algorithm>
#include <cmath>

static constexpr float PI_F = 3.14159265358979323846f;

float CharacterRotationSystem::NormalizeAngle(float degrees) {
    float a = std::fmod(degrees, 360.0f);
    if (a < 0.0f) a += 360.0f;
    return a;
}

float CharacterRotationSystem::DeltaAngle(float current, float target) {
    float delta = NormalizeAngle(target) - NormalizeAngle(current);
    if (delta > 180.0f) delta -= 360.0f;
    if (delta < -180.0f) delta += 360.0f;
    return delta;
}

float CharacterRotationSystem::MoveTowardsAngle(float current, float target, float maxDelta) {
    float delta = DeltaAngle(current, target);
    if (std::fabs(delta) <= maxDelta) return NormalizeAngle(target);
    return NormalizeAngle(current + (delta > 0.0f ? maxDelta : -maxDelta));
}

float CharacterRotationSystem::YawFromDirection(Vector3 direction) {
    float lenSq = direction.x*direction.x + direction.z*direction.z;
    if (lenSq < 0.000001f) return 0.0f;
    return NormalizeAngle(std::atan2(direction.x, direction.z) * (180.0f / PI_F));
}

Vector3 CharacterRotationSystem::ForwardFromYaw(float yawDegrees) {
    float r = yawDegrees * (PI_F / 180.0f);
    return {std::sinf(r), 0.0f, std::cosf(r)};
}

float CharacterRotationSystem::MoveTowards(float current, float target, float maxDelta) {
    if (current < target) return std::min(current + maxDelta, target);
    if (current > target) return std::max(current - maxDelta, target);
    return target;
}

float CharacterRotationSystem::ExpSmoothing(float current, float target, float speed, float dt) {
    if (dt <= 0.0f) return current;
    float t = 1.0f - std::exp(-std::max(0.0f, speed) * dt);
    return current + (target - current) * t;
}

float CharacterRotationSystem::RotationSpeedForState(CharacterMovementState state) const {
    switch (state) {
        case CharacterMovementState::Walk: return settings.walkRotationSpeed;
        case CharacterMovementState::Run: return settings.runRotationSpeed;
        case CharacterMovementState::Sprint: return settings.sprintRotationSpeed;
        case CharacterMovementState::Crouch: return settings.crouchRotationSpeed;
        case CharacterMovementState::MediumCarry: return settings.mediumCarryRotationSpeed;
        case CharacterMovementState::HeavyCarry: return settings.heavyCarryRotationSpeed;
        case CharacterMovementState::Hurt: return settings.hurtRotationSpeed;
        case CharacterMovementState::Repairing:
        case CharacterMovementState::Inspecting:
        case CharacterMovementState::EnteringVehicle:
            return settings.interactionRotationSpeed;
        case CharacterMovementState::Idle:
        case CharacterMovementState::Seated:
        default:
            return settings.turnInPlaceRotationSpeed;
    }
}

void CharacterRotationSystem::FaceDirection(Vector3 direction) {
    direction.y = 0.0f;
    float lenSq = direction.x*direction.x + direction.z*direction.z;
    if (lenSq < 0.000001f) return;
    targetYaw = YawFromDirection(direction);
    facingOverride = true;
    rotationMode = CharacterMovementRotationMode::LockedDirection;
}

void CharacterRotationSystem::FaceTarget(Vector3 characterPosition, Vector3 targetPosition) {
    Vector3 d{targetPosition.x-characterPosition.x, 0.0f, targetPosition.z-characterPosition.z};
    FaceDirection(d);
}

void CharacterRotationSystem::FaceYaw(float yawDegrees) {
    targetYaw = NormalizeAngle(yawDegrees);
    facingOverride = true;
    rotationMode = CharacterMovementRotationMode::LockedDirection;
}

void CharacterRotationSystem::ReleaseFacingOverride() {
    facingOverride = false;
    if (rotationMode == CharacterMovementRotationMode::LockedDirection)
        rotationMode = CharacterMovementRotationMode::OrientToMovement;
}

void CharacterRotationSystem::SetRotationMode(CharacterMovementRotationMode mode) {
    if (facingOverride && mode != CharacterMovementRotationMode::LockedDirection) return;
    rotationMode = mode;
}

CharacterMovementRotationMode CharacterRotationSystem::GetRotationMode() const { return rotationMode; }
void CharacterRotationSystem::SetFirstPerson(bool enabled) { firstPerson = enabled; }
void CharacterRotationSystem::SetCameraInfluenceEnabled(bool enabled) { cameraInfluenceEnabled = enabled; }

void CharacterRotationSystem::SetSeatedYaw(float seatYawDegrees) {
    seatedYawActive = true;
    seatedYaw = NormalizeAngle(seatYawDegrees);
}

void CharacterRotationSystem::ClearSeatedYaw() { seatedYawActive = false; }

void CharacterRotationSystem::SnapBodyYaw(float yawDegrees) {
    currentYaw = targetYaw = NormalizeAngle(yawDegrees);
    angularVelocity = 0.0f;
}

void CharacterRotationSystem::StepBodyRotation(float dt, float maxRotationSpeed) {
    float delta = DeltaAngle(currentYaw, targetYaw);
    angleDifference = delta;

    if (std::fabs(delta) <= settings.stopAngleTolerance) {
        currentYaw = NormalizeAngle(targetYaw);
        angularVelocity = MoveTowards(angularVelocity, 0.0f, settings.angularDeceleration * dt);
        return;
    }

    // Braking-limited desired angular speed prevents overshoot near the target.
    float brakingSpeed = std::sqrt(std::max(0.0f, 2.0f * settings.angularDeceleration * std::fabs(delta)));
    float requestedSpeed = std::min(maxRotationSpeed, brakingSpeed);
    float desiredVelocity = (delta >= 0.0f ? requestedSpeed : -requestedSpeed);

    bool sameDirection = (angularVelocity == 0.0f) || ((angularVelocity > 0.0f) == (desiredVelocity > 0.0f));
    float rate = sameDirection && std::fabs(desiredVelocity) > std::fabs(angularVelocity)
        ? settings.angularAcceleration
        : settings.angularDeceleration;

    angularVelocity = MoveTowards(angularVelocity, desiredVelocity, rate * dt);
    float step = angularVelocity * dt;

    // Never cross the target because of a large frame or angular velocity.
    if (std::fabs(step) > std::fabs(delta)) {
        currentYaw = NormalizeAngle(targetYaw);
        angularVelocity = 0.0f;
    } else {
        currentYaw = NormalizeAngle(currentYaw + step);
    }
}

void CharacterRotationSystem::UpdateTurnState(float yawDelta, bool moving) {
    float a = std::fabs(yawDelta);
    if (!moving && a < settings.turnInPlaceStartAngle * 0.45f) {
        turnState = CharacterTurnState::None;
        return;
    }

    if (a >= 150.0f) {
        turnState = CharacterTurnState::Turn180;
    } else if (a >= 70.0f) {
        turnState = yawDelta < 0.0f ? CharacterTurnState::TurnLeft90 : CharacterTurnState::TurnRight90;
    } else if (a >= 25.0f) {
        turnState = yawDelta < 0.0f ? CharacterTurnState::TurnLeft45 : CharacterTurnState::TurnRight45;
    } else {
        turnState = CharacterTurnState::None;
    }
}

void CharacterRotationSystem::UpdateHeadAndUpperBody(float dt) {
    if (!cameraInfluenceEnabled) return;

    float viewDelta = DeltaAngle(currentYaw, cameraYawValue);
    float desiredHead = std::clamp(viewDelta, -settings.headYawLimit, settings.headYawLimit);
    float desiredUpper = std::clamp(viewDelta * 0.38f, -settings.upperBodyYawLimit, settings.upperBodyYawLimit);

    headYawOffset = ExpSmoothing(headYawOffset, desiredHead, settings.headLookSpeed, dt);
    upperBodyYawOffset = ExpSmoothing(upperBodyYawOffset, desiredUpper, settings.upperBodyLookSpeed, dt);
}

void CharacterRotationSystem::Update(
    float dt,
    Vector3 movementDirection,
    float movementMagnitude,
    float cameraYaw,
    CharacterMovementState movementState
) {
    dt = std::clamp(dt, 0.0f, 0.10f);
    cameraYawValue = NormalizeAngle(cameraYaw);
    movementMagnitudeValue = std::clamp(movementMagnitude, 0.0f, 1.0f);

    movementDirection.y = 0.0f;
    float len = std::sqrt(movementDirection.x*movementDirection.x + movementDirection.z*movementDirection.z);
    if (len > 0.00001f) {
        movementDirection.x /= len;
        movementDirection.z /= len;
    } else {
        movementDirection = {0.0f,0.0f,0.0f};
    }
    movementDirectionValue = movementDirection;

    if (movementState == CharacterMovementState::Seated && seatedYawActive) {
        // Vehicle owns the world-facing root while seated.
        currentYaw = targetYaw = seatedYaw;
        angularVelocity = 0.0f;
        angleDifference = 0.0f;
        turnState = CharacterTurnState::None;
        UpdateHeadAndUpperBody(dt);
        return;
    }

    const bool moving = movementMagnitudeValue >= settings.movementDeadzone;
    float maxRotationSpeed = RotationSpeedForState(movementState);

    if (!facingOverride) {
        if (rotationMode == CharacterMovementRotationMode::OrientToMovement) {
            if (moving) {
                targetYaw = YawFromDirection(movementDirectionValue);
            } else if (cameraInfluenceEnabled) {
                // Stationary camera orbit is free until the view/body separation is large.
                float cameraDelta = DeltaAngle(currentYaw, cameraYawValue);
                float threshold = firstPerson ? settings.firstPersonBodyTurnAngle : settings.turnInPlaceStartAngle;
                if (std::fabs(cameraDelta) > threshold) {
                    targetYaw = cameraYawValue;
                    maxRotationSpeed = firstPerson ? settings.firstPersonBodyRotationSpeed : settings.turnInPlaceRotationSpeed;
                }
            }
        } else if (rotationMode == CharacterMovementRotationMode::FaceCameraDirection && cameraInfluenceEnabled) {
            // Strafing/aim-style mode: the body always tracks the camera's
            // yaw, whether idle or moving, so it never visibly lags behind
            // camera orbit the way OrientToMovement's idle-only mode does.
            targetYaw = cameraYawValue;
            maxRotationSpeed = firstPerson ? settings.firstPersonBodyRotationSpeed : maxRotationSpeed;
        }
        // LockedDirection keeps the existing targetYaw.
    } else {
        maxRotationSpeed = RotationSpeedForState(movementState);
    }

    StepBodyRotation(dt, maxRotationSpeed);
    float remaining = DeltaAngle(currentYaw, targetYaw);
    angleDifference = remaining;
    UpdateTurnState(remaining, moving);
    UpdateHeadAndUpperBody(dt);
}

float CharacterRotationSystem::GetBodyYaw() const { return currentYaw; }
float CharacterRotationSystem::GetTargetYaw() const { return targetYaw; }
float CharacterRotationSystem::GetHeadYawOffset() const { return headYawOffset; }
float CharacterRotationSystem::GetUpperBodyYawOffset() const { return upperBodyYawOffset; }
float CharacterRotationSystem::GetAngularVelocity() const { return angularVelocity; }
float CharacterRotationSystem::GetCameraYaw() const { return cameraYawValue; }
float CharacterRotationSystem::GetAngleDifference() const { return angleDifference; }
float CharacterRotationSystem::GetMovementMagnitude() const { return movementMagnitudeValue; }
Vector3 CharacterRotationSystem::GetMovementDirection() const { return movementDirectionValue; }
bool CharacterRotationSystem::IsTurning() const { return std::fabs(angularVelocity) > 1.0f || turnState != CharacterTurnState::None; }
CharacterTurnState CharacterRotationSystem::GetTurnState() const { return turnState; }

const char* CharacterRotationSystem::RotationModeName() const {
    switch (rotationMode) {
        case CharacterMovementRotationMode::OrientToMovement: return "OrientToMovement";
        case CharacterMovementRotationMode::FaceCameraDirection: return "FaceCameraDirection";
        case CharacterMovementRotationMode::LockedDirection: return "LockedDirection";
    }
    return "Unknown";
}

const char* CharacterRotationSystem::TurnStateName() const {
    switch (turnState) {
        case CharacterTurnState::None: return "None";
        case CharacterTurnState::TurnLeft45: return "TurnLeft45";
        case CharacterTurnState::TurnRight45: return "TurnRight45";
        case CharacterTurnState::TurnLeft90: return "TurnLeft90";
        case CharacterTurnState::TurnRight90: return "TurnRight90";
        case CharacterTurnState::Turn180: return "Turn180";
    }
    return "Unknown";
}
