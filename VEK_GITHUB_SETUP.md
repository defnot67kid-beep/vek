# VEK dependency for CustomVehicleGame v25

Official repository:
`https://github.com/defnot67kid-beep/vek.git`

Required runtime:
`VEK 2.7.1+`

Branch used for freshness checks and clean fallback builds:
`main`

## Automatic resolution

The game build checks `C:\vek` before downloading another copy of VEK.

It reuses the installed VEK runtime only when:

- `C:\vek\vek.exe` exists;
- a complete runtime checkout exists at `C:\vek\repo` or `C:\vek`;
- the runtime `VERSION` is 2.7.1 or newer;
- its Git `origin` is the official VEK repository;
- tracked files are clean;
- local `HEAD` exactly matches the current GitHub `main` HEAD;
- the expected VEK runtime and physics source files are present.

If any check fails, CMake performs a clean shallow clone from GitHub and uses
that checkout for the build instead. The installed copy is not silently reset
or overwritten by the game build.

For reproducible public releases, pinning an immutable VEK commit SHA is
stronger than following a moving branch. The default developer build follows
`main` so it can detect whether `C:\vek` is genuinely current.


Developer cheat panel note:
- In dev mode, the garage keypad code `37485` can be used to open the cheat panel.
