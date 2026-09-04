#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <vek/VekScriptEngine.h>

namespace vek::ui {

enum class DockRegion : unsigned char { Center=0, Left, Right, Top, Bottom, Floating };

struct DockRect { float x=0.0f,y=0.0f,width=0.0f,height=0.0f; };

struct DockLayoutEntry {
    std::string id;
    std::string group;
    DockRegion region = DockRegion::Center;
    DockRect rect{};
    bool active = false;
    bool floating = false;
};

struct DockDropZone {
    DockRegion region = DockRegion::Center;
    DockRect rect{};
};

struct DockPanelState {
    std::string id;
    std::string title;
    std::string group = "main";
    DockRegion region = DockRegion::Center;
    float splitRatio = 0.25f;
    int tabOrder = 0;
    bool open = true;
    bool active = false;
    bool closable = true;
    bool pinned = true;
    DockRect floatingRect{80,80,420,320};
};

class DockManager {
public:
    static constexpr std::size_t MaxPanels = 256;
    bool RegisterPanel(const DockPanelState& panel, std::string* error=nullptr);
    bool RemovePanel(const std::string& id);
    bool Dock(const std::string& id, DockRegion region, const std::string& group="main", float splitRatio=.25f);
    bool Float(const std::string& id, DockRect rect);
    bool Close(const std::string& id);
    bool Open(const std::string& id);
    bool SetActive(const std::string& id);
    bool SetPinned(const std::string& id, bool pinned);
    bool ResizeSplit(const std::string& id, float ratio);
    const DockPanelState* Find(const std::string& id) const;
    std::vector<DockPanelState> Panels(bool includeClosed=true) const;
    void Clear();
    VekValue Snapshot() const;
    bool Restore(const VekValue& value, std::string* error=nullptr);
    std::vector<DockLayoutEntry> ComputeLayout(DockRect workspace) const;
    std::vector<DockDropZone> ComputeDropZones(DockRect workspace, float inset=12.0f) const;
private:
    std::unordered_map<std::string,DockPanelState> panels_;
};

DockRegion ParseDockRegion(const std::string& text);
const char* DockRegionName(DockRegion region);

} // namespace vek::ui
