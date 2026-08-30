# Game v17 - VEK 1.7 Lifecycle, Grounding and Garage Completion

- Completed the engineering hangar roof and front fascia.
- Removed the oversized teal front sign/block.
- Closed segmented garage light gaps using VEK-defined panel overlap and side seals.
- Added VEK-defined garage lintel height and collision-clear fraction.
- Successful passlock access now latches the open request until the garage is physically open.
- Replaced the old Y=1.0 humanoid root assumption with VEK-defined avatar-height-aware foot grounding.
- Reset now starts a death sequence instead of instantly teleporting the player.
- Reset death sequence: ragdoll -> red/blood screen effect -> death audio cue -> respawn.
- Added original local death_reset.wav and native raylib audio playback.
- Added signed scripts/lifecycle.vek.
- Added VekLifeCycleSystem host bridge.
- Game requires VEK v1.7.0.
