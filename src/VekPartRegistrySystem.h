#pragma once
#include <VekEditorSystems.h>
#include <string>
#include <vector>
class VekPartRegistrySystem {
public:
 bool Initialize(const std::string& directory);
 bool Reload();
 const vek::PartRegistry& Registry() const{return registry;}
 vek::PartRegistry& Registry(){return registry;}
 const std::string& Error()const{return error;}
 std::size_t LoadedScriptCount()const{return loadedScripts;}
private:
 std::string dir,error;std::size_t loadedScripts=0;vek::PartRegistry registry;
};
