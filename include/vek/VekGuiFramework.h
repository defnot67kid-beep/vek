#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <vek/VekScriptEngine.h>

namespace vek {

// VEK 2.8 retained-mode GUI framework. The framework is renderer/input
// backend-neutral: hosts feed pointer/key/text events and consume a computed
// layout/render snapshot. No native GPU/window handle is exposed to scripts.

struct GuiPoint { float x = 0.0f; float y = 0.0f; };
struct GuiSize { float width = 0.0f; float height = 0.0f; };
struct GuiRectF { float x = 0.0f; float y = 0.0f; float width = 0.0f; float height = 0.0f; };
struct GuiEdges { float left = 0.0f; float top = 0.0f; float right = 0.0f; float bottom = 0.0f; };
struct GuiColor { float r = 1.0f; float g = 1.0f; float b = 1.0f; float a = 1.0f; };

bool GuiContains(const GuiRectF& rect, GuiPoint point);

// The type list deliberately covers runtime HUDs, menus and editor/tool UI.
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

enum class GuiDrawType : std::uint8_t { Box = 0, Text, Image, ClipPush, ClipPop, Custom };

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
    float opacity = 1.0f;
    float borderWidth = 0.0f;
    float radius = 0.0f;
    float fontSize = 16.0f;
    float lineHeight = 1.2f;
    float letterSpacing = 0.0f;
    std::string fontFamily;
    std::string icon;
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
    bool draggable = false;
    bool droppable = false;
    bool clipChildren = false;
    bool virtualized = false;
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
    GuiRectF rect{};
    GuiVisualStyle style{};
    std::string text;
    std::string image;
    int layer = 0;
    bool clip = false;
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
    std::string focusedId;
    std::string hoveredId;
};

class GuiFramework {
public:
    static constexpr const char* ApiVersion = "2.8";
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
    void BuildDrawNode(const std::string& id, std::vector<GuiDrawCommand>& out, int inheritedLayer) const;
    void QueueEvent(GuiEvent event);
    void UpdateHover(const std::string& next, GuiPoint p);
    std::vector<std::string> FocusOrder() const;
    void MarkDirty();

    std::unordered_map<std::string, GuiNode> nodes_;
    std::unordered_map<std::string, GuiTheme> themes_;
    std::string activeTheme_;
    GuiViewport viewport_{};
    std::deque<GuiEvent> events_;
    std::vector<GuiAnimation> animations_;
    std::string focusedId_;
    std::string hoveredId_;
    std::string pressedId_;
    GuiPoint lastPointer_{};
    std::uint64_t revision_ = 1;
    bool layoutDirty_ = true;
};

GuiWidgetType ParseGuiWidgetType(const std::string& name);
const char* GuiWidgetTypeName(GuiWidgetType type);
const char* GuiEventTypeName(GuiEventType type);

} // namespace vek
