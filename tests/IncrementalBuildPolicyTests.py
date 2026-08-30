from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
release = (ROOT / 'BUILD_WINDOWS.bat').read_text(encoding='utf-8', errors='replace')
dev = (ROOT / 'BUILD_WINDOWS_DEV.bat').read_text(encoding='utf-8', errors='replace')
cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8', errors='replace')
sync = (ROOT / 'build_support' / 'Sync-IncrementalSource.ps1').read_text(encoding='utf-8', errors='replace')
reset = (ROOT / 'RESET_INCREMENTAL_BUILD_CACHE.bat').read_text(encoding='utf-8', errors='replace')

for text in (release, dev):
    assert r'VEK\incremental\CustomVehicleGame' in text
    assert 'Sync-IncrementalSource.ps1' in text
    assert '%RANDOM%' not in text
    assert 'cmake --build' in text

assert r'%CACHE_ROOT%\release' in release
assert r'%CACHE_ROOT%\dev' in dev
assert r'%CACHE_ROOT%\source' in release and r'%CACHE_ROOT%\source' in dev

assert 'Get-FileHash' in sync and 'SHA256' in sync
assert 'SetLastWriteTimeUtc' in sync
assert "'.git', '.vs', 'build', 'build-dev'" in sync
assert 'Remove-Item' in sync

assert 'add_custom_target(CustomVehicleGameRuntimeFiles ALL' in cmake
assert 'add_dependencies(CustomVehicleGameRuntimeFiles CustomVehicleGame)' in cmake
assert 'copy_directory' in cmake
assert '-E rm -rf' in cmake
assert 'Publish-IncrementalOutput.ps1' in release and 'Publish-IncrementalOutput.ps1' in dev

assert 'incremental\\CustomVehicleGame' in reset
print('IncrementalBuildPolicyTests: PASS')
