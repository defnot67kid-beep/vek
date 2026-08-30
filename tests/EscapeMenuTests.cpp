#include "EscapeMenuSystem.h"
#include <cassert>
#include <iostream>

int main() {
    EscapeMenuSystem menu;
    assert(!menu.IsOpen());
    menu.Open();
    assert(menu.IsOpen());
    menu.Close();
    assert(!menu.IsOpen());
    menu.Toggle();
    assert(menu.IsOpen());
    menu.Toggle();
    assert(!menu.IsOpen());

    // Alt+F4 policy: first fresh press opens; second requests Leave.
    assert(menu.HandleAltF4() == EscapeMenuAction::None);
    assert(menu.IsOpen());
    assert(menu.HandleAltF4() == EscapeMenuAction::Leave);

    std::cout << "EscapeMenuSystem state + Alt+F4 tests: PASS\n";
    return 0;
}
