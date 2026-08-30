# VEK Vehicle Editor + Game Modes (v12)

## Main menu

The title screen is authored by `scripts/main_menu.vek` through VEK's renderer-neutral `GuiSystem` command buffer. The host renders two original buttons:

1. SURVIVAL
2. SANDBOX (directly underneath Survival)

The menu script is signed in secure-release builds.

## Survival

- Build costs are enabled.
- Part unlock levels are enforced.
- Survival max part count is controlled by `vehicle_editor.vek`.
- Vehicle finalization checks available money.
- Upgrade prices are VEK-controlled.
- Career money continues to save.

## Sandbox

- All editor parts are unlocked.
- Build costs resolve to zero.
- The editor supports a much larger part limit.
- Vehicle fuel is kept full through the VEK mode policy.
- Engine upgrades are free.
- Sandbox does not overwrite the Survival career money save on shutdown.

## Editor categories

- Structural
- Movement
- Mechanical
- Functional
- Experimental

The VEK 1.2 `BuildPartCatalog` includes 28 prototype components while preserving legacy IDs 0-3 for Chassis, Wheel, Small Engine and Driver Seat.

## Build controls

- `B`: enter/leave editor at workshop
- `1`-`5`: category
- `Q` / `E`: previous/next unlocked part
- Mouse: choose category/part from left panel
- Left click in world: place
- `R`: rotate placement 15 degrees
- `G`: normal/fine grid
- `H`: toggle free placement
- `X`: mirrored placement
- `Ctrl+Z` / `Ctrl+Y`: undo/redo
- `F`: run VEK validation and construct vehicle

## Security

`vehicle_editor.vek` controls costs, unlocks, placement bounds, maximum parts, build requirements, warnings, upgrades and sandbox infinite fuel. `main_menu.vek` controls frontend mode selection. Both are covered by VEK Guard release signatures.
