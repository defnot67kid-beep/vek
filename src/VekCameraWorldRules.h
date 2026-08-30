#pragma once
#include <VekGameSystems.h>
#include <VekScriptEngine.h>
#include <memory>
#include <string>

class VekCameraWorldRules {
public:
    bool Initialize(const std::string& path);
    bool Reload();
    const std::string& Error() const { return error; }

    const vek::CameraProfileDefinition* Camera() const { return cameras.Find(cameraId); }
    const vek::SkyboxDefinition* Skybox() const { return skyboxes.Find(skyboxId); }
    const vek::HumanoidRigDefinition* Rig() const { return rigs.Find(rigId); }
    const vek::WorldGameplayPolicy* Policy() const { return policies.Find(policyId); }

private:
    bool Load();
    std::string file,error;
    std::string cameraId="camera.player";
    std::string skyboxId="sky.engineering_day";
    std::string rigId="rig.player_mechanic_v2";
    std::string policyId="game.default";
    vek::CameraProfileRegistry cameras;
    vek::SkyboxRegistry skyboxes;
    vek::HumanoidRigRegistry rigs;
    vek::WorldGameplayPolicyRegistry policies;
    std::unique_ptr<VekScriptEngine> vm;
};
