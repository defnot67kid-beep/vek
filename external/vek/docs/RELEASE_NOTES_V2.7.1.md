# VEK 2.7.1 — Part Presentation Assets

VEK 2.7.1 adds renderer-neutral part presentation metadata for games and editors.

- `PartPresentationDefinition` adds `icon`, `view_model`, `world_model`, `material`, view scale/rotation/offset and tint policy.
- `part_presentation(icon, view_model, world_model, options)` helps VEK scripts declare presentation metadata.
- Top-level `icon`, `view_model`, `world_model`, and `material` aliases are accepted for concise part definitions.
- Legacy `visual` continues to work and is used as the fallback for missing presentation fields.
- Asset strings are host-defined logical IDs/URIs. VEK does not grant scripts direct filesystem or GPU access.
