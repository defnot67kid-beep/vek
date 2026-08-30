#include "CameraRotationSystem.h"
#include <algorithm>
#include <cmath>

float CameraRotationSystem::NormalizeAngle(float degrees)
{
    float a = std::fmod(degrees, 360.0f);
    if (a < 0.0f) a += 360.0f;
    return a;
}

float CameraRotationSystem::DeltaAngle(float current, float target)
{
    float delta = NormalizeAngle(target) - NormalizeAngle(current);
    if (delta > 180.0f) delta -= 360.0f;
    if (delta < -180.0f) delta += 360.0f;
    return delta;
}


int CameraRotationSystem::AlignmentDirectionForArrowKey(int keycode)
{
    // User requested inverted camera alignment:
    // physical LEFT arrow advances in +yaw, physical RIGHT arrow advances in -yaw.
    if (keycode == KEY_LEFT) return +1;
    if (keycode == KEY_RIGHT) return -1;
    return 0;
}

float CameraRotationSystem::MoveTowards(float current, float target, float maxDelta)
{
    if (std::fabs(target - current) <= maxDelta) return target;
    return current + (target > current ? maxDelta : -maxDelta);
}

void CameraRotationSystem::Snap(float yawDegrees, float pitchDegrees)
{
    currentYaw = yawClampEnabled ? std::clamp(yawDegrees, yawMin, yawMax) : NormalizeAngle(yawDegrees);
    targetYaw = currentYaw;
    currentPitch = std::clamp(pitchDegrees, settings.pitchMin, settings.pitchMax);
    targetPitch = currentPitch;
    yawVelocity = 0.0f;
    pitchVelocity = 0.0f;
}

void CameraRotationSystem::SetPitchLimits(float minDegrees, float maxDegrees)
{
    settings.pitchMin = std::min(minDegrees, maxDegrees);
    settings.pitchMax = std::max(minDegrees, maxDegrees);
    currentPitch = std::clamp(currentPitch, settings.pitchMin, settings.pitchMax);
    targetPitch = std::clamp(targetPitch, settings.pitchMin, settings.pitchMax);
}

void CameraRotationSystem::SetYawClamp(bool enabled, float minDegrees, float maxDegrees)
{
    yawClampEnabled = enabled;
    yawMin = std::min(minDegrees, maxDegrees);
    yawMax = std::max(minDegrees, maxDegrees);

    if (yawClampEnabled)
    {
        currentYaw = std::clamp(currentYaw, yawMin, yawMax);
        targetYaw = std::clamp(targetYaw, yawMin, yawMax);
    }
    else
    {
        currentYaw = NormalizeAngle(currentYaw);
        targetYaw = NormalizeAngle(targetYaw);
    }
}

void CameraRotationSystem::SetSensitivity(float yawSensitivity, float pitchSensitivity)
{
    settings.yawSensitivity = yawSensitivity;
    settings.pitchSensitivity = pitchSensitivity;
}

void CameraRotationSystem::SetPitchInputSign(float sign)
{
    pitchInputSign = sign < 0.0f ? -1.0f : 1.0f;
}

void CameraRotationSystem::AddMouseDelta(Vector2 mouseDelta)
{
    float yawInput = -mouseDelta.x * settings.yawSensitivity;
    float pitchInput = mouseDelta.y * settings.pitchSensitivity * pitchInputSign;

    if (yawClampEnabled)
        targetYaw = std::clamp(targetYaw + yawInput, yawMin, yawMax);
    else
        targetYaw = NormalizeAngle(targetYaw + yawInput);

    targetPitch = std::clamp(targetPitch + pitchInput, settings.pitchMin, settings.pitchMax);
}

void CameraRotationSystem::SetTarget(float yawDegrees, float pitchDegrees)
{
    targetYaw = yawClampEnabled ? std::clamp(yawDegrees, yawMin, yawMax) : NormalizeAngle(yawDegrees);
    targetPitch = std::clamp(pitchDegrees, settings.pitchMin, settings.pitchMax);
}

void CameraRotationSystem::AlignYawStep(int direction, float stepDegrees)
{
    if (direction == 0) return;

    float step = std::fabs(stepDegrees);
    if (step < 0.001f) return;

    // Work from the requested/target yaw so repeated key presses can queue
    // clean 45-degree brackets even while the visible camera is still easing.
    float base = targetYaw;
    constexpr float epsilon = 0.0001f;

    float aligned = base;
    if (direction > 0)
    {
        // Next bracket to the right. If already on a bracket, advance one.
        float q = std::floor((base + epsilon) / step);
        float bracket = q * step;
        aligned = (std::fabs(base - bracket) <= epsilon) ? bracket + step : (q + 1.0f) * step;
    }
    else
    {
        // Next bracket to the left. If already on a bracket, go back one.
        float q = std::ceil((base - epsilon) / step);
        float bracket = q * step;
        aligned = (std::fabs(base - bracket) <= epsilon) ? bracket - step : (q - 1.0f) * step;
    }

    targetYaw = yawClampEnabled ? std::clamp(aligned, yawMin, yawMax) : NormalizeAngle(aligned);
}

void CameraRotationSystem::StepYaw(float dt)
{
    float delta = yawClampEnabled ? (targetYaw - currentYaw) : DeltaAngle(currentYaw, targetYaw);

    if (std::fabs(delta) <= settings.stopTolerance)
    {
        currentYaw = yawClampEnabled ? targetYaw : NormalizeAngle(targetYaw);
        yawVelocity = MoveTowards(yawVelocity, 0.0f, settings.yawDeceleration * dt);
        return;
    }

    float brakingSpeed = std::sqrt(std::max(0.0f, 2.0f * settings.yawDeceleration * std::fabs(delta)));
    float requestedSpeed = std::min(settings.maxYawSpeed, brakingSpeed);
    float desiredVelocity = delta >= 0.0f ? requestedSpeed : -requestedSpeed;

    bool sameDirection = yawVelocity == 0.0f || ((yawVelocity > 0.0f) == (desiredVelocity > 0.0f));
    float rate = sameDirection && std::fabs(desiredVelocity) > std::fabs(yawVelocity)
        ? settings.yawAcceleration
        : settings.yawDeceleration;

    yawVelocity = MoveTowards(yawVelocity, desiredVelocity, rate * dt);
    float step = yawVelocity * dt;

    if (std::fabs(step) >= std::fabs(delta))
    {
        currentYaw = yawClampEnabled ? targetYaw : NormalizeAngle(targetYaw);
        yawVelocity = 0.0f;
    }
    else
    {
        currentYaw += step;
        if (!yawClampEnabled) currentYaw = NormalizeAngle(currentYaw);
        else currentYaw = std::clamp(currentYaw, yawMin, yawMax);
    }
}

void CameraRotationSystem::StepPitch(float dt)
{
    float delta = targetPitch - currentPitch;

    if (std::fabs(delta) <= settings.stopTolerance)
    {
        currentPitch = targetPitch;
        pitchVelocity = MoveTowards(pitchVelocity, 0.0f, settings.pitchDeceleration * dt);
        return;
    }

    float brakingSpeed = std::sqrt(std::max(0.0f, 2.0f * settings.pitchDeceleration * std::fabs(delta)));
    float requestedSpeed = std::min(settings.maxPitchSpeed, brakingSpeed);
    float desiredVelocity = delta >= 0.0f ? requestedSpeed : -requestedSpeed;

    bool sameDirection = pitchVelocity == 0.0f || ((pitchVelocity > 0.0f) == (desiredVelocity > 0.0f));
    float rate = sameDirection && std::fabs(desiredVelocity) > std::fabs(pitchVelocity)
        ? settings.pitchAcceleration
        : settings.pitchDeceleration;

    pitchVelocity = MoveTowards(pitchVelocity, desiredVelocity, rate * dt);
    float step = pitchVelocity * dt;

    if (std::fabs(step) >= std::fabs(delta))
    {
        currentPitch = targetPitch;
        pitchVelocity = 0.0f;
    }
    else
    {
        currentPitch = std::clamp(currentPitch + step, settings.pitchMin, settings.pitchMax);
    }
}

void CameraRotationSystem::Update(float deltaTime)
{
    float dt = std::clamp(deltaTime, 0.0f, 0.10f);
    StepYaw(dt);
    StepPitch(dt);
}

float CameraRotationSystem::GetYaw() const { return currentYaw; }
float CameraRotationSystem::GetPitch() const { return currentPitch; }
float CameraRotationSystem::GetTargetYaw() const { return targetYaw; }
float CameraRotationSystem::GetTargetPitch() const { return targetPitch; }
float CameraRotationSystem::GetYawVelocity() const { return yawVelocity; }
float CameraRotationSystem::GetPitchVelocity() const { return pitchVelocity; }
