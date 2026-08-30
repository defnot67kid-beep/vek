# v20 / VEK 2.0 — Security, Character Rig, Hair Rig, Hangar & Garage

## Security / anti-exploit architecture

- Bundled VEK runtime upgraded to **VEK 2.0.0**.
- Authority requests can require an authenticated session and are validated with deterministic checks: host role, action allow-list, capability, actor/session/nonce format, payload depth/item/string/byte limits, finite numbers, cyclic-container rejection, monotonic sequence numbers, replay nonce history and token-bucket rate limiting.
- Rate limiting uses a burst-tolerant token bucket instead of a rigid one-second counter, reducing false positives from normal packet clumping.
- Invalid/replayed/out-of-order requests do not consume a valid request's sequence/nonce state.
- Authority state has a hard global capacity and the security audit buffer is bounded.
- Duplicate native registrations, duplicate authority actions, duplicate replication schemas and duplicate replication fields are rejected.
- Replication field types are allow-listed.
- Standard library mutation helpers have hard container/key growth bounds.
- The game now pins the SHA-256 digest of **every shipped `.vek` file** inside the executable, in addition to ECDSA P-256 signatures on Windows.
- Script/signature path traversal and symlink substitutions are rejected by the game host.
- A new signing identity was generated for this release. The private signing key is intentionally **not included** in the distributable source ZIP.
- `tools/package_secure_source.py` packages source while refusing to ship private-key material or stale/missing VEK signatures/manifests.

The security design intentionally does not use heuristic auto-bans. It rejects requests only when a concrete server rule fails, which is safer for legitimate players and easier to audit.

## Character rig

- Removed the large rectangular torso/pelvis shell.
- Added tapered elliptical procedural body segments for pelvis, lower spine and upper chest.
- Independent spine/ragdoll pitch is preserved, so the body bends like a joint chain instead of one block.
- Overalls/safety vest use a narrow bib and straps rather than a giant torso plate.

## Hair rig

- Hairstyles no longer depend on one large cube/clump.
- Short, long, curly, straight, braids, ponytail and afro styles are built from articulated strand chains.
- Each strand has multiple procedural joints with movement lag, turn lag, gait bounce, jump response and speed turbulence.

## Hangar / warehouse

- Filled the oversized empty vertical space with roof trusses, structural columns, side service gantries, rails, a rear catwalk/control band, storage stacks, suspended ducts and lower work lights.
- The centre lane remains intentionally clear for large vehicle/aircraft construction.

## Garage interaction

- **B near the garage opens the garage** when access is permitted.
- Opening the garage no longer auto-enters Build Mode.
- Crossing the open threshold leaves normal player movement active, so the player can walk through the doorway and around the hangar.
- Once inside and away from the doorway, **B enters Build Mode**.
- Garage collision clears at 60% visual opening (VEK rule and native fallback default), then collision geometry refreshes immediately.
