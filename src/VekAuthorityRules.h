#pragma once
#include <VekAuthoritySystems.h>
#include <VekScriptEngine.h>
#include <memory>
#include <string>

struct VekGameShellPolicy {
    bool showHelpOverlay=false;
    bool showMapInstructionBar=false;
    std::string performanceToggleKey="F";
    bool performanceDefaultVisible=false;
    bool performanceShowFps=true;
    bool performanceShowFrameMs=true;
    bool performanceShowVekMode=true;
    bool performanceShowAuthority=true;
    float cameraFloorClearance=0.35f;
    float cameraTargetFloorClearance=0.05f;
};

class VekAuthorityRules {
public:
    bool Initialize(const std::string& path);
    bool Reload();
    const std::string& Error() const { return error; }
    const VekGameShellPolicy& Shell() const { return shell; }
    const vek::AuthorityActionRegistry& Actions() const { return actions; }
    const vek::ReplicationSchemaRegistry& Replication() const { return replication; }
    const char* AuthorityRoleName() const { return "STANDALONE"; }
    bool AllowsLocalCommit(const std::string& actionId) const;
    bool AllowsDeveloperFeature(const std::string& feature,bool developerUnlocked) const;
    bool CanHotReload() const;
private:
    bool Load();
    std::string file,error;
    VekGameShellPolicy shell;
    vek::AuthorityActionRegistry actions;
    vek::ReplicationSchemaRegistry replication;
    vek::ServerAuthoritySystem authority{vek::HostAuthorityRole::Standalone};
    vek::DeveloperFeatureGate developerGate;
    std::unique_ptr<VekScriptEngine> vm;
};
