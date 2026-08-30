CUSTOM VEHICLE GAME v26 - VEK 2.6 ANIMATED DOOR + CHARACTER UPDATE + DAY/NIGHT + DEV CHEAT PANEL

WINDOWS SETUP
1. Extract this ZIP to a normal writable folder.
2. Double-click BUILD_WINDOWS.bat for the secure release build, or BUILD_WINDOWS_DEV.bat while actively editing VEK scripts.
3. CMake downloads raylib 6.0, then checks C:\vek for a complete VEK 2.7.1+ runtime.
4. If C:\vek is official, clean, and exactly current with https://github.com/defnot67kid-beep/vek.git @ main, the build reuses it. Otherwise CMake clones a clean current VEK copy automatically.

GARAGE / BUILD MODE
- Press B near the main garage to request/open it when access policy permits.
- In Survival, a locked garage still respects the physical VEK passlock rather than bypassing security.
- Once the garage is open far enough, its collision gate is removed and world collision geometry refreshes, so you can walk through normally.
- Crossing the garage threshold no longer throws you automatically into Build Mode.
- Once you are inside and away from the garage opening, press B to enter Build Mode.
- Press B again in Build Mode to exit it.

CHARACTER RIG
- Pelvis, lower spine and upper chest now use tapered procedural body segments rather than large rectangular torso cubes.
- Spine/ragdoll sections remain independently posed.
- Hair styles use articulated multi-joint strand chains with turn lag, movement lag, gait bounce, jump response and speed turbulence.
- Arm pitch now follows the character forward axis: elbows flex forward, running arms counter-swing the opposite leg, driving hands reach toward the wheel, and raised poses no longer fold behind the body.

HANGAR
- The previously oversized empty vertical space now contains trusses, service gantries, rails, a rear catwalk/control band, storage silhouettes, ducts and lower work lights.
- The centre lane stays open for large vehicle/aircraft construction.

VEK 2.6 / SECURITY + PHYSICS
- scripts/security_authority.vek defines deterministic server-authority policy for shared gameplay actions.
- VEK 2.0 supports authenticated-session requirements, capability checks, bounded payload shape/size, replay nonces, monotonic sequences and token-bucket rate limiting.
- The policy deliberately avoids heuristic auto-bans; hard rejection happens only when a concrete rule fails.
- Secure builds accept only signed scripts whose exact SHA-256 digest is also pinned into src/generated/VekPublicKey.h.
- The private signing key is intentionally NOT included in the release ZIP.
- If you intentionally edit any .vek source, run SIGN_VEK_SCRIPTS.bat before rebuilding secure release. That creates/rotates a LOCAL private key under developer_keys/. Never distribute that folder.
- For future source releases, tools/package_secure_source.py creates a ZIP that audits VEK signatures/manifests and excludes private keys/build caches.

CAMERA / INPUT
- Mouse remains a normal desktop pointer except while a camera/editor action explicitly owns it.
- Hold RIGHT MOUSE BUTTON to look around.
- F toggles the compact FPS/performance/security HUD outside Build Mode.

SECURITY NOTE
No client-owned game can truthfully be guaranteed unhackable. For multiplayer, keep money, inventory, rewards, health/damage and important vehicle state server-authoritative. Native networking/authentication must derive actor/session identity from the trusted connection; never trust identity/authenticated flags sent by a client packet.

VEK 2.6 AUTO RESOLUTION
-----------------------
The build checks C:\vek first. A local VEK installation is only reused when its
managed runtime source is official, clean, version 2.7.1+, and exactly current
with https://github.com/defnot67kid-beep/vek.git main. Otherwise the build clones
a clean current VEK copy automatically.

============================================================
SMART INCREMENTAL BUILDS (v26.7+)
============================================================
BUILD_WINDOWS.bat and BUILD_WINDOWS_DEV.bat now reuse a persistent build cache
under %%LOCALAPPDATA%%\VEK\incremental\CustomVehicleGame.

The first v26.7 smart build is a normal full build. After that, you can download
and unzip newer source versions into completely different folder names and run
the same build BAT. The script hashes/synchronizes changed source into one stable
canonical source path, then CMake/MSBuild recompiles only invalidated files.

Dev and release caches are separate. Editing only a .vek script refreshes runtime
scripts/assets without forcing unrelated C++ files to rebuild.

Do NOT delete the smart cache between normal updates. If it becomes genuinely
corrupted, run RESET_INCREMENTAL_BUILD_CACHE.bat; the next build will be full once.

============================================================
CRASH LOGGING (v26.8.2+)
============================================================
The game creates crashlogs.txt beside CustomVehicleGame.exe. If the game exits
through an unhandled C++ exception, std::terminate, or a Windows structured
exception such as an access violation, the logger appends a diagnostic report.

Reports include the UTC time, game build, VEK runtime, last known game stage,
error/exception information and Windows crash addresses when available. Keep
crashlogs.txt after a crash and attach it when reporting the problem.

Normal clean exits do not append fake crash entries.

DEV DOOR / GARAGE COLLISION FIX (v26.8.4+)
------------------------------------------
Collision now checks vertical overlap before applying horizontal world contacts.
This fixes overhead personnel-door headers / garage lintels behaving like invisible
floor-to-ceiling walls. You should be able to enter with normal collision enabled;
DEV noclip is no longer required.

SMART CACHE STALE-FILE FIX (v26.8.3+)
-------------------------------------
The smart incremental system now treats scripts/ and assets/ as authoritative
mirrors instead of merge-copy folders. Deleted or renamed .vek files from older
ZIPs are automatically removed from both the persistent build output and your
local build/Release or build-dev/Release folder.

Your expensive C++ object/library cache is still reused. Only the small runtime
scripts/assets trees are refreshed exactly each build.

The local publisher preserves:
  - saves/
  - crashlogs.txt

Do NOT manually copy old scripts back into build-dev/Release/scripts. Edit the
current source scripts folder instead and run BUILD_WINDOWS_DEV.bat again.
