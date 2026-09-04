#pragma once
// VekUiStyle — VEK UI styling system (VEK 3.1).
//
// CSS-like cascade with widget-type / id / class / descendant / pseudo-state
// selectors, theme design tokens, recursive var() resolution (including
// fallbacks), inherited text properties, and the built-in modern desktop/game
// editor design system. The module is renderer-neutral.

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include <vek/VekScriptEngine.h>

namespace vek::ui {

bool IsInheritedStyleProperty(const std::string& propertyName);

struct StyleSimpleSelector {
    std::string type;                   // e.g. "button", empty means any type
    std::string id;                     // #foo
    std::vector<std::string> classes;   // .primary.compact
    std::vector<std::string> pseudo;    // :hover:focus-visible
};

struct StyleSelector {
    std::vector<StyleSimpleSelector> chain;
    std::string raw;
    int Specificity() const;
};

struct StyleRule {
    StyleSelector selector;
    VekMap declarations;
    int sourceOrder = 0;
};

struct StyleNodeContext {
    std::string type;
    std::string id;
    std::vector<std::string> classes;
    std::vector<std::string> pseudoStates;
};

class StyleSheet {
public:
    std::size_t Parse(const std::string& source);
    void AddRule(const std::string& selector, VekMap declarations);
    void DefineTheme(const std::string& themeName, VekMap tokens);
    void SetActiveTheme(const std::string& themeName) { activeTheme_ = themeName; }
    const std::string& ActiveTheme() const { return activeTheme_; }

    VekMap Resolve(const std::vector<StyleNodeContext>& path,
                   const VekMap& inheritedFromParent = {}) const;

    // Resolve a token/value outside of a style rule. Useful to host renderers
    // and the retained GUI when they need a component metric such as
    // --scrollbar-size or --motion-fast.
    VekValue ResolveValue(const VekValue& value) const;
    VekValue Token(const std::string& tokenName, const VekValue& fallback = {}) const;
    bool HasTheme(const std::string& themeName) const;
    const VekMap* ThemeTokens(const std::string& themeName) const;

    std::size_t RuleCount() const { return rules_.size(); }
    std::size_t ThemeTokenCount(const std::string& themeName) const;

private:
    VekValue ResolveVar(const VekValue& v, int depth = 0) const;

    std::vector<StyleRule> rules_;
    std::unordered_map<std::string, VekMap> themes_;
    std::string activeTheme_;
    int nextSourceOrder_ = 0;
};

StyleSelector ParseSelector(const std::string& text);
bool SelectorMatches(const StyleSelector& sel, const std::vector<StyleNodeContext>& path);

// Built-in modern design system. These are plain stylesheet/theme source so
// projects can copy, override, or replace them without changing C++.
std::string ModernUiThemeSource();
std::string ModernUiComponentStyleSource();
std::string ModernUiStyleSheetSource();

} // namespace vek::ui
