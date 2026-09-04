#pragma once
// VekUiCommands — VEK UI Next centralized command/action architecture
// (VEK 3.0). A single command definition (id, label, icon, shortcut,
// enabled/checked state, execute callback) can be invoked identically from
// a toolbar button, a menu item, a keyboard shortcut, a gamepad menu, or
// the command palette, instead of duplicating logic per-widget.

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vek::ui {

// A parsed keyboard shortcut / key chord, e.g. "Ctrl+Shift+Delete" or a
// two-step chord "Ctrl+K Ctrl+B".
struct KeyChordStep {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    bool super = false;
    std::string key; // canonicalized, e.g. "Delete", "K"
};
using KeyChord = std::vector<KeyChordStep>;

KeyChord ParseShortcut(const std::string& text);
std::string FormatShortcut(const KeyChord& chord);

struct CommandDescriptor {
    std::string id;
    std::string label;
    std::string icon;
    std::string shortcutText;
    KeyChord shortcut;
    bool enabled = true;
    bool checked = false;
    bool hasCheckedState = false;
    // Optional capability required to execute this command (see
    // VekAuthoritySystems); empty means no extra permission needed.
    std::string requiredCapability;
    std::function<void()> execute;
};

enum class CommandExecuteResult : std::uint8_t {
    Ok = 0,
    NotFound,
    Disabled,
    MissingCapability,
    NoExecuteCallback,
};

class CommandRegistry {
public:
    // Registers or replaces a command. Returns false if `id` is empty.
    bool Register(CommandDescriptor descriptor);
    bool Unregister(const std::string& id);
    bool Contains(const std::string& id) const { return commands_.find(id) != commands_.end(); }

    const CommandDescriptor* Find(const std::string& id) const;

    void SetEnabled(const std::string& id, bool enabled);
    void SetChecked(const std::string& id, bool checked);

    // Executes a command by id. `grantedCapabilities` models the caller's
    // available capabilities (see VekAuthoritySystems); pass an empty set
    // to require commands have no requiredCapability.
    CommandExecuteResult Execute(const std::string& id,
                                  const std::vector<std::string>& grantedCapabilities = {}) const;

    // Looks up (and executes, if found+enabled+authorized) the command bound
    // to `chord`. Returns the command id executed, or empty string.
    std::string ExecuteByShortcut(const KeyChord& chord,
                                   const std::vector<std::string>& grantedCapabilities = {}) const;

    std::vector<const CommandDescriptor*> All() const;
    std::size_t Count() const { return commands_.size(); }

private:
    std::unordered_map<std::string, CommandDescriptor> commands_;
};

} // namespace vek::ui
