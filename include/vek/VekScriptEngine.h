#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#define VEK_VERSION_MAJOR 2
#define VEK_VERSION_MINOR 2
#define VEK_VERSION_PATCH 0
#define VEK_VERSION_STRING "2.2.0"

class VekValue;
using VekArray = std::vector<VekValue>;
using VekMap = std::unordered_map<std::string, VekValue>;

class VekValue {
public:
    using ArrayPtr = std::shared_ptr<VekArray>;
    using MapPtr = std::shared_ptr<VekMap>;
    using Storage = std::variant<std::monostate, double, bool, std::string, ArrayPtr, MapPtr>;

    VekValue() = default;
    VekValue(double v) : value(v) {}
    VekValue(float v) : value(static_cast<double>(v)) {}
    VekValue(int v) : value(static_cast<double>(v)) {}
    VekValue(bool v) : value(v) {}
    VekValue(const char* v) : value(std::string(v ? v : "")) {}
    VekValue(std::string v) : value(std::move(v)) {}
    VekValue(VekArray v) : value(std::make_shared<VekArray>(std::move(v))) {}
    VekValue(VekMap v) : value(std::make_shared<VekMap>(std::move(v))) {}

    static VekValue Array();
    static VekValue Map();

    bool IsNil() const;
    bool IsNumber() const;
    bool IsBool() const;
    bool IsString() const;
    bool IsArray() const;
    bool IsMap() const;

    double AsNumber(double fallback = 0.0) const;
    bool AsBool(bool fallback = false) const;
    std::string AsString() const;
    bool Truthy() const;
    std::size_t Size() const;

    const VekArray* AsArray() const;
    VekArray* AsArray();
    const VekMap* AsMap() const;
    VekMap* AsMap();

    VekValue Get(const std::string& key) const;
    VekValue Get(std::size_t index) const;
    bool Set(const std::string& key, VekValue v);
    bool Set(std::size_t index, VekValue v);
    bool Push(VekValue v);
    std::string ToJson() const;

private:
    Storage value;
};

struct VekSecurityPolicy {
    std::size_t maxSourceBytes = 512u * 1024u;
    std::size_t maxTokens = 131072;
    std::size_t maxFunctions = 1024;
    std::size_t maxParametersPerFunction = 64;
    std::size_t maxStringBytes = 32u * 1024u;
    std::size_t maxArgumentsPerCall = 64;
    std::size_t maxCallDepth = 64;
    std::size_t maxInstructionsPerCall = 750000;
    std::size_t maxNativeCallsPerCall = 16384;
    std::size_t maxLoopIterationsPerCall = 300000;
    std::size_t maxContainerItems = 8192;
    std::size_t maxModuleCount = 128;
    std::size_t maxImportDepth = 16;
};

class VekScriptEngine {
public:
    using NativeFunction = std::function<VekValue(const std::vector<VekValue>&)>;

    VekScriptEngine();
    ~VekScriptEngine();
    VekScriptEngine(VekScriptEngine&&) noexcept;
    VekScriptEngine& operator=(VekScriptEngine&&) noexcept;
    VekScriptEngine(const VekScriptEngine&) = delete;
    VekScriptEngine& operator=(const VekScriptEngine&) = delete;

    bool LoadFile(const std::string& path);
    bool LoadSource(const std::string& source, const std::string& sourceName = "<memory>");
    void Clear();

    void SetSecurityPolicy(const VekSecurityPolicy& policy);
    const VekSecurityPolicy& GetSecurityPolicy() const;

    // Module roots are the only directories imports are allowed to read from.
    // Relative imports that escape these roots are rejected.
    void SetModuleRoots(std::vector<std::string> roots);
    const std::vector<std::string>& GetModuleRoots() const;

    bool RegisterNative(const std::string& name, NativeFunction function);
    void SealNativeRegistry();
    bool NativeRegistrySealed() const;

    bool HasFunction(const std::string& name) const;
    bool HasEvent(const std::string& name) const;
    VekValue Call(const std::string& name, const std::vector<VekValue>& args = {});
    VekValue EmitEvent(const std::string& name, const std::vector<VekValue>& args = {});

    bool IsLoaded() const;
    const std::string& LastError() const;
    const std::string& SourceName() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

// Safe, opt-in standard library for standalone VEK programs. Embedded games can
// register only the capabilities they want instead.
void VekRegisterStandardLibrary(VekScriptEngine& engine);
