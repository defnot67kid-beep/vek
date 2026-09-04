#include <vek/VekGuiFramework.h>
#include <vek/VekUiDocking.h>
#include <vek/VekColorSystems.h>
#include <vek/VekShaderSystems.h>
#include <vek/VekUiStyle.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

using namespace vek;
using namespace vek::ui;

static VekValue Fixed(double n){ VekMap m; m["mode"]="fixed"; m["value"]=n; return VekValue(m); }

static void TestDockManager(){
    DockManager d; std::string err;
    DockPanelState parts; parts.id="parts"; parts.title="Parts"; parts.region=DockRegion::Left; parts.splitRatio=.25f; parts.active=true;
    DockPanelState props; props.id="properties"; props.title="Properties"; props.region=DockRegion::Right; props.splitRatio=.30f;
    DockPanelState console; console.id="console"; console.title="Console"; console.region=DockRegion::Bottom; console.splitRatio=.20f;
    assert(d.RegisterPanel(parts,&err)); assert(d.RegisterPanel(props,&err)); assert(d.RegisterPanel(console,&err));
    auto layout=d.ComputeLayout({0,0,1200,800}); assert(layout.size()==3);
    const auto* p=d.Find("properties"); assert(p&&p->open); assert(d.Close("properties")); assert(!d.Find("properties")->open); assert(d.Open("properties"));
    assert(d.Dock("properties",DockRegion::Right,"main",.28f)); assert(d.ResizeSplit("properties",.33f)); assert(d.Find("properties")->splitRatio>.32f);
    auto zones=d.ComputeDropZones({0,0,1200,800}); assert(zones.size()==5); assert(zones[0].region==DockRegion::Center);
    auto snap=d.Snapshot(); DockManager restored; assert(restored.Restore(snap,&err)); assert(restored.Find("parts"));
}

static void TestColorAndShaderSystems(){
    auto c=NamedVekColor("vek-blue"); assert(c.a==1.0f && c.b>c.r);
    auto mixed=MixColor(NamedVekColor("vek-blue"),NamedVekColor("vek-red"),.5f); assert(mixed.r>0 && mixed.b>0);
    assert(ContrastRatio(ParseColorHex("#ffffff"),ParseColorHex("#000000"))>20.0f);
    ShaderRegistry r; std::string err; auto glass=MakeShaderPreset("frosted_glass"); assert(!glass.id.empty()); assert(r.Register(glass,&err)); assert(r.Find(glass.id));
    auto wire=MakeShaderPreset("wireframe"); assert(r.Register(wire,&err)); assert(r.Size()==2);

    VekScriptEngine e; VekRegisterStandardLibrary(e);
    assert(e.LoadSource("fn f(){ return color_hex(\"#69a9ff\"); } fn s(){ return shader_preset(\"vignette\"); }","v33"));
    auto cv=e.Call("f"); assert(cv.IsMap()); assert(cv.Get("hex").AsString().find("#69A9FF")==0);
    auto sv=e.Call("s"); assert(sv.IsMap()); assert(!sv.Get("id").AsString().empty());
}

static void TestScrollClampAndClipOrdering(){
    GuiFramework ui; ui.SetViewport({400,240,1,{}});
    VekMap scroll; scroll["id"]="scroll"; scroll["type"]="scroll"; scroll["clip_children"]=true;
    VekMap sl; sl["width"]=320; sl["height"]=120; sl["mode"]="column"; sl["overflow"]="scroll"; scroll["layout"]=sl; assert(ui.CreateValue(scroll));
    for(int i=0;i<12;i++){ VekMap n; n["id"]="row"+std::to_string(i); n["type"]="button"; n["parent"]="scroll"; n["text"]="Row"; VekMap l; l["width"]=300; l["height"]=36; n["layout"]=l; assert(ui.CreateValue(n)); }
    ui.Layout();
    GuiInputEvent wheel; wheel.type=GuiInputType::Scroll; wheel.pointer={40,40}; wheel.scrollY=5000; ui.FeedInput(wheel);
    auto* sn=ui.Find("scroll"); assert(sn); assert(sn->scrollY>=0);
    auto snap=ui.Snapshot(); (void)snap;
    auto draw=ui.BuildDrawList(); int push=-1,pop=-1,child=-1;
    for(int i=0;i<(int)draw.size();++i){ if(draw[i].nodeId=="scroll"&&draw[i].type==GuiDrawType::ClipPush)push=i; if(draw[i].nodeId=="scroll"&&draw[i].type==GuiDrawType::ClipPop)pop=i; if(draw[i].nodeId=="row0")child=i; }
    assert(push>=0 && pop>push && child>push && child<pop);

    // Shrinking content must clamp stale scroll instead of leaving children permanently off-screen.
    for(int i=3;i<12;i++) ui.Remove("row"+std::to_string(i));
    ui.Layout(); sn=ui.Find("scroll"); assert(sn->scrollY>=0 && sn->scrollY<5.0f);
}

static void TestThemeTokens(){
    StyleSheet s; s.Parse(ModernUiStyleSheetSource()); s.SetActiveTheme("vek.modern.dark");
    assert(s.Token("--vek-blue").AsString()=="#69a9ff"); assert(!s.Token("--shader-panel").AsString().empty());
    StyleNodeContext dock; dock.type="dock_space"; auto v=s.Resolve({dock}); assert(v.count("background"));
}

int main(){
    TestDockManager(); TestColorAndShaderSystems(); TestScrollClampAndClipOrdering(); TestThemeTokens();
    std::cout << "VEK 3.3 UI platform tests: PASS\n";
}
