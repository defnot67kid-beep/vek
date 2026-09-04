# VEK 3.0.3 — Installer Release Discovery Fix

VEK 3.0.3 fixes release discovery/update compatibility for GitHub Releases that do not publish `release-version.json` or separate checksum/runtime sidecar assets.

## Installer fixes

- The installer now uses the GitHub latest Release API as the primary version source.
- `release-version.json` is optional instead of mandatory.
- GitHub's release `tag_name` is accepted as the canonical published version.
- GitHub asset `digest` (`sha256:...`) is used when available.
- `.sha256` companion assets remain supported but are no longer mandatory when GitHub already exposes a digest.
- The public `/releases/latest` redirect is a secondary discovery path when the API is unavailable/rate-limited.
- A full `VekInstaller.zip` can be used as the Windows runtime bundle when a separate `VEK-vX.Y.Z-windows-x64.zip` was not published.
- Package `VERSION` and `vek.exe` are still checked before installation.
- Existing v3.0.2 releases containing only `VekInstaller.zip` are now understood by the installer.

## Release compatibility

Preferred future releases should still publish:

- `VEK-vX.Y.Z-windows-x64.zip`
- `VEK-vX.Y.Z-windows-x64.zip.sha256`
- `VekInstaller.zip`
- `VekInstaller.zip.sha256`
- `release-version.json`

The installer no longer fails solely because one of the metadata/sidecar assets is missing.
