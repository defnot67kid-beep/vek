# VEK Guard + VEK 2.0 Security Architecture

VEK Guard is the game's defensive host layer around the custom VEK runtime. VEK 2.0 adds server-authority primitives so shared gameplay can be validated by a dedicated/listen server without relying on client honesty.

## Security boundary

A client executable controlled by a player cannot be made mathematically unhackable. A determined reverse engineer can patch local machine code or memory. For multiplayer, the real trust boundary is the authoritative server: money, inventory, job rewards, health/damage, unlocks and important vehicle state must be committed by the server.

The design therefore uses layered, deterministic checks rather than invasive process scanning or heuristic auto-bans.

## 1. Signed + pinned VEK release scripts

Normal `BUILD_WINDOWS.bat` builds with `VEK_DEVELOPMENT_MODE=OFF`.

Every shipped `.vek` file has a matching `.vek.sig` using ECDSA P-256 + SHA-256 on Windows. In addition, the exact SHA-256 of every shipped VEK script is compiled into `src/generated/VekPublicKey.h`. Secure release refuses a script that is missing from this allow-list or whose bytes do not match it.

This protects against simple script replacement and also prevents a newly signed replacement from being accepted by an already-built EXE if a signing key were later exposed.

The game host also rejects parent-path traversal, mismatched signature paths and script/signature symlink substitution.

### Private key rule

**The private signing key is intentionally not shipped in this source package.** That fixes a major security mistake in older prototype packages, which contained a `DO_NOT_SHIP` PEM beside the game source.

When you intentionally edit VEK source for a new build, run:

`SIGN_VEK_SCRIPTS.bat`

That script rotates/generates a local development signing identity, signs every VEK script and rebuilds the compiled hash manifest. Keep `developer_keys/` private and never upload/distribute it.

For a source release, use `tools/package_secure_source.py`; it excludes private-key/build material and checks the VEK manifest/signatures before writing the ZIP.

## 2. VEK 2.0 deterministic server authority

For actions marked as client-requestable, an authoritative host can require all of the following before accepting a request:

- known action ID / explicit allow-list
- authoritative server host role
- client-request permission for that action
- safe actor identity format
- authenticated session requirement + safe session ID format
- capability permission
- payload depth, item-count, string-size and total-byte limits
- rejection of NaN/infinity and cyclic containers
- nonce length/format validation
- monotonic sequence validation
- replay nonce rejection
- burst-tolerant token-bucket rate limit
- bounded authority state and bounded audit history

VEK does **not** auto-ban from fuzzy anomaly scores. Telemetry may be logged, but rejection is based on a concrete rule. This reduces false positives while still failing closed on malformed or unauthorized requests.

**Important host rule:** `actorId`, `sessionId` and the `authenticated` flag must be derived by the native networking/authentication layer from the connection/session. Never copy those values from untrusted packet fields and call them authenticated.

## 3. Capability sealing + duplicate-registration defense

VEK has no automatic filesystem, process, network, memory, DLL, shell or OS access. It can call only native functions explicitly registered by the host.

The native registry can be sealed. VEK 2.0 also rejects duplicate native names instead of silently replacing an existing native. Authority action IDs, replication schema IDs and replication field names are likewise unique.

## 4. VM resource sandbox

VEK security policies bound source size, token/function/parameter counts, string sizes, call depth, instruction budget, native calls, loop iterations, container size, module count and import depth. Hardened-server defaults are tighter than development/client defaults.

Module imports are constrained to configured roots, and absolute/drive-path escape attempts are rejected.

## 5. Fail-closed native validation

Gameplay values returned by VEK are validated/clamped again by native host code where appropriate. If a VEK rule cannot load or violates a security budget, the host can fall back to conservative native rules rather than blindly trusting malformed script output.

## 6. Local tamper resistance

Offline/local values use guarded storage and authenticated `.vsave` files. On Windows, the local save-integrity key uses DPAPI for the current Windows user. This raises the cost of casual save edits and simple value scans, but it is not a replacement for server authority in multiplayer.

## What VEK Guard deliberately does not do

It does not install kernel drivers, scan unrelated processes, kill debuggers, hide from the OS, or claim an offline executable is impossible to patch. Those methods add brittleness and false-positive risk without creating a true trust boundary.

## v26.6 developer-feature gate

Development cheats are now double gated. The native cheat paths are compiled only when `VEK_DEVELOPMENT_MODE=1`, and VEK 2.6.1 `DeveloperFeatureGate` must also allow the individual capability (`dev.fly`, `dev.noclip`, `dev.god_mode`, etc.). The local passlock code only opens the panel in a development build. Hardened/release builds never compile the bypass branches and the VEK developer gate remains empty/sealed.

For multiplayer hosts, VEK 2.6.1 also supports sealed capability manifests, authenticated session-to-actor binding, bounded replay nonce history, sequence validation, token-bucket rate limits, bounded payload scans and security audit decisions. Real identity authentication and secure transport remain the host/server's responsibility.
