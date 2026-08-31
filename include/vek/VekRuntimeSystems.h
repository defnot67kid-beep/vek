#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
#include <vek/VekScriptEngine.h>
#include <vek/VekGuiFramework.h>

namespace vek {

struct FrameSample {
    double cpuMs = 0.0;
    double gpuMs = 0.0;
    double frameMs = 0.0;
    double fps = 0.0;
};

struct FrameStatistics {
    double fps = 0.0;
    double averageFps = 0.0;
    double onePercentLowFps = 0.0;
    double averageFrameMs = 0.0;
    double averageCpuMs = 0.0;
    double averageGpuMs = 0.0;
    double worstFrameMs = 0.0;
    std::uint64_t frameCount = 0;
};

class FramePerformanceTracker {
public:
    explicit FramePerformanceTracker(std::size_t window=240);
    void SetWindow(std::size_t window);
    void Reset();
    void PushFrame(double deltaSeconds,double cpuMs=0.0,double gpuMs=0.0);
    FrameStatistics Statistics() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::size_t window_=240;
    std::deque<FrameSample> samples_;
    std::uint64_t frameCount_=0;
};

enum class GpuBackend : std::uint8_t { Unknown=0, Direct3D11, Direct3D12, Vulkan, Metal, OpenGL, WebGPU, Software };
struct GpuCapabilities {
    std::string adapterName;
    std::string vendor;
    GpuBackend backend=GpuBackend::Unknown;
    std::uint64_t dedicatedMemoryBytes=0;
    std::uint64_t sharedMemoryBytes=0;
    std::uint32_t maxTextureSize=0;
    std::uint32_t maxComputeWorkgroupSize=0;
    bool computeShaders=false;
    bool rayTracing=false;
    bool meshShaders=false;
    bool variableRateShading=false;
    bool bindlessResources=false;
};
struct GpuBudgetState {
    std::uint64_t budgetBytes=0;
    std::uint64_t usedBytes=0;
    std::uint64_t reservedBytes=0;
    double Utilization() const;
};
class GpuRuntimeInfo {
public:
    void SetCapabilities(GpuCapabilities caps);
    const GpuCapabilities& Capabilities() const;
    void SetBudget(GpuBudgetState budget);
    const GpuBudgetState& Budget() const;
    VekValue ToValue() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    GpuCapabilities caps_{};
    GpuBudgetState budget_{};
};

enum class GuiNodeType : std::uint8_t { Window=0, Panel, Label, Button, Toggle, Slider, Progress, TextInput, Image, List, Scroll, Canvas, Spacer };
struct GuiNodeDefinition {
    std::string id;
    GuiNodeType type=GuiNodeType::Panel;
    std::string parentId;
    std::string text;
    float x=0,y=0,width=0,height=0;
    float minValue=0,maxValue=1,value=0;
    bool visible=true,enabled=true,focusable=false;
    std::vector<std::string> classes;
    VekValue data=VekValue::Map();
};
class GuiDefinitionRegistry {
public:
    bool Register(const GuiNodeDefinition& node,std::string* error=nullptr);
    bool RegisterValue(const VekValue& value,std::string* error=nullptr);
    const GuiNodeDefinition* Find(const std::string& id) const;
    bool Remove(const std::string& id);
    void Clear();
    std::size_t Size() const;
    std::vector<std::string> ChildrenOf(const std::string& parentId) const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,GuiNodeDefinition> nodes_;
};

enum class GameplayDefinitionType : std::uint8_t { Ability=0, InputAction, CameraMode, InventoryItem, Quest, SpawnRule, State, Interaction, DamageType, Team, Objective };
struct GameplayDefinition {
    std::string id;
    GameplayDefinitionType type=GameplayDefinitionType::State;
    std::string displayName;
    bool enabled=true;
    float cooldown=0.0f;
    float duration=0.0f;
    int priority=0;
    VekValue properties=VekValue::Map();
};
class GameplayDefinitionRegistry {
public:
    bool Register(const GameplayDefinition& definition,std::string* error=nullptr);
    bool RegisterValue(const VekValue& value,std::string* error=nullptr);
    const GameplayDefinition* Find(const std::string& id) const;
    bool Remove(const std::string& id);
    void Clear();
    std::size_t Size() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,GameplayDefinition> definitions_;
};

struct ProfilerZoneSample { std::string name; double milliseconds=0.0; std::uint64_t calls=0; };
class RuntimeProfiler {
public:
    void Record(const std::string& zone,double milliseconds);
    void Reset();
    std::vector<ProfilerZoneSample> Snapshot() const;
    VekValue ToValue() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,ProfilerZoneSample> zones_;
};

// Convenience pack for standalone engines. Hosts still opt into each instance
// explicitly, so VEK does not acquire hidden renderer/input/GPU dependencies.
struct RuntimePlatformPack {
    FramePerformanceTracker performance{};
    GpuRuntimeInfo gpu{};
    GuiDefinitionRegistry gui{}; // legacy VEK 2.7 definition registry
    GuiFramework ui{};            // VEK 2.8 retained-mode GUI framework
    GameplayDefinitionRegistry gameplay{};
    RuntimeProfiler profiler{};
    void RegisterNatives(VekScriptEngine& engine);
};

} // namespace vek
