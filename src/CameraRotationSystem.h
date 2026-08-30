#pragma once
#include "raylib.h"

struct CameraRotationSettings
{
    float yawSensitivity = 0.12f;
    float pitchSensitivity = 0.10f;

    float maxYawSpeed = 720.0f;
    float maxPitchSpeed = 540.0f;

    float yawAcceleration = 3200.0f;
    float yawDeceleration = 4200.0f;
    float pitchAcceleration = 2600.0f;
    float pitchDeceleration = 3400.0f;

    float pitchMin = -25.0f;
    float pitchMax = 65.0f;

    float stopTolerance = 0.015f;
};

// Smooth, frame-rate-independent camera orientation controller.
// The visible camera accelerates toward target yaw/pitch values. v8 gameplay
// changes yaw targets with keyboard alignment; AddMouseDelta remains an optional
// engine API but is not bound to gameplay camera controls.
class CameraRotationSystem
{
public:
    CameraRotationSettings settings;

    void Snap(float yawDegrees, float pitchDegrees);
    void AddMouseDelta(Vector2 mouseDelta);
    void Update(float deltaTime);

    void SetTarget(float yawDegrees, float pitchDegrees);
    // Move target yaw to the next fixed world-angle slot. Input mapping is
    // handled by Game. The current user-requested mapping intentionally
    // inverts the physical Left/Right Arrow keys.
    void AlignYawStep(int direction, float stepDegrees = 45.0f);
    void SetPitchLimits(float minDegrees, float maxDegrees);
    void SetYawClamp(bool enabled, float minDegrees = -180.0f, float maxDegrees = 180.0f);
    void SetSensitivity(float yawSensitivity, float pitchSensitivity);
    void SetPitchInputSign(float sign);

    float GetYaw() const;
    float GetPitch() const;
    float GetTargetYaw() const;
    float GetTargetPitch() const;
    float GetYawVelocity() const;
    float GetPitchVelocity() const;

    static float NormalizeAngle(float degrees);
    static float DeltaAngle(float current, float target);
    // Current gameplay binding uses only the physical arrow keys and is
    // intentionally inverted: Left Arrow -> +yaw, Right Arrow -> -yaw.
    // Comma/period, < > characters, and square brackets are not bindings.
    static int AlignmentDirectionForArrowKey(int keycode);

private:
    float currentYaw = 0.0f;
    float targetYaw = 0.0f;
    float currentPitch = 18.0f;
    float targetPitch = 18.0f;

    float yawVelocity = 0.0f;
    float pitchVelocity = 0.0f;

    bool yawClampEnabled = false;
    float yawMin = -180.0f;
    float yawMax = 180.0f;
    float pitchInputSign = 1.0f;

    static float MoveTowards(float current, float target, float maxDelta);
    void StepYaw(float deltaTime);
    void StepPitch(float deltaTime);
};
