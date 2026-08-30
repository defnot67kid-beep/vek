#pragma once
#include "Character.h"

class ClothingSystem {
public:
    static const char* Name(ClothingStyle style);
    static int RequiredReputation(ClothingStyle style);
    static bool IsUnlocked(ClothingStyle style, int reputation);
};
