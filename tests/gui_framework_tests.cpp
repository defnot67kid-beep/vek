#include <vek/VekGuiFramework.h>
#include <vek/VekRuntimeSystems.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>

int main() {
    vek::GuiFramework ui;
    assert(std::string(vek::GuiFramework::ApiVersion) == "2.8");
    ui.SetViewport({1000, 700, 1.5f, {10,20,10,20}});

    VekMap root;
    root["id"] = "root"; root["type"] = "root";
    VekMap rootLayout; rootLayout["mode"]="column"; rootLayout["gap"]=8;
    VekMap fill; fill["mode"]="fill";
    rootLayout["width"] = VekValue(fill); rootLayout["height"] = VekValue(fill);
    root["layout"] = VekValue(rootLayout);
    { std::string err; bool ok=ui.CreateValue(VekValue(root),&err); if(!ok) std::cerr << "root create error: " << err << "\n"; assert(ok); }

    VekMap bar; bar["id"]="toolbar"; bar["type"]="toolbar"; bar["parent"]="root";
    VekMap barLayout; barLayout["mode"]="row"; barLayout["height"]=52; barLayout["gap"]=6; bar["layout"]=VekValue(barLayout);
    assert(ui.CreateValue(VekValue(bar)));

    VekMap b1; b1["id"]="save"; b1["type"]="button"; b1["parent"]="toolbar"; b1["text"]="Save";
    VekMap b1Layout; b1Layout["width"]=110; b1Layout["height"]=40; b1["layout"]=VekValue(b1Layout);
    assert(ui.CreateValue(VekValue(b1)));
    VekMap b2=b1; b2["id"]="build"; b2["text"]="Build"; assert(ui.CreateValue(VekValue(b2)));

    VekMap content; content["id"]="content"; content["type"]="split_pane"; content["parent"]="root";
    VekMap cl; cl["mode"]="row"; cl["width"]=VekValue(fill); cl["height"]=VekValue(fill); cl["gap"]=10; content["layout"]=VekValue(cl);
    assert(ui.CreateValue(VekValue(content)));
    VekMap left; left["id"]="parts"; left["type"]="list"; left["parent"]="content"; VekMap ll; ll["width"]=280; ll["height"]=VekValue(fill); left["layout"]=VekValue(ll); assert(ui.CreateValue(VekValue(left)));
    VekMap view; view["id"]="viewport"; view["type"]="viewport"; view["parent"]="content"; VekMap vl; vl["width"]=VekValue(fill); vl["height"]=VekValue(fill); view["layout"]=VekValue(vl); assert(ui.CreateValue(VekValue(view)));

    ui.Layout();
    assert(ui.Find("root")->computed.width > 900);
    assert(ui.Find("viewport")->computed.width > ui.Find("parts")->computed.width);
    assert(ui.Find("toolbar")->computed.height > 0);

    const auto saveRect = ui.Find("save")->computed;
    const vek::GuiPoint center{saveRect.x + saveRect.width/2, saveRect.y + saveRect.height/2};
    assert(ui.HitTest(center) == "save");
    ui.FeedInput({vek::GuiInputType::PointerDown,center});
    ui.FeedInput({vek::GuiInputType::PointerUp,center});
    bool sawClick=false; vek::GuiEvent ev;
    while(ui.PollEvent(ev)) if(ev.type==vek::GuiEventType::Click && ev.targetId=="save") sawClick=true;
    assert(sawClick);
    assert(ui.FocusedId()=="save");
    assert(ui.FocusNext());
    assert(ui.FocusedId()=="build");

    vek::GuiAnimation anim; anim.nodeId="parts"; anim.property="opacity"; anim.from=0; anim.to=1; anim.duration=0.5;
    assert(ui.StartAnimation(anim)); ui.Step(0.25); assert(ui.Find("parts")->visual.opacity>0 && ui.Find("parts")->visual.opacity<1); ui.Step(0.5); assert(ui.Find("parts")->visual.opacity>0.99);

    VekMap theme; theme["id"]="test.theme"; VekMap base; VekMap bg; bg["r"]=0.2; bg["g"]=0.3; bg["b"]=0.4; bg["a"]=1.0; base["background"]=VekValue(bg); theme["base"]=VekValue(base);
    assert(ui.DefineThemeValue(VekValue(theme))); assert(ui.SetTheme("test.theme"));
    assert(!ui.BuildDrawList().empty()); assert(ui.Snapshot().IsMap());

    VekScriptEngine vm; VekRegisterStandardLibrary(vm); vek::RuntimePlatformPack pack; pack.RegisterNatives(vm);
    assert(vm.LoadSource(R"VEK(
        fn main() {
            ui_set_viewport(800, 600, 1);
            ui_create({id:"screen", type:"root", layout:{width:{mode:"fill"},height:{mode:"fill"}}});
            ui_create({id:"play", type:"button", parent:"screen", text:"PLAY", layout:{width:200,height:60}});
            ui_layout();
            return {version:ui_version(), count:ui_count(), rect:ui_rect("play")};
        }
    )VEK"));
    auto result=vm.Call("main"); assert(result.Get("version").AsString()=="2.8"); assert(result.Get("count").AsNumber()==2); assert(result.Get("rect").IsMap());

    std::cout << "VEK 2.8 GUI Framework tests: PASS\n";
}
