#include "VekJobRules.h"
#include "VekSecuritySystem.h"
#include <algorithm>
#include "VekHostSecurity.h"

namespace {
std::string StringOr(const VekValue& value, const std::string& fallback) {
    return value.IsString() ? value.AsString() : fallback;
}
}

bool VekJobRules::Initialize(const std::string& p) {
    file = p;
    return Load();
}

bool VekJobRules::Reload() {
    return Load();
}

bool VekJobRules::Load() {
    auto verified = VekSecuritySystem::VerifyScript(file, file + ".sig");
    if (!verified.ok) {
        error = verified.error;
        return false;
    }

    VekScriptEngine vm;
    ApplyGameVekSecurityPolicy(vm);
    VekRegisterStandardLibrary(vm);
    vm.SealNativeRegistry();

    if (!vm.LoadSource(verified.source, file)) {
        error = vm.LastError();
        return false;
    }

    auto v = vm.Call("job_definition");
    if (!v.IsMap()) {
        error = "VEK job script must return a map";
        return false;
    }

    JobDefinition d;
    d.id = StringOr(v.Get("id"), d.id);
    d.name = StringOr(v.Get("name"), d.name);
    d.type = StringOr(v.Get("type"), d.type);
    d.objective = StringOr(v.Get("objective"), d.objective);
    d.reward = std::clamp((int)v.Get("reward").AsNumber(d.reward), 0, 10000000);
    d.xp = std::clamp((int)v.Get("xp").AsNumber(d.xp), 0, 1000000);
    d.difficulty = std::clamp((int)v.Get("difficulty").AsNumber(d.difficulty), 1, 5);
    d.cargoMass = std::clamp((float)v.Get("cargo_mass").AsNumber(d.cargoMass), 0.0f, 1000000.0f);

    if (auto* recommended = v.Get("recommended").AsArray()) {
        for (const auto& item : *recommended) {
            if (item.IsString()) d.recommended.push_back(item.AsString());
        }
    }

    definition = std::move(d);
    engine = std::move(vm);
    error.clear();
    return true;
}

float VekJobRules::RewardMultiplier(const std::string& mode, int reputation) {
    if (!engine.HasFunction("reward_multiplier")) return 1.0f;
    return std::clamp(
        (float)engine.Call("reward_multiplier", {mode, reputation}).AsNumber(1.0),
        0.0f,
        10.0f
    );
}
