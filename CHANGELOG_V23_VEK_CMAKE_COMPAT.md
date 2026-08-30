# v23 - VEK GitHub CMake compatibility

- Fixed GitHub VEK dependency integration when the fetched repository does not export `VEK::Runtime`.
- Accepts the concrete `vek_runtime` target and creates the stable alias automatically.
- Adds a source-layout fallback that builds the five standard VEK runtime source files directly when necessary.
- Keeps the official repository at `https://github.com/defnot67kid-beep/vek.git` on `main`.
