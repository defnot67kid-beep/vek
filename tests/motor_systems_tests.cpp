#include <VekMotorSystems.h>
#include <VekScriptEngine.h>
#include <cassert>
#include <cmath>

int main() {
    // --- Native C++ API: keybind + script + network bindings -----------------
    vek::MotorBindingRegistry motors;

    vek::MotorBindingDefinition keyBind;
    keyBind.id = "wheel.left.key";
    keyBind.motorId = "wheel.left.motor";
    keyBind.source = vek::MotorInputSource::Keybind;
    keyBind.keyCode = "KeyA";
    keyBind.invert = -1.0f;
    assert(motors.Register(keyBind));

    vek::MotorBindingDefinition scriptBind;
    scriptBind.id = "wheel.left.script";
    scriptBind.motorId = "wheel.left.motor";
    scriptBind.source = vek::MotorInputSource::Script;
    scriptBind.scriptFunction = "on_wheel_throttle";
    scriptBind.scriptScope = vek::MotorScriptScope::Local;
    assert(motors.Register(scriptBind));

    vek::MotorBindingDefinition remoteBind;
    remoteBind.id = "wheel.left.remote";
    remoteBind.motorId = "wheel.left.motor";
    remoteBind.source = vek::MotorInputSource::Network;
    remoteBind.remoteActionId = "vehicle.set_throttle";
    remoteBind.scriptScope = vek::MotorScriptScope::RemoteServer;
    assert(motors.Register(remoteBind));

    // Duplicate id rejected.
    assert(!motors.Register(keyBind));
    assert(motors.Size() == 3);
    assert(motors.BindingsForMotor("wheel.left.motor").size() == 3);
    assert(motors.BindingsForKey("KeyA").size() == 1);

    auto cmd = motors.Evaluate("wheel.left.key", 0.0f, true);
    assert(cmd.valid && cmd.active);
    assert(std::fabs(cmd.targetVelocity - (-1.0f)) < 1e-5f); // inverted keybind

    // Rebind at runtime (settings-screen style rebinding).
    assert(motors.Rebind("wheel.left.key", "KeyQ"));
    assert(motors.BindingsForKey("KeyA").empty());
    assert(motors.BindingsForKey("KeyQ").size() == 1);

    assert(motors.Unregister("wheel.left.script"));
    assert(motors.BindingsForMotor("wheel.left.motor").size() == 2);

    // --- Part connection graph: wiring / linkage visualization ---------------
    vek::PartConnectionGraph graph;
    vek::PartConnectionDefinition joint;
    joint.id = "chassis-to-wheel.left";
    joint.partA = "chassis";
    joint.partB = "wheel.left";
    joint.kind = "joint";
    joint.jointId = "wheel.left.joint";
    joint.motorId = "wheel.left.motor";
    assert(graph.Connect(joint));

    vek::PartConnectionDefinition wire;
    wire.id = "remote-to-motor";
    wire.partA = "remote.controller";
    wire.partB = "wheel.left.motor";
    wire.kind = "wire";
    wire.label = "throttle signal";
    assert(graph.Connect(wire));

    assert(!graph.Connect(joint)); // duplicate id rejected
    assert(graph.Size() == 2);
    assert(graph.AreConnected("chassis", "wheel.left"));
    assert(graph.AreConnected("wheel.left", "chassis")); // order-independent
    assert(!graph.AreConnected("chassis", "remote.controller"));
    assert(graph.ConnectionsForPart("wheel.left.motor").size() == 1);
    assert(graph.All().size() == 2);

    assert(graph.Disconnect("remote-to-motor"));
    assert(graph.Size() == 1);
    assert(graph.ConnectionsForPart("remote.controller").empty());

    // --- Script-facing natives (proves cross-language reachability: every ---
    // language binding calls through VekScriptEngine natives via the C ABI). -
    VekScriptEngine vm;
    VekRegisterStandardLibrary(vm);
    vek::MotorBindingRegistry vmMotors;
    vek::PartConnectionGraph vmGraph;
    vek::VekRegisterMotorLibrary(vm, &vmMotors, &vmGraph);

    bool loaded = vm.LoadSource(R"(
        fn setup() {
            motor_binding_register({
                id: "throttle.key",
                motor_id: "engine.motor",
                source: "keybind",
                key_code: "KeyW",
                invert: 1
            });
            part_connect({
                id: "engine-link",
                part_a: "chassis",
                part_b: "engine.motor",
                kind: "motor"
            });
            return motor_binding_count() + part_connection_count();
        }
    )");
    assert(loaded);
    auto result = vm.Call("setup");
    assert(result.AsNumber() == 2.0);
    assert(vmMotors.Size() == 1);
    assert(vmGraph.Size() == 1);
    assert(vmGraph.AreConnected("chassis", "engine.motor"));

    return 0;
}
