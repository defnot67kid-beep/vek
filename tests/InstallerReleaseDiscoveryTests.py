from pathlib import Path

root = Path(__file__).resolve().parents[1]
s = (root / 'tools/github-installer/main.go').read_text(encoding='utf-8')
checks = {
    'GitHub latest release API': 'https://api.github.com/repos/' in s and '/releases/latest' in s,
    'release-version metadata optional': 'Primary path: inspect GitHub\'s actual latest published Release' in s,
    'public redirect fallback': 'func latestReleaseFromRedirect' in s,
    'installer bundle runtime fallback': 'meta.WindowsAsset = meta.InstallerAsset' in s,
    'GitHub asset digest support': 'normalizeSHA256Digest(a.Digest)' in s,
    'sidecar checksum optional': 'if err := downloadHTTPS(base+asset+".sha256"' in s,
    'package VERSION verification': 'release package does not contain VERSION=%s and vek.exe' in s,
}
failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(('PASS' if ok else 'FAIL') + ' - ' + name)
if failed:
    raise SystemExit(1)
print(f'{len(checks)}/{len(checks)} installer release-discovery policy checks passed')
