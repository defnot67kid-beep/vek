#include <vek/VekStyleTokens.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace vek::ui::tokens {

double Scale::at(std::size_t index) const {
    if (steps.empty()) return 0.0;
    if (index >= steps.size()) return steps.back();
    return steps[index];
}

Scale MakeGeometricScale(double base, double ratio, int count) {
    Scale s;
    if (count < 1) count = 1;
    double v = base;
    for (int i = 0; i < count; ++i) {
        s.steps.push_back(v);
        v *= ratio;
    }
    return s;
}

Scale MakeTypeScale(double rootPx, double ratio, int stepsUp, int stepsDown) {
    Scale s;
    std::vector<double> down;
    double v = rootPx;
    for (int i = 0; i < stepsDown; ++i) {
        v /= ratio;
        down.push_back(v);
    }
    for (auto it = down.rbegin(); it != down.rend(); ++it) s.steps.push_back(*it);
    s.steps.push_back(rootPx);
    v = rootPx;
    for (int i = 0; i < stepsUp; ++i) {
        v *= ratio;
        s.steps.push_back(v);
    }
    return s;
}

std::string PaletteRamp::ToVekColor(std::size_t index, double alpha) const {
    if (tones.empty()) return "hsla(0,0%,0%,1)";
    const Hsl& t = tones[std::min(index, tones.size() - 1)];
    char buf[96];
    std::snprintf(buf, sizeof(buf), "hsla(%.1f,%.1f%%,%.1f%%,%.3f)", t.h, t.s * 100.0, t.l * 100.0, alpha);
    return std::string(buf);
}

PaletteRamp MakePaletteRamp(double seedHueDegrees, double saturation, int steps) {
    PaletteRamp ramp;
    if (steps < 2) steps = 2;
    // Lightness sweeps from very light to very dark; hue drifts slightly
    // across the ramp so the palette reads as generated, not a flat tint
    // scale, and saturation eases off at the extremes for readable contrast.
    for (int i = 0; i < steps; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(steps - 1); // 0..1
        double lightness = 0.96 - t * 0.86;                      // 96% -> 10%
        double satFalloff = 1.0 - std::pow(std::abs(t - 0.5) * 2.0, 2.0) * 0.15;
        double hueDrift = std::sin(t * 3.14159265) * 6.0;         // gentle bow
        Hsl hsl;
        hsl.h = std::fmod(seedHueDegrees + hueDrift + 360.0, 360.0);
        hsl.s = std::clamp(saturation * satFalloff, 0.0, 1.0);
        hsl.l = std::clamp(lightness, 0.0, 1.0);
        ramp.tones.push_back(hsl);
    }
    return ramp;
}

RuleBuilder::RuleBuilder(std::string selector) : selector_(std::move(selector)) {}

RuleBuilder& RuleBuilder::State(const std::string& pseudo) {
    activePseudo_ = pseudo;
    return *this;
}

RuleBuilder& RuleBuilder::Set(const std::string& property, VekValue value) {
    std::string key = activePseudo_.empty() ? property : (property + ":" + activePseudo_);
    declarations_[key] = std::move(value);
    return *this;
}

RuleBuilder& RuleBuilder::Var(const std::string& property, const std::string& tokenName) {
    return Set(property, VekValue("var(--" + tokenName + ")"));
}

RuleBuilder& RuleBuilder::Color(const std::string& property, const std::string& vekColor) {
    return Set(property, VekValue(vekColor));
}

RuleBuilder& RuleBuilder::Number(const std::string& property, double value) {
    return Set(property, VekValue(value));
}

TokenPack& TokenPack::AddScale(const std::string& name, const Scale& scale) {
    for (std::size_t i = 0; i < scale.steps.size(); ++i) {
        tokens_[name + "-" + std::to_string(i)] = VekValue(scale.steps[i]);
    }
    return *this;
}

TokenPack& TokenPack::AddPalette(const std::string& name, const PaletteRamp& ramp) {
    for (std::size_t i = 0; i < ramp.tones.size(); ++i) {
        tokens_[name + "-" + std::to_string(i * 100)] = VekValue(ramp.ToVekColor(i));
    }
    return *this;
}

TokenPack& TokenPack::Set(const std::string& tokenName, VekValue value) {
    tokens_[tokenName] = std::move(value);
    return *this;
}

VekMap TokenPack::Build() const { return tokens_; }

TokenPack BuildAuroraTokenPack(double seedHueDegrees) {
    TokenPack pack;
    pack.AddScale("space", MakeGeometricScale(4.0, 1.5, 8));
    pack.AddScale("radius", MakeGeometricScale(2.0, 1.7, 6));
    pack.AddScale("type", MakeTypeScale(16.0, 1.2, 5, 3));
    pack.AddPalette("accent", MakePaletteRamp(seedHueDegrees, 0.62, 10));
    pack.AddPalette("neutral", MakePaletteRamp(std::fmod(seedHueDegrees + 40.0, 360.0), 0.08, 10));
    pack.AddPalette("danger", MakePaletteRamp(6.0, 0.7, 10));
    pack.AddPalette("success", MakePaletteRamp(152.0, 0.55, 10));
    pack.Set("motion-fast", VekValue(0.08));
    pack.Set("motion-base", VekValue(0.16));
    pack.Set("motion-slow", VekValue(0.32));
    return pack;
}

} // namespace vek::ui::tokens
