#include <vek/VekScriptEngine.h>
#include <vek/VekEditorSystems.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
int main(){
 VekScriptEngine vm;VekRegisterStandardLibrary(vm);
 const char* src=R"VEK(
struct EngineStats { power; torque; mass; }
event PlayerDamaged(amount) { return amount * 2; }
fn make(){
 let wheels=["road","offroad","racing"];
 let engine={ power:32000, mass:140, fuel:"petrol" };
 let stats=EngineStats { power:35000, torque:420, mass:130 };
 return wheels[1] + ":" + engine.fuel + ":" + stats.torque;
}
fn nested(){ let x={ a:{ b:7 } }; return x.a.b; }
)VEK";
 assert(vm.LoadSource(src));assert(vm.Call("make").AsString()=="offroad:petrol:420");assert(vm.Call("nested").AsNumber()==7);assert(vm.HasEvent("PlayerDamaged"));assert(vm.EmitEvent("PlayerDamaged",{5}).AsNumber()==10);
 auto tmp=std::filesystem::temp_directory_path()/"vek_v13_modules";std::filesystem::create_directories(tmp);std::ofstream(tmp/"math.vek")<<"fn imported_value(){ return 42; }\n";std::ofstream(tmp/"main.vek")<<"import \"math\";\nfn main(){ return imported_value(); }\n";VekScriptEngine modules;modules.SetModuleRoots({tmp.string()});assert(modules.LoadFile((tmp/"main.vek").string()));assert(modules.Call("main").AsNumber()==42);
 std::ofstream(tmp/"bad.vek")<<"import \"../not_allowed\";\nfn main(){return 1;}\n";VekScriptEngine bad;bad.SetModuleRoots({tmp.string()});assert(!bad.LoadFile((tmp/"bad.vek").string()));
 vek::PartRegistry reg;VekScriptEngine parts;VekRegisterStandardLibrary(parts);reg.RegisterNatives(parts);assert(parts.LoadSource(R"VEK(fn register_parts(){part_register({id:"engine.test",category:"Mechanical",display_name:"Test Engine",mass:100,price:200,size:vec3(1,1,1),components:[component("Engine",{power:25000,torque:300})],attachments:[attachment("mount","mechanical",vec3(0,-0.5,0),vec3(0,-1,0),["mechanical"])]});return part_count();})VEK"));assert(parts.Call("register_parts").AsNumber()==1);auto*p=reg.FindPart("engine.test");assert(p&&p->ComponentNumber("Engine","power")==25000);
 std::cout<<"VEK 1.3 arrays/maps/modules/structs/events/parts tests: PASS\n";
}
