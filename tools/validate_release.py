#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, os, sys, zipfile
from pathlib import Path


def fail(msg: str) -> None:
    print(f"[VEK release validation] ERROR: {msg}", file=sys.stderr)
    raise SystemExit(1)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def validate_tree(root: Path, expected_version: str) -> None:
    required_files = [
        'vek.exe', 'VekInstaller.exe', 'VERSION', 'README.md', 'LICENSE',
        'REPOSITORY.txt', 'manifest.sha256', 'include/vek/VekVersion.h',
        'lib/cmake/VEK/VEKConfig.cmake', 'lib/cmake/VEK/VEKConfigVersion.cmake',
        'lib/cmake/VEK/VEKTargets.cmake',
    ]
    for rel in required_files:
        if not (root / rel).is_file():
            fail(f"missing required runtime/SDK file: {rel}")
    version = (root/'VERSION').read_text(encoding='utf-8-sig').strip()
    if version != expected_version:
        fail(f"VERSION is {version!r}, expected {expected_version!r}")
    libs = list((root/'lib').glob('*.lib')) + list((root/'lib').glob('*.a'))
    if not libs:
        fail('lib directory contains no compiled VEK runtime library')
    # Detect the Go bootstrap by size/name layout indirectly: production package
    # must always contain the compiled SDK library and generated header above.
    print(f"[VEK release validation] runtime tree OK: v{version}")


def validate_zip(path: Path, expected_version: str) -> None:
    if not path.is_file():
        fail(f"zip not found: {path}")
    with zipfile.ZipFile(path) as z:
        names = [n.replace('\\','/') for n in z.namelist()]
        roots = sorted({n.split('/',1)[0] for n in names if '/' in n and n.split('/',1)[0]})
        prefix = ''
        if len(roots) == 1 and any(n.startswith(roots[0] + '/') for n in names):
            prefix = roots[0] + '/'
        needed = [
            'vek.exe','VekInstaller.exe','VERSION','include/vek/VekVersion.h',
            'lib/cmake/VEK/VEKConfig.cmake','lib/cmake/VEK/VEKTargets.cmake'
        ]
        for rel in needed:
            if prefix + rel not in names:
                fail(f"archive missing {rel}")
        raw = z.read(prefix+'VERSION').decode('utf-8-sig').strip()
        if raw != expected_version:
            fail(f"archive VERSION={raw!r}, expected {expected_version!r}")
        has_lib = any((n.startswith(prefix+'lib/') and (n.lower().endswith('.lib') or n.lower().endswith('.a'))) for n in names)
        if not has_lib:
            fail('archive contains no compiled runtime library under lib/')
    print(f"[VEK release validation] archive OK: {path.name}")


def main() -> None:
    ap=argparse.ArgumentParser()
    ap.add_argument('--package', type=Path)
    ap.add_argument('--zip', dest='zip_path', type=Path)
    ap.add_argument('--version', required=True)
    args=ap.parse_args()
    if args.package: validate_tree(args.package, args.version)
    if args.zip_path: validate_zip(args.zip_path, args.version)
    if not args.package and not args.zip_path: fail('nothing to validate')

if __name__ == '__main__': main()
