# VEK 3.0.2 — Installer Reliability Hotfix

VEK 3.0.2 fixes several installer/version edge cases discovered after the 3.0.1 rollout.

## Fixed

- Update discovery now follows the latest **published GitHub Release** through `release-version.json` instead of assuming every `vX.Y.Z` tag has installable Windows assets.
- `VekInstaller.exe` can be run from Downloads; it safely stages itself into `C:\vek`.
- A newer bootstrapped installer is persisted to root `C:\vek\VekInstaller.exe`.
- Git for Windows is optional for binary install/update. Exact-tag source ZIP fallback keeps `C:\vek\repo` usable when Git is absent or broken.
- The bootstrap console launcher reports its compiled version, preventing `VERSION` metadata from disguising an older/newer bootstrap executable.
- Installer status, details and version state are now actually drawn in the GUI.
- The three install actions have distinct labels and AUTO is accurately described as automatic update **checking**, not silent downloading.

## Security/integrity

Runtime and installer binary packages are still downloaded over HTTPS and accepted only after SHA-256 verification. Runtime packages must contain a `VERSION` matching the selected published release before deployment.
