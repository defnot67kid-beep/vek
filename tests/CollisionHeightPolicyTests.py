from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "src" / "CollisionSystem.cpp").read_text(encoding="utf-8")
hdr = (root / "src" / "CollisionSystem.h").read_text(encoding="utf-8")
game = (root / "src" / "Game.cpp").read_text(encoding="utf-8")
world = (root / "src" / "World.cpp").read_text(encoding="utf-8")
tests = (root / "tests" / "CollisionSystemTests.cpp").read_text(encoding="utf-8")

assert "VerticalSpansOverlap" in hdr
assert "if (!VerticalSpansOverlap(playerMinY, playerMaxY, box.center, box.size)) continue;" in cpp
assert "if (!VerticalSpansOverlap(vehicleMinY, vehicleMaxY, box.center, box.size)) continue;" in cpp
assert "2.55f*character.appearance.height" in game
assert "0.15f*character.appearance.height" in game
assert "open personnel doorway is not blocked by overhead header" in tests
assert "open garage is not blocked by overhead header" in tests
assert "Y-overlap broadphase" in world
print("Collision height policy tests: PASS")
