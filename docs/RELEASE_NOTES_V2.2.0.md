# VEK Programming Language v2.2.0 — Native GUI Installer

VEK 2.2 adds a Windows-native installation experience while keeping the portable ZIP and Git-clone workflows available.

## New `vek --install` flow

Running:

```bat
vek --install
```

prints the VEK ASCII mark, a short `1 - 2 - 3` startup sequence, and `Installing VEK...`, then opens `VekInstaller.exe` as a normal Windows graphical application.

The actual installation is not performed inside the console. The GUI offers:

- **Quick install** — install/repair at `C:\vek`
- **Choose a folder** — use a custom destination
- **Use this folder** — keep the extracted portable copy where it is and register it
- an **Add VEK to my Windows User PATH** option, enabled by default

The installer updates only the current user's PATH and broadcasts the Windows environment-change notification. Administrator rights are not intentionally required, although Windows permissions may block a protected destination.

## First-time installation

A new user who has not added VEK to PATH yet can either:

- double-click `VekInstaller.exe`, or
- open a terminal in the extracted VEK folder and run `vek.exe --install`.

After PATH setup, open a new terminal and use `vek` from anywhere.

## Portable remains supported

The older `INSTALL_PATH.cmd` / `UNINSTALL_PATH.cmd` helpers remain in the package as a simple fallback. VEK still discovers its home relative to `vek.exe`, so the distribution remains relocatable.

## Release packaging

The Windows release workflow now builds and packages both:

- `vek.exe`
- `VekInstaller.exe`

alongside the runtime DLL, headers, libraries, examples, docs, VERSION file, and integrity manifest.

The release ZIP remains self-contained and does not require Git, CMake, or Visual Studio for normal use.
