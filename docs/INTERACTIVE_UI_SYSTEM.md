# VEK Interactive UI Systems

VEK 2.5 adds renderer-independent helpers for lightweight 3D UI motion,
one-button runner mini-games, update-policy state and semantic-version checks.

The runtime does not own Win32, raylib or another renderer. A host can use
`vek::UiOrbit3D` for rotating/projected splash elements and
`vek::RunnerMiniGame` for deterministic scrolling/jump gameplay while keeping
trusted install, filesystem and networking operations in native host code.

This split lets VEK drive interactive presentation behavior without granting
an installer script unrestricted system privileges.
