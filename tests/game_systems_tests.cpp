#include <VekScriptEngine.h>
#include <VekGameSystems.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using namespace vek;

    GravityState gravity;
    GravitySettings gs;
    float y = 1.0f;
    assert(GravitySystem::BeginJump(gravity, 6.4f));
    bool landed = false;
    for (int i=0; i<600 && !landed; ++i) landed = GravitySystem::Step(gravity, gs, 1.0f/120.0f, 1.0f, y);
    assert(landed);
    assert(gravity.grounded);
    assert(gravity.lastLandingSpeed > 0.0f);

    HealthState health;
    HealthSettings hs;
    HealthSystem::Reset(health, hs);
    HealthSystem::ApplyDamage(health, hs, 30.0f);
    assert(std::fabs(health.health - 70.0f) < 0.001f);
    HealthSystem::Heal(health, hs, 15.0f);
    assert(std::fabs(health.health - 85.0f) < 0.001f);

    RagdollSettings rs;
    RagdollState ragdoll;
    assert(RagdollSystem::ShouldTrigger(60.0f, 16.0f, 10.0f, rs));
    RagdollSystem::Trigger(ragdoll, 16.0f, 1.0f, rs);
    assert(RagdollSystem::Active(ragdoll));
    for (int i=0; i<1000 && RagdollSystem::Active(ragdoll); ++i) RagdollSystem::Update(ragdoll, 1.0f/120.0f, rs);
    assert(!RagdollSystem::Active(ragdoll));

    VekScriptEngine vm;
    VekRegisterStandardLibrary(vm);
    VekRegisterGameplayLibrary(vm);
    const char* source = R"VEK(
fn test_gravity() { return gravity_step(0, 10, 30, 0.5, false); }
fn test_health() { return health_damage(100, 100, 25); }
fn test_ragdoll() { return ragdoll_should_trigger(50, 15, 10, 13, 25); }
)VEK";
    assert(vm.LoadSource(source));
    assert(std::fabs(vm.Call("test_gravity").AsNumber() + 1.0) < 0.001);
    assert(std::fabs(vm.Call("test_health").AsNumber() - 75.0) < 0.001);
    assert(vm.Call("test_ragdoll").AsBool());

    GuiSystem gui;
    VekScriptEngine guiVm;
    VekRegisterStandardLibrary(guiVm);
    gui.RegisterNatives(guiVm);
    assert(guiVm.LoadSource(R"VEK(
fn draw() {
  gui_begin_window("w", "Test", 0, 0, 400, 300);
  gui_label("Hello");
  gui_button("ok", "OK");
  gui_end_window();
}
)VEK"));
    gui.BeginFrame();
    guiVm.Call("draw");
    assert(gui.Commands().size() == 4);

    std::cout << "VEK 1.1 gameplay systems tests: PASS\n";
}
