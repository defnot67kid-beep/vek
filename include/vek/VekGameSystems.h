#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <vek/VekScriptEngine.h>
class VekScriptEngine;
namespace vek {
struct GravitySettings{float acceleration=15.5f,terminalFallSpeed=32.0f,groundedVelocity=0.0f;};
struct GravityState{float verticalVelocity=0,airborneTime=0,lastLandingSpeed=0;bool grounded=true;};
class GravitySystem{public:static bool BeginJump(GravityState&,float);static bool Step(GravityState&,const GravitySettings&,float,float,float&);};
struct HealthSettings{float maxHealth=100,hurtThreshold=0.35f;};struct HealthState{float health=100;bool alive=true;};
class HealthSystem{public:static void Reset(HealthState&,const HealthSettings&);static float ApplyDamage(HealthState&,const HealthSettings&,float);static float Heal(HealthState&,const HealthSettings&,float);static void SetHealth(HealthState&,const HealthSettings&,float);static float Normalized(const HealthState&,const HealthSettings&);static bool IsHurt(const HealthState&,const HealthSettings&);};
enum class RagdollPhase{Animated,Ragdoll,Recovering};
struct RagdollSettings{float minimumImpactSpeed=13,fatalImpactSpeed=25,minimumDuration=0.8f,maximumDuration=3,linearDamping=3.2f,angularDamping=4.4f,recoveryDuration=0.65f;};
struct RagdollState{RagdollPhase phase=RagdollPhase::Animated;float time=0,targetDuration=0,blend=0,pitch=0,roll=0,yawOffset=0,pitchVelocity=0,rollVelocity=0,yawVelocity=0,bodyDrop=0,armSpread=0,legSpread=0;};
class RagdollSystem{public:static bool ShouldTrigger(float,float,float,const RagdollSettings&);static void Trigger(RagdollState&,float,float,const RagdollSettings&);static void Update(RagdollState&,float,const RagdollSettings&);static void BeginRecovery(RagdollState&,const RagdollSettings&);static bool Active(const RagdollState&);};


struct AnimationMarker {
    std::string name;
    float time=0.0f;
    VekValue data;
};
struct AnimationDefinition {
    std::string id;
    float duration=1.0f;
    float speed=1.0f;
    float blendIn=0.12f;
    float blendOut=0.12f;
    bool loop=false;
    std::vector<std::string> tags;
    std::vector<AnimationMarker> markers;
};
struct AnimationPlaybackState {
    std::string clipId;
    float time=0.0f;
    float normalizedTime=0.0f;
    int loopCount=0;
    bool playing=false;
    bool finished=false;
};
class AnimationLibrary {
public:
    bool RegisterAnimation(const AnimationDefinition& definition);
    bool RegisterAnimationValue(const VekValue& definition,std::string* error=nullptr);
    const AnimationDefinition* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,AnimationDefinition> clips;
};
class AnimationSystem {
public:
    static bool Play(AnimationPlaybackState& state,const AnimationDefinition& clip,bool restart=true);
    static void Stop(AnimationPlaybackState& state);
    static void Update(AnimationPlaybackState& state,const AnimationDefinition& clip,float dt);
    static bool PassedMarker(const AnimationPlaybackState& previous,const AnimationPlaybackState& current,const AnimationDefinition& clip,const std::string& marker);
};

struct ProximityPromptDefinition {
    std::string id;
    std::string actionText="Interact";
    std::string objectText;
    std::string inputKey="E";
    float maxDistance=3.5f;
    float holdDuration=0.0f;
    bool enabled=true;
    bool requiresLineOfSight=false;
    int priority=0;
};
struct ProximityPromptState {
    bool visible=false;
    bool activated=false;
    float distance=0.0f;
    float holdProgress=0.0f;
};
class ProximityPromptRegistry {
public:
    bool RegisterPrompt(const ProximityPromptDefinition& definition);
    bool RegisterPromptValue(const VekValue& definition,std::string* error=nullptr);
    const ProximityPromptDefinition* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,ProximityPromptDefinition> prompts;
};
class ProximityPromptSystem {
public:
    static bool Update(ProximityPromptState& state,const ProximityPromptDefinition& definition,float distance,bool inputDown,bool inputPressed,float dt);
};

enum class GarageDoorMotion { Closed=0, Opening, Open, Closing };
struct GarageDoorDefinition {
    std::string id;
    std::string displayName="Garage Door";
    float width=24.0f;
    float height=8.0f;
    int panelCount=8;
    float openDuration=2.6f;
    float closeDuration=2.3f;
    float autoCloseDelay=8.0f;
    bool startsLocked=true;
    bool autoClose=true;
    bool allowInsideEgress=true;
    float insideOpenDistance=6.0f;
    bool holdOpenNearDoor=true;
    float panelOverlap=0.025f;
    float sideSealWidth=0.34f;
    float lintelHeight=0.68f;
    float collisionClearFraction=0.72f;
    std::string openAnimation;
    std::string closeAnimation;
    std::string accessId;
};
struct GarageDoorState {
    GarageDoorMotion motion=GarageDoorMotion::Closed;
    float openFraction=0.0f;
    float autoCloseTimer=0.0f;
    bool locked=true;
    std::string activeAnimation;
    float animationTime=0.0f;
    float animationNormalized=0.0f;
};
class GarageDoorRegistry {
public:
    bool RegisterGarage(const GarageDoorDefinition& definition);
    bool RegisterGarageValue(const VekValue& definition,std::string* error=nullptr);
    const GarageDoorDefinition* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,GarageDoorDefinition> garages;
};
class GarageDoorSystem {
public:
    static void Reset(GarageDoorState& state,const GarageDoorDefinition& definition);
    static bool RequestOpen(GarageDoorState& state,const GarageDoorDefinition& definition,bool accessGranted);
    static void RequestClose(GarageDoorState& state);
    static void HoldOpen(GarageDoorState& state,const GarageDoorDefinition& definition);
    static void Unlock(GarageDoorState& state);
    static void Lock(GarageDoorState& state);
    static void Update(GarageDoorState& state,const GarageDoorDefinition& definition,float dt);
};

enum class PasslockResult { Granted=0, Denied, LockedOut, InvalidInput };
struct PasslockDefinition {
    std::string id;
    std::string displayName="Access Control";
    std::string accessCode;
    std::string linkedGarageId;
    std::string promptId;
    float maxUseDistance=4.5f;
    bool requiresLineOfSight=true;
    bool outsideOnly=true;
    bool showWorldPrompt=false;
    bool clickOnly=true;
    int minDigits=4;
    int maxDigits=8;
    int maxAttempts=5;
    float lockoutSeconds=10.0f;
    bool allowKeyboard=true;
    bool allowMouse=true;
    bool maskInput=true;
};
struct PasslockState {
    int failedAttempts=0;
    float lockoutRemaining=0.0f;
    bool granted=false;
};
class PasslockRegistry {
public:
    bool RegisterPasslock(const PasslockDefinition& definition);
    bool RegisterPasslockValue(const VekValue& definition,std::string* error=nullptr);
    const PasslockDefinition* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,PasslockDefinition> passlocks;
};
class PasslockSystem {
public:
    static void Reset(PasslockState& state);
    static void Update(PasslockState& state,float dt);
    static PasslockResult Submit(PasslockState& state,const PasslockDefinition& definition,const std::string& code);
};

// VEK 1.7 presentation/lifecycle metadata. These systems deliberately contain
// no renderer, audio-device, filesystem or native-pointer access. They only
// describe safe host-facing behavior and timing.
struct AudioCueDefinition {
    std::string id;
    std::string assetId;
    float volume=1.0f;
    float pitch=1.0f;
};
class AudioCueRegistry {
public:
    bool RegisterCue(const AudioCueDefinition& definition);
    bool RegisterCueValue(const VekValue& definition,std::string* error=nullptr);
    const AudioCueDefinition* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,AudioCueDefinition> cues;
};

struct ScreenEffectDefinition {
    std::string id;
    float duration=2.0f;
    float fadeIn=0.10f;
    float hold=1.0f;
    float fadeOut=0.65f;
    struct Color { float r=150,g=0,b=0,a=180; } tint;
    float vignette=0.75f;
    float spatter=0.55f;
    float pulse=0.15f;
};
struct ScreenEffectState {
    bool active=false;
    float time=0.0f;
    float opacity=0.0f;
};
class ScreenEffectRegistry {
public:
    bool RegisterEffect(const ScreenEffectDefinition& definition);
    bool RegisterEffectValue(const VekValue& definition,std::string* error=nullptr);
    const ScreenEffectDefinition* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,ScreenEffectDefinition> effects;
};
class ScreenEffectSystem {
public:
    static void Start(ScreenEffectState& state);
    static void Stop(ScreenEffectState& state);
    static void Update(ScreenEffectState& state,const ScreenEffectDefinition& definition,float dt);
};

struct DeathSequenceDefinition {
    std::string id;
    float ragdollImpact=24.0f;
    float ragdollDuration=2.25f;
    float screenDelay=0.08f;
    float audioDelay=0.28f;
    float respawnDelay=2.75f;
    std::string screenEffectId;
    std::string audioCueId;
};
struct DeathSequenceState {
    bool active=false;
    float time=0.0f;
    bool screenStarted=false;
    bool audioPlayed=false;
    bool respawnIssued=false;
};
struct DeathSequenceEvents {
    bool startScreen=false;
    bool playAudio=false;
    bool respawn=false;
};
class DeathSequenceRegistry {
public:
    bool RegisterSequence(const DeathSequenceDefinition& definition);
    bool RegisterSequenceValue(const VekValue& definition,std::string* error=nullptr);
    const DeathSequenceDefinition* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,DeathSequenceDefinition> sequences;
};
class DeathSequenceSystem {
public:
    static void Begin(DeathSequenceState& state);
    static DeathSequenceEvents Update(DeathSequenceState& state,const DeathSequenceDefinition& definition,float dt);
    static void Cancel(DeathSequenceState& state);
};

struct GroundingProfile {
    std::string id;
    // Procedural humanoids are rooted near their lower body. The host supplies
    // the avatar height scale and VEK resolves a feet-on-surface root height.
    float rootOffsetPerHeight=0.15f;
    float minimumRootOffset=0.08f;
    float maximumRootOffset=0.30f;
    float snapTolerance=0.04f;
};
class GroundingRegistry {
public:
    bool RegisterProfile(const GroundingProfile& definition);
    bool RegisterProfileValue(const VekValue& definition,std::string* error=nullptr);
    const GroundingProfile* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,GroundingProfile> profiles;
};
class GroundingSystem {
public:
    static float RootYForSurface(float surfaceY,float avatarHeight,const GroundingProfile& profile);
};


// VEK 1.8 camera/environment/rig policy. These structures contain only safe
// scalar/string data. Native hosts still own actual camera matrices, GPU
// rendering, collision queries and physics bodies.
struct CameraProfileDefinition {
    std::string id;
    float thirdPersonDistance=8.0f;
    float closeDistance=4.3f;
    float targetHeight=1.15f;
    float closeTargetHeight=1.35f;
    float firstPersonEyeHeight=1.66f;
    float fov=60.0f;
    float pitchMin=-25.0f;
    float pitchMax=65.0f;
    float yawSensitivity=0.12f;
    float pitchSensitivity=0.10f;
    float maxYawSpeed=720.0f;
    float maxPitchSpeed=540.0f;
    float yawAcceleration=3200.0f;
    float yawDeceleration=4200.0f;
    float pitchAcceleration=2600.0f;
    float pitchDeceleration=3400.0f;
    float alignmentStep=45.0f;
    bool rmbLook=true;
    bool invertMouseY=false;
    float rmbYawScale=1.0f;
    float rmbPitchScale=1.0f;
    float editorMoveSpeed=12.0f;
    float editorFastMultiplier=3.0f;
    float editorFineMultiplier=0.25f;
    float editorYawSpeed=90.0f;
    float editorPitchSpeed=58.5f;
    float editorPitchMin=-85.0f;
    float editorPitchMax=85.0f;
    float editorFov=60.0f;
    float editorOrthoSize=28.0f;
    bool preventBelowWorld=true;
    float minimumWorldY=0.35f;
    float minimumTargetY=0.05f;
    std::vector<std::string> cycleModes{"third_person","close_third_person","first_person","free_inspection"};
};
class CameraProfileRegistry {
public:
    bool RegisterProfile(const CameraProfileDefinition& definition);
    bool RegisterProfileValue(const VekValue& definition,std::string* error=nullptr);
    const CameraProfileDefinition* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,CameraProfileDefinition> profiles;
};

struct SkyboxDefinition {
    std::string id;
    struct Color { float r=125,g=175,b=220,a=255; } zenith,horizon,ground,sun;
    float horizonHeight=0.0f;
    float sunYaw=38.0f;
    float sunPitch=48.0f;
    float sunSize=3.5f;
    float ambient=0.70f;
    float fogStart=125.0f;
    float fogEnd=280.0f;
    float dayLengthSeconds=1200.0f;
    bool dynamicDayNight=false;
    std::string textureAsset;
};
class SkyboxRegistry {
public:
    bool RegisterSkybox(const SkyboxDefinition& definition);
    bool RegisterSkyboxValue(const VekValue& definition,std::string* error=nullptr);
    const SkyboxDefinition* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,SkyboxDefinition> skyboxes;
};

struct RigJointDefinition {
    std::string name;
    std::string parent;
    float length=0.2f;
    float radius=0.08f;
    float mass=1.0f;
    float ragdollWeight=1.0f;
    float pitchMin=-90.0f,pitchMax=90.0f;
    float yawMin=-90.0f,yawMax=90.0f;
    float rollMin=-90.0f,rollMax=90.0f;
};
struct HumanoidRigDefinition {
    std::string id;
    float globalRagdollStrength=1.0f;
    float spineFlex=0.75f;
    float neckFlex=0.85f;
    float limbFlex=1.0f;
    std::vector<RigJointDefinition> joints;
};
class HumanoidRigRegistry {
public:
    bool RegisterRig(const HumanoidRigDefinition& definition);
    bool RegisterRigValue(const VekValue& definition,std::string* error=nullptr);
    const HumanoidRigDefinition* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,HumanoidRigDefinition> rigs;
};

struct WorldGameplayPolicy {
    std::string id="game.default";
    int targetFps=120;
    float movementWalk=2.4f;
    float movementRun=5.2f;
    float movementSprint=7.6f;
    float interactionDistance=3.5f;
    float vehicleEnterDistance=3.6f;
    float mapMarkerDistance=250.0f;
    float physicsMaxDt=0.05f;
    bool cameraRmbLook=true;
    bool skyEnabled=true;
};
class WorldGameplayPolicyRegistry {
public:
    bool RegisterPolicy(const WorldGameplayPolicy& definition);
    bool RegisterPolicyValue(const VekValue& definition,std::string* error=nullptr);
    const WorldGameplayPolicy* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,WorldGameplayPolicy> policies;
};

enum class GuiCommandType{
 BeginWindow,EndWindow,BeginModal,EndModal,BeginPanel,EndPanel,BeginDockPanel,EndDockPanel,BeginScrollPanel,EndScrollPanel,BeginGrid,EndGrid,BeginHorizontal,EndHorizontal,BeginVertical,EndVertical,BeginTabs,EndTabs,BeginTree,EndTree,BeginList,EndList,BeginContextMenu,EndContextMenu,BeginPropertyGrid,EndPropertyGrid,
 Label,Button,ImageButton,Checkbox,Slider,ProgressBar,TextInput,PasswordInput,SearchBox,ComboBox,Dropdown,StatusBadge,Keypad,Tooltip,Separator,Spacer
};
struct GuiRect{float x=0,y=0,width=0,height=0;};
struct GuiColor{float r=0,g=0,b=0,a=255;};

enum class GuiTextAlign { Left=0, Center, Right };
struct GuiTextPolicy {
    float fontSize=18.0f;
    float minFontSize=10.0f;
    float maxFontSize=24.0f;
    float lineHeight=1.15f;
    int maxLines=2;
    bool autoFit=true;
    bool wrap=true;
    bool ellipsis=true;
    bool clip=true;
    GuiTextAlign align=GuiTextAlign::Left;
};
struct GuiTextLayoutResult {
    float fontSize=18.0f;
    std::vector<std::string> lines;
    bool truncated=false;
};
using GuiMeasureTextFn=std::function<float(const std::string&,float)>;
class GuiTextLayoutSystem {
public:
    static GuiTextLayoutResult Layout(const std::string& text,float width,float height,const GuiTextPolicy& policy,const GuiMeasureTextFn& measure);
};

struct GuiStyle{
    float scale=1,opacity=1,cornerRadius=8,padding=10,spacing=6;
    float minWidth=0,maxWidth=0,minHeight=0,maxHeight=0;
    bool responsive=true;
    GuiTextPolicy text;
    GuiColor background{18,27,33,245},foreground{235,242,244,255},accent{57,174,188,255},border{74,102,110,255};
};
struct GuiCommand{
    GuiCommandType type=GuiCommandType::Label;
    std::string id,text,styleId,aux;
    GuiRect rect;
    GuiTextPolicy textPolicy;
    float value=0,minValue=0,maxValue=1;
    bool checked=false,enabled=true;
    int columns=1;
    std::vector<std::string>items;
};
class GuiSystem{
public:
 void BeginFrame();void EndFrame();void SetStyle(const GuiStyle&);const GuiStyle&GetStyle()const;void DefineStyle(const std::string&,const GuiStyle&);const GuiStyle*FindStyle(const std::string&)const;
 void BeginWindow(const std::string&,const std::string&,GuiRect={},const std::string&style={});void EndWindow();void BeginModal(const std::string&,const std::string&,GuiRect={},const std::string&style={});void EndModal();void BeginPanel(const std::string&,GuiRect={},const std::string&style={});void EndPanel();
 void BeginDockPanel(const std::string&,GuiRect={},const std::string&style={});void EndDockPanel();void BeginScrollPanel(const std::string&,GuiRect={},const std::string&style={});void EndScrollPanel();void BeginGrid(const std::string&,int columns,GuiRect={},const std::string&style={});void EndGrid();void BeginHorizontal(const std::string&,const std::string&style={});void EndHorizontal();void BeginVertical(const std::string&,const std::string&style={});void EndVertical();void BeginTabs(const std::string&,const std::vector<std::string>&,const std::string&style={});void EndTabs();void BeginTree(const std::string&,const std::string&,const std::string&style={});void EndTree();void BeginList(const std::string&,const std::string&style={});void EndList();void BeginContextMenu(const std::string&,GuiRect={},const std::string&style={});void EndContextMenu();void BeginPropertyGrid(const std::string&,const std::string&style={});void EndPropertyGrid();
 void Label(const std::string&,GuiRect={},const std::string&style={});bool Button(const std::string&,const std::string&,GuiRect={},const std::string&style={});bool ImageButton(const std::string&,const std::string&,const std::string&,GuiRect={},const std::string&style={});bool Checkbox(const std::string&,const std::string&,bool,GuiRect={},const std::string&style={});float Slider(const std::string&,const std::string&,float,float,float,GuiRect={},const std::string&style={});void ProgressBar(const std::string&,float,float,float,GuiRect={},const std::string&style={});std::string TextInput(const std::string&,const std::string&,GuiRect={},const std::string&style={});std::string PasswordInput(const std::string&,const std::string&,GuiRect={},const std::string&style={});std::string SearchBox(const std::string&,const std::string&,GuiRect={},const std::string&style={});int ComboBox(const std::string&,const std::vector<std::string>&,int,GuiRect={},const std::string&style={});int Dropdown(const std::string&,const std::vector<std::string>&,int,GuiRect={},const std::string&style={});void StatusBadge(const std::string&,const std::string&,const std::string&status,GuiRect={},const std::string&style={});void Keypad(const std::string&,const std::vector<std::string>&,GuiRect={},const std::string&style={});void Tooltip(const std::string&,const std::string&);void Separator();void Spacer(float);
 void SetPressed(const std::string&,bool);void SetValue(const std::string&,float);void SetText(const std::string&,std::string);void SetIndex(const std::string&,int);const std::vector<GuiCommand>&Commands()const;GuiTextPolicy ResolveTextPolicy(const std::string& styleId)const;void RegisterNatives(VekScriptEngine&);
private:GuiStyle style;std::unordered_map<std::string,GuiStyle>styles;std::vector<GuiCommand>commands;std::unordered_map<std::string,bool>pressed;std::unordered_map<std::string,float>values;std::unordered_map<std::string,std::string>texts;std::unordered_map<std::string,int>indices;
};
void VekRegisterGameplayLibrary(VekScriptEngine&engine);
} // namespace vek
