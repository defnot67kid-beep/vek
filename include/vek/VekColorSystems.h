#pragma once

#include <string>
#include <vek/VekScriptEngine.h>

namespace vek {

// Renderer-neutral colour type. Channels are always linearized only by the host;
// VEK stores normalized display-space RGBA values in the 0..1 range.
struct VekColor {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

VekColor ParseColorHex(const std::string& text, VekColor fallback = {});
VekColor NamedVekColor(const std::string& name, VekColor fallback = {});
VekColor MixColor(VekColor a, VekColor b, float t);
VekColor WithColorAlpha(VekColor c, float alpha);
VekColor LightenColor(VekColor c, float amount);
VekColor DarkenColor(VekColor c, float amount);
VekColor HslColor(float hueDegrees, float saturation, float lightness, float alpha = 1.0f);
float RelativeLuminance(VekColor c);
float ContrastRatio(VekColor a, VekColor b);
std::string ColorToHex(VekColor c, bool includeAlpha = true);
VekValue ColorToValue(VekColor c);
VekColor ColorFromValue(const VekValue& value, VekColor fallback = {});

// Adds pure colour helpers to a script engine. These functions do not expose a
// renderer, GPU pointer, file system or other capability.
void VekRegisterColorLibrary(VekScriptEngine& engine);

} // namespace vek
