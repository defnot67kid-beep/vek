#pragma once
#include <VekScriptEngine.h>
#include <string>
#include <vector>
struct JobDefinition {
 std::string id="delivery.workshop_supplies",name="Workshop Supplies",type="Delivery",objective="Deliver workshop supplies";
 int reward=500,xp=100,difficulty=2;float cargoMass=450;std::vector<std::string>recommended;
};
class VekJobRules{
public:bool Initialize(const std::string&path);bool Reload();const JobDefinition&Definition()const{return definition;}float RewardMultiplier(const std::string&mode,int reputation);const std::string&Error()const{return error;}
private:std::string file,error;JobDefinition definition;VekScriptEngine engine;bool Load();};
