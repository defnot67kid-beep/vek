#include <vek/VekMotorSystems.h>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace vek {
namespace {

bool SafeToken(const std::string& s, std::size_t maxLen, bool allowEmpty = false) {
    if (s.empty()) return allowEmpty;
    if (s.size() > maxLen) return false;
    for (unsigned char c : s) {
        if (std::isalnum(c)) continue;
        switch (c) { case '_': case '-': case '.': case ':': case '/': case '@': break; default: return false; }
    }
    return true;
}

MotorInputSource ParseSource(const std::string& s) {
    if (s == "keybind") return MotorInputSource::Keybind;
    if (s == "axis") return MotorInputSource::Axis;
    if (s == "script") return MotorInputSource::Script;
    if (s == "network" || s == "remote") return MotorInputSource::Network;
    return MotorInputSource::None;
}
std::string SourceName(MotorInputSource s) {
    switch (s) {
        case MotorInputSource::Keybind: return "keybind";
        case MotorInputSource::Axis: return "axis";
        case MotorInputSource::Script: return "script";
        case MotorInputSource::Network: return "network";
        default: return "none";
    }
}
MotorScriptScope ParseScope(const std::string& s) {
    return (s == "remote" || s == "remote_server" || s == "server") ? MotorScriptScope::RemoteServer : MotorScriptScope::Local;
}
std::string ScopeName(MotorScriptScope s) { return s == MotorScriptScope::RemoteServer ? "remote_server" : "local"; }

VekValue BindingToValue(const MotorBindingDefinition& d) {
    VekValue v = VekValue::Map();
    v.Set("id", d.id); v.Set("motor_id", d.motorId); v.Set("source", SourceName(d.source));
    v.Set("key_code", d.keyCode); v.Set("axis_id", d.axisId); v.Set("hold_to_activate", d.holdToActivate);
    v.Set("script_function", d.scriptFunction); v.Set("script_scope", ScopeName(d.scriptScope));
    v.Set("remote_action_id", d.remoteActionId); v.Set("invert", (double)d.invert);
    v.Set("sensitivity", (double)d.sensitivity); v.Set("min_value", (double)d.minValue); v.Set("max_value", (double)d.maxValue);
    return v;
}
VekValue CommandToValue(const MotorCommand& c) {
    VekValue v = VekValue::Map();
    v.Set("valid", c.valid); v.Set("motor_id", c.motorId); v.Set("binding_id", c.bindingId);
    v.Set("target_velocity", (double)c.targetVelocity); v.Set("active", c.active);
    v.Set("source", SourceName(c.source)); v.Set("script_scope", ScopeName(c.scriptScope));
    v.Set("script_function", c.scriptFunction); v.Set("remote_action_id", c.remoteActionId);
    return v;
}
VekValue ConnectionToValue(const PartConnectionDefinition& d) {
    VekValue v = VekValue::Map();
    v.Set("id", d.id); v.Set("part_a", d.partA); v.Set("part_b", d.partB); v.Set("kind", d.kind);
    v.Set("joint_id", d.jointId); v.Set("motor_id", d.motorId); v.Set("visible", d.visible); v.Set("label", d.label);
    return v;
}

} // namespace

bool MotorBindingRegistry::Register(const MotorBindingDefinition& d) {
    if (!SafeToken(d.id, 128) || bindings.count(d.id)) return false;
    if (!SafeToken(d.motorId, 128)) return false;
    if (d.source == MotorInputSource::Keybind && !SafeToken(d.keyCode, 64)) return false;
    if (d.source == MotorInputSource::Axis && !SafeToken(d.axisId, 64)) return false;
    if (d.source == MotorInputSource::Script && (!SafeToken(d.scriptFunction, 128) || d.scriptFunction.empty())) return false;
    if (d.source == MotorInputSource::Network && !SafeToken(d.remoteActionId, 128)) return false;
    if (!std::isfinite(d.invert) || !std::isfinite(d.sensitivity) || !std::isfinite(d.minValue) || !std::isfinite(d.maxValue)) return false;
    if (d.minValue > d.maxValue) return false;
    bindings.emplace(d.id, d);
    motorIndex[d.motorId].push_back(d.id);
    if (!d.keyCode.empty()) keyIndex[d.keyCode].push_back(d.id);
    return true;
}

bool MotorBindingRegistry::RegisterValue(const VekValue& v, std::string* error) {
    if (!v.IsMap()) { if (error) *error = "motor_binding_register expects a map"; return false; }
    MotorBindingDefinition d;
    d.id = v.Get("id").AsString();
    d.motorId = v.Get("motor_id").AsString();
    d.source = ParseSource(v.Get("source").AsString());
    d.keyCode = v.Get("key_code").AsString();
    d.axisId = v.Get("axis_id").AsString();
    auto b = v.Get("hold_to_activate"); if (!b.IsNil()) d.holdToActivate = b.AsBool();
    d.scriptFunction = v.Get("script_function").AsString();
    d.scriptScope = ParseScope(v.Get("script_scope").AsString());
    d.remoteActionId = v.Get("remote_action_id").AsString();
    d.invert = (float)v.Get("invert").AsNumber(d.invert);
    d.sensitivity = (float)v.Get("sensitivity").AsNumber(d.sensitivity);
    d.minValue = (float)v.Get("min_value").AsNumber(d.minValue);
    d.maxValue = (float)v.Get("max_value").AsNumber(d.maxValue);
    if (!Register(d)) { if (error) *error = "invalid or duplicate motor binding definition"; return false; }
    if (error) error->clear();
    return true;
}

bool MotorBindingRegistry::Unregister(const std::string& id) {
    auto it = bindings.find(id);
    if (it == bindings.end()) return false;
    auto eraseFrom = [&](std::unordered_map<std::string, std::vector<std::string>>& idx, const std::string& key) {
        auto ix = idx.find(key);
        if (ix == idx.end()) return;
        auto& v = ix->second;
        v.erase(std::remove(v.begin(), v.end(), id), v.end());
        if (v.empty()) idx.erase(ix);
    };
    eraseFrom(motorIndex, it->second.motorId);
    if (!it->second.keyCode.empty()) eraseFrom(keyIndex, it->second.keyCode);
    bindings.erase(it);
    return true;
}

const MotorBindingDefinition* MotorBindingRegistry::Find(const std::string& id) const {
    auto it = bindings.find(id);
    return it == bindings.end() ? nullptr : &it->second;
}

std::vector<const MotorBindingDefinition*> MotorBindingRegistry::BindingsForMotor(const std::string& motorId) const {
    std::vector<const MotorBindingDefinition*> out;
    auto it = motorIndex.find(motorId);
    if (it == motorIndex.end()) return out;
    for (auto& id : it->second) { auto* b = Find(id); if (b) out.push_back(b); }
    return out;
}

std::vector<const MotorBindingDefinition*> MotorBindingRegistry::BindingsForKey(const std::string& keyCode) const {
    std::vector<const MotorBindingDefinition*> out;
    auto it = keyIndex.find(keyCode);
    if (it == keyIndex.end()) return out;
    for (auto& id : it->second) { auto* b = Find(id); if (b) out.push_back(b); }
    return out;
}

bool MotorBindingRegistry::Rebind(const std::string& id, const std::string& newKeyCode) {
    auto it = bindings.find(id);
    if (it == bindings.end() || !SafeToken(newKeyCode, 64)) return false;
    MotorBindingDefinition& d = it->second;
    if (!d.keyCode.empty()) {
        auto ix = keyIndex.find(d.keyCode);
        if (ix != keyIndex.end()) {
            auto& v = ix->second;
            v.erase(std::remove(v.begin(), v.end(), id), v.end());
            if (v.empty()) keyIndex.erase(ix);
        }
    }
    d.keyCode = newKeyCode;
    if (!newKeyCode.empty()) keyIndex[newKeyCode].push_back(id);
    return true;
}

MotorCommand MotorBindingRegistry::Evaluate(const std::string& bindingId, float rawValue, bool active) const {
    MotorCommand cmd;
    const auto* d = Find(bindingId);
    if (!d) return cmd;
    if (!std::isfinite(rawValue)) rawValue = 0.0f;
    float magnitude = (d->source == MotorInputSource::Keybind) ? (active ? 1.0f : 0.0f) : rawValue;
    magnitude = std::clamp(magnitude * d->sensitivity, d->minValue, d->maxValue);
    cmd.valid = true;
    cmd.motorId = d->motorId;
    cmd.bindingId = d->id;
    cmd.targetVelocity = magnitude * d->invert;
    // For hold-to-activate bindings `active` is the live pressed state; for
    // toggle bindings this reports the raw trigger edge and the caller (host
    // input layer or script) is expected to latch it into an on/off state.
    cmd.active = active;
    cmd.source = d->source;
    cmd.scriptScope = d->scriptScope;
    cmd.scriptFunction = d->scriptFunction;
    cmd.remoteActionId = d->remoteActionId;
    return cmd;
}

void MotorBindingRegistry::Clear() { bindings.clear(); motorIndex.clear(); keyIndex.clear(); }

void MotorBindingRegistry::RegisterNatives(VekScriptEngine& e) {
    e.RegisterNative("motor_binding_register", [this](const std::vector<VekValue>& a) {
        std::string err; return VekValue(!a.empty() && RegisterValue(a[0], &err));
    });
    e.RegisterNative("motor_binding_unregister", [this](const std::vector<VekValue>& a) {
        return VekValue(!a.empty() && Unregister(a[0].AsString()));
    });
    e.RegisterNative("motor_binding_exists", [this](const std::vector<VekValue>& a) {
        return VekValue(!a.empty() && Find(a[0].AsString()) != nullptr);
    });
    e.RegisterNative("motor_binding_get", [this](const std::vector<VekValue>& a) -> VekValue {
        if (a.empty()) return VekValue();
        const auto* b = Find(a[0].AsString());
        return b ? BindingToValue(*b) : VekValue();
    });
    e.RegisterNative("motor_binding_rebind", [this](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 2 && Rebind(a[0].AsString(), a[1].AsString()));
    });
    e.RegisterNative("motor_bindings_for_motor", [this](const std::vector<VekValue>& a) {
        VekValue arr = VekValue::Array();
        if (!a.empty()) for (auto* b : BindingsForMotor(a[0].AsString())) arr.Push(BindingToValue(*b));
        return arr;
    });
    e.RegisterNative("motor_bindings_for_key", [this](const std::vector<VekValue>& a) {
        VekValue arr = VekValue::Array();
        if (!a.empty()) for (auto* b : BindingsForKey(a[0].AsString())) arr.Push(BindingToValue(*b));
        return arr;
    });
    e.RegisterNative("motor_binding_evaluate", [this](const std::vector<VekValue>& a) {
        if (a.empty()) return CommandToValue(MotorCommand{});
        float raw = a.size() > 1 ? (float)a[1].AsNumber(0.0) : 0.0f;
        bool active = a.size() > 2 ? a[2].AsBool(false) : false;
        return CommandToValue(Evaluate(a[0].AsString(), raw, active));
    });
    e.RegisterNative("motor_binding_count", [this](const std::vector<VekValue>&) { return VekValue((double)Size()); });
}

bool PartConnectionGraph::Connect(const PartConnectionDefinition& d) {
    if (!SafeToken(d.id, 128) || connections.count(d.id)) return false;
    if (!SafeToken(d.partA, 128) || !SafeToken(d.partB, 128) || d.partA == d.partB) return false;
    if (d.kind != "joint" && d.kind != "weld" && d.kind != "wire" && d.kind != "socket" && d.kind != "motor" && d.kind != "fuel_line") return false;
    connections.emplace(d.id, d);
    partIndex[d.partA].push_back(d.id);
    partIndex[d.partB].push_back(d.id);
    return true;
}

bool PartConnectionGraph::ConnectValue(const VekValue& v, std::string* error) {
    if (!v.IsMap()) { if (error) *error = "part_connect expects a map"; return false; }
    PartConnectionDefinition d;
    d.id = v.Get("id").AsString(); d.partA = v.Get("part_a").AsString(); d.partB = v.Get("part_b").AsString();
    std::string kind = v.Get("kind").AsString(); if (!kind.empty()) d.kind = kind;
    d.jointId = v.Get("joint_id").AsString(); d.motorId = v.Get("motor_id").AsString();
    auto b = v.Get("visible"); if (!b.IsNil()) d.visible = b.AsBool();
    d.label = v.Get("label").AsString();
    if (!Connect(d)) { if (error) *error = "invalid or duplicate part connection"; return false; }
    if (error) error->clear();
    return true;
}

bool PartConnectionGraph::Disconnect(const std::string& id) {
    auto it = connections.find(id);
    if (it == connections.end()) return false;
    auto eraseFrom = [&](const std::string& part) {
        auto ix = partIndex.find(part);
        if (ix == partIndex.end()) return;
        auto& v = ix->second;
        v.erase(std::remove(v.begin(), v.end(), id), v.end());
        if (v.empty()) partIndex.erase(ix);
    };
    eraseFrom(it->second.partA);
    eraseFrom(it->second.partB);
    connections.erase(it);
    return true;
}

bool PartConnectionGraph::DisconnectParts(const std::string& partA, const std::string& partB) {
    std::vector<std::string> toRemove;
    for (auto& [id, d] : connections)
        if ((d.partA == partA && d.partB == partB) || (d.partA == partB && d.partB == partA)) toRemove.push_back(id);
    for (auto& id : toRemove) Disconnect(id);
    return !toRemove.empty();
}

const PartConnectionDefinition* PartConnectionGraph::Find(const std::string& id) const {
    auto it = connections.find(id);
    return it == connections.end() ? nullptr : &it->second;
}

std::vector<const PartConnectionDefinition*> PartConnectionGraph::ConnectionsForPart(const std::string& partId) const {
    std::vector<const PartConnectionDefinition*> out;
    auto it = partIndex.find(partId);
    if (it == partIndex.end()) return out;
    for (auto& id : it->second) { auto* c = Find(id); if (c) out.push_back(c); }
    return out;
}

std::vector<const PartConnectionDefinition*> PartConnectionGraph::All() const {
    std::vector<const PartConnectionDefinition*> out;
    out.reserve(connections.size());
    for (auto& [id, d] : connections) out.push_back(&d);
    return out;
}

bool PartConnectionGraph::AreConnected(const std::string& partA, const std::string& partB) const {
    auto it = partIndex.find(partA);
    if (it == partIndex.end()) return false;
    for (auto& id : it->second) {
        auto* c = Find(id);
        if (c && ((c->partA == partA && c->partB == partB) || (c->partA == partB && c->partB == partA))) return true;
    }
    return false;
}

void PartConnectionGraph::Clear() { connections.clear(); partIndex.clear(); }

void PartConnectionGraph::RegisterNatives(VekScriptEngine& e) {
    e.RegisterNative("part_connect", [this](const std::vector<VekValue>& a) {
        std::string err; return VekValue(!a.empty() && ConnectValue(a[0], &err));
    });
    e.RegisterNative("part_disconnect", [this](const std::vector<VekValue>& a) {
        return VekValue(!a.empty() && Disconnect(a[0].AsString()));
    });
    e.RegisterNative("part_disconnect_parts", [this](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 2 && DisconnectParts(a[0].AsString(), a[1].AsString()));
    });
    e.RegisterNative("part_connections_for", [this](const std::vector<VekValue>& a) {
        VekValue arr = VekValue::Array();
        if (!a.empty()) for (auto* c : ConnectionsForPart(a[0].AsString())) arr.Push(ConnectionToValue(*c));
        return arr;
    });
    e.RegisterNative("part_connections_all", [this](const std::vector<VekValue>&) {
        VekValue arr = VekValue::Array();
        for (auto* c : All()) arr.Push(ConnectionToValue(*c));
        return arr;
    });
    e.RegisterNative("parts_are_connected", [this](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 2 && AreConnected(a[0].AsString(), a[1].AsString()));
    });
    e.RegisterNative("part_connection_count", [this](const std::vector<VekValue>&) { return VekValue((double)Size()); });
}

void VekRegisterMotorLibrary(VekScriptEngine& e, MotorBindingRegistry* bindings, PartConnectionGraph* graph) {
    if (bindings) bindings->RegisterNatives(e);
    if (graph) graph->RegisterNatives(e);
}

} // namespace vek
