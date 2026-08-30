# v6.2 Camera Input Fix

This patch fixes the v6.1 camera controls that could fail when the old `mouseCaptured` flag or keyboard layout prevented input.

## Fixed RMB hold
- Right mouse press explicitly starts a camera-drag session.
- The cursor is captured only during the RMB drag.
- Mouse delta is read only while RMB is held.
- Releasing RMB ends the drag and restores the cursor position.
- Opening map/build/avatar UI safely cancels any active camera drag.
- RMB camera drag works independently from the optional TAB mouse-lock setting.

## Fixed < and >
Angle-bracket alignment now accepts:
- Physical comma / period keys (`KEY_COMMA`, `KEY_PERIOD`)
- Printable `<` and `>` characters through `GetCharPressed()` for keyboard-layout safety
- `[` and `]` as fallback alignment keys

Each press aligns the camera target to the next 45-degree slot and uses the existing smooth camera acceleration/deceleration.
