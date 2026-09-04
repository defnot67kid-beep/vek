#include <vek/VekUiCommands.h>

#include <algorithm>
#include <cctype>
#include <sstream>

namespace vek::ui {

namespace {
std::string ToLowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}
std::string TrimCopy(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}
}

KeyChord ParseShortcut(const std::string& text) {
    KeyChord chord;
    std::stringstream steps(text);
    std::string stepText;
    while (std::getline(steps, stepText, ' ')) {
        std::string trimmed = TrimCopy(stepText);
        if (trimmed.empty()) continue;
        KeyChordStep step;
        std::stringstream parts(trimmed);
        std::string tok;
        std::string key;
        while (std::getline(parts, tok, '+')) {
            std::string lower = ToLowerCopy(TrimCopy(tok));
            if (lower == "ctrl" || lower == "control") step.ctrl = true;
            else if (lower == "shift") step.shift = true;
            else if (lower == "alt") step.alt = true;
            else if (lower == "super" || lower == "cmd" || lower == "win") step.super = true;
            else key = TrimCopy(tok);
        }
        step.key = key;
        chord.push_back(step);
    }
    return chord;
}

std::string FormatShortcut(const KeyChord& chord) {
    std::string out;
    for (std::size_t i = 0; i < chord.size(); ++i) {
        if (i > 0) out += ' ';
        const KeyChordStep& s = chord[i];
        if (s.ctrl) out += "Ctrl+";
        if (s.shift) out += "Shift+";
        if (s.alt) out += "Alt+";
        if (s.super) out += "Super+";
        out += s.key;
    }
    return out;
}

static bool ChordEquals(const KeyChord& a, const KeyChord& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto& x = a[i];
        const auto& y = b[i];
        if (x.ctrl != y.ctrl || x.shift != y.shift || x.alt != y.alt || x.super != y.super) return false;
        if (ToLowerCopy(x.key) != ToLowerCopy(y.key)) return false;
    }
    return true;
}

bool CommandRegistry::Register(CommandDescriptor descriptor) {
    if (descriptor.id.empty()) return false;
    if (descriptor.shortcut.empty() && !descriptor.shortcutText.empty()) {
        descriptor.shortcut = ParseShortcut(descriptor.shortcutText);
    }
    commands_[descriptor.id] = std::move(descriptor);
    return true;
}

bool CommandRegistry::Unregister(const std::string& id) {
    return commands_.erase(id) > 0;
}

const CommandDescriptor* CommandRegistry::Find(const std::string& id) const {
    auto it = commands_.find(id);
    return it == commands_.end() ? nullptr : &it->second;
}

void CommandRegistry::SetEnabled(const std::string& id, bool enabled) {
    auto it = commands_.find(id);
    if (it != commands_.end()) it->second.enabled = enabled;
}

void CommandRegistry::SetChecked(const std::string& id, bool checked) {
    auto it = commands_.find(id);
    if (it != commands_.end()) {
        it->second.checked = checked;
        it->second.hasCheckedState = true;
    }
}

CommandExecuteResult CommandRegistry::Execute(const std::string& id,
                                               const std::vector<std::string>& grantedCapabilities) const {
    auto it = commands_.find(id);
    if (it == commands_.end()) return CommandExecuteResult::NotFound;
    const CommandDescriptor& cmd = it->second;
    if (!cmd.enabled) return CommandExecuteResult::Disabled;
    if (!cmd.requiredCapability.empty()) {
        bool granted = std::find(grantedCapabilities.begin(), grantedCapabilities.end(),
                                  cmd.requiredCapability) != grantedCapabilities.end();
        if (!granted) return CommandExecuteResult::MissingCapability;
    }
    if (!cmd.execute) return CommandExecuteResult::NoExecuteCallback;
    cmd.execute();
    return CommandExecuteResult::Ok;
}

std::string CommandRegistry::ExecuteByShortcut(const KeyChord& chord,
                                                const std::vector<std::string>& grantedCapabilities) const {
    for (const auto& [id, cmd] : commands_) {
        if (!cmd.shortcut.empty() && ChordEquals(cmd.shortcut, chord)) {
            if (Execute(id, grantedCapabilities) == CommandExecuteResult::Ok) return id;
        }
    }
    return {};
}

std::vector<const CommandDescriptor*> CommandRegistry::All() const {
    std::vector<const CommandDescriptor*> out;
    out.reserve(commands_.size());
    for (const auto& [id, cmd] : commands_) out.push_back(&cmd);
    std::sort(out.begin(), out.end(), [](const CommandDescriptor* a, const CommandDescriptor* b) {
        return a->id < b->id;
    });
    return out;
}

} // namespace vek::ui
