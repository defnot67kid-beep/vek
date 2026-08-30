# v26.8 - Physical Personnel Door + Automatic Workshop Entry

## Personnel door interaction
- Enlarged the personnel-door grip and click target so it is easy to see and select from third person.
- The handle now has a visible spindle, lever and moving grip instead of a tiny static nub.
- Clicking the outside handle or pressing E near it starts a staged physical interaction:
  1. approach the handle,
  2. reach for it,
  3. rotate the handle down,
  4. pull the door through its real opening arc,
  5. release the grip,
  6. walk straight through the doorway,
  7. activate Build Mode after entering the workshop.

## Arm physics / IK
- Added reusable left/right `ArmPhysicsConstraint` state to the procedural character rig.
- Grip targets are followed with a damped spring (`stiffness + damping + velocity`) instead of snapping directly to a canned pose.
- Each constrained arm uses a two-bone upper-arm/forearm IK solve with the normal procedural elbow as the preferred bend direction.
- During the door pull, the right hand follows the *live moving handle position* while the door rotates.
- Held equipment is hidden while the hand is gripping an interaction target.
- First-person arms also blend toward the same physics target.

## Workshop Build Mode flow
- Completing the outside personnel-door entry automatically opens Vehicle Build Mode.
- Because Build Mode owns the editor camera and pauses walking, `B` releases the editor when the player wants to physically leave.
- That release is latched for the current workshop session so the editor does not instantly reopen while the character is still indoors.
- Crossing outside ends the auto-build session; the next physical door entry can auto-open Build Mode again.

## Compatibility
- VEK remains 2.6.1. No signed `.vek` gameplay scripts were modified by this patch.
- v26.7's smart incremental cache remains unchanged, so only the native files changed by v26.8 need recompilation.
