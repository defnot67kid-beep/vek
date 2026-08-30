#pragma once

#include "HumanoidSystem.h"
#include <VekScriptEngine.h>
#include <string>

class VekCharacterRules final : public HumanoidRuleProvider {
public:
    bool Initialize(const std::string& scriptPath = "scripts/player_systems.vek");
    bool ReloadScript();
    bool CanHotReload() const;
    bool ScriptLoaded() const { return scriptLoaded; }
    bool SignatureVerified() const { return signatureVerified; }
    const std::string& ScriptError() const { return scriptError; }

    float ResolveGravityVelocity(float currentVelocity, float dt, bool grounded,
                                 float defaultGravity, float terminalFallSpeed) override;
    float ResolveJumpVelocity(bool grounded, float defaultJumpSpeed) override;
    float ResolveDamage(float currentHealth, float maxHealth, float amount) override;
    float ResolveHeal(float currentHealth, float maxHealth, float amount) override;

    float FallDamage(float landingSpeed);
    bool ShouldRagdoll(float health, float impactSpeed, float damage);
    float RagdollDuration(float impactSpeed);
    float RagdollDirection(float horizontalSpeedX, float horizontalSpeedZ);

private:
    bool LoadScriptInternal();
    VekScriptEngine vek;
    std::string scriptFile;
    std::string scriptError;
    bool scriptLoaded = false;
    bool signatureVerified = false;
};
