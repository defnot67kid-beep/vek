#include <vek/VekEditorSystems.h>
#include <vek/VekScriptEngine.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

namespace fs=std::filesystem;
static std::string Read(const fs::path&p){std::ifstream f(p,std::ios::binary);std::ostringstream s;s<<f.rdbuf();return s.str();}

int main(){
    vek::PartRegistry registry;
    std::vector<fs::path> files;
    fs::path dir=VEK_PART_SCRIPT_DIR;
    for(auto&e:fs::directory_iterator(dir))if(e.is_regular_file()&&e.path().extension()==".vek")files.push_back(e.path());
    std::sort(files.begin(),files.end());
    for(auto&path:files){
        VekScriptEngine vm;VekRegisterStandardLibrary(vm);vek::VekRegisterVehicleEditorLibrary(vm);registry.RegisterNatives(vm);
        assert(vm.LoadSource(Read(path),path.string()));assert(vm.HasFunction("register_parts"));vm.Call("register_parts");assert(vm.LastError().empty());
    }
    assert(registry.Size()>=28);
    for(auto&p:registry.All()){
        assert(!p.presentation.icon.empty());assert(!p.presentation.viewModel.empty());assert(!p.presentation.worldModel.empty());assert(!p.presentation.material.empty());
        assert(p.presentation.icon!="nil"&&p.presentation.viewModel!="nil"&&p.presentation.worldModel!="nil");
    }
    auto*seat=registry.FindPart("seat.driver");auto*fuel=registry.FindPart("tank.petrol_80");auto*battery=registry.FindPart("battery.120");auto*engine=registry.FindPart("engine.small_petrol");
    assert(seat&&fuel&&battery&&engine);
    assert(seat->presentation.viewModel=="builtin:seat.bucket");
    assert(fuel->presentation.viewModel=="builtin:fuel.tank80");
    assert(battery->presentation.viewModel=="builtin:battery.pack120");
    assert(engine->presentation.viewModel=="builtin:engine.petrol");
    assert(seat->presentation.viewModel!=fuel->presentation.viewModel);
    assert(seat->presentation.icon!=fuel->presentation.icon);
    std::cout<<"VEK part presentation registry tests: PASS ("<<registry.Size()<<" parts)\n";
}
