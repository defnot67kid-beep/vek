# v6.3 camera input fix

- Replaced RMB `DisableCursor()+GetMouseDelta()` camera look with explicit centre-warp drag input.
- RMB now reads cursor displacement from the centre of the current window, applies it, then re-centres every frame.
- Removed `[` and `]` camera alignment fallbacks completely.
- Bare comma and period no longer trigger alignment.
- `<` is detected as Shift+Comma or printable `<`; `>` as Shift+Period or printable `>`.
- Inverted the requested angle-bracket mapping: `<` uses +45-degree alignment direction, `>` uses -45-degree direction.
- Applied the same controls to on-foot, vehicle third-person, cockpit, and free-inspection camera modes.
- F8 debug now reports RMB drag plus actual Shift+comma / Shift+period raw states.
