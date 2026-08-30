# v8 Gameplay Input Note

**Mouse camera rotation is disabled in the game.** The cursor remains free. `CameraRotationSystem` is still the reusable smoothing/angle module, but `Game` only feeds it physical Left/Right Arrow alignment during gameplay. `AddMouseDelta()` remains in the engine API for possible future editor/tools use and is not called by gameplay.

# Smooth Camera Rotation System

This upgrade adds a dedicated `CameraRotationSystem` without replacing the existing character, animation, vehicle, job, build, customization, equipment, clothing or save systems.

## What changed

The camera still uses target/current yaw smoothing. In v8 gameplay, physical Left/Right Arrow alignment changes that target. The mouse does not feed camera rotation:

1. Mouse delta changes `targetYaw` and `targetPitch`.
2. Camera angular velocity accelerates toward the required turn velocity.
3. Braking-limited rotation slows near the target.
4. Yaw uses shortest-angle wrapping, including 359° -> 0°.
5. Pitch is clamped to the camera mode's anatomical/view limits.

The normal on-foot camera, free inspection camera and cockpit look use separate instances so their state does not leak into one another.

## Default settings

| Setting | Value |
|---|---:|
| Yaw sensitivity | 0.12 |
| Pitch sensitivity | 0.10 |
| Max yaw speed | 720 deg/s |
| Max pitch speed | 540 deg/s |
| Yaw acceleration | 3200 deg/s² |
| Yaw deceleration | 4200 deg/s² |
| Pitch acceleration | 2600 deg/s² |
| Pitch deceleration | 3400 deg/s² |
| On-foot pitch limits | -25° to +65° |
| Free-camera pitch limits | -80° to +80° |
| Cockpit yaw limits | -90° to +90° |
| Cockpit pitch limits | -45° to +55° |

These are public values in `CameraRotationSettings`.

## A/D correction

The old build had inconsistent horizontal orientation in the raylib coordinate convention. The A/D mapping is now inverted from that build in:

- on-foot camera-relative movement
- vehicle steering
- free-camera strafing

This makes the visible left/right result consistent rather than having one mode feel opposite to another.

## Debugging

Press `F8`. The rotation panel now includes:

- body yaw and target
- camera/view yaw
- camera current yaw/pitch
- camera target yaw/pitch
- camera yaw/pitch velocity

The existing 3D direction lines remain:

- green: body forward
- yellow: desired movement
- blue: camera forward

## Verification

Automated source tests cover non-snapping camera motion, 359°/0° shortest yaw, pitch/yaw clamps and 30/60/120/144 FPS consistency.

## Input controls

Camera rotation is now deliberate rather than always following raw mouse movement:

- **Left Arrow / Right Arrow**: move the target yaw to the next 45-degree alignment slot.
- **Mouse movement / RMB**: no gameplay camera rotation; the cursor remains free.
- **Comma / `<`**: align one 45-degree world-angle step to the left.
- **Period / `>`**: align one 45-degree world-angle step to the right.

The alignment keys change the **target yaw**, not the visible camera yaw, so the same acceleration/deceleration system still produces a smooth movement instead of an instant snap. Repeated key presses can queue clean 45-degree brackets while the current camera is still easing.

On-foot, vehicle, cockpit and free-inspection cameras use arrow-key target alignment. Full World Map right-mouse dragging remains owned by the map because that is UI panning/waypoint input, not gameplay camera look.


## v6.3 input binding

Normal gameplay does not capture, hide, re-centre or lock the cursor. Camera alignment is keyboard-driven.

Alignment is bound only to the physical Left Arrow and Right Arrow keys. The earlier `<` / `>` interpretation was incorrect. The directions remain intentionally inverted at the user's request: Left Arrow maps to positive-yaw alignment and Right Arrow maps to negative-yaw alignment. Comma, period, `<`, `>`, `[` and `]` are not alignment controls.


## v6.4 physical arrow-key clarification
Camera alignment is now bound ONLY to the physical keyboard Left Arrow and Right Arrow keys. The earlier `<` / `>` interpretation was incorrect. Comma, period, Shift+comma, Shift+period, `<`, `>`, `[` and `]` do not trigger camera alignment. The requested inverted mapping is preserved: Left Arrow uses +yaw alignment and Right Arrow uses -yaw alignment. The old RMB hold-to-orbit behavior is removed in v8.
