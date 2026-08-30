#include "Game.h"
#include "VekGuiTextRenderer.h"
#include "VekSecuritySystem.h"
#include "Save.h"
#include "raymath.h"
#include "CharacterInteractionSystem.h"
#include <algorithm>
#include <cmath>

#ifndef VEK_DEVELOPMENT_MODE
#define VEK_DEVELOPMENT_MODE 0
#endif

static Vector3 HorizontalForward(float yawDegrees) {
    return CharacterRotationSystem::ForwardFromYaw(yawDegrees);
}

static Vector3 HorizontalRight(float yawDegrees) {
    Vector3 f = HorizontalForward(yawDegrees);
    return {f.z, 0.0f, -f.x};
}

static CameraViewMode CameraModeFromVekName(const std::string& name) {
    if(name=="close_third_person") return CameraViewMode::CloseThirdPerson;
    if(name=="first_person") return CameraViewMode::FirstPerson;
    if(name=="free_inspection") return CameraViewMode::FreeInspection;
    return CameraViewMode::ThirdPerson;
}
static const char* CameraModeVekName(CameraViewMode mode) {
    switch(mode){case CameraViewMode::CloseThirdPerson:return "close_third_person";case CameraViewMode::FirstPerson:return "first_person";case CameraViewMode::FreeInspection:return "free_inspection";default:return "third_person";}
}

static Vector3 MoveTowardsVector(Vector3 current, Vector3 target, float maxDelta) {
    Vector3 delta = Vector3Subtract(target, current);
    float len = Vector3Length(delta);
    if (len <= maxDelta || len < 0.00001f) return target;
    return Vector3Add(current, Vector3Scale(delta, maxDelta/len));
}

static float SmoothStep01(float t) {
    t=Clamp(t,0.0f,1.0f);
    return t*t*(3.0f-2.0f*t);
}

void Game::Init() {
    SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_WINDOW_RESIZABLE);
    InitWindow(1280,720,"Custom Vehicle Engineering Game - VEK Prototype");
    // raylib normally closes a window when ESC is pressed. Disable that default
    // so ESC belongs to our in-game Escape Menu instead.
    SetExitKey(0);
    SetTargetFPS(120);
    InitAudioDevice();
    audioDeviceInitialized=IsAudioDeviceReady();
    Save::LoadCareer(economy);
    character.LoadAppearance();
    character.reputation = economy.reputation;

    // VEK 1.3 defines the engineering headquarters dimensions and the complete
    // string-ID vehicle part catalog before map/collision cache world geometry.
    if(!hangarRules.Initialize("scripts/hangar.vek"))
        Say(std::string("VEK hangar fallback: ") + hangarRules.Error(),6.0f);
    world.ConfigureHangar(hangarRules.Area());
    if(!partRegistry.Initialize("scripts/parts"))
        Say(std::string("VEK part-registry fallback: ") + partRegistry.Error(),6.0f);
    if(!editorGui.Initialize("scripts/vehicle_editor_gui.vek"))
        Say(std::string("VEK editor-GUI fallback: ") + editorGui.Error(),6.0f);
    if(!jobRules.Initialize("scripts/jobs/delivery.vek"))
        Say(std::string("VEK job fallback: ") + jobRules.Error(),6.0f);
    if(!hangarInteraction.Initialize("scripts/hangar_interactions.vek"))
        Say(std::string("VEK hangar-interaction fallback: ") + hangarInteraction.Error(),6.0f);
    if(!lifeCycle.Initialize("scripts/lifecycle.vek"))
        Say(std::string("VEK lifecycle fallback: ") + lifeCycle.Error(),6.0f);
    if(!cameraWorldRules.Initialize("scripts/camera_world.vek"))
        Say(std::string("VEK camera/world fallback: ") + cameraWorldRules.Error(),6.0f);
    if(!authorityRules.Initialize("scripts/security_authority.vek"))
        Say(std::string("VEK authority/security fallback: ") + authorityRules.Error(),6.0f);
    performanceHudVisible=authorityRules.Shell().performanceDefaultVisible;
    if(const auto* policy=cameraWorldRules.Policy()) SetTargetFPS(policy->targetFps);
    if(const auto* rig=cameraWorldRules.Rig()) character.SetRigDefinition(*rig);
    LoadResetDeathAudio();
    const auto& doorSettings=hangarInteraction.Door();
    world.ConfigurePersonnelDoor(doorSettings.offsetX,doorSettings.width,doorSettings.height,doorSettings.openAngle);
    if(const auto* garage=hangarInteraction.Garage()){
        world.ConfigureGarageDoor(garage->width,garage->height,garage->panelCount,hangarInteraction.Access().passlockOffsetX,hangarInteraction.Access().passlockHeight,garage->panelOverlap,garage->sideSealWidth,garage->lintelHeight,std::min(garage->collisionClearFraction,0.36f));
        vek::GarageDoorSystem::Reset(garageDoorState,*garage);
        world.SetGarageDoorState(garageDoorState.openFraction,garageDoorState.locked);
    }
    vek::PasslockSystem::Reset(passlockState);
    { Vector3 spawn=world.PersonnelDoorApproachPoint(); spawn.z+=6.0f; spawn.y=CharacterGroundY(); player.position=spawn; player.yaw=180.0f; }
    editorCamera.ConfigureWorkspaceBounds({hangarRules.Area().center.x,hangarRules.Area().center.y,hangarRules.Area().center.z},
        {hangarRules.Area().size.x,hangarRules.Area().size.y,hangarRules.Area().size.z},hangarRules.Area().maxBuildHeight,1.0f);
    if(const auto* cam=cameraWorldRules.Camera()) editorCamera.ConfigureProfile(*cam);
    jobs.Configure(jobRules.Definition(),jobRules.RewardMultiplier("survival",economy.reputation.Get()));

    map.Initialize(world);
    map.SetInstructionBarVisible(authorityRules.Shell().showMapInstructionBar);
    MapSaveData mapSave;
    if(Save::LoadMapData(mapSave)) map.ApplySaveData(mapSave);

    // C++ performs fast contact geometry while our custom VEK language decides
    // collision behavior (solid/friction/bounce/damage).
    if(!collision.Initialize(world, "scripts/collision.vek"))
        Say(std::string("VEK collision script fallback: ") + collision.ScriptError(), 6.0f);

    // VEK 1.1 now owns the humanoid's tunable gravity, health, fall-damage and
    // ragdoll policy. HumanoidSystem remains the state container and safely
    // falls back to its native defaults if the script is missing/invalid.
    if(!characterRules.Initialize("scripts/player_systems.vek"))
        Say(std::string("VEK player-system fallback: ") + characterRules.ScriptError(), 6.0f);
    character.humanoid.SetRuleProvider(&characterRules);

    // VEK 1.2 owns vehicle-editor economy/unlock/validation policies and the
    // renderer-neutral main-menu command stream.
    if(!editorRules.Initialize("scripts/vehicle_editor.vek"))
        Say(std::string("VEK editor-rule fallback: ") + editorRules.ScriptError(), 6.0f);
    if(!mainMenu.Initialize("scripts/main_menu.vek"))
        Say(std::string("VEK main-menu fallback: ") + mainMenu.ScriptError(), 6.0f);

    character.rotation.SnapBodyYaw(player.yaw);
    // Body should always track the camera's facing (idle or moving), not just
    // snap into line once the view has swung far past a threshold.
    character.rotation.SetRotationMode(CharacterMovementRotationMode::FaceCameraDirection);

    // Gameplay camera rotation remains a target/current smoothing system, but
    // gameplay no longer feeds it mouse movement. Physical Left/Right Arrow
    // alignment changes the target and the camera eases toward that angle.
    cameraRotation.SetPitchLimits(-25.0f, 65.0f);
    cameraRotation.SetSensitivity(0.12f, 0.10f);
    cameraRotation.Snap(player.yaw, 18.0f);
    cameraYaw = cameraRotation.GetYaw();
    cameraPitch = cameraRotation.GetPitch();

    freeCameraRotation.SetPitchLimits(-80.0f, 80.0f);
    freeCameraRotation.SetSensitivity(0.12f, 0.10f);
    freeCameraRotation.SetPitchInputSign(-1.0f); // preserve existing free-camera vertical feel
    freeCameraRotation.Snap(cameraYaw, cameraPitch);

    cockpitCameraRotation.SetYawClamp(true, -90.0f, 90.0f);
    cockpitCameraRotation.SetPitchLimits(-45.0f, 55.0f);
    cockpitCameraRotation.SetSensitivity(0.10f, 0.09f);
    cockpitCameraRotation.SetPitchInputSign(-1.0f); // preserve existing cockpit vertical feel
    cockpitCameraRotation.Snap(0.0f, 0.0f);

    // Third-person vehicle camera stores a RELATIVE orbit around vehicle heading.
    // The vehicle can turn underneath it while the player's chosen orbit offset stays intact.
    vehicleOrbitCameraRotation.SetYawClamp(true,-180.0f,180.0f);
    vehicleOrbitCameraRotation.SetPitchLimits(-10.0f,65.0f);
    vehicleOrbitCameraRotation.SetSensitivity(0.12f,0.10f);
    vehicleOrbitCameraRotation.Snap(0.0f,18.0f);
    ApplyVekCameraWorldSettings();

    camera.position={8,6,-12};
    camera.target=player.position;
    camera.up={0,1,0};
    camera.fovy=60;
    camera.projection=CAMERA_PERSPECTIVE;
    // Keep a normal desktop pointer. WindowInputSystem reinforces this at both
    // the raylib/GLFW level and (on Windows) the native capture/clip level.
    windowInput.Initialize();
}

void Game::Run() {
    while(!quitRequested && !WindowShouldClose()) {
        float dt=GetFrameTime();
        float maxDt=0.05f;if(const auto* policy=cameraWorldRules.Policy())maxDt=policy->physicsMaxDt;
        if(dt>maxDt)dt=maxDt;
        Update(dt);
        Draw();
    }
}

void Game::Shutdown() {
    character.SaveAppearance();
    if(gameMode==vek::GameMode::Survival) Save::SaveCareer(economy);
    Save::SaveMapData(map.ExportSaveData());
    windowInput.Shutdown();
    if(resetDeathSoundLoaded) UnloadSound(resetDeathSound);
    if(audioDeviceInitialized) CloseAudioDevice();
    CloseWindow();
}

Vector3 Game::Pos() const { return player.driving?vehicle.position:player.position; }
void Game::Say(const std::string&s,float sec){toast=s;toastTime=sec;}

float Game::CharacterGroundY(float surfaceY) const {
    return lifeCycle.GroundRootY(surfaceY,character.appearance.height);
}

void Game::LoadResetDeathAudio(){
    if(resetDeathSoundLoaded){UnloadSound(resetDeathSound);resetDeathSoundLoaded=false;}
    if(!audioDeviceInitialized)return;
    const auto* cue=lifeCycle.ResetAudioCue();
    if(!cue||cue->assetId.empty()||cue->assetId.find("..")!=std::string::npos||cue->assetId.find(':')!=std::string::npos)return;
    std::string path="assets/"+cue->assetId;
    if(!FileExists(path.c_str()))return;
    resetDeathSound=LoadSound(path.c_str());
    resetDeathSoundLoaded=resetDeathSound.frameCount>0;
    if(resetDeathSoundLoaded){SetSoundVolume(resetDeathSound,std::clamp(cue->volume,0.0f,1.0f));SetSoundPitch(resetDeathSound,std::clamp(cue->pitch,0.25f,4.0f));}
}

void Game::ApplyVekCameraWorldSettings() {
    const auto* p=cameraWorldRules.Camera();
    if(!p)return;
    auto apply=[&](CameraRotationSystem& c,float minPitch,float maxPitch){
        c.SetPitchLimits(minPitch,maxPitch);
        c.SetSensitivity(p->yawSensitivity,p->pitchSensitivity);
        c.settings.maxYawSpeed=p->maxYawSpeed;
        c.settings.maxPitchSpeed=p->maxPitchSpeed;
        c.settings.yawAcceleration=p->yawAcceleration;
        c.settings.yawDeceleration=p->yawDeceleration;
        c.settings.pitchAcceleration=p->pitchAcceleration;
        c.settings.pitchDeceleration=p->pitchDeceleration;
        c.SetPitchInputSign(p->invertMouseY?-1.0f:1.0f);
    };
    apply(cameraRotation,p->pitchMin,p->pitchMax);
    apply(freeCameraRotation,std::max(-89.0f,p->pitchMin-45.0f),std::min(89.0f,p->pitchMax+20.0f));
    apply(cockpitCameraRotation,std::max(-70.0f,p->pitchMin),std::min(75.0f,p->pitchMax));
    apply(vehicleOrbitCameraRotation,std::max(-35.0f,p->pitchMin),std::min(80.0f,p->pitchMax));
    editorCamera.ConfigureProfile(*p);
    camera.fovy=p->fov;
    if(const auto* policy=cameraWorldRules.Policy()) SetTargetFPS(policy->targetFps);
    if(const auto* rig=cameraWorldRules.Rig()) character.SetRigDefinition(*rig);
}

void Game::ClampCameraAboveWorld(){
    const auto* cp=cameraWorldRules.Camera();
    if(cp && !cp->preventBelowWorld)return;
    float minPosition=cp?cp->minimumWorldY:0.35f;
    float minTarget=cp?cp->minimumTargetY:0.05f;
    minPosition=std::max(minPosition,authorityRules.Shell().cameraFloorClearance);
    minTarget=std::max(minTarget,authorityRules.Shell().cameraTargetFloorClearance);
    camera.position.y=std::max(camera.position.y,minPosition);
    camera.target.y=std::max(camera.target.y,minTarget);
}

float Game::EffectiveCameraYaw() const {
    if (player.driving) {
        if (cameraMode==CameraViewMode::FirstPerson)
            return CharacterRotationSystem::NormalizeAngle(vehicle.heading + cockpitLookYaw);
        return CharacterRotationSystem::NormalizeAngle(vehicle.heading);
    }
    return CharacterRotationSystem::NormalizeAngle(cameraYaw);
}


void Game::PollCameraAlignmentInput(bool& arrowLeft, bool& arrowRight) {
    // PHYSICAL keyboard arrow keys only. These are not the < and > symbols.
    // The yaw direction is intentionally inverted per the user's request.
    arrowLeft=IsKeyPressed(KEY_LEFT);
    arrowRight=IsKeyPressed(KEY_RIGHT);

    lastArrowLeftInput=arrowLeft;
    lastArrowRightInput=arrowRight;
}

void Game::UpdateLookInput(float dt) {
    // VEK 1.8 camera policy: the desktop pointer remains completely free until
    // RMB is held. While held, raw per-frame mouse delta drives the same smooth
    // target/current rotation system used by keyboard alignment.
    if(character.creatorOpen || builder.active || map.BlocksGameplayInput() || cameraMode==CameraViewMode::FreeInspection)
        return;

    const auto* cam=cameraWorldRules.Camera();
    const auto* policy=cameraWorldRules.Policy();
    float alignStep=cam?cam->alignmentStep:45.0f;
    bool rmbAllowed=(cam?cam->rmbLook:true)&&(policy?policy->cameraRmbLook:true);
    bool rmbLook=rmbAllowed&&IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    Vector2 mouseDelta=rmbLook?GetMouseDelta():Vector2{0,0};
    if(cam){mouseDelta.x*=cam->rmbYawScale;mouseDelta.y*=cam->rmbPitchScale;}

    bool arrowLeft=false,arrowRight=false;
    PollCameraAlignmentInput(arrowLeft,arrowRight);
    const int leftDirection=CameraRotationSystem::AlignmentDirectionForArrowKey(KEY_LEFT);
    const int rightDirection=CameraRotationSystem::AlignmentDirectionForArrowKey(KEY_RIGHT);

    if(player.driving) {
        if(cameraMode==CameraViewMode::FirstPerson) {
            if(arrowLeft) cockpitCameraRotation.AlignYawStep(leftDirection,alignStep);
            if(arrowRight) cockpitCameraRotation.AlignYawStep(rightDirection,alignStep);
            if(rmbLook) cockpitCameraRotation.AddMouseDelta(mouseDelta);
            cockpitCameraRotation.Update(dt);
            cockpitLookYaw=cockpitCameraRotation.GetYaw();
            cockpitLookPitch=cockpitCameraRotation.GetPitch();
        } else {
            if(arrowLeft || arrowRight) {
                CameraRotationSystem worldAlignment;
                worldAlignment.Snap(CharacterRotationSystem::NormalizeAngle(vehicle.heading+vehicleOrbitCameraRotation.GetTargetYaw()),vehicleOrbitCameraRotation.GetTargetPitch());
                worldAlignment.AlignYawStep(arrowLeft?leftDirection:rightDirection,alignStep);
                float relativeTarget=CameraRotationSystem::DeltaAngle(vehicle.heading,worldAlignment.GetTargetYaw());
                vehicleOrbitCameraRotation.SetTarget(relativeTarget,vehicleOrbitCameraRotation.GetTargetPitch());
            }
            if(rmbLook) vehicleOrbitCameraRotation.AddMouseDelta(mouseDelta);
            vehicleOrbitCameraRotation.Update(dt);
        }
        return;
    }

    if(arrowLeft) cameraRotation.AlignYawStep(leftDirection,alignStep);
    if(arrowRight) cameraRotation.AlignYawStep(rightDirection,alignStep);
    if(rmbLook) cameraRotation.AddMouseDelta(mouseDelta);
    cameraRotation.Update(dt);
    cameraYaw=cameraRotation.GetYaw();
    cameraPitch=cameraRotation.GetPitch();
}

void Game::Update(float dt) {
    character.reputation=economy.reputation;

    // The mouse is intentionally unmanaged during gameplay. It stays a normal
    // OS pointer and is never used to rotate the camera.

    // Intercept Alt+F4 BEFORE Windows/GLFW can turn it into an immediate close.
    // First fresh press opens the Escape Menu. A second fresh press while that
    // menu is already open exits through our normal save/shutdown path.
    if(windowInput.ConsumeAltF4()) {
        EscapeMenuAction altAction=escapeMenu.HandleAltF4();
        if(altAction==EscapeMenuAction::Leave) {
            quitRequested=true;
            return;
        }
    }

    // VEK-authored frontend. The two GUI buttons are generated by main_menu.vek.
    // C++ only renders the command stream and switches the host gameplay policy.
    if(mainMenu.IsOpen()) {
        mainMenu.Update();
        if(!mainMenu.IsOpen() && mainMenu.HasSelection()) {
            gameMode=mainMenu.SelectedMode();
            gameModeChosen=true;
            builder.Configure(gameMode,&editorRules,&economy,&partRegistry.Registry(),&hangarRules.Area(),&editorGui);
            jobs.Configure(jobRules.Definition(),jobRules.RewardMultiplier(gameMode==vek::GameMode::Sandbox?"sandbox":"survival",economy.reputation.Get()));
            Say(gameMode==vek::GameMode::Sandbox?"Sandbox started: all editor parts unlocked and build costs disabled.":"Survival started: build costs and progression are active.",4.0f);
        }
        return;
    }

    if(gameMode==vek::GameMode::Sandbox && editorRules.InfiniteFuel(gameMode)) vehicle.fuel=100.0f;

    if(resetDeathState.active){
        UpdateResetDeath(dt);
        SyncMapState();
        UpdateCamera(dt);
        if(toastTime>0)toastTime-=dt;
        return;
    }

    UpdateGarageAccess(dt);
    if(passlockUiOpen) {
        UpdatePasslockUi(dt);
        SyncMapState();
        UpdateCamera(dt);
        if(toastTime>0)toastTime-=dt;
        return;
    }
#if VEK_DEVELOPMENT_MODE
    if(devCheatPanelOpen) {
        UpdateDevCheatPanel();
        SyncMapState();
        UpdateCamera(dt);
        if(toastTime>0)toastTime-=dt;
        return;
    }
#endif

    // ESC is owned by our pause/escape menu, not by raylib's window-close key.
    if(IsKeyPressed(KEY_ESCAPE)) {
        if(escapeMenu.IsOpen()) escapeMenu.Close();
        else escapeMenu.Open();
    }

    if(escapeMenu.IsOpen()) {
            EscapeMenuAction action=escapeMenu.Update();
        if(action==EscapeMenuAction::Resume) {
            escapeMenu.Close();
        } else if(action==EscapeMenuAction::Reset) {
            ResetToWorkshop();
            escapeMenu.Close();
        } else if(action==EscapeMenuAction::Leave) {
            quitRequested=true;
        }
        return;
    }

    // Guarded economy values are tamper-evident in memory. If their redundant
    // representation is inconsistent, restore the last authenticated save.
    if(!economy.IntegrityOK()) {
        Save::LoadCareer(economy);
        if(!economyTamperWarningLatched) {
            Say("VEK Guard: economy memory integrity mismatch detected; restored authenticated save.",6.0f);
            economyTamperWarningLatched=true;
        }
    } else {
        economyTamperWarningLatched=false;
    }

    // VEK camera policy only uses mouse movement while RMB is held; otherwise the pointer stays free.

    if(IsKeyPressed(KEY_F8)) {
        rotationDebug=!rotationDebug;
        Say(rotationDebug?"Character rotation debug: ON":"Character rotation debug: OFF",2.0f);
    }

    if(IsKeyPressed(KEY_F10)) {
        collision.ToggleDebug();
        Say(collision.DebugEnabled()?"VEK collision debug: ON":"VEK collision debug: OFF",2.0f);
    }
    if(IsKeyPressed(KEY_F11)) {
        if(!collision.CanHotReload() || !characterRules.CanHotReload() || !editorRules.CanHotReload()) {
            Say("VEK Guard SECURE RELEASE: hot reload is locked. Use BUILD_WINDOWS_DEV.bat while authoring scripts.",5.0f);
        } else {
            bool collisionOk=collision.ReloadScript();
            bool characterOk=characterRules.ReloadScript();
            bool editorOk=editorRules.ReloadScript();
            bool partsOk=partRegistry.Reload();
            bool hangarOk=hangarRules.Reload();
            bool guiOk=editorGui.Reload();
            bool jobOk=jobRules.Reload();
            bool interactionOk=hangarInteraction.Reload();
            bool lifecycleOk=lifeCycle.Reload();
            bool cameraWorldOk=cameraWorldRules.Reload();
            bool authorityOk=authorityRules.Reload();
            if(lifecycleOk)LoadResetDeathAudio();
            if(cameraWorldOk)ApplyVekCameraWorldSettings();
            if(authorityOk){map.SetInstructionBarVisible(authorityRules.Shell().showMapInstructionBar);performanceHudVisible=authorityRules.Shell().performanceDefaultVisible;}
            if(hangarOk){world.ConfigureHangar(hangarRules.Area());editorCamera.ConfigureWorkspaceBounds({hangarRules.Area().center.x,hangarRules.Area().center.y,hangarRules.Area().center.z},{hangarRules.Area().size.x,hangarRules.Area().size.y,hangarRules.Area().size.z},hangarRules.Area().maxBuildHeight,1.0f);}
            if(interactionOk){
                const auto& ds=hangarInteraction.Door();world.ConfigurePersonnelDoor(ds.offsetX,ds.width,ds.height,ds.openAngle);
                if(const auto* gd=hangarInteraction.Garage()){
                    world.ConfigureGarageDoor(gd->width,gd->height,gd->panelCount,hangarInteraction.Access().passlockOffsetX,hangarInteraction.Access().passlockHeight,gd->panelOverlap,gd->sideSealWidth,gd->lintelHeight,std::min(gd->collisionClearFraction,0.36f));
                    vek::GarageDoorSystem::Reset(garageDoorState,*gd);garageAccessGranted=false;garageOpenRequestLatched=false;vek::PasslockSystem::Reset(passlockState);passlockUiOpen=false;
                    world.SetGarageDoorState(garageDoorState.openFraction,garageDoorState.locked);
                }
            }
            if(hangarOk||interactionOk)collision.Initialize(world,"scripts/collision.vek");
            builder.Configure(gameMode,&editorRules,&economy,&partRegistry.Registry(),&hangarRules.Area(),&editorGui);
            if(collisionOk && characterOk && editorOk && partsOk && hangarOk && guiOk && jobOk && interactionOk && lifecycleOk && cameraWorldOk && authorityOk)
                Say("VEK DEV: gameplay + authority + camera + sky + rig + hangar + parts + GUI + access + lifecycle reloaded.",2.5f);
            else Say("VEK DEV reload completed with a fallback; check VEK diagnostics.",6.0f);
        }
    }


    if(IsKeyPressed(KEY_V) && !builder.active && !character.creatorOpen && !enteringVehicle && !map.BlocksGameplayInput()) {
        const auto* cp=cameraWorldRules.Camera();
        if(cp&&!cp->cycleModes.empty()){
            std::size_t index=0;for(std::size_t i=0;i<cp->cycleModes.size();++i)if(cp->cycleModes[i]==CameraModeVekName(cameraMode)){index=i;break;}
            cameraMode=CameraModeFromVekName(cp->cycleModes[(index+1)%cp->cycleModes.size()]);
        }else cameraMode=(CameraViewMode)(((int)cameraMode+1)%4);
        if(cameraMode==CameraViewMode::FreeInspection) {
            freeCameraPosition=camera.position;
            freeCameraRotation.Snap(cameraYaw,cameraPitch);
            freeCameraYaw=freeCameraRotation.GetYaw();
            freeCameraPitch=freeCameraRotation.GetPitch();
            character.rotation.SetCameraInfluenceEnabled(false);
            character.rotation.SnapBodyYaw(character.rotation.GetBodyYaw());
        } else {
            character.rotation.SetCameraInfluenceEnabled(true);
        }
        Say(cameraMode==CameraViewMode::ThirdPerson?"Camera: third-person":
            cameraMode==CameraViewMode::CloseThirdPerson?"Camera: close third-person":
            cameraMode==CameraViewMode::FirstPerson?"Camera: first-person/cockpit":"Camera: free inspection");
    }

    if(IsKeyPressed(KEY_C) && !player.driving && !enteringVehicle && !builder.active && !map.BlocksGameplayInput() && Vector3Distance(Pos(),world.workshop)<24) {
        character.ToggleCreator();
        if(character.creatorOpen) {
                    Say("Avatar Creator opened. SHIFT+LEFT/RIGHT rotates preview.");
        } else {
            character.SaveAppearance();
                    Say("Avatar saved.");
        }
    }

    if(character.creatorOpen) {
        character.UpdateCreator();
        character.rotation.SetCameraInfluenceEnabled(false);
        character.animation.ApplyRotationData(character.rotation,0,false,false,false,false,false);
        character.Update(dt,0,false);
        SyncMapState();
        map.Update(dt);
        UpdateCamera(dt);
        if(toastTime>0)toastTime-=dt;
        return;
    }

    // M expands the persistent minimap into the full map. Build Mode keeps its
    // reduced-opacity minimap and deliberately ignores full-map input.
    if(IsKeyPressed(KEY_M) && !enteringVehicle && !builder.active) {
        map.ToggleMap();
        if(map.GetDisplayMode()==MapDisplayMode::Minimap) Save::SaveMapData(map.ExportSaveData());
    }

    SyncMapState();
    map.Update(dt);

    // VEK shell policy assigns F to a compact performance/security HUD.
    // Builder mode keeps its context-specific F focus tool.
    if(!builder.active && !character.creatorOpen && authorityRules.Shell().performanceToggleKey=="F" && IsKeyPressed(KEY_F)) {
        performanceHudVisible=!performanceHudVisible;
    }

    // Map navigation owns WASD/mouse while open, so the player/free camera can
    // never move accidentally under the world map. Closing also keeps input
    // blocked during the short shrink transition.
    if(map.BlocksGameplayInput()) {
        if(toastTime>0)toastTime-=dt;
        return;
    }

    // Survival access is now physical and click-only: the outside keypad has
    // no floating prompt and never takes control of the character. A valid PIN
    // opens the garage, then the player walks through it normally. The native
    // host enforces VEK distance/side/line-of-sight policy before opening UI.
    if(personnelDoorAutoCloseTimer>0.0f){
        personnelDoorAutoCloseTimer=std::max(0.0f,personnelDoorAutoCloseTimer-dt);
        if(personnelDoorAutoCloseTimer<=0.0f && world.PersonnelDoorIsOpen()) world.SetPersonnelDoorTarget(false);
    }
    // Personnel-door collision geometry is dynamic too. Keep the live collision
    // cache synchronized whenever the door crosses its solid/non-solid threshold.
    if(world.UpdatePersonnelDoor(dt,hangarInteraction.Door().openSpeed))
        collision.RefreshWorldGeometry(world);

    if(builder.active && IsKeyPressed(KEY_B)) {
        bool autoSession=workshopAutoBuildSession;
        ExitBuilderNow();
        if(autoSession){
            workshopAutoBuildSuppressed=true;
            Say("Build Mode released - walk out of the workshop to end this auto-build session.",2.5f);
        }
    } else if(!builder.active && !PersonnelDoorSequenceActive()) {
        if(gameMode==vek::GameMode::Survival && PasslockClicked()) OpenPasslockUi();
        if(GarageControlClicked()) {
            if(const auto* gd=hangarInteraction.Garage()){
                bool closing=garageDoorState.openFraction>0.08f && garageDoorState.motion!=vek::GarageDoorMotion::Closing;
                if(closing){garageOpenRequestLatched=false;vek::GarageDoorSystem::RequestClose(garageDoorState);Say("Inside garage control: CLOSING",2.0f);}
                else {garageAccessGranted=true;garageOpenRequestLatched=true;vek::GarageDoorSystem::Unlock(garageDoorState);vek::GarageDoorSystem::RequestOpen(garageDoorState,*gd,true);Say("Inside garage control: OPENING",2.0f);}
            }
        }

        // B is now a physical/context action first. At the garage it controls
        // the door; once you are properly inside the workshop it opens Build
        // Mode. This prevents the old behaviour where crossing the threshold
        // stole movement and made it feel like you could not simply walk in.
        if(IsKeyPressed(KEY_B) && !player.driving && !enteringVehicle) {
            const auto* gd=hangarInteraction.Garage();
            bool nearGarage=gd && world.IsNearGarageOpening(Pos(),std::max(9.0f,gd->insideOpenDistance+2.0f));
            bool inside=world.IsInsideHangar(Pos(),0.4f);
            if(nearGarage && gd) {
                bool accessAllowed=gameMode==vek::GameMode::Sandbox || garageAccessGranted || passlockState.granted || (inside && gd->allowInsideEgress);
                if(!accessAllowed) {
                    Say("Garage locked - click the VEK passlock to unlock it.",3.0f);
                } else {
                    garageAccessGranted = garageAccessGranted || gameMode==vek::GameMode::Sandbox || inside;
                    vek::GarageDoorSystem::Unlock(garageDoorState);
                    bool mostlyOpen=garageDoorState.openFraction>0.92f || garageDoorState.motion==vek::GarageDoorMotion::Open;
                    if(mostlyOpen && inside){garageOpenRequestLatched=false;vek::GarageDoorSystem::RequestClose(garageDoorState);Say("Garage closing from inside. Use the wall button too.",2.0f);}
                    else if(mostlyOpen) Say("Garage is already open - walk through.",1.5f);
                    else { garageOpenRequestLatched=true;vek::GarageDoorSystem::RequestOpen(garageDoorState,*gd,true); Say("Garage opening - collision clears with the doorway.",2.0f); }
                }
            } else if(inside) {
                EnterBuilderNow();
            }
        }
    }

    if(builder.active) {
        // Dedicated engineering camera owns editor movement. The normal humanoid
        // controller is paused and the desktop mouse remains completely free.
        editorCamera.Update(dt);
        editorCamera.Apply(camera);
        bool placing=IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&!IsKeyDown(KEY_LEFT_SHIFT)&&!IsKeyDown(KEY_RIGHT_SHIFT);
        builder.Update(camera);

        if(IsKeyPressed(KEY_F) && !(IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT)))
            editorCamera.Focus(builder.SelectedOrVehicleCenter(),8.0f);
        if(IsKeyPressed(KEY_HOME)) editorCamera.FocusVehicle(builder.VehicleCenter(),builder.VehicleSize());
        bool shift=IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT);
        if(IsKeyPressed(KEY_ONE)){if(shift)editorCamera.Rear(builder.VehicleCenter());else editorCamera.Front(builder.VehicleCenter());}
        if(IsKeyPressed(KEY_THREE)){if(shift)editorCamera.Right(builder.VehicleCenter());else editorCamera.Left(builder.VehicleCenter());}
        if(IsKeyPressed(KEY_SEVEN)){if(shift)editorCamera.Bottom(builder.VehicleCenter());else editorCamera.Top(builder.VehicleCenter());}
        if(IsKeyPressed(KEY_O))editorCamera.ToggleOrthographic();
        std::string camCommand=builder.ConsumeCameraCommand();
        if(camCommand=="cam_front")editorCamera.Front(builder.VehicleCenter());
        else if(camCommand=="cam_side")editorCamera.Left(builder.VehicleCenter());
        else if(camCommand=="cam_top")editorCamera.Top(builder.VehicleCenter());
        else if(camCommand=="cam_focus")editorCamera.Focus(builder.SelectedOrVehicleCenter(),8.0f);

        if(placing && builder.CursorValid()) {
            CharacterInteractionSystem::PlacePart(character);
            character.rotation.FaceTarget(player.position,builder.CursorPosition());
        }
        if(builder.ConsumeConstructRequested()) {
            if(!authorityRules.AllowsLocalCommit("vehicle.finalize")){Say("VEK authority denied vehicle finalize.",3.0f);}
            else if(builder.Finalize(vehicle)) {
                character.Trigger(CharacterAnimState::UseTool);
                if(gameMode==vek::GameMode::Survival) Save::SaveCareer(economy);
                Say(std::string("Vehicle constructed. ")+builder.StatusMessage()+" Press B, then E near it.");
            } else Say(builder.StatusMessage());
        }

        character.rotation.Update(dt,{0,0,0},0.0f,cameraYaw,CharacterMovementState::Idle);
        player.yaw=character.rotation.GetBodyYaw();
        character.animation.ApplyRotationData(character.rotation,0,false,character.crouching,character.carryingHeavy||character.carryingMedium,false,false);
        character.Update(dt,0,false);

        bool keepBuildFacing = character.animation.state==CharacterAnimState::Place && character.animation.stateTime<1.2f;
        if(!keepBuildFacing && character.rotation.GetRotationMode()==CharacterMovementRotationMode::LockedDirection) {
            character.rotation.ReleaseFacingOverride();
            character.rotation.SnapBodyYaw(character.rotation.GetBodyYaw());
        }

        SyncMapState();
        editorCamera.Apply(camera);
        if(toastTime>0)toastTime-=dt;
        return;
    }

    UpdateLookInput(dt);

    if(PersonnelDoorSequenceActive()) UpdatePersonnelDoorEntrySequence(dt);
    else if(enteringVehicle) UpdateVehicleEntry(dt);
    else UpdatePlayer(dt);

    if(player.driving) {
        vehicle.UpdateDriving(dt);
        collision.ResolveVehicle(vehicle,dt);
    } else {
        // Dev noclip exists only in a VEK development build; release builds
        // compile the bypass path out and always resolve authoritative collision.
#if VEK_DEVELOPMENT_MODE
        if(!devNoclipEnabled)
#endif
            collision.ResolvePlayer(player.position,playerVelocity,character.humanoid,&vehicle,enteringVehicle,
                2.55f*character.appearance.height,0.15f*character.appearance.height);
    }

    UpdateCharacterRotation(dt);
    character.Update(dt,movementAmount,player.driving);

    // Rotation data is refreshed again after animation-state selection so the
    // procedural pose has current turn/head data in the same frame.
    bool repairing = character.animation.state==CharacterAnimState::Repair || character.animation.state==CharacterAnimState::KneelRepair || character.animation.state==CharacterAnimState::Inspect;
    character.animation.ApplyRotationData(character.rotation,movementSpeed,character.sprinting,character.crouching,
        character.carryingHeavy||character.carryingMedium,repairing,player.driving);

    bool keepFacing = enteringVehicle || PersonnelDoorSequenceActive() || character.animation.state==CharacterAnimState::Repair ||
        character.animation.state==CharacterAnimState::Inspect || character.animation.state==CharacterAnimState::KneelRepair ||
        character.animation.state==CharacterAnimState::Place || character.animation.state==CharacterAnimState::OpenDoor;
    if(!keepFacing && !player.driving) character.rotation.ReleaseFacingOverride();

    if(!PersonnelDoorSequenceActive()) Interact();
    jobs.Update(Pos(),economy);
    economy.reputation=economy.xp/250;
    SyncMapState();
    if(toastTime>0)toastTime-=dt;
    UpdateCamera(dt);
}

CharacterMovementState Game::CurrentRotationMovementState() const {
    if(PersonnelDoorSequenceActive()){
        if(personnelDoorSequencePhase==PersonnelDoorSequencePhase::Approach || personnelDoorSequencePhase==PersonnelDoorSequencePhase::WalkInside) return CharacterMovementState::Walk;
        return CharacterMovementState::Inspecting;
    }
    if(player.driving) return CharacterMovementState::Seated;
    if(enteringVehicle) return CharacterMovementState::EnteringVehicle;
    if(character.hurt) return CharacterMovementState::Hurt;
    if(character.carryingHeavy) return CharacterMovementState::HeavyCarry;
    if(character.carryingMedium) return CharacterMovementState::MediumCarry;
    if(character.animation.state==CharacterAnimState::Repair || character.animation.state==CharacterAnimState::KneelRepair) return CharacterMovementState::Repairing;
    if(character.animation.state==CharacterAnimState::Inspect) return CharacterMovementState::Inspecting;
    if(character.crouching) return CharacterMovementState::Crouch;
    if(movementAmount >= character.rotation.settings.movementDeadzone) {
        if(IsKeyDown(KEY_LEFT_SHIFT)) return CharacterMovementState::Sprint;
        if(IsKeyDown(KEY_LEFT_ALT)) return CharacterMovementState::Walk;
        return CharacterMovementState::Run;
    }
    return CharacterMovementState::Idle;
}

void Game::UpdateCharacterRotation(float dt) {
    if(cameraMode==CameraViewMode::FreeInspection) {
        // Free camera must never modify root or head rotation.
        character.rotation.SetCameraInfluenceEnabled(false);
        return;
    }

    character.rotation.SetCameraInfluenceEnabled(true);
    character.rotation.SetFirstPerson(cameraMode==CameraViewMode::FirstPerson && !player.driving);

    if(player.driving) {
        character.rotation.SetSeatedYaw(vehicle.heading);
    } else {
        character.rotation.ClearSeatedYaw();
    }

    CharacterMovementState state = CurrentRotationMovementState();
    character.rotation.Update(dt,desiredMovementDirection,movementAmount,EffectiveCameraYaw(),state);
    player.yaw=character.rotation.GetBodyYaw();

    bool repairing = state==CharacterMovementState::Repairing || state==CharacterMovementState::Inspecting;
    character.animation.ApplyRotationData(character.rotation,movementSpeed,state==CharacterMovementState::Sprint,
        state==CharacterMovementState::Crouch,state==CharacterMovementState::MediumCarry||state==CharacterMovementState::HeavyCarry,
        repairing,state==CharacterMovementState::Seated);
}

void Game::UpdatePlayer(float dt) {
    // Active ragdoll temporarily owns the humanoid pose. The player cannot
    // locomote or jump until procedural recovery finishes.
    if(character.IsRagdollActive()) {
        movementAmount=0.0f;
        movementSpeed=0.0f;
        desiredMovementDirection={0,0,0};
        playerVelocity=MoveTowardsVector(playerVelocity,{0,0,0},18.0f*dt);
        playerVelocity.y=0.0f;
        character.humanoid.UpdateVertical(dt,CharacterGroundY(),player.position.y);
        return;
    }
    movementAmount=0.0f;
    movementSpeed=Vector3Length(playerVelocity);
    desiredMovementDirection={0,0,0};
    if(player.driving) return;

#if VEK_DEVELOPMENT_MODE
    if(devFlyEnabled){
        float horizontal=(IsKeyDown(KEY_A)?1.0f:0.0f)-(IsKeyDown(KEY_D)?1.0f:0.0f);
        float vertical=(IsKeyDown(KEY_W)?1.0f:0.0f)-(IsKeyDown(KEY_S)?1.0f:0.0f);
        Vector3 f=HorizontalForward(cameraYaw),r=HorizontalRight(cameraYaw);f.y=0;r.y=0;if(Vector3Length(f)>0.001f)f=Vector3Normalize(f);if(Vector3Length(r)>0.001f)r=Vector3Normalize(r);
        Vector3 move=Vector3Add(Vector3Scale(f,vertical),Vector3Scale(r,horizontal));
        move.y=(IsKeyDown(KEY_SPACE)?1.0f:0.0f)-(IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL)?1.0f:0.0f);
        if(Vector3Length(move)>0.001f)move=Vector3Normalize(move);
        float flySpeed=12.0f*devSpeedMultiplier*(IsKeyDown(KEY_LEFT_SHIFT)?2.5f:1.0f);
        playerVelocity=Vector3Scale(move,flySpeed);player.position=Vector3Add(player.position,Vector3Scale(playerVelocity,dt));
        desiredMovementDirection={move.x,0,move.z};movementAmount=Clamp(Vector3Length(desiredMovementDirection),0.0f,1.0f);movementSpeed=Vector3Length(playerVelocity);
        return;
    }
#endif

    // A/D were visually reversed in the raylib world/camera convention used by
    // this prototype, so the horizontal input is intentionally inverted here.
    float horizontal=(IsKeyDown(KEY_A)?1.0f:0.0f)-(IsKeyDown(KEY_D)?1.0f:0.0f);
    float vertical=(IsKeyDown(KEY_W)?1.0f:0.0f)-(IsKeyDown(KEY_S)?1.0f:0.0f);
    float rawMagnitude=std::sqrt(horizontal*horizontal+vertical*vertical);
    movementAmount=Clamp(rawMagnitude,0.0f,1.0f);

    // Camera-relative movement projected onto the ground plane.
    Vector3 cameraForward=HorizontalForward(cameraYaw);
    Vector3 cameraRight=HorizontalRight(cameraYaw);
    cameraForward.y=0.0f; cameraRight.y=0.0f;
    if(Vector3Length(cameraForward)>0.0001f)cameraForward=Vector3Normalize(cameraForward);
    if(Vector3Length(cameraRight)>0.0001f)cameraRight=Vector3Normalize(cameraRight);

    Vector3 move=Vector3Add(Vector3Scale(cameraForward,vertical),Vector3Scale(cameraRight,horizontal));
    if(Vector3Length(move)>0.0001f) move=Vector3Normalize(move);
    desiredMovementDirection=move;

    const auto* worldPolicy=cameraWorldRules.Policy();
    float speed=worldPolicy?worldPolicy->movementRun:6.0f;
    if(IsKeyDown(KEY_LEFT_ALT)) speed=worldPolicy?worldPolicy->movementWalk:3.3f;
    if(IsKeyDown(KEY_LEFT_SHIFT)) speed=worldPolicy?worldPolicy->movementSprint:9.2f;
    if(character.crouching) speed=2.3f;
    if(character.hurt) speed=2.8f;
    if(character.carryingHeavy) speed=2.0f;
    else if(character.carryingMedium) speed=4.0f;
#if VEK_DEVELOPMENT_MODE
    speed*=devSpeedMultiplier;
#endif

    // Estimate the upcoming turn before translation. Sprinting loses speed on
    // extreme changes of direction so 180-degree reversals retain believable weight.
    if(movementAmount>=character.rotation.settings.movementDeadzone) {
        float desiredYaw=CharacterRotationSystem::YawFromDirection(move);
        float turn=std::fabs(CharacterRotationSystem::DeltaAngle(character.rotation.GetBodyYaw(),desiredYaw));
        if(IsKeyDown(KEY_LEFT_SHIFT) && !character.carryingHeavy) {
            if(turn>150.0f) speed=std::min(speed,worldPolicy?worldPolicy->movementRun:6.0f);
            else if(turn>90.0f) speed*=0.72f;
        }
        if(character.carryingHeavy && turn>90.0f) speed*=0.72f;
    }

    Vector3 desiredVelocity = movementAmount>=character.rotation.settings.movementDeadzone
        ? Vector3Scale(move,speed)
        : Vector3{0,0,0};

    float acceleration = character.carryingHeavy ? 10.0f : (IsKeyDown(KEY_LEFT_SHIFT)?24.0f:28.0f);
    float deceleration = character.carryingHeavy ? 14.0f : 34.0f;
    float rate = Vector3Length(desiredVelocity)>0.01f ? acceleration : deceleration;
    playerVelocity=MoveTowardsVector(playerVelocity,desiredVelocity,rate*dt);
    playerVelocity.y=0.0f;
    movementSpeed=Vector3Length(playerVelocity);

    player.position=Vector3Add(player.position,Vector3Scale(playerVelocity,dt));

    if(IsKeyPressed(KEY_SPACE) && !character.crouching) {
        if(character.humanoid.BeginJump())
            character.Trigger(CharacterAnimState::Jump);
    }

    // Vertical motion now belongs to the reusable humanoid layer. This keeps
    // jump/fall/landing state ready for future health, fall damage and NPCs.
    if(character.humanoid.UpdateVertical(dt,CharacterGroundY(),player.position.y)) {
        character.Trigger(CharacterAnimState::Land);
        float impact=character.humanoid.GetLastLandingSpeed();
        float damage=characterRules.FallDamage(impact);
#if VEK_DEVELOPMENT_MODE
        if(devGodModeEnabled)damage=0.0f;
#endif
        if(damage>0.0f) {
            character.humanoid.ApplyDamage(damage);
            Say(TextFormat("Hard landing: -%.0f health",character.humanoid.GetLastDamageAmount()),2.5f);
        }
        if(characterRules.ShouldRagdoll(character.humanoid.GetHealth(),impact,damage)) {
            float dir=characterRules.RagdollDirection(playerVelocity.x,playerVelocity.z);
            character.TriggerRagdoll(impact,dir,characterRules.RagdollDuration(impact));
            playerVelocity={0,0,0};
        }
    }
}

void Game::UpdateVehicleEntry(float dt) {
    movementAmount=0.0f;
    movementSpeed=0.0f;
    desiredMovementDirection={0,0,0};
    playerVelocity={0,0,0};

    if(!vehicle.finalized) {
        enteringVehicle=false;
        character.rotation.ReleaseFacingOverride();
        return;
    }

    Vector3 entry=vehicle.EntryWorldPosition();
    entry.y=CharacterGroundY();
    character.Trigger(CharacterAnimState::EnterVehicle);

    if(vehicleEntryStage==0) {
        Vector3 toEntry=Vector3Subtract(entry,player.position);
        toEntry.y=0.0f;
        float distance=Vector3Length(toEntry);
        if(distance>0.001f) {
            Vector3 dir=Vector3Normalize(toEntry);
            character.rotation.FaceDirection(dir);
            float step=std::min(distance,2.8f*dt);
            player.position=Vector3Add(player.position,Vector3Scale(dir,step));
        }
        if(distance<0.18f) {
            player.position=entry;
            vehicleEntryStage=1;
            character.rotation.FaceYaw(vehicle.heading);
        }
    } else {
        character.rotation.FaceYaw(vehicle.heading);
        float remaining=std::fabs(CharacterRotationSystem::DeltaAngle(character.rotation.GetBodyYaw(),vehicle.heading));
        if(remaining<4.0f) {
            character.rotation.SnapBodyYaw(vehicle.heading);
            character.rotation.ReleaseFacingOverride();
            character.rotation.SetSeatedYaw(vehicle.heading);
            enteringVehicle=false;
            vehicleEntryStage=0;
            player.driving=true;
            cockpitCameraRotation.Snap(0.0f,0.0f);
            cockpitLookYaw=cockpitCameraRotation.GetYaw();
            cockpitLookPitch=cockpitCameraRotation.GetPitch();
            vehicleOrbitCameraRotation.Snap(0.0f,18.0f);
            Say("Driving: WASD, SPACE brake. Arrows align camera; hold RMB to look around.");
        }
    }
}

void Game::UpdateFreeCamera(float dt) {
    const auto* cam=cameraWorldRules.Camera();
    float alignStep=cam?cam->alignmentStep:45.0f;
    bool arrowLeft=false,arrowRight=false;
    PollCameraAlignmentInput(arrowLeft,arrowRight);
    if(arrowLeft) freeCameraRotation.AlignYawStep(CameraRotationSystem::AlignmentDirectionForArrowKey(KEY_LEFT),alignStep);
    if(arrowRight) freeCameraRotation.AlignYawStep(CameraRotationSystem::AlignmentDirectionForArrowKey(KEY_RIGHT),alignStep);
    if((cam?cam->rmbLook:true)&&IsMouseButtonDown(MOUSE_BUTTON_RIGHT)){
        Vector2 md=GetMouseDelta();if(cam){md.x*=cam->rmbYawScale;md.y*=cam->rmbPitchScale;}freeCameraRotation.AddMouseDelta(md);
    }
    freeCameraRotation.Update(dt);
    freeCameraYaw=freeCameraRotation.GetYaw();freeCameraPitch=freeCameraRotation.GetPitch();
    float y=freeCameraYaw*DEG2RAD,p=freeCameraPitch*DEG2RAD;Vector3 f{sinf(y)*cosf(p),-sinf(p),cosf(y)*cosf(p)};Vector3 right{cosf(y),0,-sinf(y)};
    float base=cam?cam->editorMoveSpeed:12.0f;float fast=cam?cam->editorFastMultiplier:2.5f;float speed=base*(IsKeyDown(KEY_LEFT_SHIFT)?fast:1.0f);
    if(IsKeyDown(KEY_W))freeCameraPosition=Vector3Add(freeCameraPosition,Vector3Scale(f,speed*dt));if(IsKeyDown(KEY_S))freeCameraPosition=Vector3Subtract(freeCameraPosition,Vector3Scale(f,speed*dt));if(IsKeyDown(KEY_A))freeCameraPosition=Vector3Add(freeCameraPosition,Vector3Scale(right,speed*dt));if(IsKeyDown(KEY_D))freeCameraPosition=Vector3Subtract(freeCameraPosition,Vector3Scale(right,speed*dt));if(IsKeyDown(KEY_Q))freeCameraPosition.y-=speed*dt;if(IsKeyDown(KEY_E))freeCameraPosition.y+=speed*dt;
    camera.position=freeCameraPosition;camera.target=Vector3Add(camera.position,Vector3Scale(f,10.0f));camera.fovy=cam?cam->fov:60.0f;camera.projection=CAMERA_PERSPECTIVE;
    ClampCameraAboveWorld();freeCameraPosition=camera.position;
}

void Game::UpdateCamera(float dt) {
    if(const auto* cp=cameraWorldRules.Camera()){camera.fovy=cp->fov;camera.projection=CAMERA_PERSPECTIVE;}
    if(character.creatorOpen) {
        camera.target=Vector3Add(player.position,{0,1.15f,0});
        camera.position=Vector3Add(player.position,{0,2.15f,-3.6f});
        ClampCameraAboveWorld();return;
    }

    if(builder.active) {
        editorCamera.Apply(camera);ClampCameraAboveWorld();
        return;
    }

    if(cameraMode==CameraViewMode::FreeInspection) { UpdateFreeCamera(dt); return; }

    Vector3 t=Pos();

    if(cameraMode==CameraViewMode::FirstPerson) {
        Vector3 eye;
        Vector3 viewForward;
        if(player.driving) {
            eye=Vector3Add(vehicle.SeatWorldPosition(),{0,0.72f,0});
            float vy=(vehicle.heading+cockpitLookYaw)*DEG2RAD, vp=cockpitLookPitch*DEG2RAD;
            viewForward={sinf(vy)*cosf(vp),-sinf(vp),cosf(vy)*cosf(vp)};
        } else {
            float eyeHeight=cameraWorldRules.Camera()?cameraWorldRules.Camera()->firstPersonEyeHeight:1.66f;
            eye=Vector3Add(player.position,{0,eyeHeight*character.appearance.height,0});
            float vy=cameraYaw*DEG2RAD,vp=cameraPitch*DEG2RAD;
            viewForward={sinf(vy)*cosf(vp),-sinf(vp),cosf(vy)*cosf(vp)};
        }
        camera.position=eye;
        camera.target=Vector3Add(eye,Vector3Scale(viewForward,10.0f));
        ClampCameraAboveWorld();return;
    }

    if(player.driving) {
        const auto* cp=cameraWorldRules.Camera();
        float distance=cameraMode==CameraViewMode::CloseThirdPerson?(cp?cp->closeDistance:4.3f):(cp?cp->thirdPersonDistance:8.0f);
        Vector3 target=Vector3Add(t,{0,1.1f,0});
        float worldYaw=CharacterRotationSystem::NormalizeAngle(vehicle.heading + vehicleOrbitCameraRotation.GetYaw())*DEG2RAD;
        float orbitPitch=vehicleOrbitCameraRotation.GetPitch()*DEG2RAD;
        Vector3 viewForward{sinf(worldYaw)*cosf(orbitPitch),-sinf(orbitPitch),cosf(worldYaw)*cosf(orbitPitch)};
        camera.target=target;
        camera.position=Vector3Subtract(target,Vector3Scale(viewForward,distance));
        ClampCameraAboveWorld();return;
    }

    const auto* cp=cameraWorldRules.Camera();
    float distance=cameraMode==CameraViewMode::CloseThirdPerson?(cp?cp->closeDistance:4.3f):(cp?cp->thirdPersonDistance:8.0f);
    float targetHeight=cameraMode==CameraViewMode::CloseThirdPerson?(cp?cp->closeTargetHeight:1.35f):(cp?cp->targetHeight:1.15f);
    Vector3 target=Vector3Add(t,{0,targetHeight,0});
    float y=cameraYaw*DEG2RAD,p=cameraPitch*DEG2RAD;
    Vector3 viewForward{sinf(y)*cosf(p),-sinf(p),cosf(y)*cosf(p)};
    camera.target=target;
    camera.position=Vector3Subtract(target,Vector3Scale(viewForward,distance));
    ClampCameraAboveWorld();
}

void Game::SyncMapState() {
    map.SetHidden(character.creatorOpen);
    map.SetBuildMode(builder.active);
    map.SetPlayerPosition(player.position);
    map.SetPlayerHeading(character.rotation.GetBodyYaw());
    map.SetVehicleState(vehicle.position,vehicle.heading,vehicle.finalized,player.driving,vehicle.SpeedKph());
    map.SetMovementContext(player.driving?vehicle.SpeedKph()/3.6f:movementSpeed,
        player.driving?MapActorMode::RoadVehicle:MapActorMode::OnFoot);
    map.SetCurrentObjective(jobs.Task());

    if(jobs.Active()) {
        MapIconType type=jobs.state==Jobs::State::Pickup?MapIconType::Pickup:MapIconType::Delivery;
        map.SetGPSDestination(jobs.Target(world.pickup,world.dropoff),jobs.Label(),type);
    } else {
        map.ClearGPSDestination();
    }
}

void Game::BeginPersonnelDoorEntrySequence() {
    if(PersonnelDoorSequenceActive() || builder.active || player.driving || enteringVehicle || passlockUiOpen) return;
    if(world.IsInsideHangar(player.position,0.0f)) return;

    personnelDoorSequenceTime=0.0f;
    personnelDoorSequenceStart=player.position;
    playerVelocity={0.0f,0.0f,0.0f};
    movementAmount=0.0f;
    movementSpeed=0.0f;
    desiredMovementDirection={0.0f,0.0f,0.0f};
    personnelDoorAutoCloseTimer=0.0f;
    character.ReleaseAllArmPhysics();
    character.forceWalkAnimation=false;
    world.SetPersonnelDoorHandleTurn(0.0f);
    character.rotation.FaceTarget(player.position,world.PersonnelDoorHandlePosition());

    if(world.PersonnelDoorOpenFraction()>=0.82f){
        if(world.SetPersonnelDoorTraversalCollisionDisabled(true))
            collision.RefreshWorldGeometry(world);
        personnelDoorSequencePhase=PersonnelDoorSequencePhase::WalkInside;
        Say("Door is open - walking into the workshop.",1.8f);
    }else{
        personnelDoorSequencePhase=PersonnelDoorSequencePhase::Approach;
        Say("Opening workshop door...",1.8f);
    }
}

void Game::UpdatePersonnelDoorEntrySequence(float dt) {
    if(!PersonnelDoorSequenceActive()) return;
    dt=Clamp(dt,0.0f,0.05f);
    personnelDoorSequenceTime+=dt;
    const float groundY=CharacterGroundY();
    player.position.y=groundY;
    playerVelocity={0.0f,0.0f,0.0f};
    movementAmount=0.0f;
    movementSpeed=0.0f;
    desiredMovementDirection={0.0f,0.0f,0.0f};
    character.forceWalkAnimation=false;

    auto changePhase=[&](PersonnelDoorSequencePhase next){
        personnelDoorSequencePhase=next;
        personnelDoorSequenceTime=0.0f;
        personnelDoorSequenceStart=player.position;
    };
    auto moveCharacter=[&](Vector3 target,float speed){
        target.y=groundY;
        Vector3 before=player.position;
        Vector3 delta=Vector3Subtract(target,before);delta.y=0.0f;
        float distance=Vector3Length(delta);
        if(distance>0.0001f){
            desiredMovementDirection=Vector3Scale(delta,1.0f/distance);
            movementAmount=Clamp(distance/0.75f,0.28f,1.0f);
            movementSpeed=speed;
            character.forceWalkAnimation=true;
            player.position=MoveTowardsVector(before,target,speed*dt);
            if(dt>0.0001f) playerVelocity=Vector3Scale(Vector3Subtract(player.position,before),1.0f/dt);
        }
        return distance;
    };

    switch(personnelDoorSequencePhase){
        case PersonnelDoorSequencePhase::Approach: {
            Vector3 target=world.PersonnelDoorApproachPoint();target.y=groundY;
            float distance=moveCharacter(target,2.35f);
            character.rotation.FaceTarget(player.position,world.PersonnelDoorHandlePosition());
            if(distance<0.09f || personnelDoorSequenceTime>1.15f){
                player.position=target;
                playerVelocity={0,0,0};
                movementAmount=0.0f;
                changePhase(PersonnelDoorSequencePhase::Reach);
                character.Trigger(CharacterAnimState::OpenDoor);
            }
            break;
        }
        case PersonnelDoorSequencePhase::Reach: {
            character.rotation.FaceTarget(player.position,world.PersonnelDoorHandlePosition());
            character.Trigger(CharacterAnimState::OpenDoor);
            character.SetArmPhysicsTarget(true,world.PersonnelDoorHandlePosition(),SmoothStep01(personnelDoorSequenceTime/0.26f));
            if(personnelDoorSequenceTime>=0.28f) changePhase(PersonnelDoorSequencePhase::TurnHandle);
            break;
        }
        case PersonnelDoorSequencePhase::TurnHandle: {
            float t=SmoothStep01(personnelDoorSequenceTime/0.34f);
            world.SetPersonnelDoorHandleTurn(t);
            character.rotation.FaceTarget(player.position,world.PersonnelDoorHandlePosition());
            character.Trigger(CharacterAnimState::OpenDoor);
            character.SetArmPhysicsTarget(true,world.PersonnelDoorHandlePosition(),1.0f);
            if(t>=0.995f){
                world.SetPersonnelDoorTarget(true);
                // The scripted pull/walk owns the avatar from this point. Remove
                // the door slab from the live collision cache immediately so an
                // old/still-closing collider cannot pin the avatar at the threshold.
                if(world.SetPersonnelDoorTraversalCollisionDisabled(true))
                    collision.RefreshWorldGeometry(world);
                changePhase(PersonnelDoorSequencePhase::PullDoor);
            }
            break;
        }
        case PersonnelDoorSequencePhase::PullDoor: {
            world.SetPersonnelDoorTarget(true);
            float open=world.PersonnelDoorOpenFraction();
            float handleHold=1.0f-SmoothStep01((open-0.42f)/0.42f);
            world.SetPersonnelDoorHandleTurn(handleHold);
            Vector3 grip=world.PersonnelDoorHandlePosition();
            character.SetArmPhysicsTarget(true,grip,1.0f);
            character.rotation.FaceTarget(player.position,grip);
            character.Trigger(CharacterAnimState::OpenDoor);

            // Follow the swinging grip with the torso while staying outside the
            // threshold. This keeps the IK arm inside its physical reach instead
            // of stretching while the door sweeps outward.
            Vector3 bodyTarget{grip.x+0.18f,groundY,grip.z+0.72f};
            player.position=MoveTowardsVector(player.position,bodyTarget,1.80f*dt);
            if(open>=0.86f){
                world.SetPersonnelDoorHandleTurn(0.0f);
                character.ReleaseArmPhysics(true);
                changePhase(PersonnelDoorSequencePhase::WalkInside);
            }
            break;
        }
        case PersonnelDoorSequencePhase::WalkInside: {
            world.SetPersonnelDoorTarget(true);
            world.SetPersonnelDoorHandleTurn(0.0f);
            character.ReleaseArmPhysics(true);
            Vector3 target=world.PersonnelDoorInsidePoint();target.y=groundY;
            float distance=moveCharacter(target,2.55f);
            character.rotation.FaceTarget(player.position,target);
            if(distance<0.10f){
                player.position=target;
                playerVelocity={0,0,0};
                movementAmount=0.0f;
                movementSpeed=0.0f;
                desiredMovementDirection={0,0,0};
                character.forceWalkAnimation=false;
                character.ReleaseAllArmPhysics();
                if(world.SetPersonnelDoorTraversalCollisionDisabled(false))
                    collision.RefreshWorldGeometry(world);
                personnelDoorSequencePhase=PersonnelDoorSequencePhase::None;
                personnelDoorSequenceTime=0.0f;
                personnelDoorAutoCloseTimer=std::max(4.5f,hangarInteraction.Door().autoCloseDelay);
                workshopAutoBuildSession=true;
                workshopAutoBuildSuppressed=false;
                EnterBuilderNow();
                Say("Workshop entered - Build Mode activated automatically.",2.8f);
            }
            break;
        }
        default:
            personnelDoorSequencePhase=PersonnelDoorSequencePhase::None;
            character.forceWalkAnimation=false;
            character.ReleaseAllArmPhysics();
            world.SetPersonnelDoorHandleTurn(0.0f);
            if(world.SetPersonnelDoorTraversalCollisionDisabled(false))
                collision.RefreshWorldGeometry(world);
            break;
    }
}

void Game::Interact() {
    bool doorByKey=IsKeyPressed(KEY_E) && !player.driving && !enteringVehicle && Vector3Distance(player.position,world.PersonnelDoorHandlePosition())<3.2f;
    bool doorByClick=PersonnelDoorHandleClicked();
    if(doorByKey||doorByClick){
        bool outside=!world.IsInsideHangar(player.position,0.0f);
        if(outside){
            // Outside interaction is a full physical entry sequence: the avatar
            // reaches the enlarged handle, turns it, pulls the door and walks in.
            BeginPersonnelDoorEntrySequence();
        }else{
            bool opening=!world.PersonnelDoorIsOpen();
            world.SetPersonnelDoorTarget(opening);
            world.SetPersonnelDoorHandleTurn(0.0f);
            personnelDoorAutoCloseTimer=opening?std::max(2.5f,hangarInteraction.Door().autoCloseDelay):0.0f;
            character.rotation.FaceTarget(player.position,world.PersonnelDoorHandlePosition());
            character.Trigger(CharacterAnimState::OpenDoor);
            Say(opening?"Personnel door opening.":"Personnel door closing.",1.5f);
        }
    }
    if(IsKeyPressed(KEY_E) && !doorByKey) {
        if(player.driving) {
            CharacterInteractionSystem::ExitVehicle(character);
            player.driving=false;
            character.rotation.ClearSeatedYaw();
            character.rotation.SnapBodyYaw(vehicle.heading);
            player.yaw=vehicle.heading;
            player.position=vehicle.EntryWorldPosition();
            player.position.y=CharacterGroundY();
            cameraRotation.Snap(vehicle.heading,cameraPitch);
            cameraYaw=cameraRotation.GetYaw();
            cameraPitch=cameraRotation.GetPitch();
            Say("Exited vehicle.");
        } else if(!enteringVehicle && vehicle.finalized&&Vector3Distance(player.position,vehicle.position)<(cameraWorldRules.Policy()?cameraWorldRules.Policy()->vehicleEnterDistance:5.0f)) {
            CharacterInteractionSystem::EnterVehicle(character);
            enteringVehicle=true;
            vehicleEntryStage=0;
            character.rotation.FaceTarget(player.position,vehicle.EntryWorldPosition());
            Say("Entering vehicle...");
        }
    }

    if(IsKeyPressed(KEY_J) && !enteringVehicle) {
        CharacterInteractionSystem::OpenJobTablet(character);
        if(!jobs.Active()){if(authorityRules.AllowsLocalCommit("job.accept"))jobs.Accept();else Say("VEK authority denied job request.");}else Say(jobs.Task());
    }
    if(IsKeyPressed(KEY_K)&&vehicle.finalized&&!enteringVehicle){vehicle.Reset();Say("Vehicle recovered to workshop.");}
    if(IsKeyPressed(KEY_P)){if(Save::SaveBlueprint(vehicle))Say("Blueprint saved.");else Say("Build and finalize a vehicle first.");}
    if(IsKeyPressed(KEY_L)&&Vector3Distance(Pos(),world.workshop)<25&&!enteringVehicle){if(Save::LoadBlueprint(vehicle))Say("Blueprint loaded at workshop.");else Say("No valid blueprint found.");}
    if(IsKeyPressed(KEY_U)&&vehicle.finalized&&Vector3Distance(vehicle.position,world.workshop)<25){
        if(!authorityRules.AllowsLocalCommit("vehicle.upgrade")){Say("VEK authority denied vehicle upgrade.");return;}
        int upgradeCost=(int)ceilf(editorRules.UpgradeCost(gameMode,300.0f));
        if(gameMode==vek::GameMode::Sandbox || economy.money.Get()>=upgradeCost){
            if(gameMode==vek::GameMode::Survival) economy.money=economy.money.Get()-upgradeCost;
            vehicle.Upgrade();if(gameMode==vek::GameMode::Survival)Save::SaveCareer(economy);
            Say(gameMode==vek::GameMode::Sandbox?"Sandbox upgrade applied for free.":TextFormat("Engine power upgraded +25%% for $%d.",upgradeCost));
        }else Say(TextFormat("Not enough money. Upgrade costs $%d.",upgradeCost));
    }

    if(!player.driving && !enteringVehicle && IsKeyPressed(KEY_Q)) { character.NextEquipment(); Say(std::string("Equipped: ")+character.equipment.Name()); }
    if(!player.driving && !enteringVehicle && IsKeyPressed(KEY_T)) {
        if(vehicle.finalized&&Vector3Distance(player.position,vehicle.position)<(cameraWorldRules.Policy()?cameraWorldRules.Policy()->interactionDistance+2.5f:6.0f))
            character.rotation.FaceTarget(player.position,vehicle.position);
        if(!authorityRules.AllowsLocalCommit("vehicle.repair")) Say("VEK authority denied vehicle repair.");
        else if(CharacterInteractionSystem::Repair(character,vehicle,player.position)) Say("Repair action: vehicle condition improved.");
        else Say("Repair animation: move closer to a vehicle to repair it.");
    }
    if(!player.driving && !enteringVehicle && IsKeyPressed(KEY_I)){
        if(vehicle.finalized&&Vector3Distance(player.position,vehicle.position)<(cameraWorldRules.Policy()?cameraWorldRules.Policy()->interactionDistance+3.5f:7.0f))
            character.rotation.FaceTarget(player.position,vehicle.position);
        CharacterInteractionSystem::Inspect(character);Say("Inspecting nearby parts with repair scanner.");
    }
}


void Game::EnterBuilderNow() {
    if(builder.active) return;
    builder.Toggle();
    character.Trigger(CharacterAnimState::Menu);
    character.rotation.SetCameraInfluenceEnabled(false);
    character.rotation.ReleaseFacingOverride();
    character.rotation.SnapBodyYaw(character.rotation.GetBodyYaw());
    editorCamera.ConfigureWorkspaceBounds(
        {hangarRules.Area().center.x,hangarRules.Area().center.y,hangarRules.Area().center.z},
        {hangarRules.Area().size.x,hangarRules.Area().size.y,hangarRules.Area().size.z},
        hangarRules.Area().maxBuildHeight,1.0f);
    editorCamera.Enter(builder.VehicleCenter());
    editorCamera.Apply(camera);
    Say("Vehicle Editor active. Build camera is locked inside the engineering workspace.",3.0f);
}

void Game::ExitBuilderNow() {
    if(!builder.active) return;
    builder.Toggle();
    editorCamera.Exit();
    character.rotation.SetCameraInfluenceEnabled(true);
    character.rotation.ReleaseFacingOverride();
    character.Trigger(CharacterAnimState::Idle);
    workspaceEntryCooldown=0.75f;
    Say("Vehicle Editor closed.",1.5f);
}

bool Game::PasslockClicked() const {
    const auto* pd=hangarInteraction.Passlock();
    if(!pd || gameMode!=vek::GameMode::Survival || builder.active || passlockUiOpen || player.driving || enteringVehicle) return false;
    if(!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return false;
    if(Vector3Distance(player.position,world.PasslockPosition())>pd->maxUseDistance) return false;
    if(pd->outsideOnly && world.IsInsideHangar(player.position,0.0f)) return false;

    Ray ray=GetScreenToWorldRay(GetMousePosition(),camera);
    RayCollision target=GetRayCollisionBox(ray,world.PasslockBounds());
    if(!target.hit) return false;

    if(pd->requiresLineOfSight){
        for(const auto& box:world.CollisionBoxes()){
            Vector3 half=Vector3Scale(box.size,0.5f);
            BoundingBox bounds{Vector3Subtract(box.center,half),Vector3Add(box.center,half)};
            RayCollision hit=GetRayCollisionBox(ray,bounds);
            if(hit.hit && hit.distance+0.025f<target.distance) return false;
        }
    }
    return true;
}

bool Game::GarageControlClicked() const {
    if(builder.active||passlockUiOpen||player.driving||enteringVehicle||!world.IsInsideHangar(player.position,0.0f))return false;
    if(Vector3Distance(player.position,world.GarageControlPosition())>4.0f)return false;
    if(!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))return false;
    Ray ray=GetScreenToWorldRay(GetMousePosition(),camera);
    return GetRayCollisionBox(ray,world.GarageControlBounds()).hit;
}

bool Game::PersonnelDoorHandleClicked() const {
    if(builder.active||passlockUiOpen||player.driving||enteringVehicle)return false;
    if(Vector3Distance(player.position,world.PersonnelDoorHandlePosition())>3.5f)return false;
    if(!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))return false;
    Ray ray=GetScreenToWorldRay(GetMousePosition(),camera);
    return GetRayCollisionBox(ray,world.PersonnelDoorHandleBounds()).hit;
}

void Game::OpenPasslockUi() {
    if(gameMode!=vek::GameMode::Survival || player.driving || enteringVehicle || builder.active) return;
    passlockUiOpen=true;passlockInput.clear();passlockStatus=passlockState.lockoutRemaining>0.0f?"LOCKED OUT":"LOCKED - ENTER PIN";
    character.rotation.FaceTarget(player.position,world.PasslockPosition());
    character.Trigger(CharacterAnimState::UseKeypad);
}

void Game::ClosePasslockUi(){passlockUiOpen=false;passlockInput.clear();character.Trigger(CharacterAnimState::Idle);}

void Game::SubmitPasslockCode(){
    const auto* pd=hangarInteraction.Passlock();const auto* gd=hangarInteraction.Garage();if(!pd||!gd){passlockStatus="ACCESS SYSTEM OFFLINE";return;}
#if VEK_DEVELOPMENT_MODE
    if(passlockInput=="37485" && authorityRules.AllowsDeveloperFeature("dev.panel",true)){
        passlockInput.clear();
        passlockUiOpen=false;
        devCheatPanelOpen=true;
        passlockStatus="DEV PANEL";
        character.Trigger(CharacterAnimState::Idle);
        Say("VEK DEV PANEL UNLOCKED",3.0f);
        return;
    }
#endif
    auto result=vek::PasslockSystem::Submit(passlockState,*pd,passlockInput);
    passlockInput.clear();
    if(result==vek::PasslockResult::Granted){
        passlockStatus="ACCESS GRANTED";garageAccessGranted=true;garageOpenRequestLatched=true;vek::GarageDoorSystem::Unlock(garageDoorState);vek::GarageDoorSystem::RequestOpen(garageDoorState,*gd,true);ClosePasslockUi();
    }else if(result==vek::PasslockResult::LockedOut)passlockStatus="LOCKED OUT - WAIT";
    else if(result==vek::PasslockResult::InvalidInput)passlockStatus="ENTER A VALID PIN";
    else passlockStatus="ACCESS DENIED";
}

namespace {
struct PasslockUiLayout {
    Rectangle modal{}, title{}, help{}, input{}, status{}, keypad{};
    float gap=8.0f;
    Rectangle KeyRect(int index) const {
        int row=index/3,col=index%3;
        float bw=(keypad.width-gap*2.0f)/3.0f;
        float bh=(keypad.height-gap*3.0f)/4.0f;
        return {keypad.x+col*(bw+gap),keypad.y+row*(bh+gap),bw,bh};
    }
};
PasslockUiLayout MakePasslockUiLayout(){
    PasslockUiLayout l;
    float sw=(float)GetScreenWidth(),sh=(float)GetScreenHeight();
    float w=std::min(520.0f,std::max(300.0f,sw-24.0f));
    float h=std::min(560.0f,std::max(430.0f,sh-24.0f));
    float x=(sw-w)*0.5f,y=(sh-h)*0.5f;
    l.modal={x,y,w,h};
    l.title={x+28,y+24,w-56,40};
    l.help={x+38,y+70,w-76,56};
    l.input={x+55,y+137,w-110,52};
    l.status={x+70,y+199,w-140,38};
    l.keypad={x+48,y+251,w-96,h-275};
    return l;
}
}

void Game::UpdatePasslockUi(float dt){
    const auto* pd=hangarInteraction.Passlock();if(!pd){ClosePasslockUi();return;}
    if(IsKeyPressed(KEY_ESCAPE)){ClosePasslockUi();return;}
    character.rotation.FaceTarget(player.position,world.PasslockPosition());
    character.animation.SetState(CharacterAnimState::UseKeypad);
    character.animation.Update(dt,0.0f);

    if(passlockState.lockoutRemaining>0.0f)passlockStatus=TextFormat("LOCKED OUT %.0fs",passlockState.lockoutRemaining);
    if(pd->allowKeyboard && passlockState.lockoutRemaining<=0.0f){
        int ch=GetCharPressed();
        while(ch>0){
            if(ch>='0'&&ch<='9'&&(int)passlockInput.size()<pd->maxDigits)passlockInput.push_back((char)ch);
            ch=GetCharPressed();
        }
        if(IsKeyPressed(KEY_BACKSPACE)&&!passlockInput.empty())passlockInput.pop_back();
        if(IsKeyPressed(KEY_ENTER))SubmitPasslockCode();
    }
    if(!passlockUiOpen||!pd->allowMouse||!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))return;

    static const char* keys[12]={"1","2","3","4","5","6","7","8","9","CLEAR","0","ENTER"};
    PasslockUiLayout layout=MakePasslockUiLayout();
    Vector2 mouse=GetMousePosition();
    for(int i=0;i<12;++i){
        Rectangle r=layout.KeyRect(i);
        if(!CheckCollisionPointRec(mouse,r))continue;
        std::string key=keys[i];
        if(key=="CLEAR")passlockInput.clear();
        else if(key=="ENTER")SubmitPasslockCode();
        else if(passlockState.lockoutRemaining<=0.0f&&(int)passlockInput.size()<pd->maxDigits)passlockInput+=key;
        break;
    }
}

void Game::UpdateGarageAccess(float dt){
    const auto* gd=hangarInteraction.Garage();if(!gd)return;
    vek::PasslockSystem::Update(passlockState,dt);
    workspaceEntryCooldown=std::max(0.0f,workspaceEntryCooldown-dt);

    bool inside=world.IsInsideHangar(player.position,0.25f);
    bool inSafetyZone=world.IsNearGarageOpening(player.position,2.75f);

    // Explicit open requests stay latched only until full travel. Access rights
    // do not force a closed door back open after the inside CLOSE control.
    if(garageOpenRequestLatched && garageDoorState.openFraction<0.995f){
        vek::GarageDoorSystem::Unlock(garageDoorState);
        vek::GarageDoorSystem::RequestOpen(garageDoorState,*gd,true);
    }
    if(garageDoorState.openFraction>=0.995f)garageOpenRequestLatched=false;

    // Safety reversal only applies at the actual threshold.
    if(!builder.active && inside && inSafetyZone && gd->allowInsideEgress && garageDoorState.motion==vek::GarageDoorMotion::Closing){
        garageOpenRequestLatched=true;
        vek::GarageDoorSystem::Unlock(garageDoorState);
        vek::GarageDoorSystem::RequestOpen(garageDoorState,*gd,true);
    }
    if(!builder.active && gd->holdOpenNearDoor && inSafetyZone && garageDoorState.openFraction>0.15f)
        vek::GarageDoorSystem::HoldOpen(garageDoorState,*gd);

    auto beforeMotion=garageDoorState.motion;float before=garageDoorState.openFraction;
    vek::GarageDoorSystem::Update(garageDoorState,*gd,dt);
    world.SetGarageDoorState(garageDoorState.openFraction,garageDoorState.locked);
    if(std::fabs(before-garageDoorState.openFraction)>0.0001f||beforeMotion!=garageDoorState.motion)collision.RefreshWorldGeometry(world);

    // Crossing the garage threshold never auto-enters Build Mode. The player
    // stays in normal movement and can walk all the way through the hangar.
    // Press B again away from the doorway while inside to open the editor.
    bool nowInside=world.IsInsideHangar(player.position,0.4f);
    if(workshopAutoBuildSession && nowInside && !builder.active && !workshopAutoBuildSuppressed && !PersonnelDoorSequenceActive() && workspaceEntryCooldown<=0.0f){
        EnterBuilderNow();
    }
    if(workshopAutoBuildSession && !nowInside && !PersonnelDoorSequenceActive()){
        workshopAutoBuildSession=false;
        workshopAutoBuildSuppressed=false;
        Say("Left workshop - automatic Build Mode session ended.",1.8f);
    }
    wasInsideHangar=nowInside;

    if(garageDoorState.motion==vek::GarageDoorMotion::Closed&&garageDoorState.locked&&!builder.active&&!inside){
        garageAccessGranted=false;passlockState.granted=false;
    }
}

void Game::DrawPasslockUi(){
    if(!passlockUiOpen)return;
    const auto* pd=hangarInteraction.Passlock();if(!pd)return;
    std::string masked=pd->maskInput?std::string(passlockInput.size(),'*'):passlockInput;
    hangarInteraction.BuildPasslockGui(masked,passlockStatus);

    PasslockUiLayout layout=MakePasslockUiLayout();
    DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,0.58f));
    DrawRectangleRounded(layout.modal,0.055f,10,{17,24,29,250});
    DrawRectangleRoundedLinesEx(layout.modal,0.055f,10,2.0f,{70,165,180,255});

    int labelIndex=0;
    for(const auto& cmd:hangarInteraction.PasslockGuiCommands()){
        if(cmd.type==vek::GuiCommandType::Label){
            Rectangle bounds=labelIndex==0?layout.title:layout.help;
            Color color=labelIndex==0?RAYWHITE:LIGHTGRAY;
            VekGuiTextRenderer::DrawTextAuto(cmd.text,bounds,cmd.textPolicy,color,true);
            ++labelIndex;
        } else if(cmd.type==vek::GuiCommandType::PasswordInput){
            DrawRectangleRounded(layout.input,0.12f,6,{8,13,16,255});
            DrawRectangleRoundedLinesEx(layout.input,0.12f,6,2,{82,105,112,255});
            std::string shown=cmd.text.empty()?"_":cmd.text;
            VekGuiTextRenderer::DrawTextAuto(shown,{layout.input.x+10,layout.input.y+4,layout.input.width-20,layout.input.height-8},cmd.textPolicy,SKYBLUE,true);
        } else if(cmd.type==vek::GuiCommandType::StatusBadge){
            Color color=passlockStatus.find("GRANTED")!=std::string::npos?GREEN:
                (passlockStatus.find("DENIED")!=std::string::npos||passlockStatus.find("LOCKED OUT")!=std::string::npos?RED:YELLOW);
            DrawRectangleRounded(layout.status,0.3f,6,Fade(color,0.18f));
            VekGuiTextRenderer::DrawTextAuto(cmd.text,{layout.status.x+8,layout.status.y+3,layout.status.width-16,layout.status.height-6},cmd.textPolicy,color,true);
        } else if(cmd.type==vek::GuiCommandType::Keypad){
            Vector2 mouse=GetMousePosition();
            for(int i=0;i<(int)cmd.items.size()&&i<12;++i){
                Rectangle r=layout.KeyRect(i);
                bool hover=CheckCollisionPointRec(mouse,r);
                DrawRectangleRounded(r,0.12f,6,hover?Color{48,76,84,255}:Color{31,43,49,255});
                DrawRectangleRoundedLinesEx(r,0.12f,6,1.5f,{75,105,114,255});
                VekGuiTextRenderer::DrawTextAuto(cmd.items[i],{r.x+6,r.y+3,r.width-12,r.height-6},cmd.textPolicy,RAYWHITE,true);
            }
        }
    }
}


void Game::UpdateDevCheatPanel(){
#if VEK_DEVELOPMENT_MODE
    if(IsKeyPressed(KEY_ESCAPE)){devCheatPanelOpen=false;return;}
    if(IsKeyPressed(KEY_ONE) && authorityRules.AllowsDeveloperFeature("dev.garage.unlock",true)){
        if(const auto* gd=hangarInteraction.Garage()){
            garageAccessGranted=true;
            garageOpenRequestLatched=true;
            passlockState.granted=true;
            vek::GarageDoorSystem::Unlock(garageDoorState);
            vek::GarageDoorSystem::RequestOpen(garageDoorState,*gd,true);
            Say("DEV: GARAGE UNLOCKED",2.0f);
        }
    }
    if(IsKeyPressed(KEY_TWO) && authorityRules.AllowsDeveloperFeature("dev.performance_hud",true)){
        performanceHudVisible=!performanceHudVisible;
        Say(performanceHudVisible?"DEV: PERFORMANCE HUD ON":"DEV: PERFORMANCE HUD OFF",2.0f);
    }
    if(IsKeyPressed(KEY_THREE) && authorityRules.AllowsDeveloperFeature("dev.fly",true)){devFlyEnabled=!devFlyEnabled;if(devFlyEnabled)devNoclipEnabled=true;Say(devFlyEnabled?"DEV: FLY + NOCLIP ON":"DEV: FLY OFF",2.0f);}
    if(IsKeyPressed(KEY_FOUR) && authorityRules.AllowsDeveloperFeature("dev.noclip",true)){devNoclipEnabled=!devNoclipEnabled;Say(devNoclipEnabled?"DEV: NOCLIP ON":"DEV: NOCLIP OFF",2.0f);}
    if(IsKeyPressed(KEY_FIVE) && authorityRules.AllowsDeveloperFeature("dev.god_mode",true)){devGodModeEnabled=!devGodModeEnabled;Say(devGodModeEnabled?"DEV: GOD MODE ON":"DEV: GOD MODE OFF",2.0f);}
    if(IsKeyPressed(KEY_SIX) && authorityRules.AllowsDeveloperFeature("dev.teleport",true)){player.position=world.GarageDoorInsidePoint();player.position.z-=7.0f;player.position.y=CharacterGroundY();playerVelocity={0,0,0};Say("DEV: TELEPORTED INSIDE WORKSHOP",2.0f);}
    if(IsKeyPressed(KEY_SEVEN) && authorityRules.AllowsDeveloperFeature("dev.movement_speed",true)){devSpeedMultiplier=devSpeedMultiplier>1.1f?1.0f:3.0f;Say(devSpeedMultiplier>1.1f?"DEV: 3X MOVEMENT":"DEV: NORMAL MOVEMENT",2.0f);}
    if(IsKeyPressed(KEY_EIGHT) && authorityRules.AllowsDeveloperFeature("dev.garage.toggle",true)){if(const auto* gd=hangarInteraction.Garage()){garageOpenRequestLatched=false;if(garageDoorState.openFraction>0.1f)vek::GarageDoorSystem::RequestClose(garageDoorState);else{garageAccessGranted=true;garageOpenRequestLatched=true;vek::GarageDoorSystem::Unlock(garageDoorState);vek::GarageDoorSystem::RequestOpen(garageDoorState,*gd,true);}Say("DEV: GARAGE TOGGLED",2.0f);}}
#else
    devCheatPanelOpen=false;
#endif
}

void Game::DrawDevCheatPanel(){
#if VEK_DEVELOPMENT_MODE
    if(!devCheatPanelOpen)return;
    float sw=(float)GetScreenWidth(),sh=(float)GetScreenHeight();
    Rectangle panel{sw*0.5f-300.0f,sh*0.5f-235.0f,600.0f,470.0f};
    DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,0.62f));
    DrawRectangleRounded(panel,0.06f,10,{13,18,22,250});
    DrawRectangleRoundedLinesEx(panel,0.06f,10,2.0f,{80,220,170,255});
    DrawText("VEK DEVELOPMENT CHEAT PANEL",(int)panel.x+32,(int)panel.y+30,24,{100,240,190,255});
    DrawText("DEV BUILD ONLY - stripped from secure release",(int)panel.x+32,(int)panel.y+70,18,LIGHTGRAY);
    DrawText("[1] Unlock/open garage",(int)panel.x+32,(int)panel.y+112,19,RAYWHITE);
    DrawText("[2] Performance HUD",(int)panel.x+32,(int)panel.y+146,19,RAYWHITE);
    DrawText(TextFormat("[3] Flyhack: %s",devFlyEnabled?"ON":"OFF"),(int)panel.x+32,(int)panel.y+180,19,RAYWHITE);
    DrawText(TextFormat("[4] Noclip: %s",devNoclipEnabled?"ON":"OFF"),(int)panel.x+32,(int)panel.y+214,19,RAYWHITE);
    DrawText(TextFormat("[5] God mode: %s",devGodModeEnabled?"ON":"OFF"),(int)panel.x+32,(int)panel.y+248,19,RAYWHITE);
    DrawText("[6] Teleport inside workshop",(int)panel.x+32,(int)panel.y+282,19,RAYWHITE);
    DrawText(TextFormat("[7] Movement speed: %.0fx",devSpeedMultiplier),(int)panel.x+32,(int)panel.y+316,19,RAYWHITE);
    DrawText("[8] Toggle garage open/close",(int)panel.x+32,(int)panel.y+350,19,RAYWHITE);
    DrawText("Fly controls: WASD + SPACE up + CTRL down",(int)panel.x+32,(int)panel.y+392,17,SKYBLUE);
    DrawText("[ESC] Close developer panel",(int)panel.x+32,(int)panel.y+425,17,GRAY);
#endif
}

void Game::ResetToWorkshop() {
    // RESET is now a VEK-timed death/respawn sequence instead of an instant
    // teleport. The native game still owns rendering/audio handles and the
    // actual respawn transform.
    if(resetDeathState.active)return;
    const auto* death=lifeCycle.ResetDeath();
    if(!death){
        CompleteRespawnToWorkshop();
        return;
    }

    if(builder.active) ExitBuilderNow();
    if(character.creatorOpen) character.ToggleCreator();
    if(passlockUiOpen) ClosePasslockUi();
    map.SetDisplayMode(MapDisplayMode::Minimap);

    if(player.driving){
        player.driving=false;
        player.position=vehicle.EntryWorldPosition();
        player.position.y=CharacterGroundY();
    }
    enteringVehicle=false;
    vehicleEntryStage=0;
    personnelDoorSequencePhase=PersonnelDoorSequencePhase::None;
    personnelDoorSequenceTime=0.0f;
    workshopAutoBuildSession=false;
    workshopAutoBuildSuppressed=false;
    character.forceWalkAnimation=false;
    character.ReleaseAllArmPhysics();
    world.SetPersonnelDoorHandleTurn(0.0f);
    if(world.SetPersonnelDoorTraversalCollisionDisabled(false))
        collision.RefreshWorldGeometry(world);
    movementAmount=0.0f;
    movementSpeed=0.0f;
    desiredMovementDirection={0,0,0};
    playerVelocity={0,0,0};

    character.humanoid.SetHealth(0.0f);
    character.rotation.ClearSeatedYaw();
    character.rotation.ReleaseFacingOverride();
    character.TriggerRagdoll(death->ragdollImpact,1.0f,death->ragdollDuration);
    vek::DeathSequenceSystem::Begin(resetDeathState);
    vek::ScreenEffectSystem::Stop(resetScreenState);

    cameraMode=CameraViewMode::ThirdPerson;
    Say("",0.0f);
}

void Game::UpdateResetDeath(float dt){
    const auto* death=lifeCycle.ResetDeath();
    if(!death){CompleteRespawnToWorkshop();return;}

    auto events=vek::DeathSequenceSystem::Update(resetDeathState,*death,dt);
    if(events.startScreen){
        if(lifeCycle.ResetScreenEffect()) vek::ScreenEffectSystem::Start(resetScreenState);
    }
    if(const auto* fx=lifeCycle.ResetScreenEffect();fx&&resetScreenState.active)
        vek::ScreenEffectSystem::Update(resetScreenState,*fx,dt);

    if(events.playAudio && resetDeathSoundLoaded) PlaySound(resetDeathSound);

    // Keep the ragdoll simulation alive while normal player input is blocked.
    character.Update(dt,0.0f,false);
    movementAmount=0.0f;movementSpeed=0.0f;playerVelocity={0,0,0};desiredMovementDirection={0,0,0};

    if(events.respawn) CompleteRespawnToWorkshop();
}

void Game::CompleteRespawnToWorkshop() {
    // Career, jobs, money, XP, appearance and blueprints are preserved.
    if(builder.active) ExitBuilderNow();
    if(character.creatorOpen) character.ToggleCreator();
    map.SetDisplayMode(MapDisplayMode::Minimap);

    enteringVehicle=false;
    vehicleEntryStage=0;
    workspaceEntryCooldown=0.0f;
    wasInsideHangar=false;
    personnelDoorSequencePhase=PersonnelDoorSequencePhase::None;
    personnelDoorSequenceTime=0.0f;
    workshopAutoBuildSession=false;
    workshopAutoBuildSuppressed=false;
    character.forceWalkAnimation=false;
    character.ReleaseAllArmPhysics();
    world.SetPersonnelDoorHandleTurn(0.0f);
    world.SetPersonnelDoorTraversalCollisionDisabled(false);
    world.SetPersonnelDoorTarget(false);
    passlockUiOpen=false;
    passlockInput.clear();
    garageAccessGranted=false;
    garageOpenRequestLatched=false;
    personnelDoorAutoCloseTimer=0.0f;
    vek::PasslockSystem::Reset(passlockState);
    collision.RefreshWorldGeometry(world);
    if(const auto* gd=hangarInteraction.Garage()){
        vek::GarageDoorSystem::Reset(garageDoorState,*gd);
        world.SetGarageDoorState(garageDoorState.openFraction,garageDoorState.locked);
        collision.RefreshWorldGeometry(world);
    }

    player.driving=false;
    player.position=world.PersonnelDoorApproachPoint();
    player.position.z+=6.0f;
    player.position.y=CharacterGroundY();
    player.yaw=180.0f;
    playerVelocity={0,0,0};
    desiredMovementDirection={0,0,0};
    movementAmount=0.0f;
    movementSpeed=0.0f;

    if(vehicle.finalized) vehicle.Reset();

    character.humanoid.Reset();
    character.ragdoll={};
    character.crouching=false;
    character.sprinting=false;
    character.carryingHeavy=false;
    character.carryingMedium=false;
    character.firstPerson=false;
    character.rotation.ClearSeatedYaw();
    character.rotation.ReleaseFacingOverride();
    character.rotation.SetCameraInfluenceEnabled(true);
    character.rotation.SetRotationMode(CharacterMovementRotationMode::FaceCameraDirection);
    character.rotation.SnapBodyYaw(180.0f);
    character.Trigger(CharacterAnimState::Idle);

    cameraMode=CameraViewMode::ThirdPerson;
    cameraRotation.Snap(180.0f,18.0f);
    cameraYaw=cameraRotation.GetYaw();
    cameraPitch=cameraRotation.GetPitch();
    cockpitCameraRotation.Snap(0.0f,0.0f);
    cockpitLookYaw=0.0f;
    cockpitLookPitch=0.0f;
    vehicleOrbitCameraRotation.Snap(0.0f,18.0f);
    freeCameraRotation.Snap(0.0f,18.0f);
    freeCameraYaw=0.0f;
    freeCameraPitch=18.0f;

    vek::DeathSequenceSystem::Cancel(resetDeathState);
    vek::ScreenEffectSystem::Stop(resetScreenState);
    Say("Respawned at the engineering hangar.",2.0f);
}

void Game::DrawResetDeathOverlay() const {
    if(!resetScreenState.active)return;
    const auto* fx=lifeCycle.ResetScreenEffect();
    if(!fx)return;
    float a=std::clamp(resetScreenState.opacity,0.0f,1.0f);
    Color tint{(unsigned char)fx->tint.r,(unsigned char)fx->tint.g,(unsigned char)fx->tint.b,(unsigned char)std::clamp(fx->tint.a*a,0.0f,255.0f)};
    DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),tint);

    // Dark/red edge vignette plus stylised screen splashes. This is a HUD
    // effect only; VEK supplies intensity and timing, not renderer pointers.
    float vig=std::clamp(fx->vignette*a,0.0f,1.0f);
    int edge=(int)(std::min(GetScreenWidth(),GetScreenHeight())*0.16f);
    Color edgeColor{45,0,0,(unsigned char)(210.0f*vig)};
    DrawRectangle(0,0,GetScreenWidth(),edge,edgeColor);
    DrawRectangle(0,GetScreenHeight()-edge,GetScreenWidth(),edge,edgeColor);
    DrawRectangle(0,0,edge,GetScreenHeight(),edgeColor);
    DrawRectangle(GetScreenWidth()-edge,0,edge,GetScreenHeight(),edgeColor);

    float sp=std::clamp(fx->spatter*a,0.0f,1.0f);
    const Vector2 spots[]={{0.07f,0.18f},{0.14f,0.07f},{0.88f,0.12f},{0.95f,0.28f},{0.08f,0.76f},{0.20f,0.90f},{0.78f,0.92f},{0.93f,0.72f}};
    for(int i=0;i<8;++i){
        float radius=(20.0f+(i%3)*14.0f)*sp;
        DrawCircleV({spots[i].x*GetScreenWidth(),spots[i].y*GetScreenHeight()},radius,{95,0,3,(unsigned char)(150.0f*sp)});
    }
}

void Game::DrawRotationDebugWorld() const {
    if(!rotationDebug || character.creatorOpen || builder.active) return;

    Vector3 origin = player.driving ? vehicle.SeatWorldPosition() : player.position;
    origin.y += 0.25f;

    Vector3 bodyF=CharacterRotationSystem::ForwardFromYaw(character.rotation.GetBodyYaw());
    Vector3 cameraF=CharacterRotationSystem::ForwardFromYaw(EffectiveCameraYaw());
    Vector3 moveF=character.rotation.GetMovementDirection();

    DrawLine3D(origin,Vector3Add(origin,Vector3Scale(bodyF,3.0f)),GREEN);
    if(Vector3Length(moveF)>0.001f)
        DrawLine3D(origin,Vector3Add(origin,Vector3Scale(moveF,3.0f)),YELLOW);
    DrawLine3D(origin,Vector3Add(origin,Vector3Scale(cameraF,3.0f)),BLUE);
}

void Game::DrawRotationDebugHUD() const {
    if(!rotationDebug) return;
    int x=GetScreenWidth()-430;
    int y=195;
    DrawRectangle(x,y,420,405,Fade(BLACK,0.86f));
    DrawText("CHARACTER + CAMERA ROTATION [F8]",x+15,y+12,20,SKYBLUE);
    DrawText(TextFormat("Body Yaw: %.2f",character.rotation.GetBodyYaw()),x+15,y+43,17,GREEN);
    DrawText(TextFormat("Body Target: %.2f",character.rotation.GetTargetYaw()),x+15,y+66,17,YELLOW);
    DrawText(TextFormat("View Yaw: %.2f",EffectiveCameraYaw()),x+15,y+89,17,BLUE);
    DrawText(TextFormat("Body Angle Diff: %.2f",character.rotation.GetAngleDifference()),x+15,y+112,17,RAYWHITE);
    DrawText(TextFormat("Body Angular Vel: %.2f deg/s",character.rotation.GetAngularVelocity()),x+15,y+135,17,RAYWHITE);
    Vector3 m=character.rotation.GetMovementDirection();
    DrawText(TextFormat("Move Dir: %.2f, %.2f, %.2f",m.x,m.y,m.z),x+15,y+158,17,RAYWHITE);
    DrawText(TextFormat("Mode: %s",character.rotation.RotationModeName()),x+15,y+181,17,RAYWHITE);
    DrawText(TextFormat("Turn: %s",character.rotation.TurnStateName()),x+15,y+204,17,RAYWHITE);

    const CameraRotationSystem* activeCam=&cameraRotation;
    if(cameraMode==CameraViewMode::FreeInspection) activeCam=&freeCameraRotation;
    else if(player.driving && cameraMode==CameraViewMode::FirstPerson) activeCam=&cockpitCameraRotation;
    else if(player.driving) activeCam=&vehicleOrbitCameraRotation;

    DrawText("CAMERA SMOOTH ROTATION",x+15,y+233,17,SKYBLUE);
    DrawText(TextFormat("Current Yaw/Pitch: %.2f / %.2f",activeCam->GetYaw(),activeCam->GetPitch()),x+15,y+256,16,RAYWHITE);
    DrawText(TextFormat("Target Yaw/Pitch: %.2f / %.2f",activeCam->GetTargetYaw(),activeCam->GetTargetPitch()),x+15,y+278,16,RAYWHITE);
    DrawText(TextFormat("Yaw/Pitch Vel: %.1f / %.1f",activeCam->GetYawVelocity(),activeCam->GetPitchVelocity()),x+15,y+300,16,RAYWHITE);
    DrawText(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)?"RMB LOOK: ACTIVE | Cursor remains free":"RMB LOOK: hold to rotate | Cursor: FREE",x+15,y+326,16,ORANGE);
    DrawText(TextFormat("LEFT arrow:%s   RIGHT arrow:%s",lastArrowLeftInput?"YES":"no",lastArrowRightInput?"YES":"no"),x+15,y+348,16,ORANGE);
    DrawText("GREEN body | YELLOW move | BLUE camera",x+15,y+376,15,LIGHTGRAY);
}

void Game::Draw() {
    BeginDrawing();
    if(mainMenu.IsOpen()) {
        mainMenu.Draw();
        EndDrawing();
        return;
    }
    ClearBackground({125,175,220,255});
    const auto* sky=cameraWorldRules.Skybox();const auto* worldPolicy=cameraWorldRules.Policy();
    if(sky&&(!worldPolicy||worldPolicy->skyEnabled)) skyRenderer.DrawBackground(camera,*sky);
    BeginMode3D(camera);
    world.Draw();

    bool firstPerson=(cameraMode==CameraViewMode::FirstPerson);
    if(!player.driving && !firstPerson) {
        float yaw=character.creatorOpen?character.avatarPreviewYaw:character.rotation.GetBodyYaw();
        character.Draw(player.position,yaw,false);
    }
    if(player.driving && !firstPerson) {
        Vector3 seat=vehicle.SeatWorldPosition();
        Vector3 seatedRoot=Vector3Subtract(seat,{0,0.86f*character.appearance.height,0});
        character.Draw(seatedRoot,vehicle.heading,true);
    }

    vehicle.Draw();
    builder.DrawWorld();

    if(firstPerson) character.DrawFirstPersonArms(camera,player.driving);

    if(jobs.Active()) {
        Vector3 z=jobs.Target(world.pickup,world.dropoff);
        DrawLine3D(Vector3Add(Pos(),{0,1.5f,0}),Vector3Add(z,{0,1.5f,0}),YELLOW);
    }
    map.DrawWorldMarkers3D();

    DrawRotationDebugWorld();
    collision.DrawDebug3D(player.position,&vehicle);
    EndMode3D();
    builder.DrawUI();

    // Build mode owns the screen HUD while the engineering editor is active.
    // Keep gameplay state alive underneath it, but hide the normal survival
    // HUD and minimap/full-map presentation until the player leaves Build Mode.
    if(!builder.active) DrawHUD();
    DrawPasslockUi();
    DrawDevCheatPanel();
    character.DrawCreatorUI();
    if(!map.BlocksGameplayInput()) {
        DrawRotationDebugHUD();
        collision.DrawDebugHUD();
    }
    if(!builder.active) map.Draw();
    escapeMenu.Draw();
    DrawResetDeathOverlay();
    EndDrawing();
}

void Game::DrawHUD() {
    if(authorityRules.Shell().showHelpOverlay && !builder.active && !character.creatorOpen) {
        DrawRectangle(10,10,480,118,Fade(BLACK,0.72f));
        DrawText("VEK HELP",25,22,23,RAYWHITE);
        DrawText("Help overlay enabled by security_authority.vek",25,54,16,SKYBLUE);
        DrawText(TextFormat("TASK: %s",jobs.Task()),25,82,16,YELLOW);
    }

    DrawRectangle(GetScreenWidth()-285,10,275,218,Fade(BLACK,0.75f));
    DrawText(gameMode==vek::GameMode::Sandbox?"Mode: SANDBOX":"Mode: SURVIVAL",GetScreenWidth()-265,22,18,SKYBLUE);
    if(gameMode==vek::GameMode::Sandbox) DrawText("Money: UNLIMITED",GetScreenWidth()-265,49,20,GREEN);
    else DrawText(TextFormat("Money: $%d",economy.money.Get()),GetScreenWidth()-265,49,20,GREEN);
    DrawText(TextFormat("XP: %d",economy.xp.Get()),GetScreenWidth()-265,76,20,RAYWHITE);
    DrawText(TextFormat("Reputation: %d",economy.reputation.Get()),GetScreenWidth()-265,103,20,RAYWHITE);
    DrawText(TextFormat("Tool: %s",character.equipment.Name()),GetScreenWidth()-265,130,17,SKYBLUE);
    DrawText(TextFormat("Health: %.0f / %.0f",character.humanoid.GetHealth(),character.humanoid.GetMaxHealth()),GetScreenWidth()-265,154,17,LIME);
    DrawText(TextFormat("Stamina: %.0f",character.humanoid.GetStamina()),GetScreenWidth()-265,177,17,YELLOW);
    DrawText(character.IsRagdollActive()?"VEK Ragdoll: ACTIVE":"VEK Ragdoll: ready",GetScreenWidth()-265,200,16,character.IsRagdollActive()?ORANGE:LIGHTGRAY);

    if(vehicle.finalized&&player.driving) {
        DrawRectangle(10,GetScreenHeight()-145,285,135,Fade(BLACK,0.75f));
        DrawText(TextFormat("Speed: %.0f km/h",vehicle.SpeedKph()),25,GetScreenHeight()-128,21,RAYWHITE);
        DrawText(TextFormat("Fuel: %.0f%%",vehicle.fuel),25,GetScreenHeight()-101,21,YELLOW);
        DrawText(TextFormat("Mass: %.0f kg",vehicle.TotalMass()),25,GetScreenHeight()-74,21,RAYWHITE);
        DrawText(TextFormat("Power: %.0f",vehicle.TotalPower()),25,GetScreenHeight()-47,21,RAYWHITE);
    }

    if(jobs.Active()) {
        Vector3 z=jobs.Target(world.pickup,world.dropoff);
        float d=Vector3Distance(Pos(),z);
        DrawRectangle(GetScreenWidth()/2-180,10,360,70,Fade(BLACK,0.78f));
        DrawText("GPS",GetScreenWidth()/2-24,18,22,YELLOW);
        DrawText(TextFormat("%s - %.0f m",jobs.Label(),d),GetScreenWidth()/2-155,48,19,RAYWHITE);
    }

    std::string s=toastTime>0?toast:(jobs.toastTime>0?jobs.toast:"");
    if(!s.empty()) {
        int w=MeasureText(s.c_str(),20)+50;if(w>GetScreenWidth()-40)w=GetScreenWidth()-40;
        int x=GetScreenWidth()/2-w/2;
        DrawRectangle(x,GetScreenHeight()-70,w,45,Fade(BLACK,0.88f));
        DrawText(s.c_str(),x+25,GetScreenHeight()-58,20,RAYWHITE);
    }
    if(performanceHudVisible)DrawPerformanceHUD();
}

void Game::DrawPerformanceHUD() const {
    const auto& p=authorityRules.Shell();
    int lines=(p.performanceShowFps?1:0)+(p.performanceShowFrameMs?1:0)+(p.performanceShowVekMode?1:0)+(p.performanceShowAuthority?1:0);
    if(lines<=0)return;
    const int x=12,y=12,w=205,lineH=20,pad=10;
    int h=pad*2+lines*lineH;
    DrawRectangle(x,y,w,h,Fade(BLACK,0.82f));
    DrawRectangleLines(x,y,w,h,Fade(SKYBLUE,0.8f));
    int cy=y+pad;
    if(p.performanceShowFps){DrawText(TextFormat("FPS: %d",GetFPS()),x+pad,cy,16,LIME);cy+=lineH;}
    if(p.performanceShowFrameMs){DrawText(TextFormat("Frame: %.2f ms",GetFrameTime()*1000.0f),x+pad,cy,16,RAYWHITE);cy+=lineH;}
    if(p.performanceShowVekMode){DrawText(TextFormat("VEK: %s",VekSecuritySystem::SecurityModeName()),x+pad,cy,15,SKYBLUE);cy+=lineH;}
    if(p.performanceShowAuthority){DrawText(TextFormat("Authority: %s",authorityRules.AuthorityRoleName()),x+pad,cy,15,ORANGE);}
}
