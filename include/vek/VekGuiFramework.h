#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <vek/VekScriptEngine.h>
#include <vek/VekUiStyle.h>
#include <vek/VekUiDocking.h>

namespace vek {

// VEK 3.1 retained-mode GUI framework. The framework remains renderer/input
// backend-neutral: hosts feed normalized input and consume a modern draw list.
// Existing widget types and public entry points remain source-compatible.

struct GuiPoint { float x = 0.0f; float y = 0.0f; };
struct GuiSize { float width = 0.0f; float height = 0.0f; };
struct GuiRectF { float x = 0.0f; float y = 0.0f; float width = 0.0f; float height = 0.0f; };
struct GuiEdges { float left = 0.0f; float top = 0.0f; float right = 0.0f; float bottom = 0.0f; };
struct GuiColor { float r = 1.0f; float g = 1.0f; float b = 1.0f; float a = 1.0f; };

bool GuiContains(const GuiRectF& rect, GuiPoint point);

enum class GuiWidgetType : std::uint16_t {
    Root = 0, Window, Modal, Panel, Card, DockSpace, SplitPane, TabView, TabItem,
    MenuBar, Menu, MenuItem, ToolBar, StatusBar,
    Label, RichText, Button, IconButton, Toggle, Checkbox, Radio,
    Slider, RangeSlider, Progress, Spinner,
    TextInput, TextArea, SearchBox, NumberInput,
    ComboBox, Dropdown,
    List, ListItem, Tree, TreeItem,
    Table, TableRow, TableCell,
    Scroll, Image, Canvas, Separator, Spacer,
    Tooltip, ContextMenu, Popup, Toast, Badge, PropertyGrid,
    ColorPicker, Keybind, Viewport, Graph, Timeline
};

enum class GuiLayoutMode : std::uint8_t { Absolute = 0, Row, Column, Grid, Overlay, Dock };
enum class GuiSizeMode : std::uint8_t { Fixed = 0, Content, Fill, Percent };
enum class GuiAlign : std::uint8_t { Start = 0, Center, End, Stretch };
enum class GuiJustify : std::uint8_t { Start = 0, Center, End, SpaceBetween, SpaceAround };
enum class GuiOverflow : std::uint8_t { Visible = 0, Hidden, ScrollX, ScrollY, ScrollBoth };
enum class GuiPositionMode : std::uint8_t { Flow = 0, Absolute, Fixed };

enum class GuiEventType : std::uint8_t {
    None = 0, Click, DoubleClick, Change, Submit, Focus, Blur,
    PointerDown, PointerUp, PointerMove, PointerEnter, PointerLeave,
    Scroll, KeyDown, KeyUp, TextInput,
    DragStart, Drag, DragEnd, Drop, Open, Close, Select
};

enum class GuiInputType : std::uint8_t {
    PointerMove = 0, PointerDown, PointerUp, Scroll, KeyDown, KeyUp, TextInput
};

// The first six values are kept stable for existing render backends.
enum class GuiDrawType : std::uint8_t {
    Box = 0, Text, Image, ClipPush, ClipPop, Custom,
    Line, Circle, Shadow, Gradient, Icon
};

enum class GuiEase : std::uint8_t { Linear = 0, EaseIn, EaseOut, EaseInOut, SmoothStep };

enum class GuiAccessibilityRole : std::uint8_t {
    None = 0, Window, Dialog, Group, Text, Button, Checkbox, Radio, Slider,
    TextField, List, ListItem, Tree, TreeItem, Table, Row, Cell, Image,
    Menu, MenuItem, Tab, TabPanel, ProgressBar, Status, Toolbar
};

struct GuiDimension {
    GuiSizeMode mode = GuiSizeMode::Content;
    float value = 0.0f;
    float min = 0.0f;
    float max = 100000.0f;
};

struct GuiLayoutStyle {
    GuiLayoutMode mode = GuiLayoutMode::Column;
    GuiPositionMode position = GuiPositionMode::Flow;
    GuiDimension width{};
    GuiDimension height{};
    GuiEdges margin{};
    GuiEdges padding{};
    float gap = 0.0f;
    int gridColumns = 1;
    GuiAlign alignItems = GuiAlign::Stretch;
    GuiAlign alignSelf = GuiAlign::Stretch;
    GuiJustify justify = GuiJustify::Start;
    GuiOverflow overflow = GuiOverflow::Visible;
    float aspectRatio = 0.0f;
    float absoluteX = 0.0f;
    float absoluteY = 0.0f;
};

struct GuiVisualStyle {
    GuiColor background{0.10f,0.10f,0.12f,1.0f};
    GuiColor foreground{0.95f,0.95f,0.97f,1.0f};
    GuiColor border{0.28f,0.28f,0.32f,1.0f};
    GuiColor accent{0.25f,0.55f,1.0f,1.0f};
    GuiColor shadowColor{0.0f,0.0f,0.0f,0.0f};
    GuiColor focusRingColor{0.25f,0.55f,1.0f,0.0f};
    GuiColor trackColor{0.20f,0.22f,0.27f,1.0f};
    GuiColor thumbColor{0.55f,0.60f,0.70f,1.0f};
    float opacity = 1.0f;
    float borderWidth = 0.0f;
    float radius = 0.0f;
    float fontSize = 16.0f;
    float lineHeight = 1.2f;
    float letterSpacing = 0.0f;
    float shadowBlur = 0.0f;
    float shadowOffsetX = 0.0f;
    float shadowOffsetY = 4.0f;
    float focusRingWidth = 0.0f;
    float backdropBlur = 0.0f;
    float scale = 1.0f;
    float translateX = 0.0f;
    float translateY = 0.0f;
    float transitionDuration = 0.12f;
    float iconSize = 16.0f;
    std::string fontFamily;
    std::string icon;
    std::string easing = "ease_out";
    std::string shader;              // renderer-neutral shader/effect descriptor id
    VekValue shaderParams = VekValue::Map();
    int zIndex = 0;
};

struct GuiAccessibility {
    GuiAccessibilityRole role = GuiAccessibilityRole::None;
    std::string label;
    std::string hint;
    std::string valueText;
    int navigationOrder = 0;
    bool hidden = false;
};

struct GuiNode {
    std::string id;
    GuiWidgetType type = GuiWidgetType::Panel;
    std::string parentId;
    std::vector<std::string> children;
    std::vector<std::string> classes;
    std::string text;
    std::string tooltip;
    std::string image;
    std::string dataBinding;
    GuiLayoutStyle layout{};
    GuiVisualStyle visual{};
    GuiVisualStyle hoverVisual{};
    GuiVisualStyle pressedVisual{};
    GuiVisualStyle focusVisual{};
    GuiVisualStyle disabledVisual{};
    bool hasVisual = false;
    bool hasHoverVisual = false;
    bool hasPressedVisual = false;
    bool hasFocusVisual = false;
    bool hasDisabledVisual = false;
    GuiAccessibility accessibility{};
    GuiRectF computed{};
    VekValue data = VekValue::Map();
    double value = 0.0;
    double value2 = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;
    double step = 0.0;
    int selectedIndex = -1;
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    bool checked = false;
    bool selected = false;
    bool expanded = false;
    bool visible = true;
    bool enabled = true;
    bool focusable = false;
    bool hovered = false;
    bool pressed = false;
    bool focused = false;
    bool focusVisible = false;
    bool draggable = false;
    bool dragging = false;
    bool droppable = false;
    bool dropTarget = false;
    bool invalid = false;
    bool warning = false;
    bool success = false;
    bool clipChildren = false;
    bool virtualized = false;
    float hoverProgress = 0.0f;
    float pressProgress = 0.0f;
    float focusProgress = 0.0f;
    float openProgress = 0.0f;
    float checkProgress = 0.0f;
    float scrollbarProgress = 0.0f;
    bool draggingScrollbarV = false;
    bool draggingScrollbarH = false;
    float scrollDragStart = 0.0f;
    float scrollOffsetStart = 0.0f;
    GuiPoint pointerDown{};
};

struct GuiTheme {
    std::string id;
    GuiVisualStyle base{};
    std::unordered_map<std::string, GuiVisualStyle> classes;
    std::unordered_map<std::string, VekValue> variables;
};

struct GuiInputEvent {
    GuiInputType type = GuiInputType::PointerMove;
    GuiPoint pointer{};
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    int button = 0;
    int key = 0;
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
    std::string text;
};

struct GuiEvent {
    GuiEventType type = GuiEventType::None;
    std::string targetId;
    std::string currentTargetId;
    GuiPoint pointer{};
    int key = 0;
    std::string text;
    double value = 0.0;
    bool checked = false;
    VekValue data = VekValue::Map();
};

struct GuiDrawCommand {
    GuiDrawType type = GuiDrawType::Box;
    std::string nodeId;
    std::string part;               // semantic sub-part, e.g. "slider.thumb"
    GuiRectF rect{};
    GuiVisualStyle style{};
    GuiColor color{};
    GuiPoint from{};
    GuiPoint to{};
    std::string text;
    std::string image;
    std::string icon;
    float thickness = 1.0f;
    float value = 0.0f;
    int layer = 0;
    bool clip = false;
    VekValue data = VekValue::Map();
};

struct GuiViewport {
    float width = 1280.0f;
    float height = 720.0f;
    float dpiScale = 1.0f;
    GuiEdges safeArea{};
};

struct GuiAnimation {
    std::string nodeId;
    std::string property;
    double from = 0.0;
    double to = 0.0;
    double duration = 0.2;
    double elapsed = 0.0;
    GuiEase ease = GuiEase::EaseOut;
    bool loop = false;
    bool pingPong = false;
    bool reverse = false;
};

struct GuiFrameStats {
    std::uint64_t revision = 0;
    std::size_t nodeCount = 0;
    std::size_t visibleNodeCount = 0;
    std::size_t drawCommandCount = 0;
    std::size_t queuedEventCount = 0;
    std::size_t activeAnimationCount = 0;
    std::size_t modernPrimitiveCount = 0;
    std::string focusedId;
    std::string hoveredId;
};

class GuiFramework {
public:
    static constexpr const char* ApiVersion = "3.3";
    static constexpr std::size_t MaxNodes = 16384;
    static constexpr std::size_t MaxEvents = 4096;
    static constexpr std::size_t MaxAnimations = 2048;

    GuiFramework();

    bool Create(const GuiNode& node, std::string* error = nullptr);
    bool CreateValue(const VekValue& definition, std::string* error = nullptr);
    bool Patch(const std::string& id, const VekValue& patch, std::string* error = nullptr);
    bool Reparent(const std::string& id, const std::string& parentId, std::string* error = nullptr);
    bool Remove(const std::string& id, bool recursive = true);
    void Clear();

    GuiNode* Find(const std::string& id);
    const GuiNode* Find(const std::string& id) const;
    std::vector<std::string> ChildrenOf(const std::string& parentId) const;
    std::vector<std::string> Roots() const;
    std::size_t Size() const { return nodes_.size(); }

    bool DefineTheme(const GuiTheme& theme, std::string* error = nullptr);
    bool DefineThemeValue(const VekValue& theme, std::string* error = nullptr);
    bool SetTheme(const std::string& id);
    const std::string& ActiveTheme() const { return activeTheme_; }

    // New in 3.1. These extend the styling layer without changing the old API.
    std::size_t LoadStyleSheet(const std::string& source);
    void UseModernStyle(bool enabled = true) { modernStyleEnabled_ = enabled; MarkDirty(); }
    bool ModernStyleEnabled() const { return modernStyleEnabled_; }
    const ui::StyleSheet& Styles() const { return styleSheet_; }

    void SetViewport(const GuiViewport& viewport);
    const GuiViewport& Viewport() const { return viewport_; }
    void Layout();

    std::string HitTest(GuiPoint point) const;
    void FeedInput(const GuiInputEvent& input);
    bool PollEvent(GuiEvent& event);
    std::vector<GuiEvent> DrainEvents(std::size_t maxCount = MaxEvents);

    bool Focus(const std::string& id);
    bool FocusNext(bool reverse = false);
    void ClearFocus();
    const std::string& FocusedId() const { return focusedId_; }
    const std::string& HoveredId() const { return hoveredId_; }

    // Advanced dock workspace state (VEK 3.3). The retained widget tree is not
    // rewritten; dock state is an additive layout/UX layer that hosts may persist.
    ui::DockManager& Docking() { return dockManager_; }
    const ui::DockManager& Docking() const { return dockManager_; }

    bool StartAnimation(const GuiAnimation& animation, std::string* error = nullptr);
    bool StopAnimations(const std::string& nodeId, const std::string& property = {});
    void Step(double dtSeconds);

    std::vector<GuiDrawCommand> BuildDrawList() const;
    GuiFrameStats Stats() const;
    VekValue Snapshot() const;

    void RegisterNatives(VekScriptEngine& engine);

private:
    bool ValidateParent(const std::string& id, const std::string& parentId, std::string* error) const;
    bool ApplyPatch(GuiNode& node, const VekValue& patch, std::string* error);
    void DetachFromParent(const GuiNode& node);
    void AttachToParent(const GuiNode& node);
    void LayoutNode(const std::string& id, GuiRectF available);
    GuiSize MeasureNode(const GuiNode& node, GuiSize available) const;
    GuiVisualStyle ResolveVisual(const GuiNode& node) const;
    VekMap ResolveStyleMap(const GuiNode& node) const;
    std::vector<ui::StyleNodeContext> StylePath(const GuiNode& node) const;
    void BuildDrawNode(const std::string& id, std::vector<GuiDrawCommand>& out, int inheritedLayer) const;
    void BuildWidgetDrawCommands(const GuiNode& node, const GuiVisualStyle& style,
                                 std::vector<GuiDrawCommand>& out, int layer) const;
    void BuildScrollbarCommands(const GuiNode& node, const GuiVisualStyle& style,
                                std::vector<GuiDrawCommand>& out, int layer) const;
    void QueueEvent(GuiEvent event);
    void UpdateHover(const std::string& next, GuiPoint p);
    std::vector<std::string> FocusOrder() const;
    void MarkDirty();

    std::unordered_map<std::string, GuiNode> nodes_;
    std::unordered_map<std::string, GuiTheme> themes_;
    std::string activeTheme_;
    ui::StyleSheet styleSheet_;
    bool modernStyleEnabled_ = true;
    ui::DockManager dockManager_;
    GuiViewport viewport_{};
    std::deque<GuiEvent> events_;
    std::vector<GuiAnimation> animations_;
    std::string focusedId_;
    std::string hoveredId_;
    std::string pressedId_;
    std::string dragId_;
    GuiPoint lastPointer_{};
    bool lastFocusFromKeyboard_ = false;
    std::uint64_t revision_ = 1;
    bool layoutDirty_ = true;
};

GuiWidgetType ParseGuiWidgetType(const std::string& name);
const char* GuiWidgetTypeName(GuiWidgetType type);
const char* GuiEventTypeName(GuiEventType type);

} // namespace vek
