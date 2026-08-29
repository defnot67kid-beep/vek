#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <vek/VekScriptEngine.h>

class VekScriptEngine;

namespace vek {

enum class SecurityTier { Development=0, HardenedClient=1, HardenedServer=2 };
enum class HostAuthorityRole { Standalone=0, Client=1, ListenServer=2, DedicatedServer=3 };

class SecurityPolicyFactory {
public:
    static VekSecurityPolicy ForTier(SecurityTier tier);
    static void Apply(VekScriptEngine& engine, SecurityTier tier);
};

class CapabilityManifest {
public:
    bool Grant(const std::string& capability);
    bool Revoke(const std::string& capability);
    bool Allows(const std::string& capability) const;
    void Seal();
    bool Sealed() const { return sealed; }
    std::size_t Size() const { return capabilities.size(); }
private:
    bool sealed=false;
    std::unordered_set<std::string> capabilities;
};

struct AuthorityActionDefinition {
    std::string id;
    bool serverAuthoritative=true;
    bool allowClientRequest=true;
    bool requireSequence=true;
    bool replayProtected=true;
    bool requireAuthenticatedSession=false;
    float maxRequestsPerSecond=10.0f;
    int burst=20;
    std::size_t maxPayloadBytes=4096;
    std::size_t maxPayloadDepth=12;
    std::size_t maxPayloadItems=256;
    std::size_t maxStringBytes=2048;
    std::size_t minNonceBytes=12;
    std::size_t maxNonceBytes=96;
    std::string requiredCapability;
};

class AuthorityActionRegistry {
public:
    bool Register(const AuthorityActionDefinition& definition);
    bool RegisterValue(const VekValue& definition,std::string* error=nullptr);
    const AuthorityActionDefinition* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const { return actions.size(); }
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,AuthorityActionDefinition> actions;
};

struct ReplicationFieldDefinition {
    std::string name;
    std::string type="number";
    bool serverOwned=true;
    bool ownerOnly=false;
};
struct ReplicationSchemaDefinition {
    std::string id;
    float maxHz=20.0f;
    bool reliable=false;
    std::vector<ReplicationFieldDefinition> fields;
};
class ReplicationSchemaRegistry {
public:
    bool Register(const ReplicationSchemaDefinition& definition);
    bool RegisterValue(const VekValue& definition,std::string* error=nullptr);
    const ReplicationSchemaDefinition* Find(const std::string& id) const;
    void Clear();
    std::size_t Size() const { return schemas.size(); }
    void RegisterNatives(VekScriptEngine& engine);
private:
    std::unordered_map<std::string,ReplicationSchemaDefinition> schemas;
};

struct AuthorityRequest {
    std::string actionId;
    std::string actorId;
    std::string sessionId;
    bool authenticated=false;
    std::uint64_t sequence=0;
    std::string nonce;
    VekValue payload;
};
enum class AuthorityDecisionCode {
    Allowed=0,
    UnknownAction,
    WrongHostRole,
    ClientCannotCommit,
    ClientRequestDisabled,
    MissingCapability,
    PayloadTooLarge,
    OutOfOrder,
    ReplayDetected,
    RateLimited,
    InvalidIdentity,
    Unauthenticated,
    InvalidSession,
    InvalidNonce,
    InvalidPayload,
    StateCapacity
};
struct AuthorityDecision {
    AuthorityDecisionCode code=AuthorityDecisionCode::Allowed;
    bool allowed=true;
    std::string reason="allowed";
};

struct SecurityAuditEvent {
    float timeSeconds=0.0f;
    std::string actorId;
    std::string actionId;
    AuthorityDecisionCode code=AuthorityDecisionCode::Allowed;
    std::string reason;
};
class SecurityAuditBuffer {
public:
    explicit SecurityAuditBuffer(std::size_t capacity=256):maxEvents(capacity){}
    void Push(SecurityAuditEvent event);
    const std::deque<SecurityAuditEvent>& Events() const { return events; }
    void Clear(){events.clear();}
private:
    std::size_t maxEvents=256;
    std::deque<SecurityAuditEvent> events;
};

class ServerAuthoritySystem {
public:
    explicit ServerAuthoritySystem(HostAuthorityRole role=HostAuthorityRole::Standalone);
    void SetRole(HostAuthorityRole role){hostRole=role;}
    HostAuthorityRole Role() const { return hostRole; }
    void ResetActor(const std::string& actorId);
    void Reset();
    void SetStateCapacity(std::size_t maxStates){maxTrackedStates=maxStates<64?64:maxStates;}
    std::size_t StateCapacity() const { return maxTrackedStates; }

    // Validates a client-originated request on an authoritative host. VEK 2.0
    // performs only deterministic checks here: authenticated-session policy,
    // capability checks, bounded payload shape/size, token-bucket rate limits,
    // monotonic sequence numbers and replay nonces. Heuristic anomaly scores do
    // not auto-ban users, which avoids turning noisy telemetry into false positives.
    AuthorityDecision ValidateClientRequest(const AuthorityRequest& request,
                                            const AuthorityActionRegistry& registry,
                                            const CapabilityManifest& actorCapabilities,
                                            float nowSeconds);

    AuthorityDecision ValidateAuthoritativeCommit(const std::string& actionId,
                                                   const AuthorityActionRegistry& registry) const;

    const SecurityAuditBuffer& Audit() const { return audit; }
private:
    struct ActorActionState {
        std::uint64_t lastSequence=0;
        bool hasSequence=false;
        float tokens=0.0f;
        float lastRefill=0.0f;
        bool bucketInitialized=false;
        std::deque<std::string> recentNonces;
        std::unordered_set<std::string> nonceSet;
    };
    HostAuthorityRole hostRole=HostAuthorityRole::Standalone;
    std::unordered_map<std::string,ActorActionState> state;
    std::size_t maxTrackedStates=32768;
    SecurityAuditBuffer audit{1024};
    AuthorityDecision Deny(const AuthorityRequest& req,AuthorityDecisionCode code,const std::string& reason,float nowSeconds);
};

void VekRegisterAuthorityLibrary(VekScriptEngine& engine,
                                 AuthorityActionRegistry* actions=nullptr,
                                 ReplicationSchemaRegistry* replication=nullptr);

} // namespace vek
