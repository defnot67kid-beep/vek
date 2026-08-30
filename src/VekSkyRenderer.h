#pragma once
#include "raylib.h"
#include <VekGameSystems.h>
class VekSkyRenderer {
public:
    void DrawBackground(const Camera3D& camera,const vek::SkyboxDefinition& sky) const;
};
