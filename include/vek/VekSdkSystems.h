#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vek/VekScriptEngine.h>

class VekScriptEngine;

namespace vek {
using ScriptEntityId = std::uint64_t;

class ScriptEntityRegistry {
public:
    ScriptEntityId Create(const std::string& archetype = {});
    bool Destroy(ScriptEntityId id);
    bool Exists(ScriptEntityId id) const;
    bool AddComponent(ScriptEntityId id,const std::string& component,VekValue data=VekValue::Map());
    bool RemoveComponent(ScriptEntityId id,const std::string& component);
    bool HasComponent(ScriptEntityId id,const std::string& component) const;
    VekValue GetComponent(ScriptEntityId id,const std::string& component) const;
    std::size_t Count() const;
    void RegisterNatives(VekScriptEngine& engine);
private:
    struct Entity { std::string archetype; std::unordered_map<std::string,VekValue> components; };
    ScriptEntityId nextId=1;
    std::unordered_map<ScriptEntityId,Entity> entities;
};
}
