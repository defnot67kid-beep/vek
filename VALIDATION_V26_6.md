# Validation - v26.6 / VEK 2.6.1

PASS - VEK runtime builds with GCC 14 / C++20.
PASS - VEK CLI reports 2.6.1.
PASS - 9/9 VEK CTest suites, including authority/security and Physics Definitions v0.2.
PASS - VekAuthorityRules.cpp compiles against VEK 2.6.1.
PASS - Arm rig orientation regression checks.
PASS - Free-mouse input policy checks.
PASS - Hair scalp fit and all five new hair-style regression checks.
PASS - Game.cpp preprocessor directive balance.
PASS - original signed hangar_interactions.vek and security_authority.vek hashes remain unchanged.
PASS - VekInstaller.exe rebuilt as Windows GUI subsystem; vek.exe remains Windows console subsystem.
PASS - installer SHA-256 manifest regenerated.

LIMITATION - full graphical game CMake configure could not run in this sandbox because FetchContent could not resolve github.com to download Raylib 6.0. The failure occurs during Raylib dependency download before game source compilation.
