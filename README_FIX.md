# VEK GitHub CMake Runtime Target Fix

Extract these files into the root of `defnot67kid-beep/vek` and replace the existing `CMakeLists.txt`.

The CMake project now provides:

- concrete target `vek_runtime`
- stable alias `VEK::Runtime`
- optional `VEK::CABI`
- CLI/test/installer switches understood by embedding projects

This is required by CustomVehicleGame's `FetchContent` integration.
