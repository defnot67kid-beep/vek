# v21 — VEK 2.2 integration + arm rig orientation fix

## Character arm rig

The procedural arm coordinate convention is now explicit: positive arm pitch points toward the character's visual forward axis (`+Z`).

Fixes:

- left/right arms now counter-swing the opposite leg during walk/run/sprint instead of moving in phase with the same-side leg;
- forearm/elbow flexion now bends forward rather than behind the torso;
- seated-drive arms reach toward the wheel/dashboard instead of rotating backward;
- celebration arms raise forward/up rather than behind the head;
- airborne arm poses remain on the forward side of the shoulders.

This addresses the backwards-rotated arm appearance without changing collision or gameplay-root orientation.

## VEK

- bundled fallback runtime updated to VEK 2.2.0;
- local fallback remains available for source ZIP builds;
- VEK Git tag hint updated to `v2.2.0`.
