#pragma once
#include "raylib.h"
#include "GuardedValue.h"
#include <vector>
#include <string>

// Legacy IDs remain only for loading old blueprints and rendering compatibility.
// New VEK-defined parts use string IDs and PartType::Dynamic; adding a new part
// no longer requires adding an enum value here.
enum class PartType {
    Chassis=0,Wheel=1,Engine=2,Seat=3,Frame=4,Beam=5,Plate=6,RollCage=7,Wing=8,OffroadWheel=9,Track=10,Propeller=11,Rotor=12,Thruster=13,DieselEngine=14,ElectricMotor=15,FuelTank=16,Battery=17,Suspension=18,Steering=19,Light=20,CargoBox=21,TowHook=22,Winch=23,ToolBox=24,JetEngine=25,HoverPad=26,SandboxReactor=27,Dynamic=1000
};
inline bool PartIsWheel(PartType t){return t==PartType::Wheel||t==PartType::OffroadWheel||t==PartType::Track;}
inline bool PartIsStructural(PartType t){return t==PartType::Chassis||t==PartType::Frame||t==PartType::Beam||t==PartType::Plate||t==PartType::RollCage||t==PartType::Wing;}
inline bool PartIsPowered(PartType t){return t==PartType::Engine||t==PartType::DieselEngine||t==PartType::ElectricMotor||t==PartType::Thruster||t==PartType::JetEngine||t==PartType::HoverPad||t==PartType::SandboxReactor;}
inline bool PartIsMovement(PartType t){return PartIsWheel(t)||t==PartType::Propeller||t==PartType::Rotor||t==PartType::Thruster||t==PartType::HoverPad;}

struct VehiclePart {
    PartType type=PartType::Dynamic;
    int legacyType=-1;
    std::string partId;
    std::string displayName;
    std::string category;
    std::string visual="box"; // legacy VEK fallback
    std::string icon="box";
    std::string viewModel="box";
    std::string worldModel="box";
    std::string material="default";
    Vector3 viewScale{1,1,1};
    Vector3 viewRotation{0,0,0};
    Vector3 viewOffset{0,0,0};
    bool allowTint=true;
    Vector3 localPosition{0,0,0};
    Vector3 size{1,1,1};
    float yaw=0,pitch=0,roll=0;
    float mass=0,power=0,torque=0,cost=0,durability=100;
    float traction=0,fuelCapacity=0,batteryCapacity=0,cargoCapacity=0;
    float wingArea=0,liftEstimate=0,drag=0,thrust=0,buoyancy=0;
    bool isWheel=false,isStructural=false,isPowered=false,isSeat=false,isMovement=false;
    unsigned char paintR=150,paintG=160,paintB=165;
};
inline bool PartIsWheel(const VehiclePart&p){return p.isWheel||PartIsWheel(p.type);}inline bool PartIsStructural(const VehiclePart&p){return p.isStructural||PartIsStructural(p.type);}inline bool PartIsPowered(const VehiclePart&p){return p.isPowered||PartIsPowered(p.type)||p.power>0||p.thrust>0;}inline bool PartIsMovement(const VehiclePart&p){return p.isMovement||PartIsMovement(p.type)||p.isWheel;}

struct PlayerState { Vector3 position{0.0f,0.15f,-8.0f}; float yaw=0.0f; bool driving=false; };
struct EconomyState { GuardedInt money=1500;GuardedInt xp=0;GuardedInt reputation=0;bool IntegrityOK()const{return money.IntegrityOK()&&xp.IntegrityOK()&&reputation.IntegrityOK();} };
