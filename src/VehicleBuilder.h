#pragma once
#include "Vehicle.h"
#include "Types.h"
#include <VekEditorSystems.h>
#include <deque>
#include <memory>
#include <set>
#include <string>
#include <vector>
class VekVehicleEditorRules;class VekVehicleEditorGuiSystem;

struct VehicleEngineeringStats{
 float mass=0,power=0,torque=0,powerToWeight=0,fuelCapacity=0,batteryCapacity=0,cargoCapacity=0,estimatedTopSpeed=0,estimatedAcceleration=0,estimatedRange=0,stability=0,wingArea=0,liftEstimate=0,thrust=0,buoyancy=0,buildCost=0;
 int partCount=0,wheelCount=0;Vector3 centerOfMass{0,0,0};Vector3 size{0,0,0};
};

class VehicleBuilder{
public:
 bool active=false;std::vector<VehiclePart> loose;
 void Configure(vek::GameMode,VekVehicleEditorRules*,EconomyState*,const vek::PartRegistry*,const vek::HangarBuildArea*,VekVehicleEditorGuiSystem*);
 void Toggle();void Update(const Camera3D&camera);void DrawWorld();void DrawUI();bool Finalize(Vehicle&vehicle);void Clear();
 Vector3 CursorPosition()const{return cursor;}bool CursorValid()const{return placement!=vek::PlacementResult::Invalid;}float EstimatedCost()const{return Stats().buildCost;}const std::string&StatusMessage()const{return status;}const std::string&SelectedPartId()const{return selectedPartId;}vek::GameMode Mode()const{return mode;}
 int SelectedPlacedIndex()const{return selectedPlaced;}Vector3 SelectedOrVehicleCenter()const;Vector3 VehicleCenter()const;Vector3 VehicleSize()const;VehicleEngineeringStats Stats()const;bool ConsumeConstructRequested();std::string ConsumeCameraCommand();
private:
 enum class ActionType{Place,Delete,Modify};struct EditorAction{ActionType type;int index;VehiclePart before,after;};
 Vector3 cursor{0,0.7f,0};float rotation=0,pitch=0,roll=0;vek::PlacementResult placement=vek::PlacementResult::Allowed;bool freePlacement=false,symmetryX=false,symmetryZ=false,showCOM=false,localTransform=false;std::size_t gridIndex=2;std::string selectedPartId="frame.chassis_basic",category="Structural",search;vek::GameMode mode=vek::GameMode::Survival;VekVehicleEditorRules*rules=nullptr;EconomyState*economy=nullptr;const vek::PartRegistry*registry=nullptr;const vek::HangarBuildArea*area=nullptr;VekVehicleEditorGuiSystem*gui=nullptr;int selectedPlaced=-1;mutable std::string status="Choose parts and engineer your vehicle.";std::vector<EditorAction>history;std::size_t historyCursor=0;std::set<std::string>favorites;std::deque<std::string>recent;bool constructRequested=false;std::string pendingCategory,pendingPart,pendingSearch,pendingCommand;
 const vek::PartDefinition*SelectedDefinition()const;std::vector<const vek::PartDefinition*>VisibleParts()const;VehiclePart MakePart(const vek::PartDefinition&,Vector3)const;vek::VehicleBuildCounts Counts()const;int ProgressionLevel()const;bool IsUnlocked(const vek::PartDefinition&)const;float GridStep()const;bool PointInEditorUI(Vector2)const;void SelectCategory(const std::string&);void CyclePart(int);void Record(EditorAction);void Undo();void Redo();void ApplyUndo(const EditorAction&);void ApplyRedo(const EditorAction&);void DeleteSelected();void DuplicateSelected();void MirrorSelected();void MoveSelected(Vector3);void RotateSelected(float);void PaintSelected();void SelectPlacedFromRay(Ray);void UpdateGhost(const Camera3D&);bool FindAttachmentSnap(const vek::PartDefinition&,Vector3&position,bool&compatible,float&supportDistance)const;bool GhostIntersects(const vek::PartDefinition&,Vector3 position)const;void DrawPart(const VehiclePart&,Color,bool wires=false)const;void DrawGrid()const;void DrawNodes()const;void ApplyPendingGuiActions();
};
