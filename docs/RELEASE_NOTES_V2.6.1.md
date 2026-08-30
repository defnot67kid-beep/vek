# VEK 2.6.1

## Security / authority hardening
- Authenticated sessions can be bound to one actor, rejecting identity swapping on the same session.
- Authority actions can require sealed capability manifests before validation.
- Replay nonce retention is now configurable per action with hard bounds (32..4096).
- Added `DeveloperFeatureGate`: developer features require Development tier + local session + host-authenticated developer + signed tooling + sealed feature capability.
- Existing deterministic payload, sequence, nonce, token-bucket and audit checks remain.

These checks are deterministic policy gates, not heuristic anti-cheat bans, which reduces false positives. Hosts remain responsible for real authentication, secure transport, key management and server-side state ownership.

## Physics Definitions v0.2
Adds a broad engine-neutral physics descriptor API and bounded VEK definition registry. No advanced solver is auto-enabled.
