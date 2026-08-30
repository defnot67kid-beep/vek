# Validation - CustomVehicleGame v26.4

VEK automatic version resolution test:

- Simulated installed runtime: VEK 2.6.0
- Simulated official stable release: VEK 2.7.0
- Result: resolver selected v2.7.0 from the release source

Observed configure output:

```text
VEK: latest stable release is v2.7.0
VEK: newest verified local runtime is v2.6.0
VEK: local v2.6.0 is behind v2.7.0; using newer stable release
VEK: resolved v2.7.0 via github-release
```

The resolver also retains the newest verified local compatible runtime as an
offline fallback.

`C:\vek\updates` is not considered a valid runtime source.
