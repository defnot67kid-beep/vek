// v26 note: animated door interaction, richer character rig, fuller hair, day/night cycle, and dev cheat panel groundwork.
#pragma once
#include "raylib.h"
#include "Types.h"
#include "Vehicle.h"
#include "VehicleBuilder.h"
#include "World.h"
#include "Jobs.h"
#include "Character.h"
#include "CameraRotationSystem.h"
#include "MapSystem.h"
#include "CollisionSystem.h"
#include "EscapeMenuSystem.h"
#include "WindowInputSystem.h"
#include "VekCharacterRules.h"
#include "VekVehicleEditorRules.h"
#include "MainMenuSystem.h"
#include "VekPartRegistrySystem.h"
#include "VekHangarRules.h"
#include "VekVehicleEditorGuiSystem.h"
#include "EditorCameraSystem.h"
#include "VekJobRules.h"
#include "VekHangarInteractionSystem.h"
#include "VekLifeCycleSystem.h"
#include "VekCameraWorldRules.h"
#include "VekSkyRenderer.h"
#include "VekAuthorityRules.h"
#include <VekEditorSystems.h>
#include <string>

enum class CameraViewMode { ThirdPerson, CloseThirdPerson, FirstPerson, FreeInspection };

enum class PersonnelDoorSequencePhase { None, Approach, Reach, TurnHandle, PullDoor, WalkInside };

class Game {
public:
    void Init();
    void Run();
    void Shutdown();

private:
    PlayerState player;
    EconomyState economy;
    Vehicle vehicle;
    VehicleBuilder builder;
    World world;
    Jobs jobs;
    PlayerCharacterSystem character;
    MapSystem map;
    CollisionSystem collision;
    EscapeMenuSystem escapeMenu;
    WindowInputSystem windowInput;
    VekCharacterRules characterRules;
    VekVehicleEditorRules editorRules;
    MainMenuSystem mainMenu;
    VekPartRegistrySystem partRegistry;
    VekHangarRules hangarRules;
    VekVehicleEditorGuiSystem editorGui;
    EditorCameraSystem editorCamera;
    VekJobRules jobRules;
    VekHangarInteractionSystem hangarInteraction;
    VekLifeCycleSystem lifeCycle;
    VekCameraWorldRules cameraWorldRules;
    VekSkyRenderer skyRenderer;
    VekAuthorityRules authorityRules;
    vek::GameMode gameMode = vek::GameMode::Survival;
    bool gameModeChosen = false;

    Camera3D camera{};
    CameraViewMode cameraMode = CameraViewMode::ThirdPerson;
    bool lastArrowLeftInput = false;
    bool lastArrowRightInput = false;
    bool quitRequested = false;
    bool economyTamperWarningLatched = false;
    bool performanceHudVisible = false;

    // Camera orientation is independent from character root orientation.
    // Each gameplay camera uses the same accelerated target/current rotation model.
    CameraRotationSystem cameraRotation;
    CameraRotationSystem freeCameraRotation;
    CameraRotationSystem cockpitCameraRotation;
    CameraRotationSystem vehicleOrbitCameraRotation;
    float cameraYaw = 0.0f;
    float cameraPitch = 18.0f;

    Vector3 freeCameraPosition{8.0f,6.0f,-12.0f};
    float freeCameraYaw = 0.0f;
    float freeCameraPitch = -15.0f;
    float cockpitLookYaw = 0.0f;
    float cockpitLookPitch = 0.0f;

    Vector3 desiredMovementDirection{0.0f,0.0f,0.0f};
    Vector3 playerVelocity{0.0f,0.0f,0.0f};
    float movementAmount = 0.0f;
    float movementSpeed = 0.0f;

    bool enteringVehicle = false;
    bool wasInsideHangar = false;
    float workspaceEntryCooldown = 0.0f;
    vek::GarageDoorState garageDoorState;
    vek::PasslockState passlockState;
    bool passlockUiOpen = false;
    std::string passlockInput;
    std::string passlockStatus = "LOCKED - ENTER PIN";
    bool garageAccessGranted = false;
    bool garageOpenRequestLatched = false;
    float personnelDoorAutoCloseTimer = 0.0f;
    PersonnelDoorSequencePhase personnelDoorSequencePhase = PersonnelDoorSequencePhase::None;
    float personnelDoorSequenceTime = 0.0f;
    Vector3 personnelDoorSequenceStart{0.0f,0.0f,0.0f};
    bool workshopAutoBuildSession = false;
    bool workshopAutoBuildSuppressed = false;
    bool devCheatPanelOpen = false;
    bool devFlyEnabled = false;
    bool devNoclipEnabled = false;
    bool devGodModeEnabled = false;
    float devSpeedMultiplier = 1.0f;

    vek::DeathSequenceState resetDeathState;
    vek::ScreenEffectState resetScreenState;
    Sound resetDeathSound{};
    bool resetDeathSoundLoaded = false;
    bool audioDeviceInitialized = false;

    int vehicleEntryStage = 0;
    bool rotationDebug = false;

    std::string toast;
    float toastTime = 0.0f;

    void Update(float dt);
    void UpdateLookInput(float dt);
    void ApplyVekCameraWorldSettings();
    void ClampCameraAboveWorld();
    void PollCameraAlignmentInput(bool& arrowLeft, bool& arrowRight);
    void UpdatePlayer(float dt);
    void UpdateVehicleEntry(float dt);
    void UpdateCamera(float dt);
    void UpdateFreeCamera(float dt);
    void UpdateCharacterRotation(float dt);
    void SyncMapState();
    void Interact();
    void ResetToWorkshop();
    void UpdateResetDeath(float dt);
    void CompleteRespawnToWorkshop();
    void DrawResetDeathOverlay() const;
    void LoadResetDeathAudio();
    float CharacterGroundY(float surfaceY=0.0f) const;
    void EnterBuilderNow();
    void ExitBuilderNow();
    void UpdateGarageAccess(float dt);
    void OpenPasslockUi();
    void ClosePasslockUi();
    void UpdatePasslockUi(float dt);
    void DrawPasslockUi();
    bool PasslockClicked() const;
    bool GarageControlClicked() const;
    bool PersonnelDoorHandleClicked() const;
    void BeginPersonnelDoorEntrySequence();
    void UpdatePersonnelDoorEntrySequence(float dt);
    bool PersonnelDoorSequenceActive() const { return personnelDoorSequencePhase!=PersonnelDoorSequencePhase::None; }
    void SubmitPasslockCode();
    void UpdateDevCheatPanel();
    void DrawDevCheatPanel();

    CharacterMovementState CurrentRotationMovementState() const;
    float EffectiveCameraYaw() const;

    void Draw();
    void DrawHUD();
    void DrawPerformanceHUD() const;
    void DrawRotationDebugWorld() const;
    void DrawRotationDebugHUD() const;

    void Say(const std::string& s, float sec=4.0f);
    Vector3 Pos() const;
};
