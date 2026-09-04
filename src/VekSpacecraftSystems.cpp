#include <vek/VekSpacecraftSystems.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_set>

namespace vek {
namespace {

constexpr double kG0 = 9.80665; // standard gravity, m/s^2 — Tsiolkovsky convention
constexpr double kPi = 3.14159265358979323846;

bool SafeToken(const std::string& s, std::size_t maxLen, bool allowEmpty = false) {
    if (s.empty()) return allowEmpty;
    if (s.size() > maxLen) return false;
    for (unsigned char c : s) {
        if (std::isalnum(c)) continue;
        switch (c) { case '_': case '-': case '.': case ':': case '/': case '@': break; default: return false; }
    }
    return true;
}

ResourceType ParseResourceType(const std::string& s) {
    if (s == "liquid_fuel") return ResourceType::LiquidFuel;
    if (s == "oxidizer") return ResourceType::Oxidizer;
    if (s == "mono_propellant") return ResourceType::MonoPropellant;
    if (s == "solid_fuel") return ResourceType::SolidFuel;
    if (s == "electric_charge") return ResourceType::ElectricCharge;
    if (s == "xenon") return ResourceType::Xenon;
    if (s == "ore") return ResourceType::Ore;
    return ResourceType::Custom;
}
std::string ResourceTypeName(ResourceType t) {
    switch (t) {
        case ResourceType::LiquidFuel: return "liquid_fuel";
        case ResourceType::Oxidizer: return "oxidizer";
        case ResourceType::MonoPropellant: return "mono_propellant";
        case ResourceType::SolidFuel: return "solid_fuel";
        case ResourceType::ElectricCharge: return "electric_charge";
        case ResourceType::Xenon: return "xenon";
        case ResourceType::Ore: return "ore";
        default: return "custom";
    }
}

VekValue ContainerToValue(const ResourceContainerDefinition& d) {
    VekValue v = VekValue::Map();
    v.Set("id", d.id); v.Set("part_id", d.partId); v.Set("type", ResourceTypeName(d.type));
    v.Set("custom_type_name", d.customTypeName); v.Set("capacity", (double)d.capacity);
    v.Set("amount", (double)d.amount); v.Set("flow_enabled", d.flowEnabled);
    return v;
}
VekValue EngineToValue(const EngineDefinition& d) {
    VekValue v = VekValue::Map();
    v.Set("id", d.id); v.Set("part_id", d.partId);
    v.Set("thrust_vacuum_kn", (double)d.thrustVacuumKn); v.Set("thrust_atmosphere_kn", (double)d.thrustAtmosphereKn);
    v.Set("isp_vacuum_seconds", (double)d.ispVacuumSeconds); v.Set("isp_atmosphere_seconds", (double)d.ispAtmosphereSeconds);
    v.Set("primary_resource", ResourceTypeName(d.primaryResource)); v.Set("secondary_resource", ResourceTypeName(d.secondaryResource));
    v.Set("mixture_ratio", (double)d.mixtureRatio); v.Set("gimbal_range_degrees", (double)d.gimbalRangeDegrees);
    v.Set("throttleable", d.throttleable); v.Set("min_throttle", (double)d.minThrottle);
    v.Set("ignited", d.ignited); v.Set("current_throttle", (double)d.currentThrottle);
    return v;
}
VekValue EngineOutputToValue(const EngineOutput& o) {
    VekValue v = VekValue::Map();
    v.Set("valid", o.valid); v.Set("thrust_kn", (double)o.thrustKn);
    v.Set("mass_flow_kg_per_second", (double)o.massFlowKgPerSecond);
    v.Set("primary_resource_units", (double)o.primaryResourceUnits);
    v.Set("secondary_resource_units", (double)o.secondaryResourceUnits);
    return v;
}
VekValue DecouplerToValue(const DecouplerDefinition& d) {
    VekValue v = VekValue::Map();
    v.Set("id", d.id); v.Set("part_id", d.partId); v.Set("ejection_force", (double)d.ejectionForce);
    v.Set("staged", d.staged); v.Set("fired", d.fired);
    return v;
}
VekValue Vec3ToValue(PhysicsVec3 v3) {
    VekValue v = VekValue::Map();
    v.Set("x", (double)v3.x); v.Set("y", (double)v3.y); v.Set("z", (double)v3.z);
    return v;
}
PhysicsVec3 Vec3FromValue(const VekValue& v) {
    PhysicsVec3 out;
    out.x = (float)v.Get("x").AsNumber(0.0); out.y = (float)v.Get("y").AsNumber(0.0); out.z = (float)v.Get("z").AsNumber(0.0);
    return out;
}
VekValue StageResultToValue(const StageActivationResult& r) {
    VekValue v = VekValue::Map();
    v.Set("valid", r.valid); v.Set("stage_index", (double)r.stageIndex);
    VekValue ig = VekValue::Array(); for (auto& s : r.ignitedEngineIds) ig.Push(s); v.Set("ignited_engine_ids", ig);
    VekValue de = VekValue::Array(); for (auto& s : r.firedDecouplerIds) de.Push(s); v.Set("fired_decoupler_ids", de);
    return v;
}
VekValue OrbitToValue(const OrbitDescription& o) {
    VekValue v = VekValue::Map();
    v.Set("valid", o.valid); v.Set("semi_major_axis_meters", o.semiMajorAxisMeters);
    v.Set("eccentricity", o.eccentricity); v.Set("apoapsis_meters", o.apoapsisMeters);
    v.Set("periapsis_meters", o.periapsisMeters); v.Set("period_seconds", o.periodSeconds);
    v.Set("hyperbolic", o.hyperbolic);
    return v;
}

} // namespace

// --- ResourceContainerRegistry ---------------------------------------------

bool ResourceContainerRegistry::Register(const ResourceContainerDefinition& d) {
    if (!SafeToken(d.id, 128) || containers.count(d.id)) return false;
    if (!SafeToken(d.partId, 128)) return false;
    if (!std::isfinite(d.capacity) || d.capacity < 0.0f) return false;
    if (!std::isfinite(d.amount) || d.amount < 0.0f || d.amount > d.capacity) return false;
    containers.emplace(d.id, d);
    partIndex[d.partId].push_back(d.id);
    return true;
}
bool ResourceContainerRegistry::RegisterValue(const VekValue& v, std::string* error) {
    if (!v.IsMap()) { if (error) *error = "resource_container_register expects a map"; return false; }
    ResourceContainerDefinition d;
    d.id = v.Get("id").AsString(); d.partId = v.Get("part_id").AsString();
    d.type = ParseResourceType(v.Get("type").AsString()); d.customTypeName = v.Get("custom_type_name").AsString();
    d.capacity = (float)v.Get("capacity").AsNumber(0.0);
    d.amount = (float)v.Get("amount").AsNumber(d.capacity);
    auto b = v.Get("flow_enabled"); if (!b.IsNil()) d.flowEnabled = b.AsBool();
    if (!Register(d)) { if (error) *error = "invalid or duplicate resource container definition"; return false; }
    if (error) error->clear();
    return true;
}
const ResourceContainerDefinition* ResourceContainerRegistry::Find(const std::string& id) const {
    auto it = containers.find(id); return it == containers.end() ? nullptr : &it->second;
}
std::vector<const ResourceContainerDefinition*> ResourceContainerRegistry::ContainersForPart(const std::string& partId) const {
    std::vector<const ResourceContainerDefinition*> out;
    auto it = partIndex.find(partId);
    if (it == partIndex.end()) return out;
    for (auto& id : it->second) { auto* c = Find(id); if (c) out.push_back(c); }
    return out;
}
std::vector<const ResourceContainerDefinition*> ResourceContainerRegistry::ContainersOfType(ResourceType type) const {
    std::vector<const ResourceContainerDefinition*> out;
    for (auto& [id, c] : containers) if (c.type == type) out.push_back(&c);
    return out;
}

std::vector<std::string> ResourceContainerRegistry::ReachablePartsWithFuel(const std::string& fromPartId, ResourceType,
                                                                            const PartConnectionGraph& fuelLines) const {
    // Breadth-first walk across "fuel_line" connection edges only, so a
    // structural weld/joint does not implicitly carry fuel (matches KSP's
    // fuel-line-required crossfeed model for radially attached tanks).
    std::vector<std::string> reachable{fromPartId};
    std::unordered_set<std::string> visited{fromPartId};
    std::size_t cursor = 0;
    while (cursor < reachable.size()) {
        const std::string current = reachable[cursor++];
        for (auto* c : fuelLines.ConnectionsForPart(current)) {
            if (c->kind != "fuel_line") continue;
            const std::string other = (c->partA == current) ? c->partB : c->partA;
            if (visited.insert(other).second) reachable.push_back(other);
        }
    }
    return reachable;
}

float ResourceContainerRegistry::Withdraw(const std::string& fromPartId, ResourceType type, float amount,
                                           const PartConnectionGraph& fuelLines) {
    if (amount <= 0.0f || !std::isfinite(amount)) return 0.0f;
    auto parts = ReachablePartsWithFuel(fromPartId, type, fuelLines);
    std::vector<ResourceContainerDefinition*> pool;
    float available = 0.0f;
    for (auto& partId : parts) {
        auto it = partIndex.find(partId);
        if (it == partIndex.end()) continue;
        for (auto& cid : it->second) {
            auto cit = containers.find(cid);
            if (cit == containers.end()) continue;
            ResourceContainerDefinition& c = cit->second;
            if (c.type != type || !c.flowEnabled || c.amount <= 0.0f) continue;
            pool.push_back(&c);
            available += c.amount;
        }
    }
    float toDraw = std::min(amount, available);
    if (toDraw <= 0.0f) return 0.0f;
    float remaining = toDraw;
    for (auto* c : pool) {
        if (remaining <= 0.0f) break;
        float share = available > 0.0f ? (c->amount / available) * toDraw : 0.0f;
        share = std::min(share, c->amount);
        share = std::min(share, remaining);
        c->amount -= share;
        remaining -= share;
    }
    // Rounding remainder: pull whatever is left from the first container that
    // still has any amount, so Withdraw never under-reports what it took.
    if (remaining > 1e-6f) {
        for (auto* c : pool) {
            if (remaining <= 0.0f) break;
            float take = std::min(c->amount, remaining);
            c->amount -= take;
            remaining -= take;
        }
    }
    return toDraw - std::max(remaining, 0.0f);
}

bool ResourceContainerRegistry::Transfer(const std::string& fromId, const std::string& toId, float amount) {
    if (amount <= 0.0f || !std::isfinite(amount)) return false;
    auto fromIt = containers.find(fromId);
    auto toIt = containers.find(toId);
    if (fromIt == containers.end() || toIt == containers.end()) return false;
    ResourceContainerDefinition& from = fromIt->second;
    ResourceContainerDefinition& to = toIt->second;
    if (from.type != to.type) return false;
    float moved = std::min({amount, from.amount, to.capacity - to.amount});
    if (moved <= 0.0f) return false;
    from.amount -= moved;
    to.amount += moved;
    return true;
}

float ResourceContainerRegistry::TotalOfType(const std::string& partId, ResourceType type, const PartConnectionGraph& fuelLines) const {
    auto parts = ReachablePartsWithFuel(partId, type, fuelLines);
    float total = 0.0f;
    for (auto& p : parts) for (auto* c : ContainersForPart(p)) if (c->type == type) total += c->amount;
    return total;
}

void ResourceContainerRegistry::Clear() { containers.clear(); partIndex.clear(); }

void ResourceContainerRegistry::RegisterNatives(VekScriptEngine& e) {
    e.RegisterNative("resource_container_register", [this](const std::vector<VekValue>& a) {
        std::string err; return VekValue(!a.empty() && RegisterValue(a[0], &err));
    });
    e.RegisterNative("resource_container_get", [this](const std::vector<VekValue>& a) -> VekValue {
        if (a.empty()) return VekValue();
        const auto* c = Find(a[0].AsString());
        return c ? ContainerToValue(*c) : VekValue();
    });
    e.RegisterNative("resource_containers_for_part", [this](const std::vector<VekValue>& a) {
        VekValue arr = VekValue::Array();
        if (!a.empty()) for (auto* c : ContainersForPart(a[0].AsString())) arr.Push(ContainerToValue(*c));
        return arr;
    });
    e.RegisterNative("resource_transfer", [this](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 3 && Transfer(a[0].AsString(), a[1].AsString(), (float)a[2].AsNumber(0.0)));
    });
    e.RegisterNative("resource_container_count", [this](const std::vector<VekValue>&) { return VekValue((double)Size()); });
}

// --- EngineRegistry ----------------------------------------------------------

bool EngineRegistry::Register(const EngineDefinition& d) {
    if (!SafeToken(d.id, 128) || engines.count(d.id)) return false;
    if (!SafeToken(d.partId, 128)) return false;
    if (!std::isfinite(d.thrustVacuumKn) || d.thrustVacuumKn < 0.0f) return false;
    if (!std::isfinite(d.thrustAtmosphereKn) || d.thrustAtmosphereKn < 0.0f) return false;
    if (!std::isfinite(d.ispVacuumSeconds) || d.ispVacuumSeconds <= 0.0f) return false;
    if (!std::isfinite(d.ispAtmosphereSeconds) || d.ispAtmosphereSeconds < 0.0f) return false;
    if (d.mixtureRatio < 0.0f || d.mixtureRatio > 1.0f) return false;
    if (d.minThrottle < 0.0f || d.minThrottle > 1.0f) return false;
    EngineDefinition e = d;
    e.currentThrottle = std::clamp(e.currentThrottle, e.minThrottle, 1.0f);
    engines.emplace(e.id, e);
    return true;
}
bool EngineRegistry::RegisterValue(const VekValue& v, std::string* error) {
    if (!v.IsMap()) { if (error) *error = "engine_register expects a map"; return false; }
    EngineDefinition d;
    d.id = v.Get("id").AsString(); d.partId = v.Get("part_id").AsString();
    d.thrustVacuumKn = (float)v.Get("thrust_vacuum_kn").AsNumber(0.0);
    d.thrustAtmosphereKn = (float)v.Get("thrust_atmosphere_kn").AsNumber(d.thrustVacuumKn);
    d.ispVacuumSeconds = (float)v.Get("isp_vacuum_seconds").AsNumber(0.0);
    d.ispAtmosphereSeconds = (float)v.Get("isp_atmosphere_seconds").AsNumber(d.ispVacuumSeconds);
    std::string prim = v.Get("primary_resource").AsString(); if (!prim.empty()) d.primaryResource = ParseResourceType(prim);
    std::string sec = v.Get("secondary_resource").AsString(); if (!sec.empty()) d.secondaryResource = ParseResourceType(sec);
    d.mixtureRatio = (float)v.Get("mixture_ratio").AsNumber(d.mixtureRatio);
    d.gimbalRangeDegrees = (float)v.Get("gimbal_range_degrees").AsNumber(0.0);
    auto b = v.Get("throttleable"); if (!b.IsNil()) d.throttleable = b.AsBool();
    d.minThrottle = (float)v.Get("min_throttle").AsNumber(d.throttleable ? 0.0 : 1.0);
    if (!Register(d)) { if (error) *error = "invalid or duplicate engine definition"; return false; }
    if (error) error->clear();
    return true;
}
const EngineDefinition* EngineRegistry::Find(const std::string& id) const {
    auto it = engines.find(id); return it == engines.end() ? nullptr : &it->second;
}
EngineDefinition* EngineRegistry::Mutable(const std::string& id) {
    auto it = engines.find(id); return it == engines.end() ? nullptr : &it->second;
}
bool EngineRegistry::SetThrottle(const std::string& id, float throttle01) {
    auto* e = Mutable(id);
    if (!e || !std::isfinite(throttle01)) return false;
    if (!e->throttleable) { e->currentThrottle = throttle01 > 0.0f ? 1.0f : 0.0f; return true; }
    e->currentThrottle = std::clamp(throttle01, e->minThrottle, 1.0f);
    return true;
}
bool EngineRegistry::SetIgnited(const std::string& id, bool ignited) {
    auto* e = Mutable(id);
    if (!e) return false;
    e->ignited = ignited;
    if (!ignited) e->currentThrottle = 0.0f;
    else if (e->currentThrottle < e->minThrottle) e->currentThrottle = e->minThrottle > 0.0f ? e->minThrottle : e->currentThrottle;
    return true;
}
float EngineRegistry::ThrustAt(const std::string& id, float atmospheres) const {
    const auto* e = Find(id);
    if (!e || !e->ignited) return 0.0f;
    float t = std::clamp(atmospheres, 0.0f, 1.0f);
    float rated = e->thrustVacuumKn + (e->thrustAtmosphereKn - e->thrustVacuumKn) * t;
    return rated * e->currentThrottle;
}
float EngineRegistry::IspAt(const std::string& id, float atmospheres) const {
    const auto* e = Find(id);
    if (!e) return 0.0f;
    float t = std::clamp(atmospheres, 0.0f, 1.0f);
    return e->ispVacuumSeconds + (e->ispAtmosphereSeconds - e->ispVacuumSeconds) * t;
}
EngineOutput EngineRegistry::Step(const std::string& id, float atmospheres, float dt) const {
    EngineOutput out;
    const auto* e = Find(id);
    if (!e || !e->ignited || dt <= 0.0f) return out;
    float thrustKn = ThrustAt(id, atmospheres);
    float isp = IspAt(id, atmospheres);
    if (isp <= 0.0f) return out;
    // massFlow = F / (isp * g0); F in Newtons.
    double massFlow = (thrustKn * 1000.0) / (isp * kG0);
    out.valid = true;
    out.thrustKn = thrustKn;
    out.massFlowKgPerSecond = (float)massFlow;
    float totalUnits = (float)(massFlow * dt); // treat resource "units" as kg for the reference implementation
    bool hasSecondary = e->secondaryResource != ResourceType::Custom;
    if (hasSecondary && e->mixtureRatio < 1.0f) {
        out.primaryResourceUnits = totalUnits * e->mixtureRatio;
        out.secondaryResourceUnits = totalUnits * (1.0f - e->mixtureRatio);
    } else {
        out.primaryResourceUnits = totalUnits;
        out.secondaryResourceUnits = 0.0f;
    }
    return out;
}
void EngineRegistry::Clear() { engines.clear(); }
void EngineRegistry::RegisterNatives(VekScriptEngine& e) {
    e.RegisterNative("engine_register", [this](const std::vector<VekValue>& a) {
        std::string err; return VekValue(!a.empty() && RegisterValue(a[0], &err));
    });
    e.RegisterNative("engine_get", [this](const std::vector<VekValue>& a) -> VekValue {
        if (a.empty()) return VekValue();
        const auto* eng = Find(a[0].AsString());
        return eng ? EngineToValue(*eng) : VekValue();
    });
    e.RegisterNative("engine_set_throttle", [this](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 2 && SetThrottle(a[0].AsString(), (float)a[1].AsNumber(0.0)));
    });
    e.RegisterNative("engine_set_ignited", [this](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 2 && SetIgnited(a[0].AsString(), a[1].AsBool(true)));
    });
    e.RegisterNative("engine_thrust_at", [this](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 2 ? (double)ThrustAt(a[0].AsString(), (float)a[1].AsNumber(0.0)) : 0.0);
    });
    e.RegisterNative("engine_isp_at", [this](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 2 ? (double)IspAt(a[0].AsString(), (float)a[1].AsNumber(0.0)) : 0.0);
    });
    e.RegisterNative("engine_step", [this](const std::vector<VekValue>& a) {
        if (a.size() < 3) return EngineOutputToValue(EngineOutput{});
        return EngineOutputToValue(Step(a[0].AsString(), (float)a[1].AsNumber(0.0), (float)a[2].AsNumber(0.0)));
    });
    e.RegisterNative("engine_count", [this](const std::vector<VekValue>&) { return VekValue((double)Size()); });
}

// --- DecouplerRegistry --------------------------------------------------------

bool DecouplerRegistry::Register(const DecouplerDefinition& d) {
    if (!SafeToken(d.id, 128) || decouplers.count(d.id)) return false;
    if (!SafeToken(d.partId, 128)) return false;
    if (!std::isfinite(d.ejectionForce) || d.ejectionForce < 0.0f) return false;
    decouplers.emplace(d.id, d);
    return true;
}
bool DecouplerRegistry::RegisterValue(const VekValue& v, std::string* error) {
    if (!v.IsMap()) { if (error) *error = "decoupler_register expects a map"; return false; }
    DecouplerDefinition d;
    d.id = v.Get("id").AsString(); d.partId = v.Get("part_id").AsString();
    d.ejectionForce = (float)v.Get("ejection_force").AsNumber(d.ejectionForce);
    auto b = v.Get("staged"); if (!b.IsNil()) d.staged = b.AsBool();
    if (!Register(d)) { if (error) *error = "invalid or duplicate decoupler definition"; return false; }
    if (error) error->clear();
    return true;
}
const DecouplerDefinition* DecouplerRegistry::Find(const std::string& id) const {
    auto it = decouplers.find(id); return it == decouplers.end() ? nullptr : &it->second;
}
bool DecouplerRegistry::Fire(const std::string& id) {
    auto it = decouplers.find(id);
    if (it == decouplers.end() || it->second.fired) return false;
    it->second.fired = true;
    return true;
}
void DecouplerRegistry::Clear() { decouplers.clear(); }
void DecouplerRegistry::RegisterNatives(VekScriptEngine& e) {
    e.RegisterNative("decoupler_register", [this](const std::vector<VekValue>& a) {
        std::string err; return VekValue(!a.empty() && RegisterValue(a[0], &err));
    });
    e.RegisterNative("decoupler_get", [this](const std::vector<VekValue>& a) -> VekValue {
        if (a.empty()) return VekValue();
        const auto* d = Find(a[0].AsString());
        return d ? DecouplerToValue(*d) : VekValue();
    });
    e.RegisterNative("decoupler_fire", [this](const std::vector<VekValue>& a) {
        return VekValue(!a.empty() && Fire(a[0].AsString()));
    });
    e.RegisterNative("decoupler_count", [this](const std::vector<VekValue>&) { return VekValue((double)Size()); });
}

// --- StagingSequencer ----------------------------------------------------------

bool StagingSequencer::AddStage(const StageDefinition& stage) {
    if (stages.count(stage.index) || stage.index < 0) return false;
    stages.emplace(stage.index, stage);
    if (!started || stage.index > currentIndex) currentIndex = stage.index;
    started = true;
    return true;
}
bool StagingSequencer::AddStageValue(const VekValue& v, std::string* error) {
    if (!v.IsMap()) { if (error) *error = "stage_add expects a map"; return false; }
    StageDefinition d;
    d.index = (int)v.Get("index").AsNumber(0);
    const auto* engineArr = v.Get("engine_ids").AsArray();
    if (engineArr) for (auto& e : *engineArr) d.engineIds.push_back(e.AsString());
    const auto* decoArr = v.Get("decoupler_ids").AsArray();
    if (decoArr) for (auto& e : *decoArr) d.decouplerIds.push_back(e.AsString());
    if (!AddStage(d)) { if (error) *error = "invalid or duplicate stage index"; return false; }
    if (error) error->clear();
    return true;
}
const StageDefinition* StagingSequencer::Find(int index) const {
    auto it = stages.find(index); return it == stages.end() ? nullptr : &it->second;
}
StageActivationResult StagingSequencer::Activate(EngineRegistry& engines, DecouplerRegistry& decouplers) {
    StageActivationResult result;
    if (currentIndex < 0) return result;
    const auto* stage = Find(currentIndex);
    if (!stage) { --currentIndex; return result; }
    result.valid = true;
    result.stageIndex = currentIndex;
    for (auto& engineId : stage->engineIds)
        if (engines.SetIgnited(engineId, true)) result.ignitedEngineIds.push_back(engineId);
    for (auto& decouplerId : stage->decouplerIds) {
        const auto* d = decouplers.Find(decouplerId);
        if (d && d->staged && decouplers.Fire(decouplerId)) result.firedDecouplerIds.push_back(decouplerId);
    }
    --currentIndex;
    return result;
}
void StagingSequencer::Reset() {
    currentIndex = -1;
    for (auto& [idx, stage] : stages) currentIndex = std::max(currentIndex, idx);
    started = !stages.empty();
}
void StagingSequencer::Clear() { stages.clear(); currentIndex = -1; started = false; }
void StagingSequencer::RegisterNatives(VekScriptEngine& e, EngineRegistry* engineRegistry, DecouplerRegistry* decouplerRegistry) {
    e.RegisterNative("stage_add", [this](const std::vector<VekValue>& a) {
        std::string err; return VekValue(!a.empty() && AddStageValue(a[0], &err));
    });
    e.RegisterNative("stage_current", [this](const std::vector<VekValue>&) { return VekValue((double)CurrentStage()); });
    e.RegisterNative("stage_count", [this](const std::vector<VekValue>&) { return VekValue((double)StageCount()); });
    e.RegisterNative("stage_reset", [this](const std::vector<VekValue>&) { Reset(); return VekValue(true); });
    e.RegisterNative("stage_activate", [this, engineRegistry, decouplerRegistry](const std::vector<VekValue>&) {
        if (!engineRegistry || !decouplerRegistry) return StageResultToValue(StageActivationResult{});
        return StageResultToValue(Activate(*engineRegistry, *decouplerRegistry));
    });
}

// --- SymmetryGroupRegistry -----------------------------------------------------

bool SymmetryGroupRegistry::CreateGroup(const SymmetryGroupDefinition& d) {
    if (!SafeToken(d.id, 128) || groups.count(d.id)) return false;
    if (!SafeToken(d.originPartId, 128)) return false;
    int count = d.mode == SymmetryMode::Mirror ? 2 : d.count;
    if (count < 2 || count > 64) return false;
    float axisLenSq = d.axis.x * d.axis.x + d.axis.y * d.axis.y + d.axis.z * d.axis.z;
    if (!std::isfinite(axisLenSq) || axisLenSq < 1e-8f) return false;
    SymmetryGroupDefinition g = d;
    g.count = count;
    groups.emplace(g.id, g);
    return true;
}
bool SymmetryGroupRegistry::CreateGroupValue(const VekValue& v, std::string* error) {
    if (!v.IsMap()) { if (error) *error = "symmetry_group_create expects a map"; return false; }
    SymmetryGroupDefinition d;
    d.id = v.Get("id").AsString(); d.originPartId = v.Get("origin_part_id").AsString();
    std::string mode = v.Get("mode").AsString();
    d.mode = (mode == "mirror") ? SymmetryMode::Mirror : SymmetryMode::Radial;
    d.count = (int)v.Get("count").AsNumber(2);
    auto axisVal = v.Get("axis");
    if (axisVal.IsMap()) d.axis = Vec3FromValue(axisVal);
    if (!CreateGroup(d)) { if (error) *error = "invalid or duplicate symmetry group"; return false; }
    if (error) error->clear();
    return true;
}
const SymmetryGroupDefinition* SymmetryGroupRegistry::Find(const std::string& id) const {
    auto it = groups.find(id); return it == groups.end() ? nullptr : &it->second;
}
bool SymmetryGroupRegistry::AddMember(const std::string& groupId, const std::string& partId) {
    auto it = groups.find(groupId);
    if (it == groups.end() || !SafeToken(partId, 128)) return false;
    it->second.memberPartIds.push_back(partId);
    return true;
}
std::vector<PhysicsVec3> SymmetryGroupRegistry::ComputePlacements(const std::string& groupId, PhysicsVec3 localOffset) const {
    std::vector<PhysicsVec3> out;
    const auto* g = Find(groupId);
    if (!g) return out;
    out.push_back(localOffset);
    if (g->mode == SymmetryMode::Mirror) {
        float len = std::sqrt(g->axis.x * g->axis.x + g->axis.y * g->axis.y + g->axis.z * g->axis.z);
        PhysicsVec3 n{g->axis.x / len, g->axis.y / len, g->axis.z / len};
        float d = localOffset.x * n.x + localOffset.y * n.y + localOffset.z * n.z;
        out.push_back({localOffset.x - 2.0f * d * n.x, localOffset.y - 2.0f * d * n.y, localOffset.z - 2.0f * d * n.z});
        return out;
    }
    // Radial: rotate localOffset around g->axis in equal angular steps.
    float len = std::sqrt(g->axis.x * g->axis.x + g->axis.y * g->axis.y + g->axis.z * g->axis.z);
    PhysicsVec3 n{g->axis.x / len, g->axis.y / len, g->axis.z / len};
    for (int i = 1; i < g->count; ++i) {
        float angle = (2.0f * (float)kPi * (float)i) / (float)g->count;
        float c = std::cos(angle), s = std::sin(angle);
        // Rodrigues' rotation formula: v_rot = v*c + (n x v)*s + n*(n.v)*(1-c)
        PhysicsVec3 v = localOffset;
        PhysicsVec3 cross{n.y * v.z - n.z * v.y, n.z * v.x - n.x * v.z, n.x * v.y - n.y * v.x};
        float dot = n.x * v.x + n.y * v.y + n.z * v.z;
        PhysicsVec3 rotated{
            v.x * c + cross.x * s + n.x * dot * (1.0f - c),
            v.y * c + cross.y * s + n.y * dot * (1.0f - c),
            v.z * c + cross.z * s + n.z * dot * (1.0f - c),
        };
        out.push_back(rotated);
    }
    return out;
}
void SymmetryGroupRegistry::Clear() { groups.clear(); }
void SymmetryGroupRegistry::RegisterNatives(VekScriptEngine& e) {
    e.RegisterNative("symmetry_group_create", [this](const std::vector<VekValue>& a) {
        std::string err; return VekValue(!a.empty() && CreateGroupValue(a[0], &err));
    });
    e.RegisterNative("symmetry_group_add_member", [this](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 2 && AddMember(a[0].AsString(), a[1].AsString()));
    });
    e.RegisterNative("symmetry_group_placements", [this](const std::vector<VekValue>& a) {
        VekValue arr = VekValue::Array();
        if (a.size() >= 2) for (auto& p : ComputePlacements(a[0].AsString(), Vec3FromValue(a[1]))) arr.Push(Vec3ToValue(p));
        return arr;
    });
    e.RegisterNative("symmetry_group_count", [this](const std::vector<VekValue>&) { return VekValue((double)Size()); });
}

// --- OrbitalMechanics ------------------------------------------------------

OrbitDescription OrbitalMechanics::DescribeOrbit(const OrbitalState& s, double mu) {
    OrbitDescription out;
    if (mu <= 0.0 || s.radiusMeters <= 0.0) return out;
    double v2 = s.speedMetersPerSecond * s.speedMetersPerSecond;
    double specificEnergy = v2 / 2.0 - mu / s.radiusMeters; // vis-viva energy
    // specific angular momentum h = r * v * cos(flight path angle from horizontal... )
    double h = s.radiusMeters * s.speedMetersPerSecond * std::cos(s.flightPathAngleRadians);
    double a = -mu / (2.0 * specificEnergy); // semi-major axis; negative energy => positive a (ellipse)
    if (specificEnergy >= 0.0) { out.valid = true; out.hyperbolic = true; out.semiMajorAxisMeters = a; return out; }
    double p = (h * h) / mu; // semi-latus rectum
    double eSq = 1.0 - p / a;
    double e = eSq > 0.0 ? std::sqrt(eSq) : 0.0;
    out.valid = true;
    out.semiMajorAxisMeters = a;
    out.eccentricity = e;
    out.apoapsisMeters = a * (1.0 + e);
    out.periapsisMeters = a * (1.0 - e);
    out.periodSeconds = OrbitalPeriod(a, mu);
    return out;
}
double OrbitalMechanics::CircularVelocity(double radiusMeters, double mu) {
    if (mu <= 0.0 || radiusMeters <= 0.0) return 0.0;
    return std::sqrt(mu / radiusMeters);
}
double OrbitalMechanics::OrbitalPeriod(double semiMajorAxisMeters, double mu) {
    if (mu <= 0.0 || semiMajorAxisMeters <= 0.0) return 0.0;
    return 2.0 * kPi * std::sqrt((semiMajorAxisMeters * semiMajorAxisMeters * semiMajorAxisMeters) / mu);
}
double OrbitalMechanics::VisViva(double radiusMeters, double semiMajorAxisMeters, double mu) {
    if (mu <= 0.0 || radiusMeters <= 0.0 || semiMajorAxisMeters == 0.0) return 0.0;
    double v2 = mu * (2.0 / radiusMeters - 1.0 / semiMajorAxisMeters);
    return v2 > 0.0 ? std::sqrt(v2) : 0.0;
}
double OrbitalMechanics::CircularizationDeltaV(double periapsisMeters, double apoapsisMeters, double mu) {
    if (mu <= 0.0 || periapsisMeters <= 0.0 || apoapsisMeters < periapsisMeters) return 0.0;
    double a = (periapsisMeters + apoapsisMeters) / 2.0;
    double vAtApoapsis = VisViva(apoapsisMeters, a, mu);
    double vCircular = CircularVelocity(apoapsisMeters, mu);
    return std::fabs(vCircular - vAtApoapsis);
}
double OrbitalMechanics::HohmannTransferDeltaV(double r1, double r2, double mu) {
    if (mu <= 0.0 || r1 <= 0.0 || r2 <= 0.0) return 0.0;
    double aTransfer = (r1 + r2) / 2.0;
    double v1 = CircularVelocity(r1, mu);
    double v2 = CircularVelocity(r2, mu);
    double vTransferAtR1 = VisViva(r1, aTransfer, mu);
    double vTransferAtR2 = VisViva(r2, aTransfer, mu);
    return std::fabs(vTransferAtR1 - v1) + std::fabs(v2 - vTransferAtR2);
}
double OrbitalMechanics::TsiolkovskyDeltaV(double ispSeconds, double wetMassKg, double dryMassKg) {
    if (ispSeconds <= 0.0 || wetMassKg <= 0.0 || dryMassKg <= 0.0 || wetMassKg < dryMassKg) return 0.0;
    return ispSeconds * kG0 * std::log(wetMassKg / dryMassKg);
}
double OrbitalMechanics::PropellantMassForDeltaV(double ispSeconds, double deltaV, double dryMassKg) {
    if (ispSeconds <= 0.0 || dryMassKg <= 0.0 || deltaV < 0.0) return 0.0;
    double wetMass = dryMassKg * std::exp(deltaV / (ispSeconds * kG0));
    return wetMass - dryMassKg;
}
double OrbitalMechanics::TotalStagedDeltaV(const std::vector<StageMassSpec>& stages) {
    double total = 0.0;
    for (auto& s : stages) total += TsiolkovskyDeltaV(s.ispSeconds, s.wetMassKg, s.dryMassKg);
    return total;
}

void VekRegisterSpacecraftLibrary(VekScriptEngine& e, ResourceContainerRegistry* resources, EngineRegistry* engines,
                                   DecouplerRegistry* decouplers, StagingSequencer* staging, SymmetryGroupRegistry* symmetry) {
    if (resources) resources->RegisterNatives(e);
    if (engines) engines->RegisterNatives(e);
    if (decouplers) decouplers->RegisterNatives(e);
    if (staging) staging->RegisterNatives(e, engines, decouplers);
    if (symmetry) symmetry->RegisterNatives(e);

    e.RegisterNative("orbit_describe", [](const std::vector<VekValue>& a) -> VekValue {
        if (a.size() < 2 || !a[0].IsMap()) return OrbitToValue(OrbitDescription{});
        OrbitalState s;
        s.radiusMeters = a[0].Get("radius_meters").AsNumber(0.0);
        s.speedMetersPerSecond = a[0].Get("speed_meters_per_second").AsNumber(0.0);
        s.flightPathAngleRadians = a[0].Get("flight_path_angle_radians").AsNumber(0.0);
        return OrbitToValue(OrbitalMechanics::DescribeOrbit(s, a[1].AsNumber(0.0)));
    });
    e.RegisterNative("orbit_circular_velocity", [](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 2 ? OrbitalMechanics::CircularVelocity(a[0].AsNumber(0.0), a[1].AsNumber(0.0)) : 0.0);
    });
    e.RegisterNative("orbit_period", [](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 2 ? OrbitalMechanics::OrbitalPeriod(a[0].AsNumber(0.0), a[1].AsNumber(0.0)) : 0.0);
    });
    e.RegisterNative("orbit_circularization_delta_v", [](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 3 ? OrbitalMechanics::CircularizationDeltaV(a[0].AsNumber(0.0), a[1].AsNumber(0.0), a[2].AsNumber(0.0)) : 0.0);
    });
    e.RegisterNative("orbit_hohmann_delta_v", [](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 3 ? OrbitalMechanics::HohmannTransferDeltaV(a[0].AsNumber(0.0), a[1].AsNumber(0.0), a[2].AsNumber(0.0)) : 0.0);
    });
    e.RegisterNative("rocket_delta_v", [](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 3 ? OrbitalMechanics::TsiolkovskyDeltaV(a[0].AsNumber(0.0), a[1].AsNumber(0.0), a[2].AsNumber(0.0)) : 0.0);
    });
    e.RegisterNative("rocket_propellant_mass_for_delta_v", [](const std::vector<VekValue>& a) {
        return VekValue(a.size() >= 3 ? OrbitalMechanics::PropellantMassForDeltaV(a[0].AsNumber(0.0), a[1].AsNumber(0.0), a[2].AsNumber(0.0)) : 0.0);
    });
    e.RegisterNative("rocket_total_staged_delta_v", [](const std::vector<VekValue>& a) {
        std::vector<OrbitalMechanics::StageMassSpec> specs;
        if (!a.empty() && a[0].IsArray()) {
            for (auto& item : *a[0].AsArray()) {
                OrbitalMechanics::StageMassSpec spec;
                spec.ispSeconds = item.Get("isp_seconds").AsNumber(0.0);
                spec.wetMassKg = item.Get("wet_mass_kg").AsNumber(0.0);
                spec.dryMassKg = item.Get("dry_mass_kg").AsNumber(0.0);
                specs.push_back(spec);
            }
        }
        return VekValue(OrbitalMechanics::TotalStagedDeltaV(specs));
    });
}

} // namespace vek
