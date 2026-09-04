# VEK 3.0.3 Full Installer — Release Discovery Fix

This bundle contains both Windows executables:

- `VekInstaller.exe` — graphical installer/update/repair UI
- `vek.exe` — console bootstrap/launcher

## What 3.0.3 fixes

The installer no longer requires `release-version.json` to exist on GitHub Releases.
It discovers the newest published release from GitHub's Release API, with a public
`/releases/latest` redirect fallback and legacy metadata fallback.

A release containing only a full `VekInstaller.zip` binary bundle is installable.
GitHub's SHA-256 asset digest is used when available; `.sha256` sidecars remain
supported but are not mandatory when a digest is already published.

This specifically supports the existing VEK v3.0.2 release layout that contains
`VekInstaller.zip` plus GitHub-generated source archives.

## Usage

Extract the ZIP and launch `VekInstaller.exe`. It may be run from Downloads; the
installer stages itself into `C:\vek` safely.
