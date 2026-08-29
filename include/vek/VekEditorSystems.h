#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <vek/VekScriptEngine.h>

class VekScriptEngine;

namespace vek {

enum class GameMode { Survival=0, Sandbox=1 };
struct GameModePolicy { GameMode mode=GameMode::Survival; bool unlimitedMoney=false,unlimitedParts=false,allPartsUnlocked=false,infiniteFuel=false,freeRepairs=false,progressionEnabled=true,jobsEnabled=true; };
class GameModeSystem { public: static GameModePolicy Policy(GameMode mode); static const char* Name(GameMode mode); };

enum class BuildPartCategory { Structural=0, Movement, Mechanical, Functional, Aircraft, Marine, Experimental };

// Backward-compatible catalog used by older VEK 1.2 hosts. New hosts should use
// PartRegistry and string IDs instead of adding new enum values in C++.
struct BuildPartDefinition {
    int id=0; std::string key,displayName; BuildPartCategory category=BuildPartCategory::Structural;
    float mass=0,cost=0,power=0; int unlockLevel=1; bool sandboxOnly=false,isWheel=false,isEngine=false,isSeat=false,isStructural=false;
};
class BuildPartCatalog { public: static const std::vector<BuildPartDefinition>& All(); static const BuildPartDefinition* Find(int id); static const BuildPartDefinition* Find(const std::string& key); static std::vector<const BuildPartDefinition*> Category(BuildPartCategory category); };

struct VekVec3 { float x=0,y=0,z=0; };
struct PartAttachmentNode {
    std::string name;
    std::string type;
    VekVec3 position;
    VekVec3 direction{0,1,0};
    std::vector<std::string> compatibleTypes;
};
struct PartComponentDefinition { std::string type; VekMap properties; };
struct PartDefinition {
    std::string id;
    std::string category="Structural";
    std::string subcategory;
    std::string displayName;
    std::string description;
    std::string visual="box";
    VekVec3 size{1,1,1};
    float mass=0;
    float price=0;
    float durability=100;
    int unlockLevel=1;
    std::string requiredTechnology;
    bool sandboxOnly=false;
    std::vector<std::string> tags;
    std::vector<PartComponentDefinition> components;
    std::vector<PartAttachmentNode> attachments;

    bool HasTag(const std::string& tag) const;
    const PartComponentDefinition* Component(const std::string& type) const;
    float ComponentNumber(const std::string& component,const std::string& key,float fallback=0) const;
    std::string ComponentString(const std::string& component,const std::string& key,const std::string& fallback={}) const;
};

class PartRegistry {
public:
    bool RegisterPart(const PartDefinition& part);
    bool RegisterPartValue(const VekValue& definition,std::string* error=nullptr);
    const PartDefinition* FindPart(const std::string& id) const;
    std::vector<const PartDefinition*> GetCategory(const std::string& category) const;
    std::vector<const PartDefinition*> Search(const std::string& query,const std::string& category={}) const;
    const std::vector<PartDefinition>& All() const;
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::vector<PartDefinition> parts;
    std::unordered_map<std::string,std::size_t> index;
};

struct HangarBuildArea {
    VekVec3 center{0,0,0};
    VekVec3 size{60,30,90};
    float gridSize=0.25f;
    float maxBuildHeight=30.0f;
    int maxParts=800;
    bool allowLargeVehicles=true;
    bool allowAircraft=true;
};

enum class PlacementResult { Allowed=0, Warning=1, Invalid=2 };

struct VehicleBuildCounts { int totalParts=0,structuralParts=0,wheels=0,engines=0,seats=0,movementParts=0; float mass=0,power=0,estimatedCost=0; };
struct BuildValidationResult { bool valid=false; std::string primaryMessage; std::vector<std::string>warnings; };
struct VehicleEditorSettings { float gridSize=0.5f,fineGridSize=0.25f,rotationStepDegrees=15;int survivalMaxParts=96,sandboxMaxParts=512;float buildRadius=12;bool allowFreePlacement=true,allowMirror=true,allowUndoRedo=true; };
class VehicleEditorSystem { public: explicit VehicleEditorSystem(VehicleEditorSettings settings={}); const VehicleEditorSettings& Settings()const;void SetSettings(const VehicleEditorSettings&);float Snap(float value,bool fineGrid=false)const;bool PositionInsideBuildArea(float x,float z)const;int MaxParts(GameMode)const;bool PartUnlocked(const BuildPartDefinition&,GameMode,int)const;float EffectivePartCost(const BuildPartDefinition&,GameMode)const;BuildValidationResult Validate(const VehicleBuildCounts&,GameMode)const;private:VehicleEditorSettings settings;};

void VekRegisterVehicleEditorLibrary(VekScriptEngine& engine);

} // namespace vek
