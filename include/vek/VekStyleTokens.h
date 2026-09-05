#pragma once
// VekStyleTokens — procedural, non-CSS styling API (VEK 3.5).
//
// VekUiStyle already ships a CSS-like cascade for hosts that want to author
// stylesheets as text. This header is a *different, VEK-native* way to build
// the same VekMap declarations: a fluent C++ builder plus generated numeric
// scales (spacing/radius/type ramps) and an OKLCH-ish procedural color
// palette generator. Nothing here parses selector text or "cascades" — it is
// pure data construction, so it never "copies CSS", it just produces the
// same VekMap tokens the runtime already understands.

#include <string>
#include <vector>

#include <vek/VekScriptEngine.h>

namespace vek::ui::tokens {

// A single generated ramp step, e.g. spacing[3] == 12.0.
struct Scale {
    std::vector<double> steps;
    double at(std::size_t index) const;
};

// Generates a geometric scale (each step = base * ratio^i), the shape most
// VEK design systems use for spacing/radius/elevation.
Scale MakeGeometricScale(double base, double ratio, int count);

// Generates a modular type scale (font sizes) around a root size.
Scale MakeTypeScale(double rootPx, double ratio, int stepsUp, int stepsDown);

struct Hsl { double h = 0.0; double s = 0.0; double l = 0.0; };

// Procedural palette: one seed hue produces a full tonal ramp (50..900 style)
// without hand-authoring each swatch, and without any CSS color literals.
struct PaletteRamp {
    std::vector<Hsl> tones;   // index 0 = lightest, back = darkest
    std::string ToVekColor(std::size_t index, double alpha = 1.0) const;
};

PaletteRamp MakePaletteRamp(double seedHueDegrees, double saturation, int steps = 10);

// Fluent, VEK-native rule builder. Produces the exact VekMap the retained
// GuiFramework / StyleSheet already consume, but the authoring surface is a
// C++ (and, via natives, VEK-script) API rather than a text stylesheet.
class RuleBuilder {
public:
    explicit RuleBuilder(std::string selector);

    RuleBuilder& Set(const std::string& property, VekValue value);
    RuleBuilder& Var(const std::string& property, const std::string& tokenName);
    RuleBuilder& Color(const std::string& property, const std::string& vekColor);
    RuleBuilder& Number(const std::string& property, double value);
    RuleBuilder& State(const std::string& pseudo); // switches subsequent Set() calls onto :pseudo

    const std::string& Selector() const { return selector_; }
    const VekMap& Declarations() const { return declarations_; }

private:
    std::string selector_;
    std::string activePseudo_;
    VekMap declarations_;
};

// A themed token pack: named scales + a generated palette, exportable as the
// VekMap a StyleSheet::DefineTheme() call expects.
class TokenPack {
public:
    TokenPack& AddScale(const std::string& name, const Scale& scale);
    TokenPack& AddPalette(const std::string& name, const PaletteRamp& ramp);
    TokenPack& Set(const std::string& tokenName, VekValue value);

    VekMap Build() const;

private:
    VekMap tokens_;
};

// Ships one ready-made, fully procedural "VEK Aurora" theme distinct from the
// hand-authored CSS-like built-in theme in VekUiStyle — generated at runtime
// from a seed hue instead of literal color strings.
TokenPack BuildAuroraTokenPack(double seedHueDegrees = 262.0);

} // namespace vek::ui::tokens
