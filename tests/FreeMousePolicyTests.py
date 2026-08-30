from pathlib import Path
root = Path(__file__).resolve().parents[1]
src = "\n".join(p.read_text(encoding="utf-8", errors="ignore") for p in (root/"src").glob("*.cpp"))
# VEK 1.8 permits RMB-delta look, but the game still must never capture, warp or
# hide the normal desktop pointer.
for token in ["DisableCursor(", "SetMousePosition(", "ClipCursor(", "SetCapture("]:
    assert token not in src, f"source still contains forbidden mouse-control call: {token}"
game=(root/"src/Game.cpp").read_text(encoding="utf-8")
assert "IsMouseButtonDown(MOUSE_BUTTON_RIGHT)" in game
assert "GetMouseDelta()" in game
assert "AddMouseDelta(" in game
editor=(root/"src/EditorCameraSystem.cpp").read_text(encoding="utf-8")
assert "IsMouseButtonDown(MOUSE_BUTTON_RIGHT)" in editor
assert "GetMouseDelta()" in editor
window=(root/"src/WindowInputSystem.cpp").read_text(encoding="utf-8")
assert window.count("SetNormalCursorOnce();") == 1
print("Free mouse + hold-RMB look source policy tests: PASS")
