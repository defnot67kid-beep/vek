# VEK UI Docking 3.3

`vek::ui::DockManager` stores editor panel/workspace state independently from any renderer. A host can persist `Snapshot()` and restore it on the next launch.

Typical VEK script flow:

```vek
ui_dock_register({id:"parts",title:"Parts",region:"left",split_ratio:0.25,closable:true});
ui_dock_register({id:"properties",title:"Properties",region:"right",split_ratio:0.30,closable:true});
ui_dock_register({id:"console",title:"Console",region:"bottom",split_ratio:0.22,closable:true});
ui_dock_close("properties");
ui_dock_open("properties");
let layout = ui_dock_layout(0, 0, 1600, 900);
let zones = ui_dock_drop_zones(0, 0, 1600, 900, 12);
```

The returned layout is backend-neutral. Hosts may render tab strips, split handles, floating windows and drag previews however they want while keeping one canonical docking state model.
