from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
game = (ROOT / "src" / "Game.cpp").read_text(encoding="utf-8")
builder = (ROOT / "src" / "VehicleBuilder.cpp").read_text(encoding="utf-8")

# Gameplay HUD and map must not render while the engineering builder is active.
assert 'if(!builder.active) DrawHUD();' in game
assert 'if(!builder.active) map.Draw();' in game

# The always-on five-line command-guide HUD requested for removal must be gone.
assert 'Grid %.2fm%s | Ghost %s | X symmetry %s | Z symmetry %s' not in builder
assert 'Click empty: place | Click part: select | SHIFT+click: force placement' not in builder
assert 'Ctrl+D duplicate | Delete | M mirror | P paint' not in builder
assert 'Shift+F/Enter build' not in builder

# Core engineering UI should remain present.
assert 'ENGINEERING ANALYSIS' in builder
assert 'gui->UpdateAndDraw' in builder

print('Build mode HUD policy tests passed')
