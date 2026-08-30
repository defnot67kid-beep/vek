#pragma once
#include "Types.h"
#include "raylib.h"
#include <string>

// Renderer owned by CustomVehicle. VEK supplies logical renderer IDs; the host
// decides how those IDs are represented. This keeps VEK backend-neutral.
namespace PartPresentationRenderer {

// Draw the editor/view model or finalized world model for a vehicle part.
void DrawModel(const VehiclePart& part, Vector3 position, float pitch, float yaw, float roll,
               Color tint, bool wires, bool worldModel);

// Draw a compact vector icon for VEK part catalogs. Unknown IDs fall back to a
// category-safe generic icon instead of making the part invisible.
void DrawCatalogIcon(Rectangle bounds, const std::string& iconId, const std::string& category,
                     bool locked=false);

// Useful for validation/tests and for hosts that want to know whether the
// built-in CustomVehicle renderer has a dedicated model/icon.
bool HasBuiltinModel(const std::string& modelId);
bool HasBuiltinIcon(const std::string& iconId);

}
