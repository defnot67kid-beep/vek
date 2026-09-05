#pragma once
// VekHud — a user-driven HUD system (replaces VekAltitudeHud).
//
// Design goal: nothing about "what a HUD shows" or "what it looks like" is
// hardcoded in C++. Two independent pieces make that possible:
//
//   1. VekValueRegistry   — a runtime table of named data sources
//                            ("altitude", "speed", "fuel.percent", ...).
//                            Any system can publish a value under a key;
//                            any HUD element can bind to a key by name.
//
//   2. VekHudPanel         — builds/updates an arbitrary tree of GuiFramework
//                            nodes from a *data* description (a VekValue,
//                            i.e. plain JSON-shaped data), not from C++ calls.
//                            A HUD is authored as data: element type, layout,
//                            style, and (optionally) a binding key + format
//                            string. Users can write, generate, save, and
//                            load these specs freely — a new HUD, or a
//                            reskinned/rearranged one, never requires
//                            touching or recompiling engine code.
//
// A single-value "readout" HUD (what VekAltitudeHud used to hardcode) is now
// just one small JSON spec a user can write themselves; see
// VekHud::MakeReadoutSpec() for a convenience *starting point*, not a preset
// users are boxed into — every field it sets can be overridden or omitted.

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <vek/VekGuiFramework.h>
#include <vek/VekScriptEngine.h>

namespace vek::hud {

// ---------------------------------------------------------------------------
// 1. Value registry: decouples "where a number/string comes from" from
//    "what displays it". Any gameplay/sim system registers a source; any HUD
//    element binds to it by name. Neither side needs to know about the other.
// ---------------------------------------------------------------------------
class VekValueRegistry {
public:
    using Provider = std::function<VekValue()>;

    // Registers/replaces the provider for `key`. Overwriting an existing key
    // is allowed on purpose — users can rebind a key at runtime (e.g. swap
    // "altitude" from a local sim value to a remote::RequestAltitudeMeters
    // call) without touching any HUD spec that reads it.
    void Bind(const std::string& key, Provider provider);

    // Convenience for a plain, frequently-updated numeric value (e.g. a
    // pointer/atomic the host updates every tick). Equivalent to Bind() with
    // a provider that reads *valuePtr.
    void BindNumber(const std::string& key, const double* valuePtr);
    void BindString(const std::string& key, const std::string* valuePtr);

    void Unbind(const std::string& key);
    bool Has(const std::string& key) const;

    // Evaluates a key's provider now. Returns VekValue() (nil) if unbound.
    VekValue Resolve(const std::string& key) const;

private:
    std::unordered_map<std::string, Provider> providers_;
};

// ---------------------------------------------------------------------------
// 2. HUD panel: builds/updates a GUI subtree from a data spec.
// ---------------------------------------------------------------------------
//
// Spec shape (all fields optional except "id"; unknown fields are ignored so
// specs stay forward-compatible):
//
// {
//   "id": "speed_label",
//   "type": "Label" | "Panel" | ... (any GuiWidgetType name),
//   "parent": "<id of another element in this spec, or omitted for root>",
//   "text": "static text (used if no binding, or as a template — see below)",
//   "bind": "speed",              // key into VekValueRegistry
//   "format": "Speed: {0:.1f} m/s", // "{0}" is replaced with the bound value;
//                                    // ":.1f" style precision is optional
//   "layout": {
//     "position": "absolute" | "flow",
//     "x": 16, "y": 16, "width": 180, "height": 48
//   },
//   "style": {
//     "background": [0.08, 0.09, 0.11, 0.82],   // r,g,b,a
//     "foreground": [0.85, 0.95, 1.0, 1.0],
//     "border": [0.30, 0.55, 0.95, 0.9],
//     "borderWidth": 1.0, "radius": 8.0, "fontSize": 16.0
//   },
//   "children": [ { ... nested element spec ... }, ... ]
// }
//
// A whole HUD (any number of top-level elements) is just a VekValue array of
// specs like the above, or a single spec with "children". Save it to disk as
// JSON and a user has a fully custom HUD file they can hand-edit, share, or
// swap at runtime with LoadSpec() — no C++ involved.
class VekHudPanel {
public:
    // Builds new nodes in `gui` for every element in `spec` (single element
    // object or an array of elements). Replaces this panel's previous nodes,
    // if any. Returns false and fills `error` on a malformed spec.
    bool LoadSpec(GuiFramework& gui, const VekValue& spec, std::string* error = nullptr);

    // Re-reads every bound element's current value from `registry`, applies
    // its format, and patches the corresponding GUI node's text. Call once
    // per frame/poll/tick; elements with no "bind" are left as static text.
    void Update(GuiFramework& gui, const VekValueRegistry& registry);

    // Removes every node this panel created.
    void Clear(GuiFramework& gui);

    const std::vector<std::string>& ElementIds() const { return elementIds_; }

private:
    struct BoundElement {
        std::string nodeId;
        std::string bindKey;
        std::string format; // may be empty -> raw value
    };

    bool BuildElement(GuiFramework& gui, const VekValue& element, const std::string& parentId,
                       std::string* error);

    std::vector<std::string> elementIds_;
    std::vector<BoundElement> boundElements_;
};

// Optional convenience: produces a spec for a single labeled, formatted
// readout (panel + label bound to one registry key) — a starting point users
// can hand-edit or ignore entirely; every value it fills in is a plain field
// in the returned VekValue, freely overridable before calling LoadSpec().
VekValue MakeReadoutSpec(const std::string& idPrefix, const std::string& bindKey,
                          const std::string& format = "{0}",
                          GuiRectF rect = {16.0f, 16.0f, 180.0f, 48.0f});

} // namespace vek::hud
