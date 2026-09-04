#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace vek {

struct UiVec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct UiVec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct ProjectedPoint {
    UiVec2 screen{};
    float depth = 0.0f;
};

// Small renderer-independent 3D transform helper intended for installer/logo
// scenes, editor splash screens, debug overlays and lightweight UI previews.
class UiOrbit3D {
public:
    void SetAngles(float yawRadians, float pitchRadians);
    void SetAngularVelocity(float yawPerSecond, float pitchPerSecond);
    void Step(float dtSeconds);

    UiVec3 Rotate(UiVec3 point) const;
    ProjectedPoint Project(UiVec3 point,
                           UiVec2 screenCenter,
                           float focalLength,
                           float cameraDistance) const;

    float Yaw() const { return yaw_; }
    float Pitch() const { return pitch_; }

private:
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float yawVelocity_ = 0.7f;
    float pitchVelocity_ = 0.18f;
};


// Renderer-independent progress model used by VEK installer/loading scenes.
// Hosts can render this value as a 2D or extruded 3D progress bar.
class ProgressBar3DModel {
public:
    void Reset(float value = 0.0f);
    void SetTarget(float normalizedProgress);
    void Step(float dtSeconds, float responsePerSecond = 7.5f);

    float Displayed() const { return displayed_; }
    float Target() const { return target_; }
    int Percent() const;
    bool Complete() const { return displayed_ >= 0.999f && target_ >= 0.999f; }
    float FilledWidth(float totalWidth) const;

private:
    float displayed_ = 0.0f;
    float target_ = 0.0f;
};

enum class RunnerObstacleShape : std::uint8_t {
    Spike,
    Block,
};

struct RunnerObstacle {
    float x = 0.0f;
    float width = 28.0f;
    float height = 32.0f;
    RunnerObstacleShape shape = RunnerObstacleShape::Spike;
};

struct RunnerSettings {
    float groundY = 300.0f;
    float playerX = 72.0f;
    float playerSize = 26.0f;
    float gravity = 1500.0f;
    float jumpVelocity = -570.0f;
    float scrollSpeed = 245.0f;
    float wrapDistance = 920.0f;
    float maxDt = 1.0f / 30.0f;
};

// Deterministic one-button runner simulation. The runtime intentionally owns
// no graphics/input APIs: hosts can render it using Win32, raylib, SDL, etc.
class RunnerMiniGame {
public:
    void Configure(const RunnerSettings& settings);
    void Reset();
    void SetObstacles(std::vector<RunnerObstacle> obstacles);
    void Jump();
    void Step(float dtSeconds);

    float PlayerY() const { return playerY_; }
    float PlayerVelocityY() const { return playerVelocityY_; }
    float ScrollOffset() const { return scrollOffset_; }
    std::uint64_t Score() const { return score_; }
    std::uint64_t Crashes() const { return crashes_; }
    bool OnGround() const { return onGround_; }
    const RunnerSettings& Settings() const { return settings_; }
    const std::vector<RunnerObstacle>& Obstacles() const { return obstacles_; }

private:
    bool Collides(const RunnerObstacle& obstacle) const;
    void ResetPlayerAfterCrash();

    RunnerSettings settings_{};
    std::vector<RunnerObstacle> obstacles_{};
    float playerY_ = 0.0f;
    float playerVelocityY_ = 0.0f;
    float scrollOffset_ = 0.0f;
    std::uint64_t score_ = 0;
    std::uint64_t crashes_ = 0;
    bool onGround_ = true;
};

enum class UpdatePolicy : std::uint8_t {
    Manual,
    Automatic,
};

struct SemanticVersion {
    int major = 0;
    int minor = 0;
    int patch = 0;
    bool valid = false;
};

SemanticVersion ParseSemanticVersion(const std::string& text);
int CompareSemanticVersion(const SemanticVersion& lhs, const SemanticVersion& rhs);
bool IsVersionOutdated(const std::string& installed, const std::string& latest);
const char* UpdatePolicyName(UpdatePolicy policy);

struct InstallerViewportLayout {
    float width = 1280.0f;
    float height = 720.0f;
    UiVec2 logoCenter{640.0f, 122.0f};
    float logoFocalLength = 520.0f;
    float downloadRowY = 245.0f;
    float policyRowY = 317.0f;
    float progressBarY = 403.0f;
    float runnerTopY = 475.0f;
    float runnerGroundY = 655.0f;
};

// Responsive fullscreen installer geometry shared by native VEK hosts.
InstallerViewportLayout ComputeFullscreenInstallerLayout(float width, float height);

// Automatic mode performs automatic version checks/notifications only.
// Downloads and file changes still require explicit user action.
bool ShouldNotifyUpdate(UpdatePolicy policy,
                        const std::string& installed,
                        const std::string& latest);

// Deprecated safety shim. VEK 2.5.4 never recommends a silent background
// installation; this function always returns false.
bool ShouldBackgroundUpdate(UpdatePolicy policy,
                            const std::string& installed,
                            const std::string& latest);

} // namespace vek
