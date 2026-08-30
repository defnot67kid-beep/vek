# VEK gameplay integration

The game uses VEK as the policy/tuning layer while C++ remains responsible for safe world state, rendering and collision geometry.

`HumanoidSystem` -> `HumanoidRuleProvider` -> `VekCharacterRules` -> `player_systems.vek`

### Gravity
`character_gravity` calls the VEK 1.1 `gravity_step` native.

### Health
`character_damage` and `character_heal` call VEK 1.1 health helpers. C++ clamps results again before accepting them.

### Ragdoll
Hard landings call VEK for fall damage, ragdoll eligibility, duration and direction. The reusable VEK `RagdollSystem` updates a procedural pose that the existing original humanoid renderer blends into.

### GUI
VEK 1.1's `GuiSystem` is available in the dependency but deliberately not registered or rendered by this game yet.
