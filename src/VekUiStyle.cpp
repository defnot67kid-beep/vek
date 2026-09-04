#include <vek/VekUiStyle.h>

#include <algorithm>
#include <cctype>
#include <sstream>

namespace vek::ui {

namespace {

std::string Trim(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

bool IsIdentChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.';
}

// Parses a single scalar token found inside a declaration value: a quoted
// string, a number, or a var("--token") reference (kept as the literal
// string "var(--token)" for later resolution).
VekValue ParseScalarToken(const std::string& raw) {
    std::string t = Trim(raw);
    if (t.empty()) return VekValue();
    if (t.rfind("var(", 0) == 0) {
        return VekValue(t); // resolved later by StyleSheet::ResolveVar
    }
    if (t.front() == '"' && t.back() == '"' && t.size() >= 2) {
        return VekValue(t.substr(1, t.size() - 2));
    }
    // Numeric?
    char* end = nullptr;
    double d = std::strtod(t.c_str(), &end);
    if (end && *end == '\0' && end != t.c_str()) return VekValue(d);
    // Bare identifier / hex color / duration like "180ms" - keep as string.
    return VekValue(t);
}

VekValue ParseDeclarationValue(const std::string& raw) {
    std::string t = Trim(raw);
    if (!t.empty() && t.front() == '[' && t.back() == ']') {
        VekValue arr = VekValue::Array();
        std::string inner = t.substr(1, t.size() - 2);
        std::stringstream ss(inner);
        std::string item;
        while (std::getline(ss, item, ',')) {
            arr.Push(ParseScalarToken(item));
        }
        return arr;
    }
    return ParseScalarToken(t);
}

// Extracts the text of the first balanced {...} block starting at or after
// `from`. Returns {blockStart, blockEnd} (exclusive of braces) or
// {npos,npos} if not found/unbalanced.
std::pair<std::size_t, std::size_t> FindBlock(const std::string& src, std::size_t from) {
    std::size_t open = src.find('{', from);
    if (open == std::string::npos) return {std::string::npos, std::string::npos};
    int depth = 1;
    std::size_t i = open + 1;
    for (; i < src.size() && depth > 0; ++i) {
        if (src[i] == '{') ++depth;
        else if (src[i] == '}') --depth;
    }
    if (depth != 0) return {std::string::npos, std::string::npos};
    return {open + 1, i - 1};
}

VekMap ParseDeclarationBlock(const std::string& block) {
    VekMap out;
    std::stringstream ss(block);
    std::string line;
    while (std::getline(ss, line, '\n')) {
        // Also allow ';'-separated declarations on one line.
        std::stringstream ls(line);
        std::string stmt;
        while (std::getline(ls, stmt, ';')) {
            std::string s = Trim(stmt);
            if (s.empty()) continue;
            std::size_t colon = s.find(':');
            if (colon == std::string::npos) continue;
            std::string key = Trim(s.substr(0, colon));
            std::string val = Trim(s.substr(colon + 1));
            if (key.empty()) continue;
            out[key] = ParseDeclarationValue(val);
        }
    }
    return out;
}

} // namespace

bool IsInheritedStyleProperty(const std::string& propertyName) {
    static const std::unordered_map<std::string, bool> kInherited = {
        {"foreground", true}, {"font_family", true}, {"fontFamily", true},
        {"font_size", true}, {"fontSize", true}, {"line_height", true},
        {"lineHeight", true}, {"letter_spacing", true}, {"letterSpacing", true},
        {"text_align", true},
    };
    auto it = kInherited.find(propertyName);
    return it != kInherited.end();
}

StyleSelector ParseSelector(const std::string& text) {
    StyleSelector sel;
    sel.raw = text;
    std::stringstream ss(Trim(text));
    std::string part;
    while (ss >> part) {
        StyleSimpleSelector simple;
        std::size_t i = 0;
        // Optional leading bare type token (e.g. "Panel") is skipped/ignored
        // for matching but doesn't error.
        while (i < part.size() && part[i] != '.' && part[i] != '#' && part[i] != ':') ++i;
        i = 0; // type-prefix matching intentionally not implemented (see header)
        while (i < part.size()) {
            char c = part[i];
            if (c == '#') {
                std::size_t j = i + 1;
                while (j < part.size() && IsIdentChar(part[j])) ++j;
                simple.id = part.substr(i + 1, j - i - 1);
                i = j;
            } else if (c == '.') {
                std::size_t j = i + 1;
                while (j < part.size() && IsIdentChar(part[j])) ++j;
                simple.classes.push_back(part.substr(i + 1, j - i - 1));
                i = j;
            } else if (c == ':') {
                std::size_t j = i + 1;
                while (j < part.size() && IsIdentChar(part[j])) ++j;
                simple.pseudo.push_back(part.substr(i + 1, j - i - 1));
                i = j;
            } else {
                ++i; // skip bare type-name characters
            }
        }
        sel.chain.push_back(std::move(simple));
    }
    return sel;
}

int StyleSelector::Specificity() const {
    int score = 0;
    for (const auto& s : chain) {
        if (!s.id.empty()) score += 100;
        score += 10 * static_cast<int>(s.classes.size());
        score += 10 * static_cast<int>(s.pseudo.size());
    }
    return score;
}

static bool SimpleMatches(const StyleSimpleSelector& simple, const StyleNodeContext& node) {
    if (!simple.id.empty() && simple.id != node.id) return false;
    for (const auto& c : simple.classes) {
        if (std::find(node.classes.begin(), node.classes.end(), c) == node.classes.end()) return false;
    }
    for (const auto& p : simple.pseudo) {
        if (std::find(node.pseudoStates.begin(), node.pseudoStates.end(), p) == node.pseudoStates.end()) return false;
    }
    return true;
}

bool SelectorMatches(const StyleSelector& sel, const std::vector<StyleNodeContext>& path) {
    if (sel.chain.empty() || path.empty()) return false;
    if (!SimpleMatches(sel.chain.back(), path.back())) return false;
    // Walk remaining chain parts right-to-left, requiring each to match some
    // ancestor further up the path (classic CSS descendant-combinator match).
    std::size_t chainIdx = sel.chain.size();
    if (chainIdx == 1) return true;
    --chainIdx; // already matched target
    std::size_t pathIdx = path.size() - 1;
    while (chainIdx > 0) {
        bool found = false;
        while (pathIdx > 0) {
            --pathIdx;
            if (SimpleMatches(sel.chain[chainIdx - 1], path[pathIdx])) {
                found = true;
                break;
            }
        }
        if (!found) return false;
        --chainIdx;
    }
    return true;
}

std::size_t StyleSheet::Parse(const std::string& source) {
    std::size_t parsed = 0;
    std::size_t pos = 0;
    while (pos < source.size()) {
        std::size_t styleKw = source.find("style", pos);
        std::size_t themeKw = source.find("theme", pos);
        if (styleKw == std::string::npos && themeKw == std::string::npos) break;
        bool isTheme = themeKw != std::string::npos && (styleKw == std::string::npos || themeKw < styleKw);
        std::size_t kwPos = isTheme ? themeKw : styleKw;
        std::size_t quote1 = source.find('"', kwPos);
        if (quote1 == std::string::npos) { pos = kwPos + 5; continue; }
        std::size_t quote2 = source.find('"', quote1 + 1);
        if (quote2 == std::string::npos) { pos = kwPos + 5; continue; }
        std::string name = source.substr(quote1 + 1, quote2 - quote1 - 1);
        auto [blockStart, blockEnd] = FindBlock(source, quote2);
        if (blockStart == std::string::npos) { pos = kwPos + 5; continue; }
        std::string block = source.substr(blockStart, blockEnd - blockStart);
        if (isTheme) {
            DefineTheme(name, ParseDeclarationBlock(block));
        } else {
            AddRule(name, ParseDeclarationBlock(block));
        }
        ++parsed;
        pos = blockEnd + 1;
    }
    return parsed;
}

void StyleSheet::AddRule(const std::string& selector, VekMap declarations) {
    StyleRule rule;
    rule.selector = ParseSelector(selector);
    rule.declarations = std::move(declarations);
    rule.sourceOrder = nextSourceOrder_++;
    rules_.push_back(std::move(rule));
}

void StyleSheet::DefineTheme(const std::string& themeName, VekMap tokens) {
    auto& existing = themes_[themeName];
    for (auto& [k, v] : tokens) existing[k] = v;
    if (activeTheme_.empty()) activeTheme_ = themeName;
}

std::size_t StyleSheet::ThemeTokenCount(const std::string& themeName) const {
    auto it = themes_.find(themeName);
    return it == themes_.end() ? 0 : it->second.size();
}

VekValue StyleSheet::ResolveVar(const VekValue& v) const {
    if (!v.IsString()) return v;
    const std::string& s = v.AsString();
    if (s.rfind("var(", 0) != 0) return v;
    std::size_t open = s.find('"');
    std::size_t close = (open == std::string::npos) ? std::string::npos : s.find('"', open + 1);
    std::string token = (open != std::string::npos && close != std::string::npos)
        ? s.substr(open + 1, close - open - 1)
        : s.substr(4, s.size() - 5);
    auto themeIt = themes_.find(activeTheme_);
    if (themeIt == themes_.end()) return VekValue(std::string());
    auto tokIt = themeIt->second.find(token);
    if (tokIt == themeIt->second.end()) return VekValue(std::string());
    return tokIt->second;
}

VekMap StyleSheet::Resolve(const std::vector<StyleNodeContext>& path, const VekMap& inheritedFromParent) const {
    // Gather matching rules, sorted by (specificity, sourceOrder) ascending
    // so later/higher-specificity rules override earlier ones when applied
    // in order (CSS cascade semantics).
    std::vector<const StyleRule*> matched;
    for (const auto& rule : rules_) {
        if (SelectorMatches(rule.selector, path)) matched.push_back(&rule);
    }
    std::sort(matched.begin(), matched.end(), [](const StyleRule* a, const StyleRule* b) {
        int sa = a->selector.Specificity(), sb = b->selector.Specificity();
        if (sa != sb) return sa < sb;
        return a->sourceOrder < b->sourceOrder;
    });

    VekMap result;
    // Seed with inheritable properties from the parent context.
    for (const auto& [k, v] : inheritedFromParent) {
        if (IsInheritedStyleProperty(k)) result[k] = v;
    }
    for (const StyleRule* rule : matched) {
        for (const auto& [k, v] : rule->declarations) {
            result[k] = ResolveVar(v);
        }
    }
    return result;
}

} // namespace vek::ui
