#pragma once

// Stable humanoid bone identifiers for future hitboxes, IK, equipment sockets,
// multiplayer replication and per-body-part damage.
enum class HumanoidBone {
    Root, Pelvis, Spine, Chest, Neck, Head,
    LeftUpperArm, LeftLowerArm, LeftHand,
    RightUpperArm, RightLowerArm, RightHand,
    LeftUpperLeg, LeftLowerLeg, LeftFoot,
    RightUpperLeg, RightLowerLeg, RightFoot
};

struct HumanoidSettings {
    float maxHealth = 100.0f;
    float maxStamina = 100.0f;
    float jumpSpeed = 6.4f;
    float gravity = 15.5f;
    float terminalFallSpeed = 32.0f;
    float hurtHealthThreshold = 0.35f;
    float staminaRecoveryPerSecond = 18.0f;
    float sprintStaminaDrainPerSecond = 10.0f;
};

// Optional rule provider lets an embedded language (VEK in this game) own the
// tunable gravity/health rules while HumanoidSystem remains reusable C++ state.
class HumanoidRuleProvider {
public:
    virtual ~HumanoidRuleProvider() = default;
    virtual float ResolveGravityVelocity(float currentVelocity, float dt, bool grounded,
                                         float defaultGravity, float terminalFallSpeed) = 0;
    virtual float ResolveJumpVelocity(bool grounded, float defaultJumpSpeed) = 0;
    virtual float ResolveDamage(float currentHealth, float maxHealth, float amount) = 0;
    virtual float ResolveHeal(float currentHealth, float maxHealth, float amount) = 0;
};

class HumanoidSystem {
public:
    HumanoidSettings settings;

    void SetRuleProvider(HumanoidRuleProvider* provider) { rules = provider; }
    HumanoidRuleProvider* GetRuleProvider() const { return rules; }

    void Reset();
    void UpdateVitals(float dt, bool sprinting);

    bool CanJump() const;
    bool BeginJump(float customJumpSpeed = -1.0f);
    bool UpdateVertical(float dt, float groundY, float& worldY);

    void ApplyDamage(float amount);
    void Heal(float amount);
    void SetHealth(float value);

    float GetHealth() const;
    float GetMaxHealth() const;
    float GetHealthNormalized() const;
    float GetStamina() const;
    float GetStaminaNormalized() const;
    float GetVerticalVelocity() const;
    float GetAirborneTime() const;
    float GetLastLandingSpeed() const;
    float GetLastDamageAmount() const;

    bool IsAlive() const;
    bool IsHurt() const;
    bool IsGrounded() const;

private:
    HumanoidRuleProvider* rules = nullptr;
    float health = 100.0f;
    float stamina = 100.0f;
    float verticalVelocity = 0.0f;
    float airborneTime = 0.0f;
    float lastLandingSpeed = 0.0f;
    float lastDamageAmount = 0.0f;
    bool grounded = true;
    bool alive = true;
};
