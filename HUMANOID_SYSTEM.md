# Humanoid System

`HumanoidSystem` is the reusable gameplay layer underneath the player's original procedural avatar.
It is deliberately independent from appearance, clothing and equipment, so later NPCs or multiplayer
characters can use the same health/jump/state logic with a different visual model.

## Current responsibilities

- `health` / `maxHealth`
- damage and healing API
- alive state
- hurt threshold
- stamina + sprint drain/recovery
- grounded state
- jump launch velocity
- gravity and terminal falling velocity
- airborne time
- last landing impact speed
- stable `HumanoidBone` identifiers for future hitboxes / IK / equipment sockets

World hazards do **not** damage the player yet. The API is intentionally ready for later systems to call:

```cpp
character.humanoid.ApplyDamage(25.0f);
character.humanoid.Heal(10.0f);
```

The player HUD shows Health and Stamina so the component can be inspected while developing.

## Future health integration

The intended next layer can add:

- fall damage based on `GetLastLandingSpeed()`
- collision damage
- fire / heat / cold / drowning hazards
- body-region damage using `HumanoidBone`
- healing items
- downed/dead/revive states
- NPC humanoid health
- multiplayer replication

Health logic belongs here rather than in `Game.cpp`.
