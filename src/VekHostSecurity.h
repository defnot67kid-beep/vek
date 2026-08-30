#pragma once
#include "VekSecuritySystem.h"
#include <VekAuthoritySystems.h>
#include <VekScriptEngine.h>

inline void ApplyGameVekSecurityPolicy(VekScriptEngine& engine){
    vek::SecurityPolicyFactory::Apply(engine,
        VekSecuritySystem::IsDevelopmentMode()?vek::SecurityTier::Development:vek::SecurityTier::HardenedClient);
}
