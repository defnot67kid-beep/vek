#include "VekCharacterRules.h"
#include "HumanoidSystem.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    VekCharacterRules rules;
    assert(rules.Initialize("scripts/player_systems.vek"));

    HumanoidSystem h;
    h.SetRuleProvider(&rules);
    h.Reset();
    assert(h.BeginJump());
    float y=1.0f;
    bool landed=false;
    for(int i=0;i<1200 && !landed;++i) landed=h.UpdateVertical(1.0f/120.0f,1.0f,y);
    assert(landed);
    assert(h.GetLastLandingSpeed()>0.0f);

    h.ApplyDamage(25.0f);
    assert(std::fabs(h.GetHealth()-75.0f)<0.001f);
    h.Heal(10.0f);
    assert(std::fabs(h.GetHealth()-85.0f)<0.001f);
    assert(rules.FallDamage(10.0f)==0.0f);
    assert(rules.FallDamage(16.0f)>0.0f);
    assert(rules.ShouldRagdoll(50.0f,16.0f,10.0f));
    std::cout << "VEK character gameplay rules tests: PASS\n";
}
