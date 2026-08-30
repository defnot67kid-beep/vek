from pathlib import Path

src = (Path(__file__).resolve().parents[1] / "src" / "Character.cpp").read_text(encoding="utf-8")
checks = {
    "left arm counters left leg": "float targetLA = s * armAmp;" in src,
    "right arm counters right leg": "float targetRA = -s * armAmp;" in src,
    "elbow flexes forward": "float r2=(swing+bend)*DEG2RAD;" in src,
    "driving arms reach forward": "targetLA = 72; targetRA = 72;" in src,
    "celebration arms raise forward": "targetLA = targetRaise; targetRA = targetRaise;" in src,
}
failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(("[OK] " if ok else "[FAIL] ") + name)
if failed:
    raise SystemExit("Arm rig regression checks failed: " + ", ".join(failed))
print("Arm rig orientation regression checks passed.")
