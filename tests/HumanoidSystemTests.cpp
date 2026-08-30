#include "HumanoidSystem.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    HumanoidSystem h;
    h.Reset();
    assert(h.IsAlive());
    assert(h.IsGrounded());
    assert(std::fabs(h.GetHealth()-100.0f) < 0.001f);

    h.ApplyDamage(70.0f);
    assert(std::fabs(h.GetHealth()-30.0f) < 0.001f);
    assert(h.IsHurt());
    h.Heal(20.0f);
    assert(std::fabs(h.GetHealth()-50.0f) < 0.001f);
    assert(!h.IsHurt());

    float y = 1.0f;
    assert(h.BeginJump());
    assert(!h.IsGrounded());
    bool landed = false;
    for (int i=0; i<600 && !landed; ++i)
        landed = h.UpdateVertical(1.0f/120.0f,1.0f,y);
    assert(landed);
    assert(h.IsGrounded());
    assert(std::fabs(y-1.0f) < 0.001f);
    assert(h.GetLastLandingSpeed() > 0.0f);

    h.ApplyDamage(1000.0f);
    assert(!h.IsAlive());
    assert(!h.BeginJump());

    std::cout << "HumanoidSystem tests: PASS\n";
    return 0;
}
