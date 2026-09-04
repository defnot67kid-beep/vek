#include <vek/VekUiStyle.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace vek::ui {
namespace {

std::string Trim(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}
std::string Lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}
bool IsIdentChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.';
}

std::string StripComments(const std::string& src) {
    std::string out;
    out.reserve(src.size());
    bool line = false, block = false, quoted = false;
    for (std::size_t i = 0; i < src.size(); ++i) {
        char c = src[i], n = i + 1 < src.size() ? src[i + 1] : '\0';
        if (!line && !block && c == '"') quoted = !quoted;
        if (!quoted && !line && !block && c == '/' && n == '/') { line = true; ++i; continue; }
        if (!quoted && !line && !block && c == '/' && n == '*') { block = true; ++i; continue; }
        if (line && c == '\n') { line = false; out.push_back(c); continue; }
        if (block && c == '*' && n == '/') { block = false; ++i; continue; }
        if (!line && !block) out.push_back(c);
    }
    return out;
}

VekValue ParseScalarToken(const std::string& raw) {
    std::string t = Trim(raw);
    if (t.empty()) return VekValue();
    if (t.rfind("var(", 0) == 0) return VekValue(t);
    if (t.front() == '"' && t.back() == '"' && t.size() >= 2) return VekValue(t.substr(1, t.size() - 2));
    if (t == "true") return VekValue(true);
    if (t == "false") return VekValue(false);
    char* end = nullptr;
    double d = std::strtod(t.c_str(), &end);
    if (end && *end == '\0' && end != t.c_str()) return VekValue(d);
    return VekValue(t);
}

std::vector<std::string> SplitTopLevel(const std::string& text, char delimiter) {
    std::vector<std::string> out;
    int paren = 0, bracket = 0;
    bool quoted = false;
    std::size_t start = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '"' && (i == 0 || text[i - 1] != '\\')) quoted = !quoted;
        if (quoted) continue;
        if (c == '(') ++paren; else if (c == ')') --paren;
        else if (c == '[') ++bracket; else if (c == ']') --bracket;
        else if (c == delimiter && paren == 0 && bracket == 0) {
            out.push_back(text.substr(start, i - start));
            start = i + 1;
        }
    }
    out.push_back(text.substr(start));
    return out;
}

VekValue ParseDeclarationValue(const std::string& raw) {
    std::string t = Trim(raw);
    if (!t.empty() && t.front() == '[' && t.back() == ']') {
        VekValue arr = VekValue::Array();
        for (const auto& item : SplitTopLevel(t.substr(1, t.size() - 2), ',')) arr.Push(ParseScalarToken(item));
        return arr;
    }
    return ParseScalarToken(t);
}

std::pair<std::size_t, std::size_t> FindBlock(const std::string& src, std::size_t from) {
    std::size_t open = src.find('{', from);
    if (open == std::string::npos) return {std::string::npos, std::string::npos};
    int depth = 1; bool quoted = false;
    std::size_t i = open + 1;
    for (; i < src.size() && depth > 0; ++i) {
        char c = src[i];
        if (c == '"' && (i == 0 || src[i - 1] != '\\')) quoted = !quoted;
        if (quoted) continue;
        if (c == '{') ++depth; else if (c == '}') --depth;
    }
    if (depth != 0) return {std::string::npos, std::string::npos};
    return {open + 1, i - 1};
}

VekMap ParseDeclarationBlock(const std::string& block) {
    VekMap out;
    for (const auto& stmtRaw : SplitTopLevel(block, ';')) {
        // Newlines are also declaration boundaries when ';' is omitted.
        std::stringstream lines(stmtRaw);
        std::string line;
        while (std::getline(lines, line)) {
            std::string s = Trim(line);
            if (s.empty()) continue;
            std::size_t colon = s.find(':');
            if (colon == std::string::npos) continue;
            std::string key = Trim(s.substr(0, colon));
            std::string val = Trim(s.substr(colon + 1));
            if (!key.empty()) out[key] = ParseDeclarationValue(val);
        }
    }
    return out;
}

} // namespace

bool IsInheritedStyleProperty(const std::string& propertyName) {
    static const std::unordered_map<std::string, bool> kInherited = {
        {"foreground",true},{"font_family",true},{"fontFamily",true},{"font_size",true},{"fontSize",true},
        {"font_weight",true},{"line_height",true},{"lineHeight",true},{"letter_spacing",true},
        {"letterSpacing",true},{"text_align",true},{"icon_size",true}
    };
    return kInherited.find(propertyName) != kInherited.end();
}

StyleSelector ParseSelector(const std::string& text) {
    StyleSelector sel; sel.raw = text;
    std::stringstream ss(Trim(text));
    std::string part;
    while (ss >> part) {
        StyleSimpleSelector simple;
        std::size_t i = 0;
        if (i < part.size() && part[i] != '.' && part[i] != '#' && part[i] != ':') {
            std::size_t j = i;
            while (j < part.size() && part[j] != '.' && part[j] != '#' && part[j] != ':') ++j;
            simple.type = Lower(part.substr(i, j - i));
            i = j;
        }
        while (i < part.size()) {
            char c = part[i]; std::size_t j = i + 1;
            while (j < part.size() && IsIdentChar(part[j])) ++j;
            if (c == '#') simple.id = part.substr(i + 1, j - i - 1);
            else if (c == '.') simple.classes.push_back(part.substr(i + 1, j - i - 1));
            else if (c == ':') simple.pseudo.push_back(part.substr(i + 1, j - i - 1));
            i = std::max(j, i + 1);
        }
        sel.chain.push_back(std::move(simple));
    }
    return sel;
}

int StyleSelector::Specificity() const {
    int score = 0;
    for (const auto& s : chain) {
        if (!s.id.empty()) score += 100;
        score += 10 * static_cast<int>(s.classes.size() + s.pseudo.size());
        if (!s.type.empty() && s.type != "*") score += 1;
    }
    return score;
}

static bool SimpleMatches(const StyleSimpleSelector& simple, const StyleNodeContext& node) {
    if (!simple.type.empty() && simple.type != "*" && Lower(node.type) != simple.type) return false;
    if (!simple.id.empty() && simple.id != node.id) return false;
    for (const auto& c : simple.classes)
        if (std::find(node.classes.begin(), node.classes.end(), c) == node.classes.end()) return false;
    for (const auto& p : simple.pseudo)
        if (std::find(node.pseudoStates.begin(), node.pseudoStates.end(), p) == node.pseudoStates.end()) return false;
    return true;
}

bool SelectorMatches(const StyleSelector& sel, const std::vector<StyleNodeContext>& path) {
    if (sel.chain.empty() || path.empty() || !SimpleMatches(sel.chain.back(), path.back())) return false;
    if (sel.chain.size() == 1) return true;
    std::size_t chainIdx = sel.chain.size() - 1, pathIdx = path.size() - 1;
    while (chainIdx > 0) {
        bool found = false;
        while (pathIdx > 0) {
            --pathIdx;
            if (SimpleMatches(sel.chain[chainIdx - 1], path[pathIdx])) { found = true; break; }
        }
        if (!found) return false;
        --chainIdx;
    }
    return true;
}

std::size_t StyleSheet::Parse(const std::string& rawSource) {
    std::string source = StripComments(rawSource);
    std::size_t parsed = 0, pos = 0;
    while (pos < source.size()) {
        std::size_t styleKw = source.find("style", pos), themeKw = source.find("theme", pos);
        if (styleKw == std::string::npos && themeKw == std::string::npos) break;
        bool isTheme = themeKw != std::string::npos && (styleKw == std::string::npos || themeKw < styleKw);
        std::size_t kwPos = isTheme ? themeKw : styleKw;
        std::size_t quote1 = source.find('"', kwPos), quote2 = quote1 == std::string::npos ? std::string::npos : source.find('"', quote1 + 1);
        if (quote1 == std::string::npos || quote2 == std::string::npos) { pos = kwPos + 5; continue; }
        std::string name = source.substr(quote1 + 1, quote2 - quote1 - 1);
        auto [blockStart, blockEnd] = FindBlock(source, quote2);
        if (blockStart == std::string::npos) { pos = kwPos + 5; continue; }
        VekMap declarations = ParseDeclarationBlock(source.substr(blockStart, blockEnd - blockStart));
        if (isTheme) DefineTheme(name, std::move(declarations)); else AddRule(name, std::move(declarations));
        ++parsed; pos = blockEnd + 1;
    }
    return parsed;
}

void StyleSheet::AddRule(const std::string& selector, VekMap declarations) {
    StyleRule rule; rule.selector = ParseSelector(selector); rule.declarations = std::move(declarations); rule.sourceOrder = nextSourceOrder_++;
    rules_.push_back(std::move(rule));
}
void StyleSheet::DefineTheme(const std::string& themeName, VekMap tokens) {
    auto& existing = themes_[themeName]; for (auto& [k,v] : tokens) existing[k] = v;
    if (activeTheme_.empty()) activeTheme_ = themeName;
}
bool StyleSheet::HasTheme(const std::string& themeName) const { return themes_.find(themeName) != themes_.end(); }
const VekMap* StyleSheet::ThemeTokens(const std::string& themeName) const {
    auto it = themes_.find(themeName); return it == themes_.end() ? nullptr : &it->second;
}
std::size_t StyleSheet::ThemeTokenCount(const std::string& themeName) const {
    auto it = themes_.find(themeName); return it == themes_.end() ? 0 : it->second.size();
}

VekValue StyleSheet::ResolveVar(const VekValue& v, int depth) const {
    if (depth > 16) return VekValue();
    if (v.IsArray()) {
        VekValue out = VekValue::Array();
        for (const auto& x : *v.AsArray()) out.Push(ResolveVar(x, depth + 1));
        return out;
    }
    if (v.IsMap()) {
        VekMap out; for (const auto& [k,x] : *v.AsMap()) out[k] = ResolveVar(x, depth + 1); return VekValue(out);
    }
    if (!v.IsString()) return v;
    const std::string s = Trim(v.AsString());
    if (s.rfind("var(",0) != 0 || s.back() != ')') return v;
    std::string inner = Trim(s.substr(4, s.size() - 5));
    auto parts = SplitTopLevel(inner, ',');
    std::string token = Trim(parts.empty() ? std::string{} : parts[0]);
    if (token.size() >= 2 && token.front() == '"' && token.back() == '"') token = token.substr(1, token.size() - 2);
    auto themeIt = themes_.find(activeTheme_);
    if (themeIt != themes_.end()) {
        auto tokIt = themeIt->second.find(token);
        if (tokIt != themeIt->second.end()) return ResolveVar(tokIt->second, depth + 1);
    }
    if (parts.size() > 1) return ResolveVar(ParseDeclarationValue(parts[1]), depth + 1);
    return VekValue();
}
VekValue StyleSheet::ResolveValue(const VekValue& value) const { return ResolveVar(value); }
VekValue StyleSheet::Token(const std::string& tokenName, const VekValue& fallback) const {
    auto themeIt = themes_.find(activeTheme_);
    if (themeIt == themes_.end()) return fallback;
    auto it = themeIt->second.find(tokenName);
    return it == themeIt->second.end() ? fallback : ResolveVar(it->second);
}

VekMap StyleSheet::Resolve(const std::vector<StyleNodeContext>& path, const VekMap& inheritedFromParent) const {
    std::vector<const StyleRule*> matched;
    for (const auto& rule : rules_) if (SelectorMatches(rule.selector, path)) matched.push_back(&rule);
    std::sort(matched.begin(), matched.end(), [](const StyleRule* a, const StyleRule* b){
        int sa=a->selector.Specificity(), sb=b->selector.Specificity(); return sa != sb ? sa < sb : a->sourceOrder < b->sourceOrder;
    });
    VekMap result;
    for (const auto& [k,v] : inheritedFromParent) if (IsInheritedStyleProperty(k)) result[k] = ResolveVar(v);
    for (const StyleRule* rule : matched) for (const auto& [k,v] : rule->declarations) result[k] = ResolveVar(v);
    return result;
}

std::string ModernUiThemeSource() {
    return R"VEKCSS(
theme "vek.modern.dark" {
  --bg-0: #0b0d11; --bg-1: #11141a; --bg-2: #171b22; --bg-3: #1d222b;
  --surface: #151920; --surface-raised: #1c212a; --surface-hover: #222834; --surface-active: #292f3b;
  --border-subtle: #242a34; --border: #303744; --border-strong: #465063;
  --text-1: #f4f6f9; --text-2: #b5bdca; --text-3: #7f8999; --text-disabled: #5f6877;
  --accent: #5ca8ff; --accent-hover: #74b5ff; --accent-active: #438fdc; --accent-soft: #183455;
  --vek-blue: #69a9ff; --vek-cyan: #59d6e8; --vek-teal: #58c7b5; --vek-green: #63cf8d; --vek-lime: #a7d968;
  --vek-yellow: #f0c866; --vek-orange: #f39a61; --vek-red: #ef6b73; --vek-pink: #e67fbd; --vek-violet: #ad8cff;
  --vek-grey-50: #f6f7f9; --vek-grey-100: #e7e9ed; --vek-grey-200: #cdd1d7; --vek-grey-300: #aeb4bd; --vek-grey-400: #8b929e; --vek-grey-500: #6d7480; --vek-grey-600: #555b65; --vek-grey-700: #3f444c; --vek-grey-800: #2b2f35; --vek-grey-900: #1a1d21;
  --success: #55c98a; --warning: #e6b45a; --danger: #ef6b73; --info: #6ea8fe;
  --focus: #86bdff; --selection: #234c78;
  --shadow: #00000088; --overlay: #05070acc;
  --space-1: 4; --space-2: 8; --space-3: 12; --space-4: 16; --space-5: 20; --space-6: 24; --space-8: 32;
  --radius-xs: 4; --radius-sm: 6; --radius-md: 9; --radius-lg: 12; --radius-xl: 16; --radius-pill: 999;
  --control-h-sm: 28; --control-h: 34; --control-h-lg: 40; --icon-sm: 14; --icon: 16; --icon-lg: 20;
  --font-xs: 11; --font-sm: 12; --font-md: 14; --font-lg: 16; --font-xl: 20; --font-xxl: 26;
  --line-tight: 1.12; --line-normal: 1.35; --line-relaxed: 1.5;
  --elevation-1-blur: 10; --elevation-2-blur: 18; --elevation-3-blur: 28;
  --motion-fast: 0.10; --motion-normal: 0.16; --motion-slow: 0.24; --ease-standard: ease_out;
  --focus-ring: 2; --scrollbar-size: 7; --scrollbar-hover-size: 10; --scrollbar-min-thumb: 28;
  --button-bg: var(--surface-raised); --button-bg-hover: var(--surface-hover); --button-bg-active: var(--surface-active);
  --button-primary-bg: var(--accent); --button-primary-hover: var(--accent-hover); --button-primary-active: var(--accent-active);
  --input-bg: #10141a; --input-border: var(--border); --input-focus: var(--focus);
  --popup-bg: #1a1f27; --tooltip-bg: #242b35; --table-zebra: #12161c;
  --scrollbar-track: #00000000; --scrollbar-thumb: #5d687966; --scrollbar-thumb-hover: #778396aa;
  --viewport-bg: #0b0e13; --viewport-vignette: #00000055;
  --dock-bg: #0e1116; --dock-tab-bg: #181d24; --dock-tab-active: #252c36; --dock-handle: #46506388; --dock-zone: #5ca8ff33; --dock-zone-active: #5ca8ff77;
  --shader-panel: vek.ui.frosted_glass; --shader-viewport: vek.ui.vignette;
}

theme "vek.modern.light" {
  --bg-0: #f3f5f8; --bg-1: #ffffff; --bg-2: #f7f8fa; --bg-3: #eef1f5;
  --surface: #ffffff; --surface-raised: #ffffff; --surface-hover: #f0f3f7; --surface-active: #e7ebf1;
  --border-subtle: #e4e8ee; --border: #d3d9e2; --border-strong: #aeb7c5;
  --text-1: #1b2029; --text-2: #4f5968; --text-3: #778293; --text-disabled: #a7afba;
  --accent: #1677d2; --accent-hover: #0f84ef; --accent-active: #0d64b2; --accent-soft: #dceeff;
  --vek-blue: #297fd1; --vek-cyan: #168ba0; --vek-teal: #1c8d7b; --vek-green: #218c56; --vek-lime: #6d902a;
  --vek-yellow: #9c7414; --vek-orange: #b65c24; --vek-red: #c8404b; --vek-pink: #b94b8f; --vek-violet: #7252bd;
  --vek-grey-50: #f6f7f9; --vek-grey-100: #e7e9ed; --vek-grey-200: #cdd1d7; --vek-grey-300: #aeb4bd; --vek-grey-400: #8b929e; --vek-grey-500: #6d7480; --vek-grey-600: #555b65; --vek-grey-700: #3f444c; --vek-grey-800: #2b2f35; --vek-grey-900: #1a1d21;
  --success: #168854; --warning: #b57212; --danger: #c8404b; --info: #2c6dc2;
  --focus: #3b8eea; --selection: #d6eaff; --shadow: #1b243022; --overlay: #11182766;
  --space-1: 4; --space-2: 8; --space-3: 12; --space-4: 16; --space-5: 20; --space-6: 24; --space-8: 32;
  --radius-xs: 4; --radius-sm: 6; --radius-md: 9; --radius-lg: 12; --radius-xl: 16; --radius-pill: 999;
  --control-h-sm: 28; --control-h: 34; --control-h-lg: 40; --icon-sm: 14; --icon: 16; --icon-lg: 20;
  --font-xs: 11; --font-sm: 12; --font-md: 14; --font-lg: 16; --font-xl: 20; --font-xxl: 26;
  --line-tight: 1.12; --line-normal: 1.35; --line-relaxed: 1.5;
  --elevation-1-blur: 10; --elevation-2-blur: 18; --elevation-3-blur: 28;
  --motion-fast: 0.10; --motion-normal: 0.16; --motion-slow: 0.24; --ease-standard: ease_out;
  --focus-ring: 2; --scrollbar-size: 7; --scrollbar-hover-size: 10; --scrollbar-min-thumb: 28;
  --button-bg: #ffffff; --button-bg-hover: #f0f3f7; --button-bg-active: #e7ebf1;
  --button-primary-bg: var(--accent); --button-primary-hover: var(--accent-hover); --button-primary-active: var(--accent-active);
  --input-bg: #ffffff; --input-border: var(--border); --input-focus: var(--focus);
  --popup-bg: #ffffff; --tooltip-bg: #202631; --table-zebra: #f8f9fb;
  --scrollbar-track: #00000000; --scrollbar-thumb: #78839155; --scrollbar-thumb-hover: #68758799;
  --viewport-bg: #e9edf3; --viewport-vignette: #00000020;
  --dock-bg: #eef1f5; --dock-tab-bg: #ffffff; --dock-tab-active: #e7ebf1; --dock-handle: #8893a055; --dock-zone: #1677d233; --dock-zone-active: #1677d277;
  --shader-panel: vek.ui.frosted_glass; --shader-viewport: vek.ui.vignette;
}
)VEKCSS";
}

std::string ModernUiComponentStyleSource() {
    return R"VEKCSS(
style "root" { background: var(--bg-0); foreground: var(--text-1); font_size: var(--font-md); line_height: var(--line-normal); }
style "window" { background: var(--bg-1); border: var(--border); border_width: 1; border_radius: var(--radius-lg); shadow_blur: var(--elevation-3-blur); shadow_color: var(--shadow); }
style "modal" { background: var(--surface-raised); border: var(--border); border_width: 1; border_radius: var(--radius-xl); shadow_blur: var(--elevation-3-blur); shadow_color: var(--shadow); }
style "panel" { background: var(--surface); border: var(--border-subtle); border_width: 1; border_radius: var(--radius-md); shader: var(--shader-panel); }
style "card" { background: var(--surface-raised); border: var(--border-subtle); border_width: 1; border_radius: var(--radius-lg); shadow_blur: var(--elevation-1-blur); shadow_color: var(--shadow); }
style "toolbar" { background: var(--bg-1); border: var(--border-subtle); border_width: 1; }
style "status_bar" { background: var(--bg-1); foreground: var(--text-2); border: var(--border-subtle); border_width: 1; font_size: var(--font-sm); }
style "label" { background: #00000000; foreground: var(--text-1); }
style "rich_text" { background: #00000000; foreground: var(--text-1); line_height: var(--line-relaxed); }

style "button" { background: var(--button-bg); foreground: var(--text-1); border: var(--border); border_width: 1; border_radius: var(--radius-md); transition_duration: var(--motion-fast); }
style "button:hover" { background: var(--button-bg-hover); border: var(--border-strong); }
style "button:pressed" { background: var(--button-bg-active); scale: 0.985; }
style "button:focus-visible" { focus_ring_color: var(--focus); focus_ring_width: var(--focus-ring); }
style "button:disabled" { foreground: var(--text-disabled); opacity: 0.48; }
style "button.primary" { background: var(--button-primary-bg); foreground: #ffffff; border: var(--button-primary-bg); }
style "button.primary:hover" { background: var(--button-primary-hover); }
style "button.primary:pressed" { background: var(--button-primary-active); }
style "button.ghost" { background: #00000000; border: #00000000; }
style "button.ghost:hover" { background: var(--surface-hover); }
style "icon_button" { background: #00000000; foreground: var(--text-2); border: #00000000; border_radius: var(--radius-md); icon_size: var(--icon); transition_duration: var(--motion-fast); }
style "icon_button:hover" { background: var(--surface-hover); foreground: var(--text-1); }
style "icon_button:pressed" { background: var(--surface-active); scale: 0.96; }
style "icon_button:focus-visible" { focus_ring_color: var(--focus); focus_ring_width: var(--focus-ring); }

style "toggle" { background: var(--border-strong); foreground: #ffffff; accent: var(--accent); border_radius: var(--radius-pill); transition_duration: var(--motion-normal); }
style "toggle:checked" { background: var(--accent); }
style "checkbox" { background: var(--input-bg); border: var(--input-border); border_width: 1; border_radius: var(--radius-xs); accent: var(--accent); }
style "checkbox:checked" { background: var(--accent); border: var(--accent); }
style "radio" { background: var(--input-bg); border: var(--input-border); border_width: 1; accent: var(--accent); }
style "radio:checked" { border: var(--accent); }

style "slider" { background: var(--border); accent: var(--accent); foreground: var(--text-2); }
style "range_slider" { background: var(--border); accent: var(--accent); foreground: var(--text-2); }
style "progress" { background: var(--border-subtle); accent: var(--accent); border_radius: var(--radius-pill); }
style "spinner" { background: #00000000; accent: var(--accent); }

style "text_input" { background: var(--input-bg); foreground: var(--text-1); border: var(--input-border); border_width: 1; border_radius: var(--radius-md); }
style "text_area" { background: var(--input-bg); foreground: var(--text-1); border: var(--input-border); border_width: 1; border_radius: var(--radius-md); }
style "search_box" { background: var(--input-bg); foreground: var(--text-1); border: var(--input-border); border_width: 1; border_radius: var(--radius-md); }
style "number_input" { background: var(--input-bg); foreground: var(--text-1); border: var(--input-border); border_width: 1; border_radius: var(--radius-md); }
style "combo_box" { background: var(--input-bg); foreground: var(--text-1); border: var(--input-border); border_width: 1; border_radius: var(--radius-md); }
style "dropdown" { background: var(--input-bg); foreground: var(--text-1); border: var(--input-border); border_width: 1; border_radius: var(--radius-md); }
style "text_input:focus-visible" { border: var(--input-focus); focus_ring_color: var(--focus); focus_ring_width: var(--focus-ring); }
style "text_area:focus-visible" { border: var(--input-focus); focus_ring_color: var(--focus); focus_ring_width: var(--focus-ring); }
style "search_box:focus-visible" { border: var(--input-focus); focus_ring_color: var(--focus); focus_ring_width: var(--focus-ring); }
style "number_input:focus-visible" { border: var(--input-focus); focus_ring_color: var(--focus); focus_ring_width: var(--focus-ring); }
style "text_input:invalid" { border: var(--danger); focus_ring_color: var(--danger); }
style "text_area:invalid" { border: var(--danger); focus_ring_color: var(--danger); }

style "tab_view" { background: var(--bg-1); border: var(--border-subtle); border_width: 1; }
style "tab" { background: #00000000; foreground: var(--text-2); border_radius: var(--radius-sm); transition_duration: var(--motion-fast); }
style "tab:hover" { background: var(--surface-hover); foreground: var(--text-1); }
style "tab:selected" { foreground: var(--text-1); accent: var(--accent); }
style "list" { background: var(--bg-1); border: var(--border-subtle); border_width: 1; border_radius: var(--radius-md); }
style "list_item" { background: #00000000; foreground: var(--text-1); border_radius: var(--radius-sm); }
style "list_item:hover" { background: var(--surface-hover); }
style "list_item:selected" { background: var(--accent-soft); foreground: var(--text-1); }
style "tree" { background: var(--bg-1); border: var(--border-subtle); border_width: 1; border_radius: var(--radius-md); }
style "tree_item" { background: #00000000; foreground: var(--text-1); border_radius: var(--radius-sm); }
style "tree_item:hover" { background: var(--surface-hover); }
style "tree_item:selected" { background: var(--accent-soft); }
style "table" { background: var(--bg-1); border: var(--border-subtle); border_width: 1; border_radius: var(--radius-md); }
style "table_row" { background: #00000000; border: var(--border-subtle); }
style "table_row:hover" { background: var(--surface-hover); }
style "table_cell" { background: #00000000; foreground: var(--text-1); }
style "property_grid" { background: var(--bg-1); border: var(--border-subtle); border_width: 1; border_radius: var(--radius-md); }

style "menu_bar" { background: var(--bg-1); border: var(--border-subtle); border_width: 1; }
style "menu" { background: var(--popup-bg); border: var(--border); border_width: 1; border_radius: var(--radius-lg); shadow_blur: var(--elevation-2-blur); shadow_color: var(--shadow); }
style "menu_item" { background: #00000000; foreground: var(--text-1); border_radius: var(--radius-sm); }
style "menu_item:hover" { background: var(--surface-hover); }
style "popup" { background: var(--popup-bg); border: var(--border); border_width: 1; border_radius: var(--radius-lg); shadow_blur: var(--elevation-2-blur); shadow_color: var(--shadow); }
style "context_menu" { background: var(--popup-bg); border: var(--border); border_width: 1; border_radius: var(--radius-lg); shadow_blur: var(--elevation-2-blur); shadow_color: var(--shadow); }
style "tooltip" { background: var(--tooltip-bg); foreground: #ffffff; border_radius: var(--radius-sm); shadow_blur: var(--elevation-1-blur); shadow_color: var(--shadow); font_size: var(--font-sm); }
style "toast" { background: var(--surface-raised); foreground: var(--text-1); border: var(--border); border_width: 1; border_radius: var(--radius-lg); shadow_blur: var(--elevation-2-blur); shadow_color: var(--shadow); }
style "badge" { background: var(--accent-soft); foreground: var(--accent); border_radius: var(--radius-pill); font_size: var(--font-xs); }
style "keybind" { background: var(--input-bg); foreground: var(--text-2); border: var(--border); border_width: 1; border_radius: var(--radius-sm); }
style "color_picker" { background: var(--surface); border: var(--border); border_width: 1; border_radius: var(--radius-md); }

style "scroll" { background: #00000000; }
style "dock_space" { background: var(--dock-bg); border: var(--border-subtle); border_width: 1; shader: var(--shader-panel); }
style "split_pane" { background: var(--dock-bg); border: var(--border-subtle); border_width: 1; }
style "separator" { background: var(--border-subtle); }
style "viewport" { background: var(--viewport-bg); border: var(--border); border_width: 1; border_radius: var(--radius-lg); shadow_blur: var(--elevation-1-blur); shadow_color: var(--shadow); shader: var(--shader-viewport); }
style "viewport:focus-visible" { border: var(--focus); focus_ring_color: var(--focus); focus_ring_width: var(--focus-ring); }
style "graph" { background: var(--bg-1); border: var(--border-subtle); border_width: 1; border_radius: var(--radius-md); accent: var(--accent); }
style "timeline" { background: var(--bg-1); border: var(--border-subtle); border_width: 1; border_radius: var(--radius-md); accent: var(--accent); }
)VEKCSS";
}

std::string ModernUiStyleSheetSource() { return ModernUiThemeSource() + ModernUiComponentStyleSource(); }

} // namespace vek::ui
