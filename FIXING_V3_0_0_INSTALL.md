# Fixing the VEK 3.0.0 installer/version mismatch

The first VEK 3.0.0 UI Next source package had mixed version metadata. `VERSION` and CMake said `3.0.0`, but the compiled runtime header and GitHub installer still embedded `2.8.0`. The legacy installer could also create `C:\vek\versions\v3.0.0` and write newer metadata without installing a matching new CLI binary.

VEK 3.0.1 fixes the release system rather than reusing the broken 3.0.0 tag.

## Publish the fixed release

From this repository:

```bat
python tools\version_manager.py check
git add .
git commit -m "VEK 3.0.1 installer and version system"
git push origin main
git tag v3.0.1
git push origin v3.0.1
```

Do **not** create only a tag and stop there. The included GitHub Actions workflow turns the tag into the binary GitHub Release assets required by the updater. Wait for **VEK Windows Release** to finish successfully in GitHub Actions.

The GitHub Release for v3.0.1 should contain:

- `VEK-v3.0.1-windows-x64.zip`
- `VEK-v3.0.1-windows-x64.zip.sha256`
- `VekInstaller.zip`
- `VekInstaller.zip.sha256`
- `release-version.json`

## Repair the existing Windows installation

After the v3.0.1 release assets exist:

1. Run `C:\vek\VekInstaller.exe`, or extract the v3.0.1 `VekInstaller.zip` to `C:\vek` and run it.
2. Choose **LATEST / UPDATE**. If the installer reports a metadata/binary mismatch, **CLEAN REPAIR** is also safe for the managed VEK runtime/source.
3. Open a **new** Command Prompt.
4. Run:

```bat
where vek
vek --version
vek info
vek doctor
```

Expected version: `VEK 3.0.1`.

`where vek` should normally show `C:\vek\vek.exe` first.

## Important Windows command behavior

Typing:

```bat
vek
```

inside a source/download directory does **not** mean "run the VEK source in this directory". Windows resolves `vek` through the current directory and PATH. A source tree normally contains source code, not a freshly built `vek.exe`, so the command generally reaches `C:\vek\vek.exe`.

Use `where vek` whenever the binary being executed is unclear.
