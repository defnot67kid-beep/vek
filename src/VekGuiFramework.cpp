#include <vek/VekGuiFramework.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <set>
#include <unordered_set>

namespace vek {
namespace {

float Finite(float v, float fallback = 0.0f) { return std::isfinite(v) ? v : fallback; }
float ClampNonNegative(float v) { return std::max(0.0f, Finite(v)); }
float Clamp01(float v) { return std::clamp(Finite(v, 1.0f), 0.0f, 1.0f); }

std::string Lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

GuiColor ColorFromValue(const VekValue& v, GuiColor fallback) {
    if (!v.IsMap()) return fallback;
    fallback.r = static_cast<float>(v.Get("r").AsNumber(fallback.r));
    fallback.g = static_cast<float>(v.Get("g").AsNumber(fallback.g));
    fallback.b = static_cast<float>(v.Get("b").AsNumber(fallback.b));
    fallback.a = static_cast<float>(v.Get("a").AsNumber(fallback.a));
    fallback.r = Clamp01(fallback.r); fallback.g = Clamp01(fallback.g);
    fallback.b = Clamp01(fallback.b); fallback.a = Clamp01(fallback.a);
    return fallback;
}

VekValue ColorToValue(const GuiColor& c) {
    VekMap m; m["r"] = c.r; m["g"] = c.g; m["b"] = c.b; m["a"] = c.a; return VekValue(m);
}

GuiEdges EdgesFromValue(const VekValue& v, GuiEdges fallback = {}) {
    if (v.IsNumber()) { float n = static_cast<float>(v.AsNumber()); return {n,n,n,n}; }
    if (!v.IsMap()) return fallback;
    fallback.left = static_cast<float>(v.Get("left").AsNumber(fallback.left));
    fallback.top = static_cast<float>(v.Get("top").AsNumber(fallback.top));
    fallback.right = static_cast<float>(v.Get("right").AsNumber(fallback.right));
    fallback.bottom = static_cast<float>(v.Get("bottom").AsNumber(fallback.bottom));
    return fallback;
}

GuiSizeMode ParseSizeMode(const std::string& value, GuiSizeMode fallback = GuiSizeMode::Content) {
    const auto s = Lower(value);
    if (s == "fixed" || s == "px") return GuiSizeMode::Fixed;
    if (s == "content" || s == "auto") return GuiSizeMode::Content;
    if (s == "fill" || s == "grow") return GuiSizeMode::Fill;
    if (s == "percent" || s == "%") return GuiSizeMode::Percent;
    return fallback;
}
GuiLayoutMode ParseLayout(const std::string& value, GuiLayoutMode fallback = GuiLayoutMode::Column) {
    const auto s=Lower(value); if(s=="absolute")return GuiLayoutMode::Absolute;if(s=="row"||s=="horizontal")return GuiLayoutMode::Row;if(s=="column"||s=="vertical")return GuiLayoutMode::Column;if(s=="grid")return GuiLayoutMode::Grid;if(s=="overlay"||s=="stack")return GuiLayoutMode::Overlay;if(s=="dock")return GuiLayoutMode::Dock;return fallback;
}
GuiAlign ParseAlign(const std::string& value, GuiAlign fallback = GuiAlign::Stretch) {
    const auto s=Lower(value);if(s=="start"||s=="left"||s=="top")return GuiAlign::Start;if(s=="center")return GuiAlign::Center;if(s=="end"||s=="right"||s=="bottom")return GuiAlign::End;if(s=="stretch"||s=="fill")return GuiAlign::Stretch;return fallback;
}
GuiJustify ParseJustify(const std::string& value, GuiJustify fallback = GuiJustify::Start) {
    const auto s=Lower(value);if(s=="start")return GuiJustify::Start;if(s=="center")return GuiJustify::Center;if(s=="end")return GuiJustify::End;if(s=="space_between"||s=="space-between")return GuiJustify::SpaceBetween;if(s=="space_around"||s=="space-around")return GuiJustify::SpaceAround;return fallback;
}
GuiOverflow ParseOverflow(const std::string& value, GuiOverflow fallback = GuiOverflow::Visible) {
    const auto s=Lower(value);if(s=="visible")return GuiOverflow::Visible;if(s=="hidden"||s=="clip")return GuiOverflow::Hidden;if(s=="scroll_x"||s=="x")return GuiOverflow::ScrollX;if(s=="scroll_y"||s=="y")return GuiOverflow::ScrollY;if(s=="scroll"||s=="both")return GuiOverflow::ScrollBoth;return fallback;
}
GuiPositionMode ParsePosition(const std::string& value, GuiPositionMode fallback = GuiPositionMode::Flow) {
    const auto s=Lower(value);if(s=="flow"||s=="relative")return GuiPositionMode::Flow;if(s=="absolute")return GuiPositionMode::Absolute;if(s=="fixed")return GuiPositionMode::Fixed;return fallback;
}
GuiEase ParseEase(const std::string& value) {
    const auto s=Lower(value);if(s=="ease_in"||s=="in")return GuiEase::EaseIn;if(s=="ease_out"||s=="out")return GuiEase::EaseOut;if(s=="ease_in_out"||s=="in_out")return GuiEase::EaseInOut;if(s=="smooth"||s=="smoothstep")return GuiEase::SmoothStep;return GuiEase::Linear;
}
GuiAccessibilityRole ParseRole(const std::string& value) {
    static const std::unordered_map<std::string,GuiAccessibilityRole> m={
        {"window",GuiAccessibilityRole::Window},{"dialog",GuiAccessibilityRole::Dialog},{"group",GuiAccessibilityRole::Group},{"text",GuiAccessibilityRole::Text},{"button",GuiAccessibilityRole::Button},{"checkbox",GuiAccessibilityRole::Checkbox},{"radio",GuiAccessibilityRole::Radio},{"slider",GuiAccessibilityRole::Slider},{"text_field",GuiAccessibilityRole::TextField},{"list",GuiAccessibilityRole::List},{"list_item",GuiAccessibilityRole::ListItem},{"tree",GuiAccessibilityRole::Tree},{"tree_item",GuiAccessibilityRole::TreeItem},{"table",GuiAccessibilityRole::Table},{"row",GuiAccessibilityRole::Row},{"cell",GuiAccessibilityRole::Cell},{"image",GuiAccessibilityRole::Image},{"menu",GuiAccessibilityRole::Menu},{"menu_item",GuiAccessibilityRole::MenuItem},{"tab",GuiAccessibilityRole::Tab},{"tab_panel",GuiAccessibilityRole::TabPanel},{"progress",GuiAccessibilityRole::ProgressBar},{"status",GuiAccessibilityRole::Status},{"toolbar",GuiAccessibilityRole::Toolbar}};
    auto it=m.find(Lower(value));return it==m.end()?GuiAccessibilityRole::None:it->second;
}

void PatchDimension(GuiDimension& d, const VekValue& v) {
    if (v.IsNumber()) { d.mode=GuiSizeMode::Fixed; d.value=static_cast<float>(v.AsNumber()); return; }
    if (!v.IsMap()) return;
    if (!v.Get("mode").IsNil()) d.mode=ParseSizeMode(v.Get("mode").AsString(), d.mode);
    if (!v.Get("value").IsNil()) d.value=static_cast<float>(v.Get("value").AsNumber(d.value));
    if (!v.Get("min").IsNil()) d.min=static_cast<float>(v.Get("min").AsNumber(d.min));
    if (!v.Get("max").IsNil()) d.max=static_cast<float>(v.Get("max").AsNumber(d.max));
    d.min=ClampNonNegative(d.min); d.max=std::max(d.min,ClampNonNegative(d.max));
}

void PatchVisual(GuiVisualStyle& s, const VekValue& v) {
    if (!v.IsMap()) return;
    if (!v.Get("background").IsNil()) s.background=ColorFromValue(v.Get("background"),s.background);
    if (!v.Get("foreground").IsNil()) s.foreground=ColorFromValue(v.Get("foreground"),s.foreground);
    if (!v.Get("border").IsNil()) s.border=ColorFromValue(v.Get("border"),s.border);
    if (!v.Get("accent").IsNil()) s.accent=ColorFromValue(v.Get("accent"),s.accent);
    if (!v.Get("opacity").IsNil()) s.opacity=Clamp01(static_cast<float>(v.Get("opacity").AsNumber(s.opacity)));
    if (!v.Get("border_width").IsNil()) s.borderWidth=ClampNonNegative(static_cast<float>(v.Get("border_width").AsNumber(s.borderWidth)));
    if (!v.Get("radius").IsNil()) s.radius=ClampNonNegative(static_cast<float>(v.Get("radius").AsNumber(s.radius)));
    if (!v.Get("font_size").IsNil()) s.fontSize=std::clamp(static_cast<float>(v.Get("font_size").AsNumber(s.fontSize)),4.0f,512.0f);
    if (!v.Get("line_height").IsNil()) s.lineHeight=std::clamp(static_cast<float>(v.Get("line_height").AsNumber(s.lineHeight)),0.5f,8.0f);
    if (!v.Get("letter_spacing").IsNil()) s.letterSpacing=static_cast<float>(v.Get("letter_spacing").AsNumber(s.letterSpacing));
    if (!v.Get("font").IsNil()) s.fontFamily=v.Get("font").AsString();
    if (!v.Get("icon").IsNil()) s.icon=v.Get("icon").AsString();
    if (!v.Get("z").IsNil()) s.zIndex=static_cast<int>(v.Get("z").AsNumber(s.zIndex));
}

VekValue RectToValue(const GuiRectF& r){VekMap m;m["x"]=r.x;m["y"]=r.y;m["width"]=r.width;m["height"]=r.height;return VekValue(m);}

VekValue EventToValue(const GuiEvent& e){VekMap m;m["type"]=GuiEventTypeName(e.type);m["target"]=e.targetId;m["current_target"]=e.currentTargetId;m["x"]=e.pointer.x;m["y"]=e.pointer.y;m["key"]=e.key;m["text"]=e.text;m["value"]=e.value;m["checked"]=e.checked;m["data"]=e.data;return VekValue(m);}

float EaseValue(GuiEase ease,float t){t=std::clamp(t,0.0f,1.0f);switch(ease){case GuiEase::EaseIn:return t*t;case GuiEase::EaseOut:return 1.0f-(1.0f-t)*(1.0f-t);case GuiEase::EaseInOut:return t<0.5f?2*t*t:1-std::pow(-2*t+2,2.0f)/2.0f;case GuiEase::SmoothStep:return t*t*(3-2*t);default:return t;}}

float ResolveDimension(const GuiDimension& d,float available,float content){float v=content;switch(d.mode){case GuiSizeMode::Fixed:v=d.value;break;case GuiSizeMode::Fill:v=available;break;case GuiSizeMode::Percent:v=available*std::clamp(d.value,0.0f,1.0f);break;case GuiSizeMode::Content:default:v=content;break;}return std::clamp(ClampNonNegative(v),d.min,std::max(d.min,d.max));}

bool IsFocusableType(GuiWidgetType t){switch(t){case GuiWidgetType::Button:case GuiWidgetType::IconButton:case GuiWidgetType::Toggle:case GuiWidgetType::Checkbox:case GuiWidgetType::Radio:case GuiWidgetType::Slider:case GuiWidgetType::RangeSlider:case GuiWidgetType::TextInput:case GuiWidgetType::TextArea:case GuiWidgetType::SearchBox:case GuiWidgetType::NumberInput:case GuiWidgetType::ComboBox:case GuiWidgetType::Dropdown:case GuiWidgetType::MenuItem:case GuiWidgetType::Keybind:case GuiWidgetType::ColorPicker:return true;default:return false;}}

} // namespace

bool GuiContains(const GuiRectF& r,GuiPoint p){return p.x>=r.x&&p.y>=r.y&&p.x<=r.x+r.width&&p.y<=r.y+r.height;}

GuiWidgetType ParseGuiWidgetType(const std::string& name){
    static const std::unordered_map<std::string,GuiWidgetType> m={
        {"root",GuiWidgetType::Root},{"window",GuiWidgetType::Window},{"modal",GuiWidgetType::Modal},{"panel",GuiWidgetType::Panel},{"card",GuiWidgetType::Card},{"dock_space",GuiWidgetType::DockSpace},{"dock",GuiWidgetType::DockSpace},{"split_pane",GuiWidgetType::SplitPane},{"tabs",GuiWidgetType::TabView},{"tab_view",GuiWidgetType::TabView},{"tab",GuiWidgetType::TabItem},{"menu_bar",GuiWidgetType::MenuBar},{"menu",GuiWidgetType::Menu},{"menu_item",GuiWidgetType::MenuItem},{"toolbar",GuiWidgetType::ToolBar},{"status_bar",GuiWidgetType::StatusBar},{"label",GuiWidgetType::Label},{"rich_text",GuiWidgetType::RichText},{"button",GuiWidgetType::Button},{"icon_button",GuiWidgetType::IconButton},{"toggle",GuiWidgetType::Toggle},{"checkbox",GuiWidgetType::Checkbox},{"radio",GuiWidgetType::Radio},{"slider",GuiWidgetType::Slider},{"range_slider",GuiWidgetType::RangeSlider},{"progress",GuiWidgetType::Progress},{"spinner",GuiWidgetType::Spinner},{"text_input",GuiWidgetType::TextInput},{"text_area",GuiWidgetType::TextArea},{"search",GuiWidgetType::SearchBox},{"search_box",GuiWidgetType::SearchBox},{"number_input",GuiWidgetType::NumberInput},{"combo",GuiWidgetType::ComboBox},{"combo_box",GuiWidgetType::ComboBox},{"dropdown",GuiWidgetType::Dropdown},{"list",GuiWidgetType::List},{"list_item",GuiWidgetType::ListItem},{"tree",GuiWidgetType::Tree},{"tree_item",GuiWidgetType::TreeItem},{"table",GuiWidgetType::Table},{"table_row",GuiWidgetType::TableRow},{"table_cell",GuiWidgetType::TableCell},{"scroll",GuiWidgetType::Scroll},{"image",GuiWidgetType::Image},{"canvas",GuiWidgetType::Canvas},{"separator",GuiWidgetType::Separator},{"spacer",GuiWidgetType::Spacer},{"tooltip",GuiWidgetType::Tooltip},{"context_menu",GuiWidgetType::ContextMenu},{"popup",GuiWidgetType::Popup},{"toast",GuiWidgetType::Toast},{"badge",GuiWidgetType::Badge},{"property_grid",GuiWidgetType::PropertyGrid},{"color_picker",GuiWidgetType::ColorPicker},{"keybind",GuiWidgetType::Keybind},{"viewport",GuiWidgetType::Viewport},{"graph",GuiWidgetType::Graph},{"timeline",GuiWidgetType::Timeline}};
    auto it=m.find(Lower(name));return it==m.end()?GuiWidgetType::Panel:it->second;
}

const char* GuiWidgetTypeName(GuiWidgetType t){switch(t){case GuiWidgetType::Root:return"root";case GuiWidgetType::Window:return"window";case GuiWidgetType::Modal:return"modal";case GuiWidgetType::Panel:return"panel";case GuiWidgetType::Card:return"card";case GuiWidgetType::DockSpace:return"dock_space";case GuiWidgetType::SplitPane:return"split_pane";case GuiWidgetType::TabView:return"tab_view";case GuiWidgetType::TabItem:return"tab";case GuiWidgetType::MenuBar:return"menu_bar";case GuiWidgetType::Menu:return"menu";case GuiWidgetType::MenuItem:return"menu_item";case GuiWidgetType::ToolBar:return"toolbar";case GuiWidgetType::StatusBar:return"status_bar";case GuiWidgetType::Label:return"label";case GuiWidgetType::RichText:return"rich_text";case GuiWidgetType::Button:return"button";case GuiWidgetType::IconButton:return"icon_button";case GuiWidgetType::Toggle:return"toggle";case GuiWidgetType::Checkbox:return"checkbox";case GuiWidgetType::Radio:return"radio";case GuiWidgetType::Slider:return"slider";case GuiWidgetType::RangeSlider:return"range_slider";case GuiWidgetType::Progress:return"progress";case GuiWidgetType::Spinner:return"spinner";case GuiWidgetType::TextInput:return"text_input";case GuiWidgetType::TextArea:return"text_area";case GuiWidgetType::SearchBox:return"search_box";case GuiWidgetType::NumberInput:return"number_input";case GuiWidgetType::ComboBox:return"combo_box";case GuiWidgetType::Dropdown:return"dropdown";case GuiWidgetType::List:return"list";case GuiWidgetType::ListItem:return"list_item";case GuiWidgetType::Tree:return"tree";case GuiWidgetType::TreeItem:return"tree_item";case GuiWidgetType::Table:return"table";case GuiWidgetType::TableRow:return"table_row";case GuiWidgetType::TableCell:return"table_cell";case GuiWidgetType::Scroll:return"scroll";case GuiWidgetType::Image:return"image";case GuiWidgetType::Canvas:return"canvas";case GuiWidgetType::Separator:return"separator";case GuiWidgetType::Spacer:return"spacer";case GuiWidgetType::Tooltip:return"tooltip";case GuiWidgetType::ContextMenu:return"context_menu";case GuiWidgetType::Popup:return"popup";case GuiWidgetType::Toast:return"toast";case GuiWidgetType::Badge:return"badge";case GuiWidgetType::PropertyGrid:return"property_grid";case GuiWidgetType::ColorPicker:return"color_picker";case GuiWidgetType::Keybind:return"keybind";case GuiWidgetType::Viewport:return"viewport";case GuiWidgetType::Graph:return"graph";case GuiWidgetType::Timeline:return"timeline";default:return"panel";}}
const char* GuiEventTypeName(GuiEventType t){switch(t){case GuiEventType::Click:return"click";case GuiEventType::DoubleClick:return"double_click";case GuiEventType::Change:return"change";case GuiEventType::Submit:return"submit";case GuiEventType::Focus:return"focus";case GuiEventType::Blur:return"blur";case GuiEventType::PointerDown:return"pointer_down";case GuiEventType::PointerUp:return"pointer_up";case GuiEventType::PointerMove:return"pointer_move";case GuiEventType::PointerEnter:return"pointer_enter";case GuiEventType::PointerLeave:return"pointer_leave";case GuiEventType::Scroll:return"scroll";case GuiEventType::KeyDown:return"key_down";case GuiEventType::KeyUp:return"key_up";case GuiEventType::TextInput:return"text_input";case GuiEventType::DragStart:return"drag_start";case GuiEventType::Drag:return"drag";case GuiEventType::DragEnd:return"drag_end";case GuiEventType::Drop:return"drop";case GuiEventType::Open:return"open";case GuiEventType::Close:return"close";case GuiEventType::Select:return"select";default:return"none";}}

GuiFramework::GuiFramework(){
    GuiTheme dark;dark.id="vek.dark";dark.base.background={0.075f,0.08f,0.095f,1};dark.base.foreground={0.94f,0.95f,0.97f,1};dark.base.border={0.20f,0.22f,0.27f,1};dark.base.accent={0.25f,0.58f,1.0f,1};dark.base.radius=6;dark.base.fontSize=16;themes_[dark.id]=dark;
    GuiTheme light=dark;light.id="vek.light";light.base.background={0.95f,0.96f,0.98f,1};light.base.foreground={0.08f,0.09f,0.11f,1};light.base.border={0.72f,0.74f,0.78f,1};themes_[light.id]=light;
    activeTheme_=dark.id;
}

bool GuiFramework::ValidateParent(const std::string& id,const std::string& parent,std::string* error) const{
    if(parent.empty())return true;if(parent==id){if(error)*error="GUI node cannot parent itself";return false;}auto p=nodes_.find(parent);if(p==nodes_.end()){if(error)*error="GUI parent does not exist: "+parent;return false;}std::unordered_set<std::string> seen;std::string cur=parent;while(!cur.empty()){if(cur==id){if(error)*error="GUI parent cycle detected";return false;}if(!seen.insert(cur).second)break;auto it=nodes_.find(cur);if(it==nodes_.end())break;cur=it->second.parentId;}return true;
}
void GuiFramework::DetachFromParent(const GuiNode& n){if(n.parentId.empty())return;auto it=nodes_.find(n.parentId);if(it==nodes_.end())return;auto& c=it->second.children;c.erase(std::remove(c.begin(),c.end(),n.id),c.end());}
void GuiFramework::AttachToParent(const GuiNode& n){if(n.parentId.empty())return;auto it=nodes_.find(n.parentId);if(it==nodes_.end())return;auto& c=it->second.children;if(std::find(c.begin(),c.end(),n.id)==c.end())c.push_back(n.id);}
void GuiFramework::MarkDirty(){layoutDirty_=true;++revision_;}

bool GuiFramework::Create(const GuiNode& input,std::string* error){
    if(input.id.empty()||input.id.size()>192){if(error)*error="GUI id is empty or too long";return false;}if(nodes_.size()>=MaxNodes&&!nodes_.count(input.id)){if(error)*error="GUI node limit reached";return false;}if(nodes_.count(input.id)){if(error)*error="GUI id already exists: "+input.id;return false;}if(!ValidateParent(input.id,input.parentId,error))return false;GuiNode n=input;if(IsFocusableType(n.type)&&!n.focusable)n.focusable=true;n.children.clear();nodes_.emplace(n.id,n);AttachToParent(nodes_.at(n.id));MarkDirty();return true;
}

bool GuiFramework::ApplyPatch(GuiNode& n,const VekValue& p,std::string* error){
    if(!p.IsMap()){if(error)*error="GUI patch must be a map";return false;}
    if(!p.Get("text").IsNil())n.text=p.Get("text").AsString();if(!p.Get("tooltip").IsNil())n.tooltip=p.Get("tooltip").AsString();if(!p.Get("image").IsNil())n.image=p.Get("image").AsString();if(!p.Get("binding").IsNil())n.dataBinding=p.Get("binding").AsString();
    if(!p.Get("visible").IsNil())n.visible=p.Get("visible").AsBool();if(!p.Get("enabled").IsNil())n.enabled=p.Get("enabled").AsBool();if(!p.Get("focusable").IsNil())n.focusable=p.Get("focusable").AsBool();if(!p.Get("draggable").IsNil())n.draggable=p.Get("draggable").AsBool();if(!p.Get("droppable").IsNil())n.droppable=p.Get("droppable").AsBool();if(!p.Get("clip_children").IsNil())n.clipChildren=p.Get("clip_children").AsBool();if(!p.Get("virtualized").IsNil())n.virtualized=p.Get("virtualized").AsBool();if(!p.Get("checked").IsNil())n.checked=p.Get("checked").AsBool();if(!p.Get("selected").IsNil())n.selected=p.Get("selected").AsBool();if(!p.Get("expanded").IsNil())n.expanded=p.Get("expanded").AsBool();
    if(!p.Get("value").IsNil())n.value=p.Get("value").AsNumber(n.value);if(!p.Get("min").IsNil())n.minValue=p.Get("min").AsNumber(n.minValue);if(!p.Get("max").IsNil())n.maxValue=p.Get("max").AsNumber(n.maxValue);if(!p.Get("step").IsNil())n.step=p.Get("step").AsNumber(n.step);if(!p.Get("selected_index").IsNil())n.selectedIndex=(int)p.Get("selected_index").AsNumber(n.selectedIndex);if(!p.Get("scroll_x").IsNil())n.scrollX=(float)p.Get("scroll_x").AsNumber(n.scrollX);if(!p.Get("scroll_y").IsNil())n.scrollY=(float)p.Get("scroll_y").AsNumber(n.scrollY);if(!p.Get("data").IsNil())n.data=p.Get("data");
    if(auto a=p.Get("classes");a.IsArray()){n.classes.clear();for(const auto& x:*a.AsArray())n.classes.push_back(x.AsString());}
    auto l=p.Get("layout");if(l.IsMap()){
        if(!l.Get("mode").IsNil())n.layout.mode=ParseLayout(l.Get("mode").AsString(),n.layout.mode);if(!l.Get("position").IsNil())n.layout.position=ParsePosition(l.Get("position").AsString(),n.layout.position);if(!l.Get("width").IsNil())PatchDimension(n.layout.width,l.Get("width"));if(!l.Get("height").IsNil())PatchDimension(n.layout.height,l.Get("height"));if(!l.Get("margin").IsNil())n.layout.margin=EdgesFromValue(l.Get("margin"),n.layout.margin);if(!l.Get("padding").IsNil())n.layout.padding=EdgesFromValue(l.Get("padding"),n.layout.padding);if(!l.Get("gap").IsNil())n.layout.gap=ClampNonNegative((float)l.Get("gap").AsNumber(n.layout.gap));if(!l.Get("columns").IsNil())n.layout.gridColumns=std::max(1,(int)l.Get("columns").AsNumber(n.layout.gridColumns));if(!l.Get("align").IsNil())n.layout.alignItems=ParseAlign(l.Get("align").AsString(),n.layout.alignItems);if(!l.Get("align_self").IsNil())n.layout.alignSelf=ParseAlign(l.Get("align_self").AsString(),n.layout.alignSelf);if(!l.Get("justify").IsNil())n.layout.justify=ParseJustify(l.Get("justify").AsString(),n.layout.justify);if(!l.Get("overflow").IsNil())n.layout.overflow=ParseOverflow(l.Get("overflow").AsString(),n.layout.overflow);if(!l.Get("aspect").IsNil())n.layout.aspectRatio=(float)l.Get("aspect").AsNumber(n.layout.aspectRatio);if(!l.Get("x").IsNil())n.layout.absoluteX=(float)l.Get("x").AsNumber(n.layout.absoluteX);if(!l.Get("y").IsNil())n.layout.absoluteY=(float)l.Get("y").AsNumber(n.layout.absoluteY);
    }
    if(!p.Get("style").IsNil()){PatchVisual(n.visual,p.Get("style"));n.hasVisual=true;}if(!p.Get("hover_style").IsNil()){PatchVisual(n.hoverVisual,p.Get("hover_style"));n.hasHoverVisual=true;}if(!p.Get("pressed_style").IsNil()){PatchVisual(n.pressedVisual,p.Get("pressed_style"));n.hasPressedVisual=true;}if(!p.Get("focus_style").IsNil()){PatchVisual(n.focusVisual,p.Get("focus_style"));n.hasFocusVisual=true;}if(!p.Get("disabled_style").IsNil()){PatchVisual(n.disabledVisual,p.Get("disabled_style"));n.hasDisabledVisual=true;}
    auto ac=p.Get("accessibility");if(ac.IsMap()){if(!ac.Get("role").IsNil())n.accessibility.role=ParseRole(ac.Get("role").AsString());if(!ac.Get("label").IsNil())n.accessibility.label=ac.Get("label").AsString();if(!ac.Get("hint").IsNil())n.accessibility.hint=ac.Get("hint").AsString();if(!ac.Get("value").IsNil())n.accessibility.valueText=ac.Get("value").AsString();if(!ac.Get("order").IsNil())n.accessibility.navigationOrder=(int)ac.Get("order").AsNumber();if(!ac.Get("hidden").IsNil())n.accessibility.hidden=ac.Get("hidden").AsBool();}
    if(n.maxValue<n.minValue)std::swap(n.maxValue,n.minValue);n.value=std::clamp(n.value,n.minValue,n.maxValue);return true;
}

bool GuiFramework::CreateValue(const VekValue& v,std::string* error){if(!v.IsMap()){if(error)*error="GUI definition must be a map";return false;}GuiNode n;n.id=v.Get("id").AsString();n.type=v.Get("type").IsNil()?GuiWidgetType::Panel:ParseGuiWidgetType(v.Get("type").AsString());n.parentId=v.Get("parent").IsNil()?std::string{}:v.Get("parent").AsString();if(!ApplyPatch(n,v,error))return false;return Create(n,error);}
bool GuiFramework::Patch(const std::string&id,const VekValue&p,std::string*error){auto it=nodes_.find(id);if(it==nodes_.end()){if(error)*error="GUI node not found: "+id;return false;}std::string newParent=it->second.parentId;if(!p.Get("parent").IsNil())newParent=p.Get("parent").AsString();if(newParent!=it->second.parentId&&!ValidateParent(id,newParent,error))return false;const std::string oldParent=it->second.parentId;if(!ApplyPatch(it->second,p,error))return false;if(newParent!=oldParent){DetachFromParent(it->second);it->second.parentId=newParent;AttachToParent(it->second);}MarkDirty();return true;}
bool GuiFramework::Reparent(const std::string&id,const std::string&parent,std::string*error){VekMap p;p["parent"]=parent;return Patch(id,VekValue(p),error);}

bool GuiFramework::Remove(const std::string&id,bool recursive){auto it=nodes_.find(id);if(it==nodes_.end())return false;if(!recursive&&!it->second.children.empty())return false;auto children=it->second.children;for(const auto& c:children)Remove(c,true);GuiNode copy=it->second;DetachFromParent(copy);nodes_.erase(id);if(focusedId_==id)focusedId_.clear();if(hoveredId_==id)hoveredId_.clear();if(pressedId_==id)pressedId_.clear();animations_.erase(std::remove_if(animations_.begin(),animations_.end(),[&](const GuiAnimation&a){return a.nodeId==id;}),animations_.end());MarkDirty();return true;}
void GuiFramework::Clear(){nodes_.clear();events_.clear();animations_.clear();focusedId_.clear();hoveredId_.clear();pressedId_.clear();MarkDirty();}
GuiNode*GuiFramework::Find(const std::string&id){auto it=nodes_.find(id);return it==nodes_.end()?nullptr:&it->second;}const GuiNode*GuiFramework::Find(const std::string&id)const{auto it=nodes_.find(id);return it==nodes_.end()?nullptr:&it->second;}
std::vector<std::string>GuiFramework::ChildrenOf(const std::string&p)const{auto it=nodes_.find(p);return it==nodes_.end()?std::vector<std::string>{}:it->second.children;}
std::vector<std::string>GuiFramework::Roots()const{std::vector<std::string>r;for(auto&[id,n]:nodes_)if(n.parentId.empty())r.push_back(id);std::sort(r.begin(),r.end());return r;}

bool GuiFramework::DefineTheme(const GuiTheme&t,std::string*error){if(t.id.empty()||t.id.size()>128){if(error)*error="GUI theme id invalid";return false;}themes_[t.id]=t;++revision_;return true;}
bool GuiFramework::DefineThemeValue(const VekValue&v,std::string*error){if(!v.IsMap()){if(error)*error="GUI theme must be a map";return false;}GuiTheme t;t.id=v.Get("id").AsString();PatchVisual(t.base,v.Get("base"));auto vars=v.Get("variables");if(vars.IsMap())for(auto&[k,x]:*vars.AsMap())t.variables[k]=x;auto classes=v.Get("classes");if(classes.IsMap())for(auto&[k,x]:*classes.AsMap()){GuiVisualStyle s=t.base;PatchVisual(s,x);t.classes[k]=s;}return DefineTheme(t,error);}
bool GuiFramework::SetTheme(const std::string&id){if(!themes_.count(id))return false;activeTheme_=id;MarkDirty();return true;}
void GuiFramework::SetViewport(const GuiViewport&v){viewport_=v;viewport_.width=std::clamp(Finite(v.width,1280),1.0f,65536.0f);viewport_.height=std::clamp(Finite(v.height,720),1.0f,65536.0f);viewport_.dpiScale=std::clamp(Finite(v.dpiScale,1),0.25f,8.0f);MarkDirty();}

GuiSize GuiFramework::MeasureNode(const GuiNode&n,GuiSize avail)const{float textW=std::min(avail.width,std::max(24.0f,(float)n.text.size()*n.visual.fontSize*0.55f+16.0f));float textH=n.visual.fontSize*n.visual.lineHeight+12.0f;float w=ResolveDimension(n.layout.width,avail.width,textW+n.layout.padding.left+n.layout.padding.right);float h=ResolveDimension(n.layout.height,avail.height,textH+n.layout.padding.top+n.layout.padding.bottom);if(n.layout.aspectRatio>0){if(n.layout.width.mode!=GuiSizeMode::Content&&n.layout.height.mode==GuiSizeMode::Content)h=w/n.layout.aspectRatio;else if(n.layout.height.mode!=GuiSizeMode::Content&&n.layout.width.mode==GuiSizeMode::Content)w=h*n.layout.aspectRatio;}return{w,h};}

void GuiFramework::LayoutNode(const std::string&id,GuiRectF available){
    auto it=nodes_.find(id);if(it==nodes_.end())return;GuiNode& n=it->second;if(!n.visible){n.computed={};return;}GuiSize measured=MeasureNode(n,{available.width,available.height});float x=available.x+n.layout.margin.left;float y=available.y+n.layout.margin.top;if(n.layout.position!=GuiPositionMode::Flow){x=(n.layout.position==GuiPositionMode::Fixed?viewport_.safeArea.left:available.x)+n.layout.absoluteX;y=(n.layout.position==GuiPositionMode::Fixed?viewport_.safeArea.top:available.y)+n.layout.absoluteY;}float maxW=std::max(0.0f,available.width-n.layout.margin.left-n.layout.margin.right);float maxH=std::max(0.0f,available.height-n.layout.margin.top-n.layout.margin.bottom);float w=std::min(measured.width,maxW);float h=std::min(measured.height,maxH);if(n.layout.alignSelf==GuiAlign::Stretch&&n.layout.position==GuiPositionMode::Flow&&n.layout.width.mode==GuiSizeMode::Content)w=maxW;n.computed={x,y,w,h};
    GuiRectF content{x+n.layout.padding.left,y+n.layout.padding.top,std::max(0.0f,w-n.layout.padding.left-n.layout.padding.right),std::max(0.0f,h-n.layout.padding.top-n.layout.padding.bottom)};if(n.children.empty())return;
    std::vector<std::string> flow;std::vector<std::string> absolute;for(auto&c:n.children){auto ci=nodes_.find(c);if(ci==nodes_.end()||!ci->second.visible)continue;(ci->second.layout.position==GuiPositionMode::Flow?flow:absolute).push_back(c);}for(auto&c:absolute)LayoutNode(c,content);
    const float gap=n.layout.gap; if(n.layout.mode==GuiLayoutMode::Overlay||n.layout.mode==GuiLayoutMode::Absolute){for(auto&c:flow)LayoutNode(c,content);return;}
    if(n.layout.mode==GuiLayoutMode::Grid){int cols=std::max(1,n.layout.gridColumns);float cellW=std::max(0.0f,(content.width-gap*(cols-1))/cols);for(std::size_t i=0;i<flow.size();++i){int col=(int)(i%cols),row=(int)(i/cols);float rowH=80.0f;auto ci=nodes_.find(flow[i]);if(ci!=nodes_.end())rowH=MeasureNode(ci->second,{cellW,content.height}).height;LayoutNode(flow[i],{content.x+col*(cellW+gap),content.y+row*(rowH+gap),cellW,rowH});}return;}
    bool row=n.layout.mode==GuiLayoutMode::Row;float cursor=row?content.x:content.y;float axisAvail=row?content.width:content.height;std::vector<float> sizes;float used=0;int fills=0;for(auto&c:flow){auto&cn=nodes_.at(c);auto ms=MeasureNode(cn,{content.width,content.height});float s=row?ms.width:ms.height;bool fill=row?cn.layout.width.mode==GuiSizeMode::Fill:cn.layout.height.mode==GuiSizeMode::Fill;if(fill){sizes.push_back(-1);fills++;}else{sizes.push_back(s);used+=s;}}used+=gap*std::max<int>(0,(int)flow.size()-1);float remaining=std::max(0.0f,axisAvail-used);float fillSize=fills?remaining/fills:0;float total=used+(fills?remaining:0);float extra=std::max(0.0f,axisAvail-total);float localGap=gap;if(n.layout.justify==GuiJustify::Center)cursor+=extra*0.5f;else if(n.layout.justify==GuiJustify::End)cursor+=extra;else if(n.layout.justify==GuiJustify::SpaceBetween&&flow.size()>1)localGap+=extra/(flow.size()-1);else if(n.layout.justify==GuiJustify::SpaceAround&&!flow.empty()){localGap+=extra/flow.size();cursor+=localGap*0.5f;}
    for(std::size_t i=0;i<flow.size();++i){auto&cn=nodes_.at(flow[i]);float axis=sizes[i]<0?fillSize:sizes[i];auto ms=MeasureNode(cn,{content.width,content.height});GuiRectF a;if(row){float cross=cn.layout.alignSelf==GuiAlign::Stretch?content.height:ms.height;float cy=content.y;if(cn.layout.alignSelf==GuiAlign::Center)cy+=std::max(0.0f,(content.height-cross)*0.5f);else if(cn.layout.alignSelf==GuiAlign::End)cy+=std::max(0.0f,content.height-cross);a={cursor,cy,axis,cross};cursor+=axis+localGap;}else{float cross=cn.layout.alignSelf==GuiAlign::Stretch?content.width:ms.width;float cx=content.x;if(cn.layout.alignSelf==GuiAlign::Center)cx+=std::max(0.0f,(content.width-cross)*0.5f);else if(cn.layout.alignSelf==GuiAlign::End)cx+=std::max(0.0f,content.width-cross);a={cx,cursor,cross,axis};cursor+=axis+localGap;}LayoutNode(flow[i],a);}
}

void GuiFramework::Layout(){GuiRectF root{viewport_.safeArea.left,viewport_.safeArea.top,std::max(0.0f,viewport_.width-viewport_.safeArea.left-viewport_.safeArea.right),std::max(0.0f,viewport_.height-viewport_.safeArea.top-viewport_.safeArea.bottom)};for(auto&id:Roots())LayoutNode(id,root);layoutDirty_=false;}

GuiVisualStyle GuiFramework::ResolveVisual(const GuiNode&n)const{GuiVisualStyle s;auto ti=themes_.find(activeTheme_);if(ti!=themes_.end()){s=ti->second.base;for(auto&cls:n.classes){auto ci=ti->second.classes.find(cls);if(ci!=ti->second.classes.end())s=ci->second;}}if(n.hasVisual)s=n.visual;if(!n.enabled&&n.hasDisabledVisual)s=n.disabledVisual;else if(n.pressed&&n.hasPressedVisual)s=n.pressedVisual;else if(n.focused&&n.hasFocusVisual)s=n.focusVisual;else if(n.hovered&&n.hasHoverVisual)s=n.hoverVisual;return s;}

std::string GuiFramework::HitTest(GuiPoint p)const{std::string best;int bestZ=std::numeric_limits<int>::min();float bestArea=std::numeric_limits<float>::max();for(auto&[id,n]:nodes_){if(!n.visible||!n.enabled||n.computed.width<=0||n.computed.height<=0||!GuiContains(n.computed,p))continue;int z=ResolveVisual(n).zIndex;float area=n.computed.width*n.computed.height;if(z>bestZ||(z==bestZ&&area<bestArea)){best=id;bestZ=z;bestArea=area;}}return best;}
void GuiFramework::QueueEvent(GuiEvent e){if(events_.size()>=MaxEvents)events_.pop_front();events_.push_back(std::move(e));}
void GuiFramework::UpdateHover(const std::string&next,GuiPoint p){if(next==hoveredId_)return;if(auto*old=Find(hoveredId_)){old->hovered=false;QueueEvent({GuiEventType::PointerLeave,old->id,old->id,p});}hoveredId_=next;if(auto*n=Find(hoveredId_)){n->hovered=true;QueueEvent({GuiEventType::PointerEnter,n->id,n->id,p});}++revision_;}

void GuiFramework::FeedInput(const GuiInputEvent&i){if(layoutDirty_)Layout();lastPointer_=i.pointer;std::string hit=HitTest(i.pointer);if(i.type==GuiInputType::PointerMove){UpdateHover(hit,i.pointer);if(!pressedId_.empty()){auto*n=Find(pressedId_);if(n&&n->draggable)QueueEvent({GuiEventType::Drag,n->id,n->id,i.pointer});}if(!hit.empty())QueueEvent({GuiEventType::PointerMove,hit,hit,i.pointer});return;}if(i.type==GuiInputType::PointerDown){UpdateHover(hit,i.pointer);pressedId_=hit;if(auto*n=Find(hit)){n->pressed=true;if(n->focusable)Focus(hit);QueueEvent({GuiEventType::PointerDown,hit,hit,i.pointer});if(n->draggable)QueueEvent({GuiEventType::DragStart,hit,hit,i.pointer});}return;}if(i.type==GuiInputType::PointerUp){UpdateHover(hit,i.pointer);std::string was=pressedId_;if(auto*n=Find(was)){n->pressed=false;QueueEvent({GuiEventType::PointerUp,was,was,i.pointer});if(n->draggable)QueueEvent({GuiEventType::DragEnd,was,was,i.pointer});if(!was.empty()&&was==hit){if(n->type==GuiWidgetType::Checkbox||n->type==GuiWidgetType::Toggle){n->checked=!n->checked;GuiEvent ch{GuiEventType::Change,n->id,n->id,i.pointer};ch.checked=n->checked;ch.value=n->checked?1.0:0.0;QueueEvent(ch);}QueueEvent({GuiEventType::Click,was,was,i.pointer});}if(!hit.empty()&&hit!=was){auto*d=Find(hit);if(d&&d->droppable){GuiEvent drop{GuiEventType::Drop,hit,hit,i.pointer};drop.text=was;QueueEvent(drop);}}}pressedId_.clear();++revision_;return;}if(i.type==GuiInputType::Scroll){if(!hit.empty()){auto*n=Find(hit);if(n){n->scrollX+=i.scrollX;n->scrollY+=i.scrollY;GuiEvent e{GuiEventType::Scroll,hit,hit,i.pointer};e.value=i.scrollY;QueueEvent(e);++revision_;}}return;}if(i.type==GuiInputType::KeyDown){if(i.key==9){FocusNext(i.shift);return;}if(!focusedId_.empty()){GuiEvent e{GuiEventType::KeyDown,focusedId_,focusedId_,i.pointer};e.key=i.key;QueueEvent(e);if(i.key==13)QueueEvent({GuiEventType::Submit,focusedId_,focusedId_,i.pointer});}return;}if(i.type==GuiInputType::KeyUp){if(!focusedId_.empty()){GuiEvent e{GuiEventType::KeyUp,focusedId_,focusedId_,i.pointer};e.key=i.key;QueueEvent(e);}return;}if(i.type==GuiInputType::TextInput&&!focusedId_.empty()){auto*n=Find(focusedId_);if(n&&(n->type==GuiWidgetType::TextInput||n->type==GuiWidgetType::TextArea||n->type==GuiWidgetType::SearchBox||n->type==GuiWidgetType::NumberInput)){n->text+=i.text;GuiEvent e{GuiEventType::TextInput,n->id,n->id,i.pointer};e.text=i.text;QueueEvent(e);GuiEvent ch{GuiEventType::Change,n->id,n->id,i.pointer};ch.text=n->text;QueueEvent(ch);++revision_;}}}

bool GuiFramework::PollEvent(GuiEvent&e){if(events_.empty())return false;e=std::move(events_.front());events_.pop_front();return true;}
std::vector<GuiEvent>GuiFramework::DrainEvents(std::size_t max){std::vector<GuiEvent>out;max=std::min(max,events_.size());out.reserve(max);while(max--){out.push_back(std::move(events_.front()));events_.pop_front();}return out;}
std::vector<std::string>GuiFramework::FocusOrder()const{std::vector<const GuiNode*>v;for(auto&[id,n]:nodes_)if(n.visible&&n.enabled&&n.focusable&&!n.accessibility.hidden)v.push_back(&n);std::sort(v.begin(),v.end(),[](auto*a,auto*b){if(a->accessibility.navigationOrder!=b->accessibility.navigationOrder)return a->accessibility.navigationOrder<b->accessibility.navigationOrder;if(a->computed.y!=b->computed.y)return a->computed.y<b->computed.y;if(a->computed.x!=b->computed.x)return a->computed.x<b->computed.x;return a->id<b->id;});std::vector<std::string>r;for(auto*n:v)r.push_back(n->id);return r;}
bool GuiFramework::Focus(const std::string&id){auto*n=Find(id);if(!n||!n->visible||!n->enabled||!n->focusable)return false;if(focusedId_==id)return true;if(auto*old=Find(focusedId_)){old->focused=false;QueueEvent({GuiEventType::Blur,old->id,old->id,lastPointer_});}focusedId_=id;n->focused=true;QueueEvent({GuiEventType::Focus,id,id,lastPointer_});++revision_;return true;}
bool GuiFramework::FocusNext(bool reverse){if(layoutDirty_)Layout();auto order=FocusOrder();if(order.empty())return false;auto it=std::find(order.begin(),order.end(),focusedId_);std::size_t idx=0;if(it!=order.end()){std::size_t cur=(std::size_t)std::distance(order.begin(),it);idx=reverse?(cur+order.size()-1)%order.size():(cur+1)%order.size();}else idx=reverse?order.size()-1:0;return Focus(order[idx]);}
void GuiFramework::ClearFocus(){if(auto*n=Find(focusedId_)){n->focused=false;QueueEvent({GuiEventType::Blur,n->id,n->id,lastPointer_});}focusedId_.clear();++revision_;}

bool GuiFramework::StartAnimation(const GuiAnimation&a,std::string*error){if(!Find(a.nodeId)){if(error)*error="animation node not found";return false;}if(a.property.empty()){if(error)*error="animation property missing";return false;}if(animations_.size()>=MaxAnimations){if(error)*error="animation limit reached";return false;}GuiAnimation x=a;x.duration=std::clamp(std::isfinite(x.duration)?x.duration:0.2,0.001,3600.0);x.elapsed=0;animations_.push_back(x);return true;}
bool GuiFramework::StopAnimations(const std::string&id,const std::string&property){auto old=animations_.size();animations_.erase(std::remove_if(animations_.begin(),animations_.end(),[&](const GuiAnimation&a){return a.nodeId==id&&(property.empty()||a.property==property);}),animations_.end());return old!=animations_.size();}
void GuiFramework::Step(double dt){if(!std::isfinite(dt)||dt<=0)return;dt=std::min(dt,0.25);for(std::size_t i=0;i<animations_.size();){auto&a=animations_[i];a.elapsed+=dt;float t=(float)std::clamp(a.elapsed/a.duration,0.0,1.0);float e=EaseValue(a.ease,t);double val=a.from+(a.to-a.from)*e;auto*n=Find(a.nodeId);if(n){if(a.property=="opacity")n->visual.opacity=Clamp01((float)val);else if(a.property=="x")n->layout.absoluteX=(float)val;else if(a.property=="y")n->layout.absoluteY=(float)val;else if(a.property=="width"){n->layout.width.mode=GuiSizeMode::Fixed;n->layout.width.value=(float)val;}else if(a.property=="height"){n->layout.height.mode=GuiSizeMode::Fixed;n->layout.height.value=(float)val;}else if(a.property=="value")n->value=std::clamp(val,n->minValue,n->maxValue);else if(a.property=="scroll_x")n->scrollX=(float)val;else if(a.property=="scroll_y")n->scrollY=(float)val;else if(a.property=="radius")n->visual.radius=std::max(0.0f,(float)val);}bool done=t>=1.0f;if(done&&a.loop){a.elapsed=0;if(a.pingPong){std::swap(a.from,a.to);a.reverse=!a.reverse;}++i;}else if(done)animations_.erase(animations_.begin()+i);else ++i;}MarkDirty();}

void GuiFramework::BuildDrawNode(const std::string&id,std::vector<GuiDrawCommand>&out,int inherited)const{auto it=nodes_.find(id);if(it==nodes_.end())return;const auto&n=it->second;if(!n.visible||n.computed.width<=0||n.computed.height<=0)return;auto style=ResolveVisual(n);int layer=inherited+style.zIndex;bool clips=n.clipChildren||n.layout.overflow!=GuiOverflow::Visible;if(clips){GuiDrawCommand c;c.type=GuiDrawType::ClipPush;c.nodeId=id;c.rect=n.computed;c.layer=layer;c.clip=true;out.push_back(c);}GuiDrawCommand box;box.type=(n.type==GuiWidgetType::Image?GuiDrawType::Image:GuiDrawType::Box);box.nodeId=id;box.rect=n.computed;box.style=style;box.image=n.image;box.layer=layer;out.push_back(box);if(!n.text.empty()){GuiDrawCommand text;text.type=GuiDrawType::Text;text.nodeId=id;text.rect=n.computed;text.style=style;text.text=n.text;text.layer=layer+1;out.push_back(text);}for(auto&c:n.children)BuildDrawNode(c,out,layer);if(clips){GuiDrawCommand c;c.type=GuiDrawType::ClipPop;c.nodeId=id;c.rect=n.computed;c.layer=layer;c.clip=true;out.push_back(c);}}
std::vector<GuiDrawCommand>GuiFramework::BuildDrawList()const{auto*self=const_cast<GuiFramework*>(this);if(layoutDirty_)self->Layout();std::vector<GuiDrawCommand>out;for(auto&id:Roots())BuildDrawNode(id,out,0);std::stable_sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.layer<b.layer;});return out;}
GuiFrameStats GuiFramework::Stats()const{GuiFrameStats s;s.revision=revision_;s.nodeCount=nodes_.size();for(auto&[id,n]:nodes_)if(n.visible)++s.visibleNodeCount;s.drawCommandCount=BuildDrawList().size();s.queuedEventCount=events_.size();s.activeAnimationCount=animations_.size();s.focusedId=focusedId_;s.hoveredId=hoveredId_;return s;}
VekValue GuiFramework::Snapshot()const{auto*self=const_cast<GuiFramework*>(this);if(layoutDirty_)self->Layout();VekArray arr;arr.reserve(nodes_.size());std::vector<std::string>ids;for(auto&[id,n]:nodes_)ids.push_back(id);std::sort(ids.begin(),ids.end());for(auto&id:ids){auto&n=nodes_.at(id);VekMap m;m["id"]=n.id;m["type"]=GuiWidgetTypeName(n.type);m["parent"]=n.parentId;m["text"]=n.text;m["rect"]=RectToValue(n.computed);m["visible"]=n.visible;m["enabled"]=n.enabled;m["focused"]=n.focused;m["hovered"]=n.hovered;m["checked"]=n.checked;m["value"]=n.value;m["selected_index"]=n.selectedIndex;m["data"]=n.data;arr.emplace_back(m);}VekMap root;root["api_version"]=ApiVersion;root["theme"]=activeTheme_;root["width"]=viewport_.width;root["height"]=viewport_.height;root["dpi_scale"]=viewport_.dpiScale;root["nodes"]=VekValue(arr);return VekValue(root);}

void GuiFramework::RegisterNatives(VekScriptEngine&e){
    e.RegisterNative("ui_version",[](const std::vector<VekValue>&){return VekValue(ApiVersion);});
    e.RegisterNative("ui_create",[this](const std::vector<VekValue>&a){std::string err;return VekValue(!a.empty()&&CreateValue(a[0],&err));});
    e.RegisterNative("ui_patch",[this](const std::vector<VekValue>&a){std::string err;return VekValue(a.size()>=2&&Patch(a[0].AsString(),a[1],&err));});
    e.RegisterNative("ui_remove",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Remove(a[0].AsString(),a.size()<2||a[1].AsBool(true)));});
    e.RegisterNative("ui_clear",[this](const std::vector<VekValue>&){Clear();return VekValue();});
    e.RegisterNative("ui_count",[this](const std::vector<VekValue>&){return VekValue((double)Size());});
    e.RegisterNative("ui_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);});
    e.RegisterNative("ui_set_viewport",[this](const std::vector<VekValue>&a){if(a.size()>=2){GuiViewport v=viewport_;v.width=(float)a[0].AsNumber(v.width);v.height=(float)a[1].AsNumber(v.height);if(a.size()>2)v.dpiScale=(float)a[2].AsNumber(v.dpiScale);SetViewport(v);}return VekValue();});
    e.RegisterNative("ui_layout",[this](const std::vector<VekValue>&){Layout();return VekValue();});
    e.RegisterNative("ui_rect",[this](const std::vector<VekValue>&a){if(a.empty())return VekValue();if(layoutDirty_)Layout();auto*n=Find(a[0].AsString());return n?RectToValue(n->computed):VekValue();});
    e.RegisterNative("ui_hit_test",[this](const std::vector<VekValue>&a){if(a.size()<2)return VekValue("");if(layoutDirty_)Layout();return VekValue(HitTest({(float)a[0].AsNumber(),(float)a[1].AsNumber()}));});
    e.RegisterNative("ui_focus",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Focus(a[0].AsString()));});
    e.RegisterNative("ui_focus_next",[this](const std::vector<VekValue>&a){return VekValue(FocusNext(!a.empty()&&a[0].AsBool()));});
    e.RegisterNative("ui_focused",[this](const std::vector<VekValue>&){return VekValue(focusedId_);});
    e.RegisterNative("ui_feed_pointer",[this](const std::vector<VekValue>&a){if(a.size()>=3){GuiInputEvent i;auto t=Lower(a[0].AsString());i.type=t=="down"?GuiInputType::PointerDown:t=="up"?GuiInputType::PointerUp:GuiInputType::PointerMove;i.pointer={(float)a[1].AsNumber(),(float)a[2].AsNumber()};if(a.size()>3)i.button=(int)a[3].AsNumber();FeedInput(i);}return VekValue();});
    e.RegisterNative("ui_feed_scroll",[this](const std::vector<VekValue>&a){if(a.size()>=4){GuiInputEvent i;i.type=GuiInputType::Scroll;i.pointer={(float)a[0].AsNumber(),(float)a[1].AsNumber()};i.scrollX=(float)a[2].AsNumber();i.scrollY=(float)a[3].AsNumber();FeedInput(i);}return VekValue();});
    e.RegisterNative("ui_feed_key",[this](const std::vector<VekValue>&a){if(a.size()>=2){GuiInputEvent i;i.type=Lower(a[0].AsString())=="up"?GuiInputType::KeyUp:GuiInputType::KeyDown;i.key=(int)a[1].AsNumber();if(a.size()>2)i.shift=a[2].AsBool();FeedInput(i);}return VekValue();});
    e.RegisterNative("ui_feed_text",[this](const std::vector<VekValue>&a){if(!a.empty()){GuiInputEvent i;i.type=GuiInputType::TextInput;i.text=a[0].AsString();FeedInput(i);}return VekValue();});
    e.RegisterNative("ui_next_event",[this](const std::vector<VekValue>&){GuiEvent ev;return PollEvent(ev)?EventToValue(ev):VekValue();});
    e.RegisterNative("ui_define_theme",[this](const std::vector<VekValue>&a){std::string err;return VekValue(!a.empty()&&DefineThemeValue(a[0],&err));});
    e.RegisterNative("ui_set_theme",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&SetTheme(a[0].AsString()));});
    e.RegisterNative("ui_theme",[this](const std::vector<VekValue>&){return VekValue(activeTheme_);});
    e.RegisterNative("ui_animate",[this](const std::vector<VekValue>&a){if(a.size()<5)return VekValue(false);GuiAnimation x;x.nodeId=a[0].AsString();x.property=a[1].AsString();x.from=a[2].AsNumber();x.to=a[3].AsNumber();x.duration=a[4].AsNumber(0.2);if(a.size()>5)x.ease=ParseEase(a[5].AsString());std::string err;return VekValue(StartAnimation(x,&err));});
    e.RegisterNative("ui_step",[this](const std::vector<VekValue>&a){if(!a.empty())Step(a[0].AsNumber());return VekValue();});
    e.RegisterNative("ui_snapshot",[this](const std::vector<VekValue>&){return Snapshot();});
    e.RegisterNative("ui_stats",[this](const std::vector<VekValue>&){auto s=Stats();VekMap m;m["revision"]=(double)s.revision;m["nodes"]=(double)s.nodeCount;m["visible"]=(double)s.visibleNodeCount;m["draw_commands"]=(double)s.drawCommandCount;m["events"]=(double)s.queuedEventCount;m["animations"]=(double)s.activeAnimationCount;m["focused"]=s.focusedId;m["hovered"]=s.hoveredId;return VekValue(m);});
    e.RegisterNative("ui_draw_list",[this](const std::vector<VekValue>&){VekArray arr;for(auto&d:BuildDrawList()){VekMap m;m["type"]=(double)(int)d.type;m["node"]=d.nodeId;m["rect"]=RectToValue(d.rect);m["text"]=d.text;m["image"]=d.image;m["layer"]=d.layer;m["background"]=ColorToValue(d.style.background);m["foreground"]=ColorToValue(d.style.foreground);arr.emplace_back(m);}return VekValue(arr);});
}

} // namespace vek
