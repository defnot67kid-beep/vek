#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <vek/VekScriptEngine.h>

class VekScriptEngine;

namespace vek {

// Motor & Wiring Systems (VEK 3.4) -------------------------------------------
// This module answers three related questions that sit on top of the existing
// JointDefinition/MotorDefinition pair in VekPhysicsSystems.h:
//   1. What drives a motor's target velocity right now (a keybind, an analog
//      axis, a local script call, or a server-authoritative remote command)?
//   2. Which parts are connected to which other parts, so an editor/HUD can
//      draw the wiring/linkage graph the same way a host would draw a node
//      graph of joints, welds and sockets?
//   3. How does that stay usable from any language? Every operation here is
//      exposed as script natives via RegisterNatives(), and the existing C
//      ABI (vek_c.h) already forwards script natives to every language
//      binding (C#, Go, Java, Node, Python, Rust) without new per-language
//      glue, so this module is "multi-language" the same way the rest of VEK
//      is: bind once, call from anywhere that can load vek.

// What is currently driving a motor. Local/RemoteServer describe *where the
// decision is made*, not the transport: a Script binding calls a function in
// whichever script (local .vek/.lua file or embedder-provided module) is
// loaded into the engine, while a Network binding is satisfied by a remote
// command arriving through the existing AuthorityActionRegistry pipeline
// (see VekAuthoritySystems.h) so server scripts remain the source of truth
// for server-authoritative motors.
enum class MotorInputSource { None = 0, Keybind = 1, Axis = 2, Script = 3, Network = 4 };
enum class MotorScriptScope { Local = 0, RemoteServer = 1 };

// A single input->motor binding. One motor can have multiple bindings (e.g. a
// keybind for a discrete nudge and an axis for analog throttle); bindings are
// looked up and evaluated independently, and each carries its own id so a
// host UI can list/rebind/remove them individually.
struct MotorBindingDefinition {
    std::string id;
    std::string motorId;          // id of the JointDefinition/motor being driven
    MotorInputSource source = MotorInputSource::Keybind;

    // Keybind/Axis input.
    std::string keyCode;          // e.g. "KeyE", "GamepadRT"; used when source==Keybind
    std::string axisId;           // logical axis name; used when source==Axis
    bool holdToActivate = true;   // false = press toggles the motor on/off

    // Script input.
    std::string scriptFunction;   // function invoked when this binding fires
    MotorScriptScope scriptScope = MotorScriptScope::Local;

    // Network/remote input. Reuses the authority action pipeline so a
    // "remote script" (dedicated server, listen server) stays authoritative
    // over the motor instead of trusting the client's local value.
    std::string remoteActionId;

    float invert = 1.0f;          // -1 flips direction without new bindings
    float sensitivity = 1.0f;     // scales incoming axis/analog values
    float minValue = -1.0f;
    float maxValue = 1.0f;
};

// Result of feeding raw input into a binding: the concrete command a physics
// backend (or the authority layer, for Network bindings) should apply.
struct MotorCommand {
    bool valid = false;
    std::string motorId;
    std::string bindingId;
    float targetVelocity = 0.0f;
    bool active = false;
    MotorInputSource source = MotorInputSource::None;
    MotorScriptScope scriptScope = MotorScriptScope::Local;
    std::string scriptFunction;
    std::string remoteActionId;
};

class MotorBindingRegistry {
public:
    bool Register(const MotorBindingDefinition& definition);
    bool RegisterValue(const VekValue& value, std::string* error = nullptr);
    bool Unregister(const std::string& id);
    const MotorBindingDefinition* Find(const std::string& id) const;
    std::vector<const MotorBindingDefinition*> BindingsForMotor(const std::string& motorId) const;
    std::vector<const MotorBindingDefinition*> BindingsForKey(const std::string& keyCode) const;

    // Rebinding at runtime, e.g. from a "press any key" settings screen.
    bool Rebind(const std::string& id, const std::string& newKeyCode);

    // Evaluate a single binding against a raw input sample. `rawValue` is the
    // axis/analog magnitude (ignored for simple keybinds, where 1.0/0.0 is
    // implied by `active`). Returns an invalid command if the binding id is
    // unknown or the value is out of range.
    MotorCommand Evaluate(const std::string& bindingId, float rawValue, bool active) const;

    void Clear();
    std::size_t Size() const { return bindings.size(); }
    void RegisterNatives(VekScriptEngine& engine);

private:
    std::unordered_map<std::string, MotorBindingDefinition> bindings;
    std::unordered_map<std::string, std::vector<std::string>> motorIndex; // motorId -> binding ids
    std::unordered_map<std::string, std::vector<std::string>> keyIndex;   // keyCode -> binding ids
};

// A connection between two parts: a joint, weld, wire or socket. This is
// intentionally decoupled from the physics solver's JointDefinition — a
// connection can exist purely to describe wiring/linkage for an editor or HUD
// (e.g. "this remote is wired to that motor") even when no physical joint
// backs it, and a physical joint can also register a matching connection so
// the same graph drives both the solver and the visualization.
struct PartConnectionDefinition {
    std::string id;
    std::string partA;
    std::string partB;
    std::string kind = "joint";  // "joint" | "weld" | "wire" | "socket" | "motor" | "fuel_line"
    std::string jointId;         // optional: matching VekPhysicsSystems JointDefinition id
    std::string motorId;         // optional: motor this connection carries commands for
    bool visible = true;         // whether a host editor should draw this link
    std::string label;           // optional human-readable annotation for the graph view
};

class PartConnectionGraph {
public:
    bool Connect(const PartConnectionDefinition& definition);
    bool ConnectValue(const VekValue& value, std::string* error = nullptr);
    bool Disconnect(const std::string& id);
    bool DisconnectParts(const std::string& partA, const std::string& partB);

    const PartConnectionDefinition* Find(const std::string& id) const;
    std::vector<const PartConnectionDefinition*> ConnectionsForPart(const std::string& partId) const;
    std::vector<const PartConnectionDefinition*> All() const;
    bool AreConnected(const std::string& partA, const std::string& partB) const;

    void Clear();
    std::size_t Size() const { return connections.size(); }
    void RegisterNatives(VekScriptEngine& engine);

private:
    std::unordered_map<std::string, PartConnectionDefinition> connections;
    std::unordered_map<std::string, std::vector<std::string>> partIndex; // partId -> connection ids
};

// Registers both registries' natives in one call, matching the pattern used
// by VekRegisterAuthorityLibrary for the authority/replication pair.
void VekRegisterMotorLibrary(VekScriptEngine& engine, MotorBindingRegistry* bindings, PartConnectionGraph* graph);

} // namespace vek
