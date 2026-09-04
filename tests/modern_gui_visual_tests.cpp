#include <vek/VekGuiFramework.h>
#include <vek/VekInteractiveUiSystems.h>
#include <vek/VekUiStyle.h>
#include <vek/VekUiVirtualization.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <set>

using namespace vek;
using namespace vek::ui;

static void TestModernThemesAndTokenCascade() {
    StyleSheet sheet;
    const auto count = sheet.Parse(ModernUiStyleSheetSource());
    assert(count > 20);
    assert(sheet.HasTheme("vek.modern.dark"));
    assert(sheet.HasTheme("vek.modern.light"));
    sheet.SetActiveTheme("vek.modern.dark");
    assert(sheet.Token("--accent").AsString() == "#5ca8ff");
    StyleNodeContext b; b.type="button"; b.classes={"primary"}; b.pseudoStates={"hover"};
    auto r=sheet.Resolve({b});
    assert(r.at("background").AsString()=="#74b5ff");
    assert(r.at("border_radius").AsNumber()>0);
}

static void TestTypeSelectorsAndVarFallback() {
    StyleSheet sheet;
    sheet.Parse("theme \"t\" { --a: var(--b, #123456); } style \"button\" { background: var(--a); }");
    sheet.SetActiveTheme("t");
    StyleNodeContext b; b.type="button";
    assert(sheet.Resolve({b})["background"].AsString()=="#123456");
    StyleNodeContext label; label.type="label";
    assert(sheet.Resolve({label}).find("background")==sheet.Resolve({label}).end());
}

static VekValue Fill(){VekMap m;m["mode"]="fill";return VekValue(m);}

static void TestModernWidgetDrawParts() {
    GuiFramework ui;
    ui.SetViewport({1000,700,1.0f,{}});
    VekMap root;root["id"]="root";root["type"]="root";VekMap rl;rl["width"]=Fill();rl["height"]=Fill();rl["mode"]="column";root["layout"]=rl;assert(ui.CreateValue(root));
    auto add=[&](const char*id,const char*type){VekMap n;n["id"]=id;n["type"]=type;n["parent"]="root";VekMap l;l["width"]=300;l["height"]=type==std::string("viewport")?180:36;n["layout"]=l;return ui.CreateValue(n);};
    assert(add("btn","button")); assert(add("toggle","toggle")); assert(add("slider","slider")); assert(add("search","search_box")); assert(add("scroll","scroll")); assert(add("viewport","viewport")); assert(add("graph","graph")); assert(add("timeline","timeline"));
    ui.Find("btn")->classes={"primary"};ui.Find("btn")->text="Build";ui.Find("toggle")->checked=true;ui.Find("slider")->value=.5;ui.Find("search")->data.Set("placeholder","Search parts");ui.Find("viewport")->data.Set("toolbar",true);
    ui.Layout();
    const auto draw=ui.BuildDrawList();
    std::set<std::string> parts;for(const auto&d:draw)parts.insert(d.part);
    assert(parts.count("toggle.track"));assert(parts.count("toggle.thumb"));assert(parts.count("slider.track"));assert(parts.count("slider.thumb"));assert(parts.count("search.icon"));assert(parts.count("viewport.frame"));assert(parts.count("viewport.content"));assert(parts.count("viewport.toolbar"));assert(parts.count("graph.curves"));assert(parts.count("timeline.keyframes"));
    assert(ui.Stats().modernPrimitiveCount>0);
}

static void TestModernFocusAndInteraction() {
    GuiFramework ui; ui.SetViewport({500,300,1,{}});
    VekMap b;b["id"]="b";b["type"]="button";VekMap l;l["width"]=120;l["height"]=36;b["layout"]=l;assert(ui.CreateValue(b));ui.Layout();
    ui.FeedInput({GuiInputType::KeyDown,{0,0},0,0,0,9});
    assert(ui.FocusedId()=="b");assert(ui.Find("b")->focusVisible);
    ui.Step(.05);assert(ui.Find("b")->focusProgress>0);
}

static void TestScrollbarMetricsAndShadows() {
    auto m=ComputeScrollbarMetrics(5000,500,1000,480,28);
    assert(m.scrollable);assert(m.thumbLength>=28);assert(m.thumbStart>0);assert(m.normalized>0);
    auto s=ComputeScrollShadowMetrics(5000,500,1000,24);assert(s.leadingOpacity>0);assert(s.trailingOpacity>0);
    assert(ComputeAdaptiveOverscan(4,4000,40,32)>4);
}

static void TestInteractionMotionHelper() {
    UiInteractionMotion m;m.SetHovered(true);m.SetFocused(true);m.Step(.05f);assert(m.State().hover>0);assert(m.State().focus>0);m.SetPressed(true);m.Step(.05f);assert(m.State().press>0);
}

int main(){
    TestModernThemesAndTokenCascade();TestTypeSelectorsAndVarFallback();TestModernWidgetDrawParts();TestModernFocusAndInteraction();TestScrollbarMetricsAndShadows();TestInteractionMotionHelper();
    std::cout<<"VEK 3.1 modern GUI visual tests: PASS\n";
}
