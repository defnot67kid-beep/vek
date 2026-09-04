# VEK GUI Framework 2.8

`GuiFramework` is VEK's retained-mode, backend-neutral UI runtime. Hosts own the OS window, renderer, font rasterizer, textures and input device APIs. VEK owns UI structure, state, layout, focus, event routing, animation and render descriptions.

## Architecture

```text
VEK script / host language
        |
      ui_*
        |
  GuiFramework
  ├─ retained node tree
  ├─ theme/style resolver
  ├─ layout engine
  ├─ focus + hit testing
  ├─ event queue
  ├─ animation engine
  └─ draw-list builder
        |
 host renderer/input adapter
 (raylib / SDL / Win32 / Vulkan / D3D / etc.)
```

No direct GPU pointer, HWND, device context or native input handle is exposed to a VEK script.

## Widget families

Containers/tooling: root, window, modal, panel, card, dock space, split pane, tabs, menu bar, menu, toolbar, status bar, popup, context menu, property grid, viewport, graph and timeline.

Controls: button, icon button, toggle, checkbox, radio, slider, range slider, progress, spinner, text input, text area, search box, number input, combo box, dropdown, color picker and keybind.

Data/display: label, rich text, list, tree, table, image, canvas, badge, toast, tooltip, separator and spacer.

## Script example

```vek
fn build_ui() {
    ui_set_viewport(1280, 720, 1.0);

    ui_create({
        id:"root",
        type:"root",
        layout:{
            mode:"column",
            width:{mode:"fill"},
            height:{mode:"fill"},
            gap:8,
            padding:12
        }
    });

    ui_create({
        id:"toolbar",
        type:"toolbar",
        parent:"root",
        layout:{mode:"row", height:52, gap:8}
    });

    ui_create({
        id:"save",
        type:"button",
        parent:"toolbar",
        text:"Save",
        layout:{width:110, height:40},
        accessibility:{role:"button", label:"Save project"}
    });

    ui_layout();
}

fn update(dt) {
    ui_step(dt);
    let event = ui_next_event();
    while event != nil {
        if event.type == "click" && event.target == "save" {
            println("save requested");
        }
        event = ui_next_event();
    }
}
```

## Host flow

1. Register `RuntimePlatformPack` natives.
2. Set viewport size/DPI/safe area as the window changes.
3. Feed pointer/keyboard/text/scroll input into `GuiFramework`.
4. Call `Step(dt)` and `Layout()` as needed.
5. Consume `BuildDrawList()` and render each command with the host renderer.
6. Poll VEK-visible events using `ui_next_event()` or the native C++ event queue.

## Safety limits

The framework bounds node count, event queue size and active animations. Input/rendering are host-mediated. This keeps the GUI system useful for untrusted game scripts without giving them direct graphics-driver or OS-window access.
