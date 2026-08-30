#pragma once
#include "Character.h"

class AvatarSaveSystem {
public:
    static bool Save(const AvatarAppearance& appearance);
    static bool Load(AvatarAppearance& appearance);
};
