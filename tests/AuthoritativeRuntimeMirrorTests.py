from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8', errors='replace')
sync = (ROOT / 'build_support' / 'Sync-IncrementalSource.ps1').read_text(encoding='utf-8', errors='replace')
publish = (ROOT / 'build_support' / 'Publish-IncrementalOutput.ps1').read_text(encoding='utf-8', errors='replace')
release = (ROOT / 'BUILD_WINDOWS.bat').read_text(encoding='utf-8', errors='replace')
dev = (ROOT / 'BUILD_WINDOWS_DEV.bat').read_text(encoding='utf-8', errors='replace')

# Cached runtime scripts/assets must be replacement mirrors, never merge-only.
assert 'COMMAND ${CMAKE_COMMAND} -E rm -rf' in cmake
assert '"$<TARGET_FILE_DIR:CustomVehicleGame>/scripts"' in cmake
assert '"$<TARGET_FILE_DIR:CustomVehicleGame>/assets"' in cmake
assert cmake.index('-E rm -rf') < cmake.index('-E copy_directory')

# Source mirror still deletes files removed from the new ZIP and ignores editor leftovers.
assert 'Remove-Item -LiteralPath $file.FullName -Force' in sync
for extension in ("'.tmp'", "'.bak'", "'.old'", "'.orig'", "'.rej'"):
    assert extension in sync
assert "EndsWith('~')" in sync

# Local output gets an ownership manifest and first-run legacy stale-tree purge.
assert '.vek-build-output-manifest.txt' in publish
assert "@('scripts', 'assets')" in publish
assert 'Remove-Item -LiteralPath $runtimePath -Recurse -Force' in publish
assert "crashlogs.txt" in publish
assert "saves" in publish
assert 'removed stale published file' in publish

for text in (release, dev):
    assert 'Publish-IncrementalOutput.ps1' in text
    assert 'xcopy "%SMART_BUILD%\\Release\\*"' not in text
    assert '[4/4] Publishing an authoritative runtime copy' in text

# Old accidental editor backup must not ship anymore.
assert not (ROOT / 'src' / 'Save.cpp.tmp').exists()

print('AuthoritativeRuntimeMirrorTests: PASS')
