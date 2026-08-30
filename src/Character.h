// v26 note: animated door interaction, richer character rig, fuller hair, day/night cycle, and dev cheat panel groundwork.
#pragma once
#include "raylib.h"
#include "CharacterRotationSystem.h"
#include "HumanoidSystem.h"
#include <VekGameSystems.h>
#include <VekPhysicsSystems.h>
#include <string>
#include <vector>

// Fully original procedural humanoid avatar for the custom C++ game.
// The body is driven by a small hierarchical joint rig, so every major body
// section can be posed independently without requiring a copyrighted model.

enum class Presentation { Neutral, Masculine, Feminine };
enum class FaceShape { Oval, Round, Square, Heart };
enum class HairStyle { Short, Long, Curly, Straight, Braids, Ponytail, Afro, Wavy, Locs, Bun, Bob, Coils };
static constexpr int HairStyleCount = 12;
enum class ClothingStyle { WorkShirt, Tshirt, Overalls, Utility, Jacket, SafetyVest, PilotGear, MarineGear, IndustrialGear, CompanyUniform };
enum class AccessoryStyle { None, Glasses, Goggles, Cap, Helmet, Backpack };
enum class EquipmentType { Wrench, Hammer, Welder, Scanner, Tablet, Flashlight, FuelCan, TowStrap, GPS };

enum class CharacterAnimState {
    Idle, Walk, Run, Sprint, Jump, Land, Crouch, TurnInPlace,
    Pickup, Place, Push, Pull, RotatePart, Repair, UseTool, Menu,
    EnterVehicle, ExitVehicle, OpenDoor, UseKeypad, SeatedDrive, CarryMedium, CarryHeavy,
    Inspect, KneelRepair, Celebrate, Hurt
};

struct AvatarAppearance {
    Presentation presentation = Presentation::Neutral;
    float bodySize = 1.0f;       // 0.78..1.28
    float height = 1.0f;         // 0.86..1.16
    float shoulderWidth = 1.0f;  // 0.78..1.25
    float armSize = 1.0f;        // 0.82..1.20
    float legSize = 1.0f;        // 0.82..1.20
    int skinTone = 4;            // palette index
    FaceShape faceShape = FaceShape::Oval;
    int eyeStyle = 0;
    int eyebrowStyle = 0;
    int noseStyle = 0;
    int lipStyle = 0;
    int jawStyle = 0;
    int earStyle = 0;
    bool freckles = false;
    HairStyle hair = HairStyle::Curly;
    int hairColor = 2;
    ClothingStyle clothing = ClothingStyle::Overalls;
    int clothingColor = 0;
    AccessoryStyle accessory = AccessoryStyle::None;
    bool gloves = true;
};

class CharacterAnimationSystem {
public:
    CharacterAnimState state = CharacterAnimState::Idle;
    CharacterAnimState previous = CharacterAnimState::Idle;
    float stateTime = 0.0f;
    float gait = 0.0f;
    float blend = 1.0f;

    float leftArmSwing = 0.0f;
    float rightArmSwing = 0.0f;
    float leftLegSwing = 0.0f;
    float rightLegSwing = 0.0f;
    float crouch = 0.0f;
    float torsoLean = 0.0f;
    float handRaise = 0.0f;
    float kneeBend = 0.0f;

    // Higher-quality procedural locomotion layers. These are local pose
    // offsets, not world-space character rotations.
    float torsoYawTwist = 0.0f;
    float pelvisYawTwist = 0.0f;
    float rootBob = 0.0f;
    float shoulderBob = 0.0f;
    float leftFootLift = 0.0f;
    float rightFootLift = 0.0f;
    float jumpTuck = 0.0f;
    float landingCompression = 0.0f;
    float airborneVerticalVelocity = 0.0f;
    float airborneTime = 0.0f;
    float lastLandingSpeed = 0.0f;

    // Rotation/locomotion data supplied by CharacterRotationSystem.
    float movementSpeed = 0.0f;
    Vector3 movementDirection{0.0f,0.0f,0.0f};
    float turnAngle = 0.0f;
    float rotationAngularVelocity = 0.0f;
    float headYawOffset = 0.0f;
    float upperBodyYawOffset = 0.0f;
    float leftFootYawOffset = 0.0f;
    float rightFootYawOffset = 0.0f;
    bool isTurning = false;
    bool isMoving = false;
    bool isSprinting = false;
    bool isCrouching = false;
    bool isCarrying = false;
    bool isRepairing = false;
    bool isSeated = false;
    CharacterTurnState turnState = CharacterTurnState::None;

    void SetState(CharacterAnimState next);
    void Update(float dt, float movementAmount);
    void ApplyRotationData(const CharacterRotationSystem& rotation, float speed, bool sprinting, bool crouching, bool carrying, bool repairing, bool seated);
    void ApplyHumanoidData(const HumanoidSystem& humanoid);
};

class EquipmentSystem {
public:
    EquipmentType selected = EquipmentType::Wrench;
    bool visible = true;
    void Next();
    const char* Name() const;
};

struct ArmPhysicsConstraint {
    bool enabled = false;
    Vector3 desiredTarget{0.0f,0.0f,0.0f};
    Vector3 simulatedTarget{0.0f,0.0f,0.0f};
    Vector3 velocity{0.0f,0.0f,0.0f};
    float weight = 0.0f;
    float targetWeight = 0.0f;
    float grip = 0.0f;
};

struct HairRigState {
    float time = 0.0f;
    float sideLag = 0.0f;
    float backLag = 0.0f;
    float bounce = 0.0f;
    float turbulence = 0.0f;
    float previousSpeed = 0.0f;
    float previousVerticalVelocity = 0.0f;
    HairStyle configuredStyle = HairStyle::Curly;
    bool physicsInitialized = false;
    std::vector<vek::SpringChain3D> strands;
};

class PlayerCharacterSystem {
public:
    AvatarAppearance appearance;
    CharacterAnimationSystem animation;
    CharacterRotationSystem rotation;
    HumanoidSystem humanoid;
    EquipmentSystem equipment;
    vek::RagdollState ragdoll;
    vek::RagdollSettings ragdollSettings;
    vek::HumanoidRigDefinition rigDefinition;
    HairRigState hairRig;
    // Soft spring constraints drive the procedural arm IK. This is intentionally
    // renderer-independent: gameplay supplies a world-space grip point and the
    // arm follows with damped physical lag instead of snapping like a mannequin.
    ArmPhysicsConstraint leftArmPhysics;
    ArmPhysicsConstraint rightArmPhysics;

    void SetRigDefinition(const vek::HumanoidRigDefinition& rig) { rigDefinition = rig; }

    bool creatorOpen = false;
    bool firstPerson = false;
    bool crouching = false;
    bool sprinting = false;
    bool carryingHeavy = false;
    bool carryingMedium = false;
    bool hurt = false;
    bool forceWalkAnimation = false;

    // Creator selection row.
    int creatorRow = 0;
    int reputation = 0;
    float avatarPreviewYaw = 180.0f;

    void Update(float dt, float movementAmount, bool driving);
    void UpdateCreator();
    void Draw(Vector3 position, float yawDegrees, bool seated = false) const;
    void DrawFirstPersonArms(const Camera3D& camera, bool driving) const;
    void DrawCreatorUI() const;
    void DrawEquipment(Vector3 handPosition, float yawDegrees) const;

    void Trigger(CharacterAnimState state);
    void ToggleCreator();
    void CycleCamera();
    void NextEquipment();
    void TriggerRagdoll(float impactSpeed, float directionSign, float durationOverride = -1.0f);
    void UpdateRagdoll(float dt);
    bool IsRagdollActive() const;
    void SetArmPhysicsTarget(bool rightArm, Vector3 targetWorld, float grip=1.0f);
    void ReleaseArmPhysics(bool rightArm);
    void ReleaseAllArmPhysics();
    bool ArmPhysicsActive(bool rightArm) const;

    bool SaveAppearance() const;
    bool LoadAppearance();

    Color SkinColor() const;
    Color HairColor() const;
    Color ClothingColor() const;

private:
    void DrawHead(Vector3 center, float yaw, float scale) const;
    void DrawHair(Vector3 head, float yaw, float scale) const;
    void DrawAccessory(Vector3 head, float yaw, float scale) const;
    void DrawLimb(Vector3 a, Vector3 b, float radius, Color color) const;
    void DrawYawedBox(Vector3 center, Vector3 size, float yawDegrees, Color color) const;
    void DrawOrientedBox(Vector3 center, Vector3 size, float yawDegrees, float pitchDegrees, float rollDegrees, Color color) const;
    struct HairStrandSpec {
        vek::PhysicsVec3 root{};
        float length = 0.2f;
        int segments = 4;
        float curl = 0.0f;
        float phase = 0.0f;
        float radius = 0.025f;
        float stiffnessScale = 1.0f;
        float gravityScale = 1.0f;
    };
    void DrawTaperedBodySegment(Vector3 center, float bottomWidth, float topWidth, float height, float bottomDepth, float topDepth, float yawDegrees, float pitchDegrees, float rollDegrees, Color color, int sides=16) const;
    std::vector<HairStrandSpec> BuildHairStrandSpecs() const;
    vek::SecondaryMotionProfile HairPhysicsProfile() const;
    void DrawHairStrand(std::size_t strandIndex, Vector3 head, float yaw, float scale, const HairStrandSpec& spec, Color color) const;
    void UpdateHairRig(float dt);
    void UpdateArmPhysics(float dt);
    Vector3 RotateY(Vector3 local, float yaw) const;
    const char* CreatorLabel(int row) const;
    void AdjustCreatorValue(int row, int direction);
};
