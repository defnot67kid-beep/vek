#include "VekScriptEngine.h"
#include <cmath>
#include <iostream>

static int failures = 0;
static void Check(bool condition, const char* name) {
    if (!condition) { std::cerr << "FAIL: " << name << "\n"; ++failures; }
}

int main() {
    VekScriptEngine vek;
    double captured = 0.0;
    vek.RegisterNative("capture", [&](const std::vector<VekValue>& args) -> VekValue {
        if (!args.empty()) captured = args[0].AsNumber();
        return {};
    });

    const char* source = R"VEK(
        fn math_test(a, b) {
            let result = a + b * 2;
            return result;
        }

        fn branch_test(speed) {
            if speed > 7.5 {
                let damage_value = (speed - 7.5) * 1.25;
                capture(damage_value);
                return true;
            } else {
                capture(0);
                return false;
            }
        }
    )VEK";

    Check(vek.LoadSource(source, "unit-test.vek"), "VEK parses functions/let/if/arithmetic");
    Check(std::fabs(vek.Call("math_test", {3.0, 4.0}).AsNumber() - 11.0) < 0.0001, "operator precedence");
    Check(vek.Call("branch_test", {10.0}).AsBool(), "if true branch");
    Check(std::fabs(captured - 3.125) < 0.0001, "native call receives calculated value");
    Check(!vek.Call("branch_test", {5.0}).AsBool(), "else branch");
    Check(std::fabs(captured) < 0.0001, "else native call");

    VekScriptEngine broken;
    Check(!broken.LoadSource("fn broken( {", "broken.vek"), "invalid syntax rejected");
    Check(!broken.LastError().empty(), "syntax error reported");

    if (failures == 0) {
        std::cout << "VEK language tests: PASS\n";
        return 0;
    }
    std::cout << "VEK language tests: FAIL (" << failures << ")\n";
    return 1;
}
