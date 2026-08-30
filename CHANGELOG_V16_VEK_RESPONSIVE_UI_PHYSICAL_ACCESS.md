# Game v16 - VEK 1.6 Responsive UI + Physical Garage Access

## Interaction changes

- Removed the forced walk-to-garage and walk-inside character movement sequence.
- The Survival keypad no longer shows a floating proximity prompt or E-key instruction.
- The keypad is click-only and only works from the exterior side.
- Native ray tests reject clicks when a hangar wall or other collision box is between the camera and keypad.
- A correct PIN only unlocks/opens the garage; the player remains under normal control and walks through manually.
- Crossing the open central garage threshold physically activates the Vehicle Editor in Survival.
- Leaving Vehicle Editor returns normal character control inside the hangar.
- Approaching the garage from inside automatically opens it for egress when VEK policy allows it.
- The garage stays open while a player is near the doorway and automatically closes later.
- Sandbox retains quick B access without scripted character movement.

## GUI changes

- Game v16 consumes VEK 1.6 `GuiTextPolicy` and `GuiTextLayoutSystem`.
- Passlock UI now auto-fits, wraps, clips and ellipsizes text inside its rectangles.
- Vehicle-editor cards, buttons, search field and tooltips use responsive VEK text layout.
- Main-menu title/button text uses the same auto-fit system.
- Engineering analysis and build-control panels now resize for narrower windows instead of generating negative/off-screen widths.

## Security boundary

VEK defines interaction policy and GUI layout rules. Native C++ still owns ray casting, collision geometry, rendering, mouse hit testing, memory, OS integration, signature verification and cryptography.
