#pragma once
#include "Character.h"
#include "Vehicle.h"

class CharacterInteractionSystem {
public:
    static void EnterVehicle(PlayerCharacterSystem& character);
    static void ExitVehicle(PlayerCharacterSystem& character);
    static void OpenJobTablet(PlayerCharacterSystem& character);
    static bool Repair(PlayerCharacterSystem& character, Vehicle& vehicle, Vector3 playerPosition);
    static void Inspect(PlayerCharacterSystem& character);
    static void PlacePart(PlayerCharacterSystem& character);
};
