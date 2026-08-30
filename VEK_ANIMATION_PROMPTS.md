# VEK Animation & Proximity Prompt Integration

VEK 1.4 adds safe renderer-independent animation and proximity prompt systems.

The game registers an `AnimationLibrary` and a `ProximityPromptRegistry` with a sealed VEK VM. VEK scripts can define clip metadata, markers, prompt text/range/input behavior, and door timing. Native C++ remains responsible for character pose execution, raylib drawing, collision geometry, keyboard polling, and world transforms.

Current hangar flow:

SURVIVAL: approach personnel door -> VEK prompt -> E -> scripted walk -> VEK-timed open-door animation -> door opens -> walk inside -> Vehicle Editor.

SANDBOX: press B near hangar -> same animated entry sequence -> Vehicle Editor.

While the Vehicle Editor is active, `EditorCameraSystem` clamps the camera to the VEK-defined `HangarBuildArea`.


## VEK 1.5 garage/passlock extension

Animation metadata can now be referenced by `GarageDoorDefinition` through separate open/close clip IDs. GarageDoorState exposes active animation ID/time/normalized progress. Passlocks add safe numeric access policy and new modal/password/status/keypad GUI commands. See `VEK_GARAGE_PASSLOCK_SYSTEM.md`.
