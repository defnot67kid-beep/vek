# CustomVehicleGame v26.2 - Windows CMake quoting fix

Fixes a Windows batch/CMake parsing failure when the extracted source folder contains spaces or names such as `(1)`.

Changes:
- Normalizes `%~dp0` through `for %%I in ("%~dp0.") do set "SRC_DIR=%%~fI"`.
- Removes the trailing source-directory backslash before passing the path to CMake.
- Quotes each CMake `-D` definition as its own argument.
- Keeps the short `%LOCALAPPDATA%\VEK\builds\...` MSBuild path introduced in v26.1.
- Applies the same fix to development and secure-release build scripts.
