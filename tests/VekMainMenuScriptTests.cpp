#include <VekScriptEngine.h>
#include <VekGameSystems.h>
#include <VekEditorSystems.h>
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>

#ifndef VEK_TEST_MENU_SCRIPT_PATH
#define VEK_TEST_MENU_SCRIPT_PATH "scripts/main_menu.vek"
#endif

int main(){
    std::ifstream in(VEK_TEST_MENU_SCRIPT_PATH); std::ostringstream ss; ss<<in.rdbuf();
    VekScriptEngine e; vek::VekRegisterGameplayLibrary(e); vek::VekRegisterVehicleEditorLibrary(e); vek::GuiSystem gui; gui.RegisterNatives(e); e.SealNativeRegistry();
    assert(e.LoadSource(ss.str(),VEK_TEST_MENU_SCRIPT_PATH));
    gui.BeginFrame(); gui.SetPressed("survival",true); auto s=e.Call("main_menu"); gui.EndFrame(); assert(s.AsString()=="survival");
    gui.BeginFrame(); gui.SetPressed("sandbox",true); auto b=e.Call("main_menu"); gui.EndFrame(); assert(b.AsString()=="sandbox");
    std::cout<<"VEK main-menu GUI script tests: PASS\n";
}
