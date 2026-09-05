#pragma once
// VekStdLibExtras — additive VEK-script native functions (VEK 3.5).
//
// VekRegisterStandardLibrary() stays the minimal, security-reviewed core.
// This module is an opt-in extra pack a host can register alongside it to
// give scripts direct access to the new physics mechanics, easing/animation
// math, procedural style tokens, and misc utilities — without growing the
// core stdlib's attack surface for hosts that don't want it.

#include <vek/VekScriptEngine.h>

namespace vek {

// Registers: raycast_sphere, raycast_aabb, sphere_sphere_resolve,
// projectile_step, predict_landing, ease(name,t), lerp, inverse_lerp,
// remap, rand_range, rand_seed, palette_color, token_scale.
void VekRegisterExtraStandardLibrary(VekScriptEngine& engine);

} // namespace vek
