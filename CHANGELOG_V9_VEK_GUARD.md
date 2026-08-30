# v9 - VEK Guard

- Added signed VEK release-script verification (ECDSA P-256 + SHA-256 on Windows).
- Added secure-release vs development build modes.
- Disabled F11 hot reload in secure-release builds.
- Added VEK VM source/token/function/string/argument limits.
- Added instruction, native-call and recursive call-depth budgets.
- Added sealable native capability registry.
- Collision VM now fails closed to safe C++ rules after script/runtime/integrity failure.
- Added non-finite output rejection and C++ response clamps.
- Added GuardedInt for money, XP and reputation.
- Added authenticated `.vsave` storage and Windows DPAPI-protected local save key.
- Added VEK signing/key-generation developer tools.
- Added VEK Guard and GuardedValue automated tests.
- No kernel anti-cheat, process scanning, or debugger killing was added.
