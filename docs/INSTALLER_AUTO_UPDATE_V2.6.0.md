# VEK Installer Auto-Update - v2.6.0

VEK 2.6.0 introduces a stable installer bootstrap. On startup, VekInstaller.exe checks the official GitHub tags. If a newer installer exists, it downloads `VekInstaller.zip` and `VekInstaller.zip.sha256` from that release, verifies SHA-256, extracts into `C:\\vek\\versions\\vX.Y.Z`, launches the newer installer, and exits.

The running installer is never overwritten. If verification fails, the current installer continues normally.
