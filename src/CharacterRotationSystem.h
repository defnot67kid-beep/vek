#pragma once
#include "raylib.h"

// Controls how the character's root chooses a facing target.
// Camera yaw is deliberately NOT the same thing as body yaw.
enum class CharacterMovementRotationMode {
    OrientToMovement,
    FaceCameraDirection,
    LockedDirection
};

enum class CharacterMovementState {
    Idle,
    Walk,
    Run,
    Sprint,
    Crouch,
    MediumCarry,
    HeavyCarry,
    Hurt,
    Repairing,
    Inspecting,
    EnteringVehicle,
    Seated
};

enum class CharacterTurnState {
    None,
    TurnLeft45,
    TurnRight45,
    TurnLeft90,
    TurnRight90,
    Turn180
};

struct CharacterRotationSettings {
    float walkRotationSpeed = 240.0f;
    float runRotationSpeed = 360.0f;
    float sprintRotationSpeed = 450.0f;
    float crouchRotationSpeed = 180.0f;
    float mediumCarryRotationSpeed = 170.0f;
    float heavyCarryRotationSpeed = 120.0f;
    float hurtRotationSpeed = 140.0f;
    float interactionRotationSpeed = 210.0f;
    float turnInPlaceRotationSpeed = 220.0f;
    float firstPersonBodyRotationSpeed = 250.0f;

    float angularAcceleration = 900.0f;
    float angularDeceleration = 1200.0f;
    float movementDeadzone = 0.08f;

    float turnInPlaceStartAngle = 70.0f;
    float firstPersonBodyTurnAngle = 55.0f;
    float turnInPlaceStrongAngle = 120.0f;
    float stopAngleTolerance = 0.20f;

    float headYawLimit = 65.0f;
    float upperBodyYawLimit = 25.0f;
    float headLookSpeed = 8.0f;
    float upperBodyLookSpeed = 6.0f;
};

class CharacterRotationSystem {
public:
    CharacterRotationSettings settings;

    void Update(
        float deltaTime,
        Vector3 movementDirection,
        float movementMagnitude,
        float cameraYaw,
        CharacterMovementState movementState
    );

    // Smooth-facing requests for interactions. They remain active until
    // ReleaseFacingOverride() is called.
    void FaceDirection(Vector3 direction);
    void FaceTarget(Vector3 characterPosition, Vector3 targetPosition);
    void FaceYaw(float yawDegrees);
    void ReleaseFacingOverride();

    void SetRotationMode(CharacterMovementRotationMode mode);
    CharacterMovementRotationMode GetRotationMode() const;

    void SetFirstPerson(bool enabled);
    void SetCameraInfluenceEnabled(bool enabled);

    // Seated mode deliberately bypasses walking rotation and aligns the root
    // to the vehicle/seat heading while preserving independent head look.
    void SetSeatedYaw(float seatYawDegrees);
    void ClearSeatedYaw();

    // Used only for initial spawn / completed seat alignment, never normal locomotion.
    void SnapBodyYaw(float yawDegrees);

    float GetBodyYaw() const;
    float GetTargetYaw() const;
    float GetHeadYawOffset() const;
    float GetUpperBodyYawOffset() const;
    float GetAngularVelocity() const;
    float GetCameraYaw() const;
    float GetAngleDifference() const;
    float GetMovementMagnitude() const;
    Vector3 GetMovementDirection() const;
    bool IsTurning() const;
    CharacterTurnState GetTurnState() const;

    const char* RotationModeName() const;
    const char* TurnStateName() const;

    static float NormalizeAngle(float degrees);
    static float DeltaAngle(float current, float target);
    static float MoveTowardsAngle(float current, float target, float maxDelta);
    static float YawFromDirection(Vector3 direction);
    static Vector3 ForwardFromYaw(float yawDegrees);

private:
    float currentYaw = 0.0f;
    float targetYaw = 0.0f;
    float angularVelocity = 0.0f;
    float headYawOffset = 0.0f;
    float upperBodyYawOffset = 0.0f;

    float cameraYawValue = 0.0f;
    float angleDifference = 0.0f;
    float movementMagnitudeValue = 0.0f;
    Vector3 movementDirectionValue{0.0f,0.0f,0.0f};

    CharacterMovementRotationMode rotationMode = CharacterMovementRotationMode::OrientToMovement;
    CharacterTurnState turnState = CharacterTurnState::None;

    bool firstPerson = false;
    bool cameraInfluenceEnabled = true;
    bool facingOverride = false;
    bool seatedYawActive = false;
    float seatedYaw = 0.0f;

    float RotationSpeedForState(CharacterMovementState state) const;
    void UpdateHeadAndUpperBody(float dt);
    void UpdateTurnState(float yawDelta, bool moving);
    void StepBodyRotation(float dt, float maxRotationSpeed);
    static float MoveTowards(float current, float target, float maxDelta);
    static float ExpSmoothing(float current, float target, float speed, float dt);
};
