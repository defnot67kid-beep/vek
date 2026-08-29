# VEK 2.5.4

VEK 2.5.4 changes the Windows installer architecture after Defender ML false positives were observed on earlier unsigned builds.

- No silent/background installer mode.
- No installer self-copy/self-replacement.
- AUTO mode is notification-only until the user presses Download.
- Installer should be extracted to `C:\vek` before use.
- Git remains user-initiated and progress is displayed in the GUI.
- No obfuscation, packing, Defender exclusions, or antivirus bypass behavior.
