#include "raylib.h"
#pragma once
#include <VekGameSystems.h>
#include <VekEditorSystems.h>
#include <VekScriptEngine.h>
#include <string>
#include <vector>

struct VehicleEditorGuiActions {
 std::string selectedPartId;
 std::string category;
 std::string search;
 std::string command;
};

class VekVehicleEditorGuiSystem {
public:
 bool Initialize(const std::string& scriptPath);
 bool Reload();
 VehicleEditorGuiActions UpdateAndDraw(vek::GameMode mode,const std::string& search,const std::string& category,const std::string& selected,const std::vector<const vek::PartDefinition*>& parts,int progressionLevel);
 bool MouseOverUI(Vector2 mouse)const;
 const std::string& Error()const{return error;}
private:
 VekScriptEngine engine;vek::GuiSystem gui;std::string file,error;std::string activeTextId;float scrollOffset=0;
 std::string cacheKey;VekValue cachedParts;
 bool Load();VekValue BuildPartValue(const vek::PartDefinition&,int level,vek::GameMode mode)const;
 VehicleEditorGuiActions Render(const std::string& currentSearch);
};
