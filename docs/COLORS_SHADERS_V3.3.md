# VEK Colors and Shaders 3.3

VEK 3.3 adds safe presentation descriptors rather than granting scripts raw renderer/GPU access.

```vek
let blue = color_named("vek-blue");
let soft = color_alpha(color_mix(blue, color_named("vek-grey-900"), 0.35), 220);
let glass = shader_preset("frosted_glass");
let material_desc = material("editor.panel", glass, {tint:soft});
```

The host decides whether the requested shader/effect is supported and performs real compilation/resource creation. Descriptors are bounded by registry limits and may declare required capabilities.
