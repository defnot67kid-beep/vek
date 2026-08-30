# VEK 0.1 — Vehicle Engineering Kernel

VEK is the custom scripting language for this game.

File extension: `.vek`

The C++20 engine remains responsible for rendering, memory, input, fast collision detection and other performance-critical work. VEK is the editable gameplay layer used for rules, events and behaviors. This is the same general architecture used by games that embed a scripting language on top of a native engine.

## Why VEK exists

VEK lets the project gradually move gameplay rules out of giant C++ switch statements. Collision behavior is the first system using it. Future uses can include jobs, world events, part behavior, NPC logic, weather rules and mission scripting.

## Current language features

VEK 0.1 supports:

- `fn` functions
- function parameters
- `let` local variables
- numbers
- strings
- booleans (`true`, `false`)
- `if` / `else`
- `return`
- arithmetic: `+ - * /`
- comparisons: `== != > >= < <=`
- logic: `&& || !`
- function calls
- C++ native functions exposed to scripts
- `//` and `#` comments

Example:

```vek
fn impact_damage(speed) {
    if speed > 7.5 {
        let amount = (speed - 7.5) * 1.25;
        return amount;
    }
    return 0;
}
```

## Collision API exposed by C++

The collision script currently receives:

```vek
fn on_collision(a, b, speed)
```

Where:

- `a` = first actor tag
- `b` = second actor tag
- `speed` = impact/approach speed in metres per second

Native functions:

```vek
pair(a, b, "player", "world")
has_tag(a, "player")
solid(true)
friction(0.65)
bounce(0.10)
damage(12.0)
min(a, b)
max(a, b)
clamp(value, minimum, maximum)
```

## Hot reload

Press **F11** during gameplay.

The engine reparses `scripts/collision.vek` and swaps to the new rules without recompiling the C++ project.

If the script contains a syntax/runtime error, the collision system keeps a safe C++ fallback so the player does not suddenly fall through the game world.

## Planned VEK versions

VEK 0.2 can add:

- arrays
- maps/objects
- `for` / `while`
- persistent script objects
- event registration
- vectors (`vec2`, `vec3`)
- entity handles
- modules/imports
- job scripting
- vehicle-part callbacks

VEK should remain sandboxed: scripts should only access native functions explicitly exposed by the engine.

---

## v9 security model

VEK now runs inside **VEK Guard**. Release scripts are signed and the VM has execution/resource
limits. The normal `BUILD_WINDOWS.bat` is a secure release build and does not allow F11 hot
reload. Use `BUILD_WINDOWS_DEV.bat` while actively editing `.vek` files, then run
`SIGN_VEK_SCRIPTS.bat` before rebuilding the secure version.

See `VEK_SECURITY.md` for the full security architecture and limitations.
