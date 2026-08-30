#include "VekPartRegistrySystem.h"
#include "VekSecuritySystem.h"
#include <VekScriptEngine.h>
#include <VekEditorSystems.h>
#include <filesystem>
#include <algorithm>
#include "VekHostSecurity.h"
namespace fs=std::filesystem;
bool VekPartRegistrySystem::Initialize(const std::string&d){dir=d;return Reload();}
bool VekPartRegistrySystem::Reload(){vek::PartRegistry next;std::vector<fs::path>files;std::error_code ec;if(!fs::exists(dir)){error="VEK parts directory missing: "+dir;return false;}for(auto&e:fs::directory_iterator(dir,ec))if(e.is_regular_file()&&e.path().extension()==".vek")files.push_back(e.path());std::sort(files.begin(),files.end());std::size_t count=0;for(auto&path:files){auto verified=VekSecuritySystem::VerifyScript(path.string(),path.string()+".sig");if(!verified.ok){error=verified.error;return false;}VekScriptEngine vm;ApplyGameVekSecurityPolicy(vm);VekRegisterStandardLibrary(vm);vek::VekRegisterVehicleEditorLibrary(vm);next.RegisterNatives(vm);vm.SealNativeRegistry();if(!vm.LoadSource(verified.source,path.string())){error=vm.LastError();return false;}if(!vm.HasFunction("register_parts")){error="VEK part file missing register_parts(): "+path.string();return false;}vm.Call("register_parts");if(!vm.LastError().empty()){error=vm.LastError();return false;}count++;}if(next.Size()==0){error="VEK part registry loaded zero parts";return false;}registry=std::move(next);loadedScripts=count;error.clear();return true;}
