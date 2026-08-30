# VEK Garage + Passlock System

Game v15 uses VEK 1.5 for reusable garage/access-control definitions.

## Survival flow

1. Walk to the keypad beside the left personnel door.
2. Press E or click the physical keypad while within range.
3. The VEK-described access modal opens.
4. Enter PIN `2580` in this prototype.
5. VEK PasslockSystem validates the code and access-attempt policy.
6. GarageDoorSystem unlocks and begins the VEK-defined garage-open animation/timing.
7. The character walks to the central garage, waits for safe clearance, walks inside, and Vehicle Editor starts.

## Sandbox flow

Press B near the hangar. Sandbox bypasses the PIN and requests garage access directly, but it still uses the same garage animation and entry sequence.

## VEK-owned configuration

`scripts/hangar_interactions.vek` defines:

- garage width/height/panel count
- open/close durations
- auto-close delay
- lock-on-close policy
- garage animation IDs
- passlock ID and numeric PIN
- maximum attempts and lockout duration
- interaction prompt
- passlock GUI command description
- character interaction clip IDs

Native C++ owns the actual raylib rendering, world collision refresh, keyboard/mouse input, camera, memory and signature verification.

## VEK 1.5 UI commands used

- `gui_begin_modal`
- `gui_password_input`
- `gui_status_badge`
- `gui_keypad`
- `gui_end_modal`

## Security note

The prototype PIN is gameplay content, not a cryptographic secret. VEK Guard prevents unsigned script modification in the Windows secure build, but an offline game cannot make a locally stored gameplay PIN secret from a determined machine owner.
