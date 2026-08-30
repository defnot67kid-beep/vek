#!/usr/bin/env python3
"""Create a source ZIP while refusing to leak VEK private signing material."""
from __future__ import annotations
from pathlib import Path
import hashlib
import re
import sys
import zipfile

root = Path(__file__).resolve().parents[1]
out = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root.parent / f"{root.name}_SECURE_SOURCE.zip"

excluded_dir_names = {"developer_keys", "build", "build_check", ".git", ".vs", "__pycache__"}
excluded_suffixes = {".pem", ".key", ".p12", ".pfx"}

header = root / "src" / "generated" / "VekPublicKey.h"
if not header.exists():
    raise SystemExit("Release audit failed: src/generated/VekPublicKey.h is missing")
header_text = header.read_text(encoding="utf-8")

scripts = sorted((root / "scripts").rglob("*.vek"))
if not scripts:
    raise SystemExit("Release audit failed: no VEK scripts found")

for script in scripts:
    rel = script.relative_to(root).as_posix()
    sig = Path(str(script) + ".sig")
    if not sig.exists():
        raise SystemExit(f"Release audit failed: missing signature for {rel}")
    sig_text = sig.read_text(encoding="ascii").strip()
    if not re.fullmatch(r"[0-9a-fA-F]{128}", sig_text):
        raise SystemExit(f"Release audit failed: malformed signature for {rel}")
    digest = hashlib.sha256(script.read_bytes()).digest()
    expected = ", ".join(f"0x{b:02x}" for b in digest)
    marker = f'ScriptHashEntry{{"{rel}", {{{expected}}}}}'
    if marker not in header_text:
        raise SystemExit(f"Release audit failed: compiled hash manifest is stale/missing for {rel}. Run SIGN_VEK_SCRIPTS.bat")

files: list[Path] = []
for path in root.rglob("*"):
    if not path.is_file():
        continue
    rel_parts = path.relative_to(root).parts
    if any(part in excluded_dir_names for part in rel_parts):
        continue
    if path.suffix.lower() in excluded_suffixes:
        raise SystemExit(f"Release audit failed: private-key-like file outside excluded developer_keys/: {path.relative_to(root)}")
    files.append(path)

out.parent.mkdir(parents=True, exist_ok=True)
if out.exists():
    out.unlink()
with zipfile.ZipFile(out, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for path in files:
        arc = Path(root.name) / path.relative_to(root)
        z.write(path, arc.as_posix())

print(f"Secure source package created: {out}")
print(f"Files: {len(files)} | VEK scripts audited: {len(scripts)}")
print("Private signing keys and build/cache directories were excluded.")
