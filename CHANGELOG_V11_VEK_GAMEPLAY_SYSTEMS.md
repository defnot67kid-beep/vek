# v11 - VEK 1.1 Gameplay Systems

## Added
- `VekCharacterRules` bridge from the game to VEK 1.1.
- `scripts/player_systems.vek` with gravity, jump, health, fall damage and ragdoll rules.
- `HumanoidRuleProvider` hook so the existing HumanoidSystem remains the state owner while VEK controls gameplay formulas.
- VEK-driven hard-landing damage.
- Procedural ragdoll collapse/recovery using the VEK 1.1 `vek::RagdollSystem`.
- Signed secure-release support for `player_systems.vek`.
- DEV F11 reload now reloads both gameplay VEK scripts.

## Not used yet
VEK 1.1 includes an advanced renderer-independent GUI command system, but v11 intentionally does not use it. Existing game UI is unchanged.
