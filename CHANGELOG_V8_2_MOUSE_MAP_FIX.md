# v8.2 — Mouse Freedom + E/W Map Correction

## Mouse
v8.1 repeatedly manipulated cursor/capture state every frame. v8.2 removes that
policy completely. The game selects normal cursor mode once at startup and then
leaves the OS pointer alone.

There are no gameplay/native calls to DisableCursor, SetMousePosition,
GetMouseDelta, ClipCursor, ReleaseCapture or SetCapture. Mouse movement does not
rotate the camera. Physical Left/Right Arrow keys remain the camera alignment
controls.

Alt+F4 interception remains, but its Windows hook only handles the close-key
message and does not touch mouse capture.

## Map
The map's horizontal axis is now consistently mirrored to the requested compass
convention: N top, S bottom, E left, W right. This applies to map geometry,
roads, GPS, waypoints, cursor-to-world conversion and player/vehicle heading,
not merely the compass letters.
