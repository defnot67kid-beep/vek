#include <vek/VekScriptEngine.h>
#include <cassert>
#include <string>

#ifndef VEK_EXPECTED_VERSION
#error VEK_EXPECTED_VERSION must be provided by CMake
#endif

int main() {
    const std::string runtime = VEK_VERSION_STRING;
    const std::string expected = VEK_EXPECTED_VERSION;
    assert(runtime == expected);
    assert(VEK_VERSION_MAJOR >= 0);
    assert(VEK_VERSION_MINOR >= 0);
    assert(VEK_VERSION_PATCH >= 0);
    return 0;
}
