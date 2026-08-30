#include "VekCharacterRules.h"
#include "VekSecuritySystem.h"
#include <VekGameSystems.h>
#include "VekHostSecurity.h"

#include <algorithm>
#include <cmath>

namespace {
float finiteOr(float value, float fallback) {
    return std::isfinite(value) ? value : fallback;
}
}

bool VekCharacterRules::Initialize(const std::string& scriptPath) {
    scriptFile = scriptPath;
    return LoadScriptInternal();
}

bool VekCharacterRules::LoadScriptInternal() {
    VekScriptEngine fresh;
    ApplyGameVekSecurityPolicy(fresh);
    vek::VekRegisterGameplayLibrary(fresh);
    fresh.SealNativeRegistry();

    VekVerifiedScript verified = VekSecuritySystem::VerifyScript(scriptFile, scriptFile + ".sig");
    if (!verified.ok) {
        scriptLoaded = false;
        signatureVerified = false;
        scriptError = verified.error;
        return false;
    }
    signatureVerified = verified.signatureVerified;
    if (!fresh.LoadSource(verified.source, scriptFile)) {
        scriptLoaded = false;
        scriptError = fresh.LastError();
        return false;
    }
    const char* required[] = {
        "character_gravity", "character_jump", "character_damage", "character_heal",
        "fall_damage", "should_ragdoll", "ragdoll_duration", "ragdoll_direction"
    };
    for (const char* fn : required) {
        if (!fresh.HasFunction(fn)) {
            scriptLoaded = false;
            scriptError = std::string("VEK: player_systems.vek missing fn ") + fn;
            return false;
        }
    }
    vek = std::move(fresh);
    scriptLoaded = true;
    scriptError.clear();
    return true;
}

bool VekCharacterRules::ReloadScript() {
    if (!CanHotReload()) {
        scriptError = "VEK Guard: character-rule hot reload is disabled in secure release";
        return false;
    }
    return LoadScriptInternal();
}

bool VekCharacterRules::CanHotReload() const { return VekSecuritySystem::IsDevelopmentMode(); }

float VekCharacterRules::ResolveGravityVelocity(float currentVelocity, float dt, bool grounded,
                                                float defaultGravity, float terminalFallSpeed) {
    float fallback = grounded ? 0.0f : std::max(currentVelocity - defaultGravity * std::clamp(dt,0.0f,0.1f), -terminalFallSpeed);
    if (!scriptLoaded) return fallback;
    VekValue v = vek.Call("character_gravity", {currentVelocity, dt, grounded, defaultGravity, terminalFallSpeed});
    if (!vek.LastError().empty()) { scriptError = vek.LastError(); return fallback; }
    return std::clamp(finiteOr((float)v.AsNumber(), fallback), -std::fabs(terminalFallSpeed), 100.0f);
}

float VekCharacterRules::ResolveJumpVelocity(bool grounded, float defaultJumpSpeed) {
    float fallback = grounded ? defaultJumpSpeed : 0.0f;
    if (!scriptLoaded) return fallback;
    VekValue v = vek.Call("character_jump", {grounded, defaultJumpSpeed});
    if (!vek.LastError().empty()) { scriptError = vek.LastError(); return fallback; }
    return std::clamp(finiteOr((float)v.AsNumber(), fallback), 0.0f, 30.0f);
}

float VekCharacterRules::ResolveDamage(float currentHealth, float maxHealth, float amount) {
    float fallback = std::clamp(currentHealth - std::max(0.0f,amount), 0.0f, maxHealth);
    if (!scriptLoaded) return fallback;
    VekValue v = vek.Call("character_damage", {currentHealth, maxHealth, amount});
    if (!vek.LastError().empty()) { scriptError = vek.LastError(); return fallback; }
    return std::clamp(finiteOr((float)v.AsNumber(), fallback), 0.0f, maxHealth);
}

float VekCharacterRules::ResolveHeal(float currentHealth, float maxHealth, float amount) {
    float fallback = std::clamp(currentHealth + std::max(0.0f,amount), 0.0f, maxHealth);
    if (!scriptLoaded) return fallback;
    VekValue v = vek.Call("character_heal", {currentHealth, maxHealth, amount});
    if (!vek.LastError().empty()) { scriptError = vek.LastError(); return fallback; }
    return std::clamp(finiteOr((float)v.AsNumber(), fallback), 0.0f, maxHealth);
}

float VekCharacterRules::FallDamage(float landingSpeed) {
    float fallback = landingSpeed > 11.0f ? std::min(100.0f,(landingSpeed-11.0f)*4.0f) : 0.0f;
    if (!scriptLoaded) return fallback;
    VekValue v = vek.Call("fall_damage", {landingSpeed});
    if (!vek.LastError().empty()) { scriptError = vek.LastError(); return fallback; }
    return std::clamp(finiteOr((float)v.AsNumber(), fallback), 0.0f, 100.0f);
}

bool VekCharacterRules::ShouldRagdoll(float health, float impactSpeed, float damage) {
    bool fallback = health <= 0.0f || (damage > 0.0f && impactSpeed >= 13.0f);
    if (!scriptLoaded) return fallback;
    VekValue v = vek.Call("should_ragdoll", {health, impactSpeed, damage});
    if (!vek.LastError().empty()) { scriptError = vek.LastError(); return fallback; }
    return v.AsBool(fallback);
}

float VekCharacterRules::RagdollDuration(float impactSpeed) {
    float fallback = std::clamp(0.8f + (impactSpeed-13.0f)*0.12f, 0.8f, 3.0f);
    if (!scriptLoaded) return fallback;
    VekValue v = vek.Call("ragdoll_duration", {impactSpeed});
    if (!vek.LastError().empty()) { scriptError = vek.LastError(); return fallback; }
    return std::clamp(finiteOr((float)v.AsNumber(), fallback), 0.25f, 8.0f);
}

float VekCharacterRules::RagdollDirection(float x, float z) {
    float fallback = (std::fabs(x) > std::fabs(z) ? x : z) < 0.0f ? -1.0f : 1.0f;
    if (!scriptLoaded) return fallback;
    VekValue v = vek.Call("ragdoll_direction", {x,z});
    if (!vek.LastError().empty()) { scriptError = vek.LastError(); return fallback; }
    float n = finiteOr((float)v.AsNumber(), fallback);
    return n < 0.0f ? -1.0f : 1.0f;
}
