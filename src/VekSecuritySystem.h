#pragma once

#include <string>

struct VekVerifiedScript {
    bool ok = false;
    bool signatureVerified = false;
    std::string source;
    std::string error;
};

class VekSecuritySystem {
public:
    // Secure release verifies a developer signature before a .vek script is
    // allowed into the VM. Development builds can deliberately bypass this so
    // the developer can hot-reload scripts while authoring the game.
    static VekVerifiedScript VerifyScript(const std::string& scriptPath,
                                          const std::string& signaturePath);
    static const char* SecurityModeName();
    static bool IsDevelopmentMode();
};
