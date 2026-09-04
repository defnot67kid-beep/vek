#include <vek/VekUiDocking.h>

#include <algorithm>
#include <cctype>
#include <cmath>

namespace vek::ui {
namespace {
std::string Lower(std::string s){for(char&c:s)c=(char)std::tolower((unsigned char)c);return s;}
bool ValidId(const std::string&s){return !s.empty()&&s.size()<=160;}
float Ratio(float v){return std::clamp(std::isfinite(v)?v:.25f,.10f,.90f);}
DockRect RectFromValue(const VekValue&v,DockRect f){if(!v.IsMap())return f;f.x=(float)v.Get("x").AsNumber(f.x);f.y=(float)v.Get("y").AsNumber(f.y);f.width=std::clamp((float)v.Get("width").AsNumber(f.width),80.0f,16384.0f);f.height=std::clamp((float)v.Get("height").AsNumber(f.height),60.0f,16384.0f);return f;}
VekValue RectValue(DockRect r){VekMap m;m["x"]=r.x;m["y"]=r.y;m["width"]=r.width;m["height"]=r.height;return VekValue(std::move(m));}
}

DockRegion ParseDockRegion(const std::string&t){auto s=Lower(t);if(s=="left")return DockRegion::Left;if(s=="right")return DockRegion::Right;if(s=="top")return DockRegion::Top;if(s=="bottom")return DockRegion::Bottom;if(s=="floating"||s=="float")return DockRegion::Floating;return DockRegion::Center;}
const char*DockRegionName(DockRegion r){switch(r){case DockRegion::Left:return"left";case DockRegion::Right:return"right";case DockRegion::Top:return"top";case DockRegion::Bottom:return"bottom";case DockRegion::Floating:return"floating";default:return"center";}}

bool DockManager::RegisterPanel(const DockPanelState&p,std::string*error){if(!ValidId(p.id)){if(error)*error="dock panel id invalid";return false;}if(!panels_.count(p.id)&&panels_.size()>=MaxPanels){if(error)*error="dock panel limit reached";return false;}DockPanelState x=p;x.splitRatio=Ratio(x.splitRatio);if(x.group.empty())x.group="main";panels_[x.id]=x;if(x.active)SetActive(x.id);return true;}
bool DockManager::RemovePanel(const std::string&id){return panels_.erase(id)!=0;}
bool DockManager::Dock(const std::string&id,DockRegion r,const std::string&group,float ratio){auto it=panels_.find(id);if(it==panels_.end())return false;it->second.region=r;it->second.group=group.empty()?"main":group;it->second.splitRatio=Ratio(ratio);it->second.open=true;it->second.pinned=true;return true;}
bool DockManager::Float(const std::string&id,DockRect rect){auto it=panels_.find(id);if(it==panels_.end())return false;rect.width=std::clamp(rect.width,80.0f,16384.0f);rect.height=std::clamp(rect.height,60.0f,16384.0f);it->second.region=DockRegion::Floating;it->second.floatingRect=rect;it->second.open=true;it->second.pinned=false;return true;}
bool DockManager::Close(const std::string&id){auto it=panels_.find(id);if(it==panels_.end()||!it->second.closable)return false;it->second.open=false;it->second.active=false;return true;}
bool DockManager::Open(const std::string&id){auto it=panels_.find(id);if(it==panels_.end())return false;it->second.open=true;return true;}
bool DockManager::SetActive(const std::string&id){auto it=panels_.find(id);if(it==panels_.end()||!it->second.open)return false;for(auto&kv:panels_)if(kv.second.group==it->second.group)kv.second.active=false;it->second.active=true;return true;}
bool DockManager::SetPinned(const std::string&id,bool p){auto it=panels_.find(id);if(it==panels_.end())return false;it->second.pinned=p;return true;}
bool DockManager::ResizeSplit(const std::string&id,float ratio){auto it=panels_.find(id);if(it==panels_.end())return false;it->second.splitRatio=Ratio(ratio);return true;}
const DockPanelState*DockManager::Find(const std::string&id)const{auto it=panels_.find(id);return it==panels_.end()?nullptr:&it->second;}
std::vector<DockPanelState>DockManager::Panels(bool closed)const{std::vector<DockPanelState>v;for(auto&kv:panels_)if(closed||kv.second.open)v.push_back(kv.second);std::sort(v.begin(),v.end(),[](const auto&a,const auto&b){if(a.group!=b.group)return a.group<b.group;if(a.tabOrder!=b.tabOrder)return a.tabOrder<b.tabOrder;return a.id<b.id;});return v;}
void DockManager::Clear(){panels_.clear();}
VekValue DockManager::Snapshot()const{VekArray a;for(auto&p:Panels(true)){VekMap m;m["id"]=p.id;m["title"]=p.title;m["group"]=p.group;m["region"]=DockRegionName(p.region);m["split_ratio"]=p.splitRatio;m["tab_order"]=p.tabOrder;m["open"]=p.open;m["active"]=p.active;m["closable"]=p.closable;m["pinned"]=p.pinned;m["floating_rect"]=RectValue(p.floatingRect);a.emplace_back(std::move(m));}VekMap root;root["version"]=1;root["panels"]=VekValue(std::move(a));return VekValue(std::move(root));}
bool DockManager::Restore(const VekValue&v,std::string*error){if(!v.IsMap()||!v.Get("panels").IsArray()){if(error)*error="dock snapshot must contain panels array";return false;}std::unordered_map<std::string,DockPanelState> next;for(const auto&x:*v.Get("panels").AsArray()){if(!x.IsMap())continue;DockPanelState p;p.id=x.Get("id").AsString();p.title=x.Get("title").AsString();p.group=x.Get("group").AsString();p.region=ParseDockRegion(x.Get("region").AsString());p.splitRatio=Ratio((float)x.Get("split_ratio").AsNumber(.25));p.tabOrder=(int)x.Get("tab_order").AsNumber();p.open=x.Get("open").AsBool(true);p.active=x.Get("active").AsBool();p.closable=x.Get("closable").AsBool(true);p.pinned=x.Get("pinned").AsBool(true);p.floatingRect=RectFromValue(x.Get("floating_rect"),p.floatingRect);if(!ValidId(p.id)||next.size()>=MaxPanels){if(error)*error="invalid dock snapshot";return false;}next[p.id]=p;}panels_=std::move(next);return true;}


std::vector<DockLayoutEntry> DockManager::ComputeLayout(DockRect workspace)const{
    workspace.width=std::max(0.0f,workspace.width);workspace.height=std::max(0.0f,workspace.height);
    auto open=Panels(false);std::vector<DockLayoutEntry> out;out.reserve(open.size());
    DockRect center=workspace;
    auto consume=[&](DockRegion region){
        float ratio=0.0f;for(const auto&p:open)if(p.region==region)ratio=std::max(ratio,Ratio(p.splitRatio));if(ratio<=0)return DockRect{};
        DockRect r=center;
        if(region==DockRegion::Left||region==DockRegion::Right){float w=std::clamp(workspace.width*ratio,120.0f,std::max(120.0f,center.width-120.0f));r.width=std::min(w,center.width);if(region==DockRegion::Right)r.x=center.x+center.width-r.width;if(region==DockRegion::Left){center.x+=r.width;center.width-=r.width;}else center.width-=r.width;}
        else {float h=std::clamp(workspace.height*ratio,90.0f,std::max(90.0f,center.height-90.0f));r.height=std::min(h,center.height);if(region==DockRegion::Bottom)r.y=center.y+center.height-r.height;if(region==DockRegion::Top){center.y+=r.height;center.height-=r.height;}else center.height-=r.height;}
        return r;
    };
    DockRect left=consume(DockRegion::Left),right=consume(DockRegion::Right),top=consume(DockRegion::Top),bottom=consume(DockRegion::Bottom);
    auto regionRect=[&](DockRegion r)->DockRect{switch(r){case DockRegion::Left:return left;case DockRegion::Right:return right;case DockRegion::Top:return top;case DockRegion::Bottom:return bottom;default:return center;}};
    for(const auto&p:open){DockLayoutEntry e;e.id=p.id;e.group=p.group;e.region=p.region;e.active=p.active;e.floating=p.region==DockRegion::Floating;e.rect=e.floating?p.floatingRect:regionRect(p.region);out.push_back(e);}return out;
}
std::vector<DockDropZone> DockManager::ComputeDropZones(DockRect w,float inset)const{
    inset=std::max(0.0f,inset);w.x+=inset;w.y+=inset;w.width=std::max(0.0f,w.width-inset*2);w.height=std::max(0.0f,w.height-inset*2);
    float edge=std::clamp(std::min(w.width,w.height)*.20f,48.0f,180.0f);std::vector<DockDropZone> z;z.reserve(5);
    z.push_back({DockRegion::Center,{w.x+edge,w.y+edge,std::max(0.0f,w.width-edge*2),std::max(0.0f,w.height-edge*2)}});
    z.push_back({DockRegion::Left,{w.x,w.y,edge,w.height}});z.push_back({DockRegion::Right,{w.x+w.width-edge,w.y,edge,w.height}});z.push_back({DockRegion::Top,{w.x,w.y,w.width,edge}});z.push_back({DockRegion::Bottom,{w.x,w.y+w.height-edge,w.width,edge}});return z;
}

} // namespace vek::ui
