# VEK Collision System

## Architecture

The system deliberately separates fast geometry from editable game rules.

```text
C++ world/actor geometry
        ↓
CollisionSystem broad/narrow contact check
        ↓
VEK on_collision(a, b, speed)
        ↓
solid / friction / bounce / damage response
        ↓
C++ applies safe physical correction
```

VEK does not iterate every triangle or perform low-level collision geometry. That would make an interpreted language do work C++ is much better at. VEK controls the *meaning* of the contact.

## Current colliders

Static world collision currently mirrors the solid prototype geometry:

- workshop back wall
- workshop left wall
- workshop right wall
- test block
- industrial warehouse
- delivery building
- four invisible prototype-world limits

Dynamic actors:

- humanoid player — circular ground footprint
- parked custom vehicle — circular ground footprint
- driven custom vehicle — circular ground footprint

These simple shapes are intentional for the current procedural prototype. The interface can later be expanded to capsules, oriented boxes, convex hulls, per-part vehicle colliders and Jolt Physics without changing VEK's gameplay rule layer.

## Player collision

Player movement is resolved after horizontal movement. Wall-normal velocity is removed while tangential velocity remains, allowing the avatar to slide naturally along walls rather than sticking or passing through them.

The parked vehicle is also a collision object while the player is on foot. During the enter-vehicle sequence, that one collider is temporarily ignored so the avatar can reach the seat interaction position.

## Vehicle collision

The driven vehicle uses its velocity to calculate impact speed. `collision.vek` currently gives vehicle/world contacts a small bounce and applies damage above a configurable speed threshold.

Vehicle damage is rate-limited so resting against a wall does not damage the vehicle every frame.

## Debugging

**F10** toggles collision debug.

It draws:

- magenta static collision bounds
- blue player footprint
- orange vehicle footprint
- green last-contact normal

The HUD displays:

- whether VEK loaded or safe fallback is active
- contacts this frame
- actors involved
- object name
- impact speed
- penetration
- solid/friction/bounce/damage result

**F11** hot reloads `scripts/collision.vek`.

## Future collision upgrades

- capsule humanoid collision
- oriented vehicle boxes
- individual vehicle-part colliders
- ramps/terrain height collision
- triggers and sensor volumes
- water volumes
- collision layers/masks
- per-material surface response
- per-bone humanoid hitboxes
- Jolt Physics integration for rigid-body constraints

VEK can remain the rule/event layer even after the detector is upgraded.
