# Game v15 - VEK 1.5 Garage, Passlock and Access UI

- Replaces the open central hangar gap with a segmented overhead garage door.
- Garage door rolls upward on VEK-defined timing and automatically closes/locks.
- Garage collision remains solid until the opening is safely clear.
- Adds a 3D access keypad beside the left personnel-door area.
- Survival: approach the keypad and press E or click the keypad to open the access UI.
- Prototype Survival PIN is 2580 and is defined in signed `scripts/hangar_interactions.vek`.
- Correct PIN unlocks/opens the central garage, then the character walks to the garage, waits for clearance, walks inside and enters Vehicle Editor.
- Sandbox: B near the hangar grants access and runs the same garage-entry sequence without requiring a PIN.
- Adds a procedural UseKeypad character pose.
- Vehicle Editor camera remains clamped inside the VEK-defined engineering workspace.
- F11 DEV reload refreshes garage/passlock definitions along with existing VEK systems.
- Secure Release signs the updated interaction script.
