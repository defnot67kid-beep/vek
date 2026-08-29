#include <vek/VekInteractiveUiSystems.h>

#include <algorithm>
#include <cmath>
#include <sstream>

namespace vek {
namespace {
constexpr float kTau = 6.2831853071795864769f;

float ClampDt(float dt, float maxDt) {
    if (!std::isfinite(dt) || dt <= 0.0f) return 0.0f;
    return std::min(dt, std::max(0.001f, maxDt));
}
}

void UiOrbit3D::SetAngles(float yawRadians, float pitchRadians) {
    yaw_ = std::isfinite(yawRadians) ? yawRadians : 0.0f;
    pitch_ = std::isfinite(pitchRadians) ? pitchRadians : 0.0f;
}

void UiOrbit3D::SetAngularVelocity(float yawPerSecond, float pitchPerSecond) {
    yawVelocity_ = std::isfinite(yawPerSecond) ? yawPerSecond : 0.0f;
    pitchVelocity_ = std::isfinite(pitchPerSecond) ? pitchPerSecond : 0.0f;
}

void UiOrbit3D::Step(float dtSeconds) {
    const float dt = ClampDt(dtSeconds, 0.1f);
    yaw_ = std::fmod(yaw_ + yawVelocity_ * dt, kTau);
    pitch_ = std::fmod(pitch_ + pitchVelocity_ * dt, kTau);
}

UiVec3 UiOrbit3D::Rotate(UiVec3 p) const {
    const float cy = std::cos(yaw_);
    const float sy = std::sin(yaw_);
    const float cp = std::cos(pitch_);
    const float sp = std::sin(pitch_);

    UiVec3 yawed{
        p.x * cy + p.z * sy,
        p.y,
        -p.x * sy + p.z * cy,
    };
    return {
        yawed.x,
        yawed.y * cp - yawed.z * sp,
        yawed.y * sp + yawed.z * cp,
    };
}

ProjectedPoint UiOrbit3D::Project(UiVec3 p,
                                  UiVec2 screenCenter,
                                  float focalLength,
                                  float cameraDistance) const {
    const UiVec3 r = Rotate(p);
    const float safeCamera = std::max(0.1f, cameraDistance);
    const float denominator = std::max(0.08f, safeCamera - r.z);
    const float scale = std::max(0.0f, focalLength) / denominator;
    return {{screenCenter.x + r.x * scale, screenCenter.y - r.y * scale}, r.z};
}

void RunnerMiniGame::Configure(const RunnerSettings& settings) {
    settings_ = settings;
    settings_.playerSize = std::max(4.0f, settings_.playerSize);
    settings_.gravity = std::max(0.0f, settings_.gravity);
    settings_.scrollSpeed = std::max(0.0f, settings_.scrollSpeed);
    settings_.wrapDistance = std::max(120.0f, settings_.wrapDistance);
    settings_.maxDt = std::clamp(settings_.maxDt, 0.001f, 0.1f);
    Reset();
}

void RunnerMiniGame::Reset() {
    playerY_ = settings_.groundY - settings_.playerSize;
    playerVelocityY_ = 0.0f;
    scrollOffset_ = 0.0f;
    score_ = 0;
    crashes_ = 0;
    onGround_ = true;
}

void RunnerMiniGame::SetObstacles(std::vector<RunnerObstacle> obstacles) {
    obstacles_ = std::move(obstacles);
    for (auto& obstacle : obstacles_) {
        obstacle.width = std::max(4.0f, obstacle.width);
        obstacle.height = std::max(4.0f, obstacle.height);
    }
}

void RunnerMiniGame::Jump() {
    if (!onGround_) return;
    playerVelocityY_ = settings_.jumpVelocity;
    onGround_ = false;
}

bool RunnerMiniGame::Collides(const RunnerObstacle& obstacle) const {
    const float playerLeft = settings_.playerX;
    const float playerRight = playerLeft + settings_.playerSize;
    const float playerTop = playerY_;
    const float playerBottom = playerY_ + settings_.playerSize;

    const float obstacleLeft = obstacle.x;
    const float obstacleRight = obstacle.x + obstacle.width;
    const float obstacleTop = settings_.groundY - obstacle.height;
    const float obstacleBottom = settings_.groundY;

    if (playerRight <= obstacleLeft || playerLeft >= obstacleRight ||
        playerBottom <= obstacleTop || playerTop >= obstacleBottom) {
        return false;
    }

    // Spike collision is intentionally slightly forgiving near the tip to
    // avoid frustrating false-looking hits in tiny installer mini-games.
    if (obstacle.shape == RunnerObstacleShape::Spike) {
        const float overlap = std::min(playerRight, obstacleRight) - std::max(playerLeft, obstacleLeft);
        if (overlap < settings_.playerSize * 0.18f && playerBottom < obstacleBottom - obstacle.height * 0.35f) {
            return false;
        }
    }
    return true;
}

void RunnerMiniGame::ResetPlayerAfterCrash() {
    ++crashes_;
    score_ = 0;
    playerY_ = settings_.groundY - settings_.playerSize;
    playerVelocityY_ = 0.0f;
    onGround_ = true;
}

void RunnerMiniGame::Step(float dtSeconds) {
    const float dt = ClampDt(dtSeconds, settings_.maxDt);
    if (dt <= 0.0f) return;

    scrollOffset_ += settings_.scrollSpeed * dt;
    if (scrollOffset_ >= settings_.wrapDistance) {
        scrollOffset_ = std::fmod(scrollOffset_, settings_.wrapDistance);
    }

    playerVelocityY_ += settings_.gravity * dt;
    playerY_ += playerVelocityY_ * dt;
    const float floorY = settings_.groundY - settings_.playerSize;
    if (playerY_ >= floorY) {
        playerY_ = floorY;
        playerVelocityY_ = 0.0f;
        onGround_ = true;
    }

    for (auto& obstacle : obstacles_) {
        obstacle.x -= settings_.scrollSpeed * dt;
        if (obstacle.x + obstacle.width < 0.0f) {
            obstacle.x += settings_.wrapDistance;
            ++score_;
        }
        if (Collides(obstacle)) {
            ResetPlayerAfterCrash();
            // Keep obstacles moving so a crash cannot immediately retrigger.
            obstacle.x = std::max(obstacle.x, settings_.playerX + 180.0f);
            break;
        }
    }
}

SemanticVersion ParseSemanticVersion(const std::string& text) {
    std::string value = text;
    while (!value.empty() && (value.front() == 'v' || value.front() == 'V' || value.front() == ' ' || value.front() == '\t')) {
        value.erase(value.begin());
    }

    SemanticVersion out{};
    char dot1 = 0;
    char dot2 = 0;
    std::istringstream stream(value);
    if (stream >> out.major >> dot1 >> out.minor >> dot2 >> out.patch && dot1 == '.' && dot2 == '.') {
        out.valid = out.major >= 0 && out.minor >= 0 && out.patch >= 0;
    }
    return out;
}

int CompareSemanticVersion(const SemanticVersion& lhs, const SemanticVersion& rhs) {
    if (!lhs.valid || !rhs.valid) return 0;
    if (lhs.major != rhs.major) return lhs.major < rhs.major ? -1 : 1;
    if (lhs.minor != rhs.minor) return lhs.minor < rhs.minor ? -1 : 1;
    if (lhs.patch != rhs.patch) return lhs.patch < rhs.patch ? -1 : 1;
    return 0;
}

bool IsVersionOutdated(const std::string& installed, const std::string& latest) {
    const auto a = ParseSemanticVersion(installed);
    const auto b = ParseSemanticVersion(latest);
    return a.valid && b.valid && CompareSemanticVersion(a, b) < 0;
}

const char* UpdatePolicyName(UpdatePolicy policy) {
    return policy == UpdatePolicy::Automatic ? "auto" : "manual";
}

} // namespace vek
