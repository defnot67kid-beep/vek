# Procedural Animation Upgrade

## Torso rotation fix

The old procedural avatar rotated shoulder/head joint positions, but the torso itself was rendered with
`DrawCube`, which is always axis-aligned. That made the chest visually appear stuck even when the avatar's
root/arms turned.

The upgraded avatar uses `DrawCubePro` for the torso, pelvis, feet, clothing overlays, backpack and carried
cargo. This means the visible geometry now has a real yaw orientation.

The hierarchy is:

```text
Character root yaw
  -> pelvis yaw + small counter-twist
    -> torso yaw + camera-look offset + locomotion twist
      -> neck/head look offset
      -> arms
    -> legs / feet
```

No bone is being given an independent world-space character facing. These are local procedural pose layers.

## Improved running

Run and sprint now include:

- cadence scaled by real movement speed
- larger sprint stride
- alternating chest twist
- counter-rotating pelvis
- shoulder rise/fall
- stronger bent-elbow arm drive
- visible foot lift during swing phase
- vertical body bob
- shorter inside-leg stride during turns
- existing foot-yaw alignment retained

## Improved jumping

Jump animation is no longer one frozen pose. It reads actual vertical velocity from `HumanoidSystem` and has:

1. **Takeoff/rise** - body opens and follows launch momentum.
2. **Apex** - knees tuck and arms react as vertical speed approaches zero.
3. **Fall** - legs begin preparing for the ground.
4. **Landing** - compression scales with actual landing speed, then recovers smoothly.

This is still procedural animation rather than motion-capture clips, but the architecture now exposes the
values needed to blend authored animations or add foot IK later.
