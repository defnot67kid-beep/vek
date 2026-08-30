# Game v14 - VEK Animation, Proximity Prompts & Hangar Door

Built on the working v13.1 project without removing previous systems.

## Added
- VEK 1.4 dependency (`v1.4.0`)
- VEK-authored animation metadata and proximity prompt definitions
- Animated personnel door in the engineering hangar
- Survival-only proximity prompt at the personnel door (`E`)
- Sandbox quick builder entry remains `B`
- Scripted build-entry sequence: walk to door -> open door -> walk inside -> activate Vehicle Editor
- Procedural `OpenDoor` character animation pose
- Door collision switches safely when sufficiently open
- Door automatically closes after editor entry
- Editor free camera is clamped to the VEK-defined hangar workspace and cannot fly outside it
- Player/reset spawn moved to the apron outside the personnel entrance

## VEK file
`scripts/hangar_interactions.vek` defines door settings, prompt data and animation clip timing.

## Security
The new script is included in the existing recursive VEK signing workflow. Development builds may hot reload it; secure Windows builds require a valid signature.
