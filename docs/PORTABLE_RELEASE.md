# VEK 2.2 Windows Release Architecture

VEK 2.2 keeps the relocatable portable layout introduced in 2.1 and adds a separate native Windows GUI installer.

The release root is intentionally self-contained:

```text
VEK-v2.2.0-windows-x64/
├── vek.exe
├── VekInstaller.exe
├── vek.dll
├── VERSION
├── manifest.sha256
├── INSTALL_PATH.cmd
├── UNINSTALL_PATH.cmd
├── include/
├── lib/
├── examples/
└── docs/
```

`vek.exe` discovers VEK_HOME from its own location. A hard-coded install path is not required.

## Setup paths

Normal users can choose any of these:

- double-click `VekInstaller.exe`;
- run `vek.exe --install` from an extracted package;
- once on PATH, run `vek --install` from anywhere;
- keep using `INSTALL_PATH.cmd`;
- add the extracted directory to User PATH manually;
- use Git clone for source development.

The graphical installer does not replace the portable architecture. It is a convenience layer that can copy or register the same relocatable package.

## Integrity

The GitHub release workflow creates `manifest.sha256` after assembling the final package. `vek verify` checks installed files against that local manifest. The release ZIP also receives its own SHA-256 file.
