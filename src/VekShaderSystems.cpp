#include <vek/VekShaderSystems.h>

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace vek {
namespace {
std::string Lower(std::string s){for(char&c:s)c=(char)std::tolower((unsigned char)c);return s;}
bool ValidId(const std::string&id){if(id.empty()||id.size()>160)return false;for(char c:id)if(!(std::isalnum((unsigned char)c)||c=='_'||c=='-'||c=='.'||c=='/'))return false;return true;}
std::size_t SourceBytes(const ShaderDescriptor&d){return d.vertexSource.size()+d.fragmentSource.size()+d.computeSource.size();}
VekValue UniformToValue(const ShaderUniformDescriptor&u){VekMap m;m["name"]=u.name;m["type"]=(double)(int)u.type;m["default"]=u.defaultValue;return VekValue(std::move(m));}
}

const char* ShaderLanguageName(ShaderLanguage l){switch(l){case ShaderLanguage::GLSL:return"glsl";case ShaderLanguage::HLSL:return"hlsl";case ShaderLanguage::WGSL:return"wgsl";case ShaderLanguage::MSL:return"msl";case ShaderLanguage::SPIRV:return"spirv";default:return"unknown";}}
ShaderLanguage ParseShaderLanguage(const std::string& n){auto s=Lower(n);if(s=="glsl")return ShaderLanguage::GLSL;if(s=="hlsl")return ShaderLanguage::HLSL;if(s=="wgsl")return ShaderLanguage::WGSL;if(s=="msl")return ShaderLanguage::MSL;if(s=="spirv"||s=="spv")return ShaderLanguage::SPIRV;return ShaderLanguage::Unknown;}

bool ShaderRegistry::Register(const ShaderDescriptor&d,std::string*error){
    if(!ValidId(d.id)){if(error)*error="shader id is invalid";return false;}
    if(!shaders_.count(d.id)&&shaders_.size()>=MaxShaders){if(error)*error="shader registry limit reached";return false;}
    if(SourceBytes(d)>MaxSourceBytes){if(error)*error="shader source exceeds VEK descriptor limit";return false;}
    if(d.uniforms.size()>MaxUniforms){if(error)*error="shader uniform limit reached";return false;}
    if(d.defines.size()>MaxDefines){if(error)*error="shader define limit reached";return false;}
    std::unordered_set<std::string> names;for(const auto&u:d.uniforms){if(!ValidId(u.name)||!names.insert(u.name).second){if(error)*error="shader contains invalid or duplicate uniform";return false;}}
    shaders_[d.id]=d;return true;
}
bool ShaderRegistry::Remove(const std::string&id){return shaders_.erase(id)!=0;}
const ShaderDescriptor*ShaderRegistry::Find(const std::string&id)const{auto it=shaders_.find(id);return it==shaders_.end()?nullptr:&it->second;}
void ShaderRegistry::Clear(){shaders_.clear();}
VekValue ShaderRegistry::Snapshot()const{VekArray a;std::vector<std::string>ids;for(auto&kv:shaders_)ids.push_back(kv.first);std::sort(ids.begin(),ids.end());for(auto&id:ids)a.push_back(ShaderDescriptorToValue(shaders_.at(id)));return VekValue(std::move(a));}

VekValue ShaderDescriptorToValue(const ShaderDescriptor&d){VekMap m;m["__type"]="ShaderDescriptor";m["id"]=d.id;m["language"]=ShaderLanguageName(d.language);m["vertex_entry"]=d.vertexEntry;m["fragment_entry"]=d.fragmentEntry;m["compute_entry"]=d.computeEntry;m["trusted_builtin"]=d.trustedBuiltIn;VekArray caps;for(auto&c:d.requiredCapabilities)caps.emplace_back(c);m["required_capabilities"]=VekValue(std::move(caps));VekArray uniforms;for(auto&u:d.uniforms)uniforms.emplace_back(UniformToValue(u));m["uniforms"]=VekValue(std::move(uniforms));VekMap defs;for(auto&kv:d.defines)defs[kv.first]=kv.second;m["defines"]=VekValue(std::move(defs));return VekValue(std::move(m));}

ShaderDescriptor MakeShaderPreset(const std::string&name){
    std::string n=Lower(name);ShaderDescriptor d;d.id="vek.ui."+n;d.trustedBuiltIn=true;d.language=ShaderLanguage::Unknown;
    if(n=="glass"||n=="frosted_glass"){d.uniforms={{"tint",ShaderUniformType::Color,{}},{"opacity",ShaderUniformType::Float,VekValue(.92)},{"blur",ShaderUniformType::Float,VekValue(12.0)}};d.requiredCapabilities={"ui.backdrop_blur"};}
    else if(n=="vignette"){d.uniforms={{"strength",ShaderUniformType::Float,VekValue(.22)},{"radius",ShaderUniformType::Float,VekValue(.75)}};}
    else if(n=="outline"){d.uniforms={{"color",ShaderUniformType::Color,{}},{"width",ShaderUniformType::Float,VekValue(1.5)}};}
    else if(n=="selection_glow"){d.uniforms={{"color",ShaderUniformType::Color,{}},{"intensity",ShaderUniformType::Float,VekValue(.65)}};}
    else if(n=="wireframe"){d.requiredCapabilities={"render.wireframe"};}
    else if(n=="heat_haze"){d.uniforms={{"amount",ShaderUniformType::Float,VekValue(.12)},{"speed",ShaderUniformType::Float,VekValue(1.0)}};d.requiredCapabilities={"render.distortion"};}
    else if(n=="color_grade"){d.uniforms={{"exposure",ShaderUniformType::Float,VekValue(0.0)},{"contrast",ShaderUniformType::Float,VekValue(1.0)},{"saturation",ShaderUniformType::Float,VekValue(1.0)}};}
    else {d.id="vek.ui.none";}
    return d;
}

void VekRegisterShaderDescriptorLibrary(VekScriptEngine&e){
    e.RegisterNative("shader_preset",[](const std::vector<VekValue>&a){return ShaderDescriptorToValue(MakeShaderPreset(a.empty()?"none":a[0].AsString()));});
    e.RegisterNative("shader_effect",[](const std::vector<VekValue>&a){VekMap m;m["__type"]="ShaderEffect";m["shader"]=a.empty()?"vek.ui.none":a[0].AsString();m["params"]=a.size()>1?a[1]:VekValue::Map();return VekValue(std::move(m));});
    e.RegisterNative("material",[](const std::vector<VekValue>&a){VekMap m;m["__type"]="Material";m["base_color"]=a.size()>0?a[0]:VekValue::Map();m["shader"]=a.size()>1?a[1]:VekValue("vek.ui.none");m["params"]=a.size()>2?a[2]:VekValue::Map();return VekValue(std::move(m));});
}

} // namespace vek
