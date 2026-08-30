# Validation - CustomVehicleGame v26.8.3 / VEK 2.6.1+

## Smart-cache stale runtime fix

- Confirmed the canonical incremental source synchronizer removes files that no
  longer exist in the newly downloaded source tree.
- Confirmed temporary/editor backup files are excluded from the canonical mirror.
- Confirmed cached executable runtime `scripts/` and `assets/` directories are
  removed before the current source trees are copied.
- Smoke-tested authoritative runtime mirroring by seeding deleted legacy files;
  the seeded stale `.vek` and asset files were removed.
- Replaced raw local `xcopy` publishing with an ownership-manifest publisher.
- First v26.8.3 publish purges legacy `scripts/`/`assets/` left by v26.7-v26.8.2.
- Local publisher preserves `saves/` and `crashlogs.txt`.
- Removed accidental `src/Save.cpp.tmp` editor backup from the shipped source.

## Regression checks

Python source-policy/regression suites passed:

- ArmRigOrientationTests.py
- AuthoritativeRuntimeMirrorTests.py
- CrashLogPolicyTests.py
- DoorWorkshopEntryTests.py
- FreeMousePolicyTests.py
- HairScalpFitTests.py
- IncrementalBuildPolicyTests.py

All 43 files under `scripts/` are byte-for-byte identical to v26.8.2, including
all `.sig` files.

The expensive CMake/MSBuild compiled-object cache remains incremental; only the
small dynamic runtime data trees are exact-refreshed every build.
