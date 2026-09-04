#include <VekScriptEngine.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>

int main(){
    VekScriptEngine vm;
    const char* src=R"VEK(
fn factorial(n) {
    let result = 1;
    let i = 2;
    while i <= n {
        result = result * i;
        i = i + 1;
    }
    return result;
}
fn choose(x) {
    if x < 0 { return "negative"; }
    else if x == 0 { return "zero"; }
    else { return "positive"; }
}
fn loop_control() {
    let i = 0;
    let sum = 0;
    while i < 10 {
        i = i + 1;
        if i == 3 { continue; }
        if i == 7 { break; }
        sum = sum + i;
    }
    return sum;
}
fn main(){ return factorial(6); }
)VEK";
    assert(vm.LoadSource(src));
    assert(vm.Call("factorial",{6}).AsNumber()==720.0);
    assert(vm.Call("choose",{-1}).AsString()=="negative");
    assert(vm.Call("choose",{0}).AsString()=="zero");
    assert(vm.Call("choose",{2}).AsString()=="positive");
    assert(vm.Call("loop_control").AsNumber()==18.0);
    assert(vm.Call("main").AsNumber()==720.0);
    VekScriptEngine bad; assert(!bad.LoadSource("fn x(){ let a = ; }"));
    std::cout<<"VEK 1.1 language tests: PASS\n";
}
