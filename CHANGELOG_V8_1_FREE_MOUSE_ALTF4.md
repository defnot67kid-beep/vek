# v8.1 - True Free Mouse + Alt+F4 Escape Flow

## Fixed
- Added `WindowInputSystem` to own desktop cursor/window behavior.
- Added a separate Win32 message hook implementation so Windows headers never
  collide with raylib API names.
- Cursor is explicitly enabled/shown through raylib.
- Windows `ClipCursor` is cleared every frame.
- Windows mouse capture is released every frame.
- Mouse movement remains completely disconnected from gameplay camera rotation.

## Alt+F4
- First fresh Alt+F4 opens the Escape Menu instead of immediately closing.
- Second fresh Alt+F4 while the menu is already open requests Leave.
- Alt+F4 key-repeat is swallowed, so holding the keys cannot immediately quit.
- Normal title-bar X close behavior remains available.

## Existing controls preserved
- ESC toggles Escape Menu.
- Physical Left/Right Arrow keys align camera.
- Map, Build Mode and Character Creator keep their mouse UI input.
