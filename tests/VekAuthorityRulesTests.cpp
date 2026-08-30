#include "VekAuthorityRules.h"
#include <cassert>
#include <iostream>
#ifndef VEK_TEST_AUTHORITY_SCRIPT
#define VEK_TEST_AUTHORITY_SCRIPT "scripts/security_authority.vek"
#endif
int main(){
    VekAuthorityRules rules;
    assert(rules.Initialize(VEK_TEST_AUTHORITY_SCRIPT));
    assert(rules.Actions().Find("economy.purchase"));
    assert(rules.Actions().Find("vehicle.finalize"));
    assert(rules.Replication().Find("replication.player"));
    assert(!rules.Shell().showHelpOverlay);
    assert(!rules.Shell().showMapInstructionBar);
    assert(rules.Shell().performanceToggleKey=="F");
    assert(rules.Shell().cameraFloorClearance>=0.3f);
    std::cout<<"Game VEK authority/shell policy tests: PASS\n";
}
