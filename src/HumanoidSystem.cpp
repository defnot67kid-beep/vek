#include "HumanoidSystem.h"
#include <algorithm>
#include <cmath>

void HumanoidSystem::Reset() {
    health = settings.maxHealth;
    stamina = settings.maxStamina;
    verticalVelocity = 0.0f;
    airborneTime = 0.0f;
    lastLandingSpeed = 0.0f;
    lastDamageAmount = 0.0f;
    grounded = true;
    alive = true;
}

void HumanoidSystem::UpdateVitals(float dt, bool sprinting) {
    dt = std::clamp(dt, 0.0f, 0.10f);
    if (!alive) return;
    if (sprinting && grounded) stamina -= settings.sprintStaminaDrainPerSecond * dt;
    else stamina += settings.staminaRecoveryPerSecond * dt;
    stamina = std::clamp(stamina, 0.0f, settings.maxStamina);
}

bool HumanoidSystem::CanJump() const { return alive && grounded; }

bool HumanoidSystem::BeginJump(float customJumpSpeed) {
    if (!CanJump()) return false;
    float base = customJumpSpeed > 0.0f ? customJumpSpeed : settings.jumpSpeed;
    float resolved = rules ? rules->ResolveJumpVelocity(true, base) : base;
    if (resolved <= 0.0f) return false;
    grounded = false;
    airborneTime = 0.0f;
    lastLandingSpeed = 0.0f;
    verticalVelocity = resolved;
    return true;
}

bool HumanoidSystem::UpdateVertical(float dt, float groundY, float& worldY) {
    dt = std::clamp(dt, 0.0f, 0.10f);
    if (grounded) {
        worldY = groundY;
        verticalVelocity = 0.0f;
        return false;
    }
    airborneTime += dt;
    if (rules) verticalVelocity = rules->ResolveGravityVelocity(verticalVelocity, dt, false, settings.gravity, settings.terminalFallSpeed);
    else {
        verticalVelocity -= settings.gravity * dt;
        verticalVelocity = std::max(verticalVelocity, -settings.terminalFallSpeed);
    }
    if (!std::isfinite(verticalVelocity)) verticalVelocity = -settings.terminalFallSpeed;
    verticalVelocity = std::clamp(verticalVelocity, -settings.terminalFallSpeed, 100.0f);
    worldY += verticalVelocity * dt;
    if (worldY <= groundY) {
        lastLandingSpeed = std::fabs(verticalVelocity);
        worldY = groundY;
        verticalVelocity = 0.0f;
        grounded = true;
        airborneTime = 0.0f;
        return true;
    }
    return false;
}

void HumanoidSystem::ApplyDamage(float amount) {
    lastDamageAmount = 0.0f;
    if (amount <= 0.0f || !alive) return;
    float before = health;
    health = rules ? rules->ResolveDamage(health, settings.maxHealth, amount)
                   : std::max(0.0f, health - amount);
    health = std::clamp(health, 0.0f, settings.maxHealth);
    lastDamageAmount = std::max(0.0f, before - health);
    alive = health > 0.0f;
}

void HumanoidSystem::Heal(float amount) {
    if (amount <= 0.0f) return;
    health = rules ? rules->ResolveHeal(health, settings.maxHealth, amount)
                   : std::min(settings.maxHealth, health + amount);
    health = std::clamp(health, 0.0f, settings.maxHealth);
    if (health > 0.0f) alive = true;
}

void HumanoidSystem::SetHealth(float value) {
    health = std::clamp(value, 0.0f, settings.maxHealth);
    alive = health > 0.0f;
}

float HumanoidSystem::GetHealth() const { return health; }
float HumanoidSystem::GetMaxHealth() const { return settings.maxHealth; }
float HumanoidSystem::GetHealthNormalized() const { return settings.maxHealth > 0.0f ? health/settings.maxHealth : 0.0f; }
float HumanoidSystem::GetStamina() const { return stamina; }
float HumanoidSystem::GetStaminaNormalized() const { return settings.maxStamina > 0.0f ? stamina/settings.maxStamina : 0.0f; }
float HumanoidSystem::GetVerticalVelocity() const { return verticalVelocity; }
float HumanoidSystem::GetAirborneTime() const { return airborneTime; }
float HumanoidSystem::GetLastLandingSpeed() const { return lastLandingSpeed; }
float HumanoidSystem::GetLastDamageAmount() const { return lastDamageAmount; }
bool HumanoidSystem::IsAlive() const { return alive; }
bool HumanoidSystem::IsHurt() const { return alive && GetHealthNormalized() <= settings.hurtHealthThreshold; }
bool HumanoidSystem::IsGrounded() const { return grounded; }
