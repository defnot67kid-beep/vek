#pragma once
#include <VekEditorSystems.h>
#include <string>
class VekHangarRules {
public:
 bool Initialize(const std::string& path);bool Reload();const vek::HangarBuildArea& Area()const{return area;}const std::string&Error()const{return error;}
private:std::string file,error;vek::HangarBuildArea area;bool Load();};
