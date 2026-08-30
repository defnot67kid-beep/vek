# v6.4 Camera Alignment Clarification

- Corrected the alignment-key interpretation: the user meant the physical Left Arrow / Right Arrow keys, not `<` and `>` characters.
- Removed all gameplay alignment handling for comma, period, Shift+comma, Shift+period, `<`, `>`, `[` and `]`.
- Left Arrow and Right Arrow are the only gameplay camera alignment bindings.
- Preserved the requested inverted mapping: Left Arrow -> +yaw 45-degree slot, Right Arrow -> -yaw 45-degree slot.
- Works in normal third-person, close third-person, cockpit/first-person driving, third-person vehicle orbit, and free-inspection camera.
- Full World Map and Character Creator keep ownership of their own arrow-key controls, so camera alignment does not interfere there.
- RMB hold-to-orbit keeps the v6.3 recentered-drag implementation.
- F8 debug now reports physical LEFT/RIGHT arrow input.
