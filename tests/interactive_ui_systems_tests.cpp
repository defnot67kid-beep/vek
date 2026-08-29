#include <vek/VekInteractiveUiSystems.h>
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using namespace vek;

    UiOrbit3D orbit;
    orbit.SetAngles(0.0f, 0.0f);
    orbit.SetAngularVelocity(1.0f, 0.0f);
    orbit.Step(0.05f);
    assert(orbit.Yaw() > 0.0f);
    const auto projected = orbit.Project({1.0f, 0.0f, 0.0f}, {100.0f, 100.0f}, 100.0f, 5.0f);
    assert(std::isfinite(projected.screen.x));
    assert(std::isfinite(projected.screen.y));

    RunnerMiniGame game;
    RunnerSettings settings;
    settings.groundY = 300.0f;
    settings.playerX = 50.0f;
    settings.scrollSpeed = 100.0f;
    game.Configure(settings);
    game.SetObstacles({{300.0f, 30.0f, 40.0f, RunnerObstacleShape::Spike}});
    const float groundY = game.PlayerY();
    game.Jump();
    game.Step(1.0f / 60.0f);
    assert(game.PlayerY() < groundY);
    assert(!game.OnGround());

    assert(ParseSemanticVersion("v2.5.0").valid);
    assert(IsVersionOutdated("2.4.0", "v2.5.0"));
    assert(!IsVersionOutdated("2.5.0", "v2.5.0"));
    assert(std::string(UpdatePolicyName(UpdatePolicy::Automatic)) == "auto");

    std::cout << "VEK interactive UI systems tests passed\n";
    return 0;
}
