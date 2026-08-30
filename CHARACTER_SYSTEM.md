# Original Playable Human Character System

This character system was created specifically for this project. It does not use a model, rig, animation, clothing design, or character copied from another game or franchise.

## Current implementation

The avatar is a procedural full-body 3D humanoid with independent body segments driven by a small custom joint rig:

- head
- neck
- torso/pelvis
- left/right upper and lower arms
- left/right hands
- left/right upper and lower legs
- left/right feet

The procedural approach makes the prototype self-contained: there is no external character asset to license, and body proportions can change at runtime.

## Systems

### `PlayerCharacterSystem`
Owns the current avatar state and coordinates rendering, movement context, equipment display, creator state, first-person arms and gameplay animation state.


### `CharacterRotationSystem`
Owns world-facing/root yaw independently from camera yaw. It provides shortest-angle
rotation, configurable state-specific turn speeds, angular acceleration/deceleration,
input deadzone handling, turn-in-place states, first-person body catch-up, independent
head/upper-body yaw offsets, interaction target facing, seated vehicle alignment and
rotation debug data.

`PlayerCharacterSystem` owns this modular rotation component, while `Game` supplies
camera-relative movement direction and active camera yaw. The animation system consumes
rotation data but does not own world-facing yaw.

### `CharacterAnimationSystem`
Custom animation state machine with smooth interpolation between procedural joint poses. States include:

- Idle
- Walk
- Run
- Sprint
- Jump
- Land
- Crouch
- TurnInPlace
- Pickup
- Place
- Push
- Pull
- RotatePart
- Repair
- UseTool
- Menu
- EnterVehicle
- ExitVehicle
- SeatedDrive
- CarryMedium
- CarryHeavy
- Inspect
- KneelRepair
- Celebrate
- Hurt

The current prototype uses procedural animation rather than mocap clips. It is designed so imported skeletal animation clips can replace or augment individual states later without changing gameplay systems.

### Avatar creator
Press `C` while near the workshop.

Customizable values:

- presentation: neutral / masculine / feminine
- body size
- height
- shoulder width
- arm length/size
- leg length/size
- 8 skin tones
- 4 face shapes
- eye style
- eyebrow style
- nose style
- lip style
- jaw style
- ear style
- freckles
- short hair
- long hair
- curly hair
- straight hair
- braids
- ponytail
- afro
- 8 hair colours
- clothing style
- 8 clothing colours
- glasses
- goggles
- cap
- helmet
- backpack
- mechanic gloves

Use `UP/DOWN` to select a creator row and `LEFT/RIGHT` to change it. Hold `SHIFT` and use `LEFT/RIGHT` to rotate the preview avatar through 360 degrees without modifying gameplay body yaw. Press `ENTER` to save. Press `C` to leave the creator.

The avatar is saved separately in `saves/avatar.txt` by `AvatarSaveSystem`.

## Clothing progression

`ClothingSystem` supports reputation requirements:

- starter work clothing: Rep 0
- workshop jacket: Rep 2
- safety vest: Rep 3
- pilot gear: Rep 5
- marine gear: Rep 7
- heavy industrial gear: Rep 10
- player's company uniform: Rep 15

This is a framework for later shops, outfit ownership, branding and company colours.

## Equipment

`Q` cycles equipment:

1. Wrench
2. Hammer
3. Welding tool
4. Repair scanner
5. Job tablet
6. Flashlight
7. Fuel can
8. Tow strap
9. GPS device

The current framework renders equipment at the right-hand attachment point. Future versions can add tool-specific gameplay, sounds, particles, inventory durability and two-handed IK.

## Character gameplay controls

- `WASD` move
- `LEFT ALT` walk slowly
- normal WASD = run
- `LEFT SHIFT` sprint
- `SPACE` jump
- `X` crouch
- `E` enter/exit vehicle
- `Q` next tool
- `T` repair/use wrench
- `I` inspect with scanner
- `N` kneel repair pose
- `G` celebrate
- `Y` carry medium cargo mode
- `H` carry heavy cargo mode
- `C` avatar creator at workshop
- `V` cycle camera modes

## Camera modes

`V` cycles:

1. third-person follow
2. close third-person
3. first-person / cockpit when driving
4. free inspection camera

Build Mode automatically uses a raised build camera.

In cockpit mode the mouse allows the seated character to look left/right/up/down while driving. First-person walking and cockpit views render the avatar's arms/hands.

Free inspection camera:

- `WASD` move
- `Q/E` down/up
- mouse look
- `SHIFT` faster movement

## Vehicle integration

The character is drawn in the actual seat position of the custom vehicle. In third-person driving the complete body remains visible inside the car. In first-person/cockpit mode the full body is hidden to avoid camera clipping and the hands/arms remain visible.

## Build integration

The avatar remains a physical world character instead of becoming a floating camera. Build placement triggers the character's place-part pose. Finalizing a vehicle triggers a tool-use pose. This provides hooks for later hand IK, carrying individual vehicle parts, welding, tightening bolts, pushing chassis pieces and animated snapping.

## Current limitations and next quality step

This version is a self-contained **procedural game-ready prototype**, not a final AAA human mesh. It intentionally uses generated 3D body geometry and joint posing so the game can run without Blender/model downloads or copyrighted assets.

For a production-quality character, the next visual step should be an original sculpted mesh made specifically for this project, skinned to the same logical skeleton. The current animation/gameplay interfaces can remain in place while the render layer is upgraded to glTF skeletal meshes, IK and authored animation clips.

## Rotation integration

The procedural rig now follows a root-first hierarchy. Root yaw controls locomotion;
upper-body and head look are local pose offsets. Limbs are not repeatedly assigned
independent world-space rotations. Tight turns reduce stride length and bias the inside
leg shorter than the outside leg, with foot-yaw fields reserved for future IK.

See `ROTATION_SYSTEM.md` for tuning values, debug controls and tests.

## V5 humanoid foundation

The avatar now owns a `HumanoidSystem` for health/stamina/alive state and vertical locomotion.
The visible procedural rig also uses true yawed torso/pelvis geometry plus local chest/pelvis
counter-rotation during locomotion. See `HUMANOID_SYSTEM.md` and `ANIMATION_UPGRADE.md`.
