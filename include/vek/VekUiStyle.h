#pragma once
// VekUiStyle — VEK UI Next styling system (VEK 3.0).
//
// A CSS-like cascade: style sheets made of selectors (id / class /
// descendant / pseudo-state) resolved with specificity + source-order
// tie-breaking, theme design tokens ("--name") referenced with var(),
// and inherited properties. Deliberately decoupled from GuiNode so it can
// be unit tested and reused; VekGuiFramework applies resolved properties
// onto GuiVisualStyle/GuiLayoutStyle.

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <vek/VekScriptEngine.h>

namespace vek::ui {

// Properties that cascade from parent to child when not explicitly set
// (mirrors CSS "inherited properties" for text-ish properties).
bool IsInheritedStyleProperty(const std::string& propertyName);

// One simple selector node in a (possibly descendant) selector chain, e.g.
// ".primary-button:hover" -> classes={"primary-button"}, pseudo={"hover"}.
struct StyleSimpleSelector {
    std::string id;                     // "#foo" -> "foo", empty if none
    std::vector<std::string> classes;   // ".a.b" -> {"a","b"}
    std::vector<std::string> pseudo;    // ":hover:focused" -> {"hover","focused"}
};

// A full selector is a descendant chain: "A B C" matches C when C matches
// the last simple selector and some ancestor matches B, and some ancestor
// of that matches A (space = descendant combinator, the only combinator
// VEK 3.0 style sheets support in this generation).
struct StyleSelector {
    std::vector<StyleSimpleSelector> chain; // chain.back() is the target node
    std::string raw;
    int Specificity() const;
};

struct StyleRule {
    StyleSelector selector;
    VekMap declarations;
    int sourceOrder = 0;
};

// Context describing one node (and, via ancestors, its lineage) for
// selector matching. `pseudoStates` are the currently-active interaction
// states: hover, pressed, focused, focus-visible, selected, checked,
// disabled, expanded, dragging, drop-target, invalid, warning, success.
struct StyleNodeContext {
    std::string id;
    std::vector<std::string> classes;
    std::vector<std::string> pseudoStates;
};

class StyleSheet {
public:
    // Parses VEK style-sheet syntax:
    //   style ".sel" { prop: value }
    //   theme "name" { --token: value }
    // Multiple style/theme blocks may appear in one source string.
    // Returns the number of rules successfully parsed; malformed blocks are
    // skipped (diagnostics-friendly: does not throw).
    std::size_t Parse(const std::string& source);

    void AddRule(const std::string& selector, VekMap declarations);
    void DefineTheme(const std::string& themeName, VekMap tokens);
    void SetActiveTheme(const std::string& themeName) { activeTheme_ = themeName; }
    const std::string& ActiveTheme() const { return activeTheme_; }

    // Resolves the cascade for `path` (path.back() is the target node, the
    // rest are ancestors from root to parent) and returns the merged
    // declaration map with var(--token) references substituted using the
    // active theme. `inheritedFromParent` supplies values for inherited
    // properties (IsInheritedStyleProperty) not otherwise set.
    VekMap Resolve(const std::vector<StyleNodeContext>& path,
                    const VekMap& inheritedFromParent = {}) const;

    std::size_t RuleCount() const { return rules_.size(); }
    std::size_t ThemeTokenCount(const std::string& themeName) const;

private:
    VekValue ResolveVar(const VekValue& v) const;

    std::vector<StyleRule> rules_;
    std::unordered_map<std::string, VekMap> themes_;
    std::string activeTheme_;
    int nextSourceOrder_ = 0;
};

// Parses one selector string ("#id", ".a.b", ".a:hover", "Panel .row .cell")
// into a StyleSelector. Element-type tokens (bare words) are accepted but
// not matched against GuiWidgetType in this generation — they always match,
// documented as a known limitation (see docs/UI_STYLING.md).
StyleSelector ParseSelector(const std::string& text);

// True if `sel` matches the node at path.back() given its ancestors.
bool SelectorMatches(const StyleSelector& sel, const std::vector<StyleNodeContext>& path);

} // namespace vek::ui
