#include <vek/VekSdkSystems.h>
#include <vek/VekScriptEngine.h>
namespace vek {
ScriptEntityId ScriptEntityRegistry::Create(const std::string&a){auto id=nextId++;entities[id].archetype=a;return id;}
bool ScriptEntityRegistry::Destroy(ScriptEntityId id){return entities.erase(id)!=0;}
bool ScriptEntityRegistry::Exists(ScriptEntityId id)const{return entities.count(id)!=0;}
bool ScriptEntityRegistry::AddComponent(ScriptEntityId id,const std::string&c,VekValue d){auto it=entities.find(id);if(it==entities.end()||c.empty())return false;it->second.components[c]=std::move(d);return true;}
bool ScriptEntityRegistry::RemoveComponent(ScriptEntityId id,const std::string&c){auto it=entities.find(id);return it!=entities.end()&&it->second.components.erase(c)!=0;}
bool ScriptEntityRegistry::HasComponent(ScriptEntityId id,const std::string&c)const{auto it=entities.find(id);return it!=entities.end()&&it->second.components.count(c)!=0;}
VekValue ScriptEntityRegistry::GetComponent(ScriptEntityId id,const std::string&c)const{auto it=entities.find(id);if(it==entities.end())return {};auto ci=it->second.components.find(c);return ci==it->second.components.end()?VekValue():ci->second;}
std::size_t ScriptEntityRegistry::Count()const{return entities.size();}
void ScriptEntityRegistry::RegisterNatives(VekScriptEngine&e){e.RegisterNative("entity_create",[this](const std::vector<VekValue>&a){return VekValue((double)Create(a.empty()?"":a[0].AsString()));});e.RegisterNative("entity_destroy",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Destroy((ScriptEntityId)a[0].AsNumber()));});e.RegisterNative("entity_add_component",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=2&&AddComponent((ScriptEntityId)a[0].AsNumber(),a[1].AsString(),a.size()>2?a[2]:VekValue::Map()));});e.RegisterNative("entity_has_component",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=2&&HasComponent((ScriptEntityId)a[0].AsNumber(),a[1].AsString()));});e.RegisterNative("entity_get_component",[this](const std::vector<VekValue>&a){return a.size()>=2?GetComponent((ScriptEntityId)a[0].AsNumber(),a[1].AsString()):VekValue();});}
}
