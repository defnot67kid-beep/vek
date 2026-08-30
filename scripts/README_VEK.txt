VEK GAME SCRIPTS — VEK 2.3
==========================
collision.vek             collision response and impact rules
player_systems.vek        gravity, health and ragdoll rules
main_menu.vek             Survival/Sandbox menu UI and selection
vehicle_editor.vek        editor costs, unlocks, placement and validation
vehicle_editor_gui.vek    VEK-driven editor UI policy
hangar.vek                build-area and hangar rules
hangar_interactions.vek   personnel/garage door interaction policy
camera_world.vek          camera/world constraints
lifecycle.vek             lifecycle policy
security_authority.vek    deterministic server-authority + replication policy
jobs/*.vek                job rules
parts/*.vek               part definitions/rules

DEVELOPMENT
- BUILD_WINDOWS_DEV.bat allows unsigned VEK and supported hot reload.

SECURE RELEASE
- Every shipped .vek must have a .sig and an exact SHA-256 entry compiled into
  src/generated/VekPublicKey.h.
- Run SIGN_VEK_SCRIPTS.bat after intentionally changing any VEK source.
- Never distribute developer_keys/ or any private PEM.
- Prefer tools/package_secure_source.py when creating a source release ZIP.
