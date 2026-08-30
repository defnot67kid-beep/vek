#include "VekVehicleEditorRules.h"
#include <cassert>
#include <iostream>

#ifndef VEK_TEST_EDITOR_SCRIPT_PATH
#define VEK_TEST_EDITOR_SCRIPT_PATH "scripts/vehicle_editor.vek"
#endif

int main(){
    VekVehicleEditorRules r;
    assert(r.Initialize(VEK_TEST_EDITOR_SCRIPT_PATH));
    assert(r.PartCost(vek::GameMode::Sandbox,400.0f)==0.0f);
    assert(r.PartCost(vek::GameMode::Survival,400.0f)==400.0f);
    assert(r.PartUnlocked(vek::GameMode::Sandbox,0,99,true));
    assert(!r.PartUnlocked(vek::GameMode::Survival,1,5,false));
    assert(r.MaxParts(vek::GameMode::Sandbox)>=512);
    assert(r.PlacementValid(vek::GameMode::Survival,2,2,3));
    assert(!r.PlacementValid(vek::GameMode::Survival,20,2,3));
    assert(r.BuildValid(vek::GameMode::Survival,1,1,1,4,4,5000,1000));
    assert(!r.BuildValid(vek::GameMode::Survival,1,1,0,4,4,5000,1000));
    assert(r.InfiniteFuel(vek::GameMode::Sandbox));
    assert(!r.InfiniteFuel(vek::GameMode::Survival));
    std::cout<<"VEK vehicle-editor rules tests: PASS\n";
}
