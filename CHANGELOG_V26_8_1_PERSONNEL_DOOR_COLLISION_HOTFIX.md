# v26.8.1 - Personnel Door Traversal Collision Hotfix

## Fixed
- Fixed the personnel door becoming an invisible solid blocker during the scripted enter-workshop animation.
- `World::UpdatePersonnelDoor` now reports collision-topology changes so the live `CollisionSystem` refreshes when the personnel door crosses its solid/non-solid threshold.
- Added a temporary personnel-door traversal collision override. Once the avatar finishes turning the handle and owns the scripted pull/walk sequence, the door slab is removed from collision immediately.
- The traversal override is released after the avatar reaches the safe inside point. The visually open door remains non-solid, and collision is automatically restored when the door later closes below the clearance threshold.
- Reset/death cleanup now always releases the traversal override so an interrupted sequence cannot leave a permanently ghosted doorway.

## Build behavior
- Keeps the v26.7+ shared smart incremental build architecture. Only changed native files should need recompilation after the existing cache has been seeded.
- VEK runtime remains 2.6.1.
