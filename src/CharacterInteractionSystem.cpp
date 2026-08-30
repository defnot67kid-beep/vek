#include "CharacterInteractionSystem.h"
#include "raymath.h"

void CharacterInteractionSystem::EnterVehicle(PlayerCharacterSystem& c){c.Trigger(CharacterAnimState::EnterVehicle);}
void CharacterInteractionSystem::ExitVehicle(PlayerCharacterSystem& c){c.Trigger(CharacterAnimState::ExitVehicle);}
void CharacterInteractionSystem::OpenJobTablet(PlayerCharacterSystem& c){c.equipment.selected=EquipmentType::Tablet;c.Trigger(CharacterAnimState::Menu);}
void CharacterInteractionSystem::Inspect(PlayerCharacterSystem& c){c.equipment.selected=EquipmentType::Scanner;c.Trigger(CharacterAnimState::Inspect);}
void CharacterInteractionSystem::PlacePart(PlayerCharacterSystem& c){c.Trigger(CharacterAnimState::Place);}
bool CharacterInteractionSystem::Repair(PlayerCharacterSystem& c,Vehicle& v,Vector3 playerPosition){
    c.equipment.selected=EquipmentType::Wrench;
    c.Trigger(CharacterAnimState::Repair);
    if(!v.finalized||Vector3Distance(playerPosition,v.position)>=4.5f)return false;
    v.health=Clamp(v.health+15.0f,0.0f,100.0f);
    return true;
}
