#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <vek/VekScriptEngine.h>

namespace vek {

enum class ShaderLanguage : unsigned char { Unknown=0, GLSL, HLSL, WGSL, MSL, SPIRV };
enum class ShaderStage : unsigned char { Vertex=0, Fragment, Compute };
enum class ShaderUniformType : unsigned char { Float=0, Float2, Float3, Float4, Int, Bool, Matrix4, Texture2D, Color };

struct ShaderUniformDescriptor {
    std::string name;
    ShaderUniformType type = ShaderUniformType::Float;
    VekValue defaultValue{};
};

struct ShaderDescriptor {
    std::string id;
    ShaderLanguage language = ShaderLanguage::Unknown;
    std::string vertexSource;
    std::string fragmentSource;
    std::string computeSource;
    std::string vertexEntry = "main";
    std::string fragmentEntry = "main";
    std::string computeEntry = "main";
    std::vector<ShaderUniformDescriptor> uniforms;
    std::unordered_map<std::string,std::string> defines;
    std::vector<std::string> requiredCapabilities;
    bool trustedBuiltIn = false;
};

// Safe registry: it stores validated descriptors only. Compilation and native GPU
// object creation remain host responsibilities, so scripts never receive raw GPU
// handles or backend pointers.
class ShaderRegistry {
public:
    static constexpr std::size_t MaxShaders = 256;
    static constexpr std::size_t MaxSourceBytes = 256 * 1024;
    static constexpr std::size_t MaxUniforms = 256;
    static constexpr std::size_t MaxDefines = 128;

    bool Register(const ShaderDescriptor& descriptor, std::string* error = nullptr);
    bool Remove(const std::string& id);
    const ShaderDescriptor* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const { return shaders_.size(); }
    VekValue Snapshot() const;
private:
    std::unordered_map<std::string,ShaderDescriptor> shaders_;
};

const char* ShaderLanguageName(ShaderLanguage language);
ShaderLanguage ParseShaderLanguage(const std::string& name);
VekValue ShaderDescriptorToValue(const ShaderDescriptor& descriptor);
ShaderDescriptor MakeShaderPreset(const std::string& presetName);

// Pure descriptor helpers available in ordinary VEK scripts. These describe an
// effect/material but do not compile or execute GPU code.
void VekRegisterShaderDescriptorLibrary(VekScriptEngine& engine);

} // namespace vek
