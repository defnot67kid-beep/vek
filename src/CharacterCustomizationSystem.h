#pragma once
#include "Character.h"

class CharacterCustomizationSystem {
public:
    static constexpr int RowCount = 21;
    static const char* Label(int row);
    static void Adjust(AvatarAppearance& appearance, int row, int direction, int reputation);
};
