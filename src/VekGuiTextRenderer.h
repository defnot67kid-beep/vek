#pragma once
#include "raylib.h"
#include <VekGameSystems.h>
#include <string>

namespace VekGuiTextRenderer {
void DrawTextAuto(const std::string& text, Rectangle bounds, const vek::GuiTextPolicy& policy, Color color, bool verticalCenter=true);
}
