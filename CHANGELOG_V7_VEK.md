# v7 — VEK Custom Language + Scripted Collision

## New custom language

- Added VEK (Vehicle Engineering Kernel), `.vek` files.
- Added lexer, parser, AST interpreter and C++ native-function bridge.
- Added variables, functions, if/else, arithmetic, comparisons, booleans, strings and native calls.
- Added syntax/runtime error reporting.
- Added F11 hot reload for collision scripts.

## Collision system

- Added modular `CollisionSystem`.
- Added cached static world colliders sourced from `World`.
- Avatar now collides with workshop walls/buildings/world limits.
- Avatar collides with parked custom vehicles.
- Vehicle entry temporarily ignores parked-vehicle blocking while retaining world collision.
- Driven vehicles collide with static world geometry.
- VEK decides solid, friction, bounce and impact damage.
- Added safe C++ fallback if VEK script fails.
- Added vehicle impact damage cooldown.
- Added F10 3D/HUD collision debugger.

## Tests

- Added `VekLanguageTests.cpp`.
- Added `CollisionSystemTests.cpp`.
- `RUN_SYSTEM_TESTS.bat` now includes both.
