#include <vek/VekEditorSystems.h>
#include <vek/VekScriptEngine.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using namespace vek;
    auto survival=GameModeSystem::Policy(GameMode::Survival);
    auto sandbox=GameModeSystem::Policy(GameMode::Sandbox);
    assert(!survival.unlimitedParts && sandbox.unlimitedParts && sandbox.infiniteFuel);

    VehicleEditorSystem editor;
    assert(std::fabs(editor.Snap(1.24f)-1.0f)<0.001f);
    assert(std::fabs(editor.Snap(1.24f,true)-1.25f)<0.001f);
    assert(editor.PositionInsideBuildArea(12.0f,-12.0f));
    assert(!editor.PositionInsideBuildArea(13.0f,0.0f));

    auto engine=BuildPartCatalog::Find("engine_small");
    auto reactor=BuildPartCatalog::Find("sandbox_reactor");
    assert(engine && reactor);
    assert(editor.PartUnlocked(*engine,GameMode::Survival,1));
    assert(!editor.PartUnlocked(*reactor,GameMode::Survival,99));
    assert(editor.PartUnlocked(*reactor,GameMode::Sandbox,0));
    assert(editor.EffectivePartCost(*engine,GameMode::Sandbox)==0.0f);

    VehicleBuildCounts c; c.totalParts=7;c.structuralParts=1;c.wheels=4;c.engines=1;c.seats=1;c.mass=500;c.power=22000;
    auto valid=editor.Validate(c,GameMode::Survival); assert(valid.valid);
    c.seats=0; assert(!editor.Validate(c,GameMode::Survival).valid);

    VekScriptEngine vek; VekRegisterVehicleEditorLibrary(vek);
    assert(vek.LoadSource("fn test(){ return editor_part_unlocked(\"sandbox\", 0, 99, true) && editor_basic_valid(1,1,1,4,0); }"));
    assert(vek.Call("test").AsBool());

    std::cout << "VEK 1.2 editor/mode systems tests: PASS\n";
}
