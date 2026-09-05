#include <vek/VekHud.h>

#include <cstdio>

namespace vek::hud {

// ---------------------------------------------------------------------------
// VekValueRegistry
// ---------------------------------------------------------------------------

void VekValueRegistry::Bind(const std::string& key, Provider provider) {
    providers_[key] = std::move(provider);
}

void VekValueRegistry::BindNumber(const std::string& key, const double* valuePtr) {
    Bind(key, [valuePtr]() { return VekValue(*valuePtr); });
}

void VekValueRegistry::BindString(const std::string& key, const std::string* valuePtr) {
    Bind(key, [valuePtr]() { return VekValue(*valuePtr); });
}

void VekValueRegistry::Unbind(const std::string& key) {
    providers_.erase(key);
}

bool VekValueRegistry::Has(const std::string& key) const {
    return providers_.find(key) != providers_.end();
}

VekValue VekValueRegistry::Resolve(const std::string& key) const {
    auto it = providers_.find(key);
    if (it == providers_.end()) return VekValue();
    return it->second();
}

// ---------------------------------------------------------------------------
// Formatting: a tiny "{0}" / "{0:.Nf}" substitution, not a full format lib,
// so users can write layouts like "Speed: {0:.1f} m/s" in a spec without any
// engine changes. Falls back to plain string conversion on anything else.
// ---------------------------------------------------------------------------
namespace {

std::string ApplyFormat(const std::string& format, const VekValue& value) {
    if (format.empty()) return value.AsString();

    auto pos = format.find("{0");
    if (pos == std::string::npos) return format; // no placeholder: static text

    auto close = format.find('}', pos);
    if (close == std::string::npos) return format;

    std::string spec = format.substr(pos + 2, close - (pos + 2)); // e.g. ":.1f" or ""
    std::string rendered;

    if (!spec.empty() && spec[0] == ':' && spec.back() == 'f') {
        int precision = 1;
        auto dot = spec.find('.');
        if (dot != std::string::npos) {
            precision = std::atoi(spec.c_str() + dot + 1);
        }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.*f", precision, value.AsNumber());
        rendered = buf;
    } else {
        rendered = value.IsString() ? value.AsString()
                                     : (value.IsNumber() ? std::to_string(value.AsNumber())
                                                          : value.AsString());
    }

    return format.substr(0, pos) + rendered + format.substr(close + 1);
}

GuiColor ColorFromValue(const VekValue& v, GuiColor fallback) {
    const VekArray* arr = v.AsArray();
    if (!arr || arr->size() < 3) return fallback;
    GuiColor c = fallback;
    c.r = static_cast<float>((*arr)[0].AsNumber());
    c.g = static_cast<float>((*arr)[1].AsNumber());
    c.b = static_cast<float>((*arr)[2].AsNumber());
    c.a = arr->size() > 3 ? static_cast<float>((*arr)[3].AsNumber()) : fallback.a;
    return c;
}

GuiWidgetType TypeFromValue(const VekValue& v) {
    if (!v.IsString()) return GuiWidgetType::Panel;
    return ParseGuiWidgetType(v.AsString());
}

} // namespace

// ---------------------------------------------------------------------------
// VekHudPanel
// ---------------------------------------------------------------------------

bool VekHudPanel::BuildElement(GuiFramework& gui, const VekValue& element,
                                const std::string& parentId, std::string* error) {
    const VekMap* map = element.AsMap();
    if (!map) {
        if (error) *error = "hud element must be an object";
        return false;
    }

    VekValue idVal = element.Get("id");
    if (!idVal.IsString() || idVal.AsString().empty()) {
        if (error) *error = "hud element missing required \"id\"";
        return false;
    }
    const std::string id = idVal.AsString();

    GuiNode node;
    node.id = id;
    node.parentId = parentId;
    node.type = TypeFromValue(element.Get("type"));

    VekValue textVal = element.Get("text");
    if (textVal.IsString()) node.text = textVal.AsString();

    VekValue layout = element.Get("layout");
    if (layout.IsMap()) {
        VekValue posVal = layout.Get("position");
        node.layout.position = (posVal.IsString() && posVal.AsString() == "absolute")
                                    ? GuiPositionMode::Absolute
                                    : GuiPositionMode::Flow;
        node.layout.absoluteX = static_cast<float>(layout.Get("x").AsNumber());
        node.layout.absoluteY = static_cast<float>(layout.Get("y").AsNumber());
        if (layout.Get("width").IsNumber())
            node.layout.width = GuiDimension{GuiSizeMode::Fixed,
                                              static_cast<float>(layout.Get("width").AsNumber()),
                                              0.0f, 100000.0f};
        if (layout.Get("height").IsNumber())
            node.layout.height = GuiDimension{GuiSizeMode::Fixed,
                                               static_cast<float>(layout.Get("height").AsNumber()),
                                               0.0f, 100000.0f};
    }

    VekValue style = element.Get("style");
    if (style.IsMap()) {
        node.hasVisual = true;
        node.visual.background = ColorFromValue(style.Get("background"), node.visual.background);
        node.visual.foreground = ColorFromValue(style.Get("foreground"), node.visual.foreground);
        node.visual.border = ColorFromValue(style.Get("border"), node.visual.border);
        if (style.Get("borderWidth").IsNumber())
            node.visual.borderWidth = static_cast<float>(style.Get("borderWidth").AsNumber());
        if (style.Get("radius").IsNumber())
            node.visual.radius = static_cast<float>(style.Get("radius").AsNumber());
        if (style.Get("fontSize").IsNumber())
            node.visual.fontSize = static_cast<float>(style.Get("fontSize").AsNumber());
    }

    if (!gui.Create(node, error)) return false;
    elementIds_.push_back(id);

    VekValue bindVal = element.Get("bind");
    if (bindVal.IsString() && !bindVal.AsString().empty()) {
        VekValue formatVal = element.Get("format");
        boundElements_.push_back(BoundElement{
            id, bindVal.AsString(), formatVal.IsString() ? formatVal.AsString() : std::string("{0}")});
    }

    VekValue children = element.Get("children");
    if (const VekArray* arr = children.AsArray()) {
        for (const auto& child : *arr) {
            if (!BuildElement(gui, child, id, error)) return false;
        }
    }

    return true;
}

bool VekHudPanel::LoadSpec(GuiFramework& gui, const VekValue& spec, std::string* error) {
    Clear(gui);

    if (const VekArray* arr = spec.AsArray()) {
        for (const auto& element : *arr) {
            if (!BuildElement(gui, element, std::string(), error)) return false;
        }
        return true;
    }
    return BuildElement(gui, spec, std::string(), error);
}

void VekHudPanel::Update(GuiFramework& gui, const VekValueRegistry& registry) {
    for (const auto& bound : boundElements_) {
        if (!registry.Has(bound.bindKey)) continue;
        VekValue value = registry.Resolve(bound.bindKey);
        VekMap patch;
        patch["text"] = std::string(ApplyFormat(bound.format, value));
        gui.Patch(bound.nodeId, VekValue(std::move(patch)));
    }
}

void VekHudPanel::Clear(GuiFramework& gui) {
    for (const auto& id : elementIds_) {
        gui.Remove(id, /*recursive=*/true);
    }
    elementIds_.clear();
    boundElements_.clear();
}

// ---------------------------------------------------------------------------
// Convenience spec builder — a starting point, not a preset. Every field set
// here is a normal field in the returned VekValue; callers can inspect,
// overwrite, or delete any of it before passing the result to LoadSpec().
// ---------------------------------------------------------------------------
VekValue MakeReadoutSpec(const std::string& idPrefix, const std::string& bindKey,
                          const std::string& format, GuiRectF rect) {
    VekValue panelStyle = VekValue::Map();
    panelStyle.Set("background", [] {
        VekValue a = VekValue::Array();
        a.Push(0.08); a.Push(0.09); a.Push(0.11); a.Push(0.82);
        return a;
    }());
    panelStyle.Set("border", [] {
        VekValue a = VekValue::Array();
        a.Push(0.30); a.Push(0.55); a.Push(0.95); a.Push(0.9);
        return a;
    }());
    panelStyle.Set("borderWidth", 1.0);
    panelStyle.Set("radius", 8.0);

    VekValue panelLayout = VekValue::Map();
    panelLayout.Set("position", std::string("absolute"));
    panelLayout.Set("x", rect.x);
    panelLayout.Set("y", rect.y);
    panelLayout.Set("width", rect.width);
    panelLayout.Set("height", rect.height);

    VekValue label = VekValue::Map();
    label.Set("id", idPrefix + "_label");
    label.Set("type", std::string("Label"));
    label.Set("text", std::string("--"));
    label.Set("bind", bindKey);
    label.Set("format", format);
    VekValue labelStyle = VekValue::Map();
    labelStyle.Set("foreground", [] {
        VekValue a = VekValue::Array();
        a.Push(0.85); a.Push(0.95); a.Push(1.0); a.Push(1.0);
        return a;
    }());
    labelStyle.Set("fontSize", 16.0);
    label.Set("style", labelStyle);
    VekValue labelLayout = VekValue::Map();
    labelLayout.Set("position", std::string("absolute"));
    labelLayout.Set("x", 12.0);
    labelLayout.Set("y", 14.0);
    label.Set("layout", labelLayout);

    VekValue children = VekValue::Array();
    children.Push(label);

    VekValue panel = VekValue::Map();
    panel.Set("id", idPrefix + "_panel");
    panel.Set("type", std::string("Panel"));
    panel.Set("layout", panelLayout);
    panel.Set("style", panelStyle);
    panel.Set("children", children);

    return panel;
}

} // namespace vek::hud
