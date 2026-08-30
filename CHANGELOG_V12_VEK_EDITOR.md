# v12 - VEK Vehicle Editor + Survival/Sandbox

- Added VEK 1.2 dependency.
- Added signed `main_menu.vek` and `vehicle_editor.vek`.
- Added VEK GuiSystem-driven main menu with Survival and Sandbox.
- Added `MainMenuSystem` raylib host renderer.
- Added `VekVehicleEditorRules` bridge.
- Expanded part catalog from 4 to 28 prototype components.
- Added Structural / Movement / Mechanical / Functional / Experimental editor categories.
- Added Survival build costs and unlock levels.
- Added Sandbox free construction, all unlocks, larger build limits and infinite fuel.
- Added grid/fine/free placement, mirror placement, undo/redo, live mass/power/cost stats and VEK build validation.
- Preserved legacy PartType IDs 0-3 for blueprint compatibility.
- VEK Guard signatures now cover collision, humanoid, vehicle-editor and main-menu scripts.
