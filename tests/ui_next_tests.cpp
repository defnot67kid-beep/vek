// Tests for VEK UI Next (VEK 3.0) foundational systems: reactive state,
// style cascade, command architecture, and virtualization.
#include <vek/VekUiReactive.h>
#include <vek/VekUiStyle.h>
#include <vek/VekUiCommands.h>
#include <vek/VekUiVirtualization.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>

using namespace vek;
using namespace vek::ui;

static void TestSignalBasic() {
    Signal<double> count(0.0);
    assert(count.Get() == 0.0);
    count.Set(5.0);
    assert(count.Get() == 5.0);
}

static void TestComputedTracksAndCaches() {
    Signal<double> a(2.0), b(3.0);
    int evalCount = 0;
    Computed<double> sum([&] {
        ++evalCount;
        return a.Get() + b.Get();
    });
    assert(sum.Get() == 5.0);
    assert(evalCount == 1);
    // Multiple reads without writes must not recompute (lazy caching).
    (void)sum.Get(); (void)sum.Get();
    assert(evalCount == 1);
    a.Set(10.0);
    assert(sum.Get() == 13.0);
    assert(evalCount == 2);
    // Writing an unrelated signal (b already tracked) still triggers.
    b.Set(1.0);
    assert(sum.Get() == 11.0);
    assert(evalCount == 3);
}

static void TestEffectReruns() {
    Signal<double> x(1.0);
    int runs = 0;
    double observed = 0.0;
    auto effect = Watch([&] {
        ++runs;
        observed = x.Get();
    });
    assert(runs == 1);
    assert(observed == 1.0);
    x.Set(42.0);
    assert(runs == 2);
    assert(observed == 42.0);
}

static void TestBatchCoalescesNotifications() {
    Signal<double> a(0.0), b(0.0);
    int runs = 0;
    auto effect = Watch([&] {
        ++runs;
        (void)a.Get();
        (void)b.Get();
    });
    assert(runs == 1);
    ReactiveScope::Instance().Batch([&] {
        a.Set(1.0);
        b.Set(2.0);
    });
    // Batching multiple writes to two independently-tracked sources must
    // still only trigger one re-run of the effect, not two.
    assert(runs == 2);
}

static void TestListRendersOnlyKeyedItemsConceptually() {
    // Reactive layer doesn't know about GuiNode lists directly in this
    // generation; this test exercises the pattern components use: a
    // computed derived from a signal-held array, re-evaluated once per
    // structural change.
    VekValue arr = VekValue::Array();
    arr.Push(VekValue(std::string("engine")));
    arr.Push(VekValue(std::string("wheel")));
    Signal<VekValue> items(arr);
    int evalCount = 0;
    Computed<std::size_t> count([&] {
        ++evalCount;
        return items.Get().Size();
    });
    assert(count.Get() == 2);
    VekValue arr2 = VekValue::Array();
    arr2.Push(VekValue(std::string("engine")));
    arr2.Push(VekValue(std::string("wheel")));
    arr2.Push(VekValue(std::string("seat")));
    items.Set(arr2);
    assert(count.Get() == 3);
    assert(evalCount == 2);
}

static void TestStyleClassAndIdSelectors() {
    StyleSheet sheet;
    VekMap decl1; decl1["background"] = VekValue(std::string("#111111"));
    sheet.AddRule(".card", decl1);
    VekMap decl2; decl2["background"] = VekValue(std::string("#222222"));
    sheet.AddRule("#hero-card", decl2); // higher specificity, should win

    StyleNodeContext node;
    node.id = "hero-card";
    node.classes = {"card"};
    auto resolved = sheet.Resolve({node});
    assert(resolved["background"].AsString() == "#222222");
}

static void TestStylePseudoStatesAndCascadeOrder() {
    StyleSheet sheet;
    VekMap base; base["opacity"] = VekValue(1.0);
    sheet.AddRule(".primary-button", base);
    VekMap disabled; disabled["opacity"] = VekValue(0.45);
    sheet.AddRule(".primary-button:disabled", disabled);

    StyleNodeContext enabled;
    enabled.classes = {"primary-button"};
    assert(sheet.Resolve({enabled})["opacity"].AsNumber() == 1.0);

    StyleNodeContext disabledNode;
    disabledNode.classes = {"primary-button"};
    disabledNode.pseudoStates = {"disabled"};
    assert(sheet.Resolve({disabledNode})["opacity"].AsNumber() == 0.45);
}

static void TestStyleDescendantSelector() {
    StyleSheet sheet;
    VekMap decl; decl["foreground"] = VekValue(std::string("#ffffff"));
    sheet.AddRule(".toolbar .icon-button", decl);

    StyleNodeContext toolbar; toolbar.classes = {"toolbar"};
    StyleNodeContext button; button.classes = {"icon-button"};
    assert(sheet.Resolve({toolbar, button})["foreground"].AsString() == "#ffffff");

    StyleNodeContext lonelyButton; lonelyButton.classes = {"icon-button"};
    assert(sheet.Resolve({lonelyButton}).find("foreground") == sheet.Resolve({lonelyButton}).end());
}

static void TestThemeVariablesAndVarResolution() {
    StyleSheet sheet;
    VekMap tokens;
    tokens["--accent"] = VekValue(std::string("#45a3ff"));
    sheet.DefineTheme("vehicle.dark", tokens);
    sheet.SetActiveTheme("vehicle.dark");

    VekMap decl;
    decl["background"] = VekValue(std::string("var(\"--accent\")"));
    sheet.AddRule(".primary-button", decl);

    StyleNodeContext node; node.classes = {"primary-button"};
    assert(sheet.Resolve({node})["background"].AsString() == "#45a3ff");
}

static void TestStyleSheetParsing() {
    StyleSheet sheet;
    std::string src =
        "theme \"vehicle.dark\" {\n"
        "    --accent: \"#45a3ff\"\n"
        "}\n"
        "style \".primary-button\" {\n"
        "    background: var(\"--accent\")\n"
        "    border_radius: 10\n"
        "}\n"
        "style \".primary-button:hover\" {\n"
        "    background: \"#5bb0ff\"\n"
        "}\n";
    std::size_t parsed = sheet.Parse(src);
    assert(parsed == 3);
    sheet.SetActiveTheme("vehicle.dark");
    StyleNodeContext hover; hover.classes = {"primary-button"}; hover.pseudoStates = {"hover"};
    auto resolved = sheet.Resolve({hover});
    assert(resolved["background"].AsString() == "#5bb0ff");
    assert(resolved["border_radius"].AsNumber() == 10.0);
}

static void TestInheritedProperties() {
    VekMap parentProps;
    parentProps["foreground"] = VekValue(std::string("#ffffff"));
    parentProps["background"] = VekValue(std::string("#000000")); // not inherited
    StyleSheet sheet;
    StyleNodeContext child;
    auto resolved = sheet.Resolve({child}, parentProps);
    assert(resolved["foreground"].AsString() == "#ffffff");
    assert(resolved.find("background") == resolved.end());
}

static void TestCommandRegistryBasic() {
    CommandRegistry registry;
    bool executed = false;
    CommandDescriptor cmd;
    cmd.id = "editor.delete";
    cmd.label = "Delete";
    cmd.shortcutText = "Delete";
    cmd.execute = [&] { executed = true; };
    assert(registry.Register(cmd));
    assert(registry.Execute("editor.delete") == CommandExecuteResult::Ok);
    assert(executed);
}

static void TestCommandDisabledAndMissingCapability() {
    CommandRegistry registry;
    CommandDescriptor cmd;
    cmd.id = "editor.wipe_save";
    cmd.enabled = false;
    cmd.execute = [] {};
    registry.Register(cmd);
    assert(registry.Execute("editor.wipe_save") == CommandExecuteResult::Disabled);
    registry.SetEnabled("editor.wipe_save", true);

    CommandDescriptor privileged;
    privileged.id = "editor.export_native";
    privileged.requiredCapability = "native.export";
    privileged.execute = [] {};
    registry.Register(privileged);
    assert(registry.Execute("editor.export_native") == CommandExecuteResult::MissingCapability);
    assert(registry.Execute("editor.export_native", {"native.export"}) == CommandExecuteResult::Ok);
}

static void TestCommandSharedAcrossInvocationSurfaces() {
    // The same command must be invocable identically whether triggered by
    // toolbar/menu ("Execute by id") or a keyboard shortcut ("ExecuteByShortcut").
    CommandRegistry registry;
    int invocations = 0;
    CommandDescriptor cmd;
    cmd.id = "editor.delete";
    cmd.shortcutText = "Delete";
    cmd.execute = [&] { ++invocations; };
    registry.Register(cmd);

    assert(registry.Execute("editor.delete") == CommandExecuteResult::Ok);
    std::string firedId = registry.ExecuteByShortcut(ParseShortcut("Delete"));
    assert(firedId == "editor.delete");
    assert(invocations == 2);
}

static void TestVirtualListUniformRows() {
    // 50,000 rows: only the visible window + overscan should be rendered.
    VirtualRange range = ComputeVirtualRange(50000, 48.0f, 4800.0f, 600.0f, 8);
    assert(!range.empty);
    assert(range.renderedCount < 40); // far less than 50,000
    assert(range.firstIndex <= 100);
    assert(range.lastIndex >= range.firstIndex);
    assert(range.totalContentSize == 48.0f * 50000.0f);
}

static void TestVirtualListScrolledToEnd() {
    VirtualRange range = ComputeVirtualRange(1000, 20.0f, 1000000.0f /*way past end*/, 400.0f, 2);
    assert(!range.empty);
    assert(range.lastIndex == 999);
}

static void TestVirtualListVariableHeights() {
    std::vector<float> heights(1000, 30.0f);
    heights[500] = 300.0f; // one big row
    VirtualRange range = ComputeVirtualRangeVariable(heights, 15000.0f, 400.0f, 2);
    assert(!range.empty);
    assert(range.renderedCount < 100);
}

static void TestVirtualGrid() {
    VirtualGridRange grid = ComputeVirtualGridRange(10000, 96.0f, 96.0f, 800.0f, 600.0f, 2000.0f, 1);
    assert(grid.columnsPerRow >= 8);
    assert(grid.FirstItemIndex() > 0);
    assert(grid.LastItemIndex(10000) < 10000);
}

int main() {
    TestSignalBasic();
    TestComputedTracksAndCaches();
    TestEffectReruns();
    TestBatchCoalescesNotifications();
    TestListRendersOnlyKeyedItemsConceptually();

    TestStyleClassAndIdSelectors();
    TestStylePseudoStatesAndCascadeOrder();
    TestStyleDescendantSelector();
    TestThemeVariablesAndVarResolution();
    TestStyleSheetParsing();
    TestInheritedProperties();

    TestCommandRegistryBasic();
    TestCommandDisabledAndMissingCapability();
    TestCommandSharedAcrossInvocationSurfaces();

    TestVirtualListUniformRows();
    TestVirtualListScrolledToEnd();
    TestVirtualListVariableHeights();
    TestVirtualGrid();

    std::cout << "vek_ui_next_tests: all assertions passed\n";
    return 0;
}
