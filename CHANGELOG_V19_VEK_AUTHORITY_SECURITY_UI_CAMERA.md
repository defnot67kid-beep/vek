# Game v19 / VEK 1.9

## UI cleanup
- Removed the always-visible top-left controls/help panel through VEK shell policy.
- Removed the full-world-map instruction/filter strip through VEK shell policy.
- F toggles a compact performance/security HUD during normal gameplay.
- Full-map player recenter moved to HOME so F can be the performance toggle.
- Builder keeps its context-specific F selected-part focus command.

## Camera floor safety
- Gameplay/free cameras are clamped above the VEK-defined world floor.
- `camera_world.vek` owns `prevent_below_world`, `minimum_world_y` and `minimum_target_y`.
- `security_authority.vek` can raise the host floor-clearance policy further.

## VEK authority/security
- Added signed `scripts/security_authority.vek`.
- Added `VekAuthorityRules` host adapter.
- Local standalone vehicle finalize, upgrades, repairs and job acceptance pass through VEK authority policy.
- Added server-authoritative action definitions for economy, progression, inventory, jobs, player state and vehicles.
- Added VEK replication schemas for player, economy and vehicle state.
- All VEK host VMs now receive Development or Hardened Client execution policies before loading game scripts.
- VEK Guard now rejects non-.vek paths, `..` traversal, symlink script files and embedded NUL source bytes in the game host.

## Multiplayer architecture
VEK 1.9 provides authority validation and replication policy, not a socket transport. A dedicated/listen server must authenticate a connection natively, assign a trusted actor ID/capabilities, then pass requests to VEK authority validation before committing server-owned state.
