# v10 - External VEK Language Repository

- Split VEK into its own independent C++20 repository/project.
- Removed the game's private compiled copy of `VekScriptEngine.cpp/.h`.
- Game now consumes the `VEK::Runtime` CMake target.
- Added GitHub FetchContent support through `VEK_GITHUB_REPO.txt` and `VEK_GITHUB_TAG.txt`.
- Added bundled `external/vek` fallback so the project builds before a GitHub URL is configured.
- VEK security/signing remains game-side and continues to protect `.vek` gameplay scripts.
- VEK 1.0 adds assignment, `while`, `break`, `continue`, modulo, `nil`, CLI, REPL and a standard library.
