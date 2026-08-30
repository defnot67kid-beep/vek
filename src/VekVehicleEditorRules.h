#pragma once
#include <VekScriptEngine.h>
#include <VekEditorSystems.h>
#include <string>
#include <vector>
class VekVehicleEditorRules {
public:
 bool Initialize(const std::string& path);bool ReloadScript();bool CanHotReload()const;
 float PartCost(vek::GameMode,float)const;bool PartUnlocked(vek::GameMode,int,int,bool)const;int MaxParts(vek::GameMode)const;bool PlacementValid(vek::GameMode,float,float,int)const;
 std::vector<float> GridSteps()const;vek::PlacementResult PlacementStatus(bool intersects,bool compatible,float supportDistance)const;bool AttachmentCompatible(const std::string&a,const std::string&b)const;bool FreePlacementAllowed(vek::GameMode)const;bool SymmetryAllowed(vek::GameMode)const;
 bool BuildValid(vek::GameMode,int,int,int,int,int,int,float)const;std::string BuildMessage(vek::GameMode,int,int,int,int,int,int,float)const;std::string PowerWarning(float,float)const;float UpgradeCost(vek::GameMode,float)const;bool InfiniteFuel(vek::GameMode)const;
 const std::string&ScriptError()const{return error;}
private:mutable VekScriptEngine engine;std::string file;mutable std::string error;bool loaded=false;bool LoadInternal();static std::string ModeName(vek::GameMode);
};
