# v26.8.4 - 3D Height Collision Fix

- Fixed the real personnel-door blocker that remained in DEV builds.
- CollisionSystem used to resolve every world box only in X/Z, so the personnel-door header above the opening behaved like an invisible floor-to-ceiling wall.
- Added a vertical-overlap broadphase before horizontal player collision response.
- Player collision height now scales with avatar height and uses the VEK grounding root offset.
- Added the same vertical filtering to vehicle/world contacts so high garage lintels and overhead geometry do not block ground vehicles that fit underneath them.
- Preserved the explicit scripted personnel-door traversal collision override from v26.8.1 as an additional safety layer.
- Added regressions for both the personnel-door header and the main garage header.
- No signed `.vek` gameplay scripts were changed.
- Smart incremental build cache remains compatible; only changed native translation units should rebuild.
