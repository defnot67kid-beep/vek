#include <vek/VekScriptEngine.h>
#include <vek/VekDiagnosticsSystems.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <filesystem>
#include <iostream>

int main(){
    VekScriptEngine vm;VekRegisterStandardLibrary(vm);
    vek::VekDebugger dbg;dbg.Attach(true);dbg.AddFunctionBreakpoint("inner");vm.SetDebugger(&dbg);
    assert(vm.LoadSource("fn inner(){ return 1; } fn main(){ return inner(); }","debug.vek"));
    auto paused=vm.Call("main");(void)paused;
    assert(dbg.Paused());assert(dbg.PausedFunction()=="inner");assert(vm.LastError().find("VEK debugger")!=std::string::npos);
    dbg.Continue();
    assert(vm.Call("main").AsNumber()==1);
    dbg.RemoveFunctionBreakpoint("inner");
    assert(vm.LoadSource("fn line_test(){\n let x=7;\n return x;\n}\n","line_debug.vek"));
    dbg.AddLineBreakpoint("line_debug.vek",2);
    vm.Call("line_test");
    assert(dbg.Paused());assert(dbg.PauseReason()==vek::DebugPauseReason::LineBreakpoint);
    dbg.Continue();
    assert(vm.Call("line_test").AsNumber()==7);
    dbg.RemoveLineBreakpoint("line_debug.vek",2);
    assert(vm.LoadSource("fn inner(){ return 1/0; } fn main(){ return inner(); }","runtime_error.vek"));
    vm.Call("main");
    assert(!vm.LastError().empty());
    auto d=vm.LastDiagnostic();assert(d.domain==vek::DiagnosticDomain::Runtime);assert(!d.stack.empty());assert(d.stack.back().function=="inner");assert(d.line>0);
    auto tmp=std::filesystem::temp_directory_path()/"vek_27_crashlogs.txt";std::filesystem::remove(tmp);
    auto& crash=vek::VekCrashHandler::Instance();crash.Install(tmp.string());crash.SetContext({"VEK",VEK_VERSION_STRING,"test","diagnostics","runtime_error.vek","manual smoke"});assert(crash.WriteManualCrash("test crash report",d.stack));assert(std::filesystem::exists(tmp));crash.Uninstall();
    std::cout<<"VEK 2.7 diagnostics/debugger/crash tests: PASS\n";
}
