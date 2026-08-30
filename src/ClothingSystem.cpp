#include "ClothingSystem.h"

const char* ClothingSystem::Name(ClothingStyle s) {
    switch(s) {
        case ClothingStyle::WorkShirt: return "Work Shirt";
        case ClothingStyle::Tshirt: return "T-Shirt";
        case ClothingStyle::Overalls: return "Overalls";
        case ClothingStyle::Utility: return "Utility Trousers";
        case ClothingStyle::Jacket: return "Workshop Jacket";
        case ClothingStyle::SafetyVest: return "Safety Vest";
        case ClothingStyle::PilotGear: return "Pilot Gear";
        case ClothingStyle::MarineGear: return "Marine Gear";
        case ClothingStyle::IndustrialGear: return "Industrial Gear";
        case ClothingStyle::CompanyUniform: return "Company Uniform";
    }
    return "Outfit";
}
int ClothingSystem::RequiredReputation(ClothingStyle s) {
    switch(s) {
        case ClothingStyle::Jacket: return 2;
        case ClothingStyle::SafetyVest: return 3;
        case ClothingStyle::PilotGear: return 5;
        case ClothingStyle::MarineGear: return 7;
        case ClothingStyle::IndustrialGear: return 10;
        case ClothingStyle::CompanyUniform: return 15;
        default: return 0;
    }
}
bool ClothingSystem::IsUnlocked(ClothingStyle s,int reputation){return reputation>=RequiredReputation(s);}
