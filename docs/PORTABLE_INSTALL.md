# VEK Windows Installation

VEK 2.2 can be used as a relocatable portable language or registered through its native Windows installer.

## Recommended first-time setup

1. Download `VEK-v2.2.0-windows-x64.zip` from the GitHub Release.
2. Extract the ZIP.
3. Double-click `VekInstaller.exe`.
4. Choose one of the Windows installer options:
   - **Quick install** — `C:\vek`
   - **Choose a folder** — select your own destination
   - **Use this folder** — keep the extracted copy where it is
5. Leave **Add VEK to my Windows User PATH** checked unless you specifically do not want a global `vek` command.
6. Finish the installer.
7. Close old Command Prompt windows and open a new one.
8. Run:

```bat
vek --version
vek info
vek doctor
```

## Use the CLI to open the installer

If you are already in the extracted VEK folder, you can run:

```bat
vek.exe --install
```

After VEK is on PATH, you can run from anywhere:

```bat
vek --install
```

The CLI prints a VEK ASCII splash and a short `1 - 2 - 3` sequence, then hands installation to the graphical Windows installer.

## Portable/manual setup still works

You can skip the GUI completely:

1. Extract VEK anywhere, for example `C:\vek` or `D:\Dev\vek`.
2. Run `INSTALL_PATH.cmd`, or manually add that folder to your Windows User PATH.
3. Open a new terminal.
4. Run `vek --version`.

`VEK_HOME` is optional. VEK normally discovers its installation from the location of `vek.exe`.

## Moving VEK later

VEK itself remains relocatable. If you move the folder after registering PATH, update the PATH entry to the new location. The installer can also use **Use this folder** to register a relocated copy.

## Git clone remains supported

Developers and contributors can still use:

```bat
git clone https://github.com/defnot67kid-beep/VekProgramingLanguage.git
cd VekProgramingLanguage
BUILD_WINDOWS.bat
```

The release ZIP is the normal-user path. Git clone is for source development.

## VEK-driven graphical installer

On Windows, `vek --install` launches `VekInstaller.exe`. VEK first prints an animated console boot sequence, checks for an existing installation, then opens the custom VEK installer window.

The installer window is not a stock pre-install wizard. Its layout and button/state decisions are emitted by `share/vek/installer_ui.vek` through VEK's `GuiSystem`, while a small hardened native Windows host performs filesystem and PATH operations.

Existing installations are detected from `C:\vek`, `VEK_HOME`, process PATH, and User PATH. If found, the primary action changes to **REPAIR / UPDATE VEK**.

Installation uses ten visible stages (`0/10` to `10/10`). On success the window says that VEK is installed and automatically closes after five seconds unless the user closes it first.
