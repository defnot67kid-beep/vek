#include <vek/VekAuthoritySystems.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>
#include <unordered_set>

namespace vek {
namespace {

bool SafeToken(const std::string& s,std::size_t maxLen,bool allowEmpty=false){
    if(s.empty()) return allowEmpty;
    if(s.size()>maxLen) return false;
    for(unsigned char c:s){
        if(std::isalnum(c)) continue;
        switch(c){case '_':case '-':case '.':case ':':case '/':case '@':case '~':case '+':case '=':break;default:return false;}
    }
    return true;
}

bool ValidReplicationType(const std::string& t){
    static const std::unordered_set<std::string> allowed={"number","int","bool","string","vec3"};
    return allowed.count(t)!=0;
}

struct PayloadShapeScan {
    std::size_t nodes=0;
    std::size_t maxDepthSeen=0;
    std::unordered_set<const void*> stack;
};

bool ScanPayload(const VekValue& v,const AuthorityActionDefinition& d,std::size_t depth,PayloadShapeScan& scan,std::string& reason){
    if(depth>d.maxPayloadDepth){reason="payload nesting exceeds action limit";return false;}
    scan.maxDepthSeen=std::max(scan.maxDepthSeen,depth);
    if(++scan.nodes>d.maxPayloadItems){reason="payload item count exceeds action limit";return false;}
    if(v.IsNil()||v.IsBool()) return true;
    if(v.IsNumber()){
        if(!std::isfinite(v.AsNumber())){reason="payload contains a non-finite number";return false;}
        return true;
    }
    if(v.IsString()){
        if(v.AsString().size()>d.maxStringBytes){reason="payload string exceeds action limit";return false;}
        return true;
    }
    if(v.IsArray()){
        const auto* a=v.AsArray();const void* p=a;
        if(!a){reason="payload contains an invalid array";return false;}
        if(scan.stack.count(p)){reason="payload contains a cyclic container";return false;}
        scan.stack.insert(p);
        for(const auto& item:*a) if(!ScanPayload(item,d,depth+1,scan,reason)){scan.stack.erase(p);return false;}
        scan.stack.erase(p);return true;
    }
    if(v.IsMap()){
        const auto* m=v.AsMap();const void* p=m;
        if(!m){reason="payload contains an invalid map";return false;}
        if(scan.stack.count(p)){reason="payload contains a cyclic container";return false;}
        scan.stack.insert(p);
        for(const auto& kv:*m){
            if(kv.first.empty()||kv.first.size()>d.maxStringBytes){reason="payload map key exceeds action limit";scan.stack.erase(p);return false;}
            for(unsigned char c:kv.first)if(c<0x20){reason="payload map key contains control characters";scan.stack.erase(p);return false;}
            if(!ScanPayload(kv.second,d,depth+1,scan,reason)){scan.stack.erase(p);return false;}
        }
        scan.stack.erase(p);return true;
    }
    reason="payload contains an unsupported value";return false;
}

} // namespace

VekSecurityPolicy SecurityPolicyFactory::ForTier(SecurityTier tier){
    VekSecurityPolicy p;
    if(tier==SecurityTier::Development){
        p.maxSourceBytes=1024u*1024u;p.maxTokens=262144;p.maxFunctions=2048;p.maxParametersPerFunction=96;
        p.maxStringBytes=64u*1024u;p.maxArgumentsPerCall=96;p.maxCallDepth=96;p.maxInstructionsPerCall=1500000;
        p.maxNativeCallsPerCall=32768;p.maxLoopIterationsPerCall=600000;p.maxContainerItems=16384;p.maxModuleCount=256;p.maxImportDepth=24;
    }else if(tier==SecurityTier::HardenedServer){
        // VEK 2.0 server defaults deliberately trade some script headroom for a
        // smaller abuse surface. They remain high enough for normal gameplay rules.
        p.maxSourceBytes=192u*1024u;p.maxTokens=49152;p.maxFunctions=384;p.maxParametersPerFunction=32;
        p.maxStringBytes=12u*1024u;p.maxArgumentsPerCall=32;p.maxCallDepth=32;p.maxInstructionsPerCall=300000;
        p.maxNativeCallsPerCall=6000;p.maxLoopIterationsPerCall=110000;p.maxContainerItems=3072;p.maxModuleCount=48;p.maxImportDepth=8;
    }else{
        p.maxSourceBytes=320u*1024u;p.maxTokens=81920;p.maxFunctions=640;p.maxParametersPerFunction=40;
        p.maxStringBytes=20u*1024u;p.maxArgumentsPerCall=40;p.maxCallDepth=40;p.maxInstructionsPerCall=420000;
        p.maxNativeCallsPerCall=8192;p.maxLoopIterationsPerCall=160000;p.maxContainerItems=4096;p.maxModuleCount=80;p.maxImportDepth=10;
    }
    return p;
}
void SecurityPolicyFactory::Apply(VekScriptEngine& e,SecurityTier tier){e.SetSecurityPolicy(ForTier(tier));}

bool CapabilityManifest::Grant(const std::string& c){if(sealed||!SafeToken(c,128))return false;return capabilities.insert(c).second;}
bool CapabilityManifest::Revoke(const std::string& c){if(sealed)return false;return capabilities.erase(c)>0;}
bool CapabilityManifest::Allows(const std::string& c)const{return c.empty()||capabilities.count(c)>0;}
void CapabilityManifest::Seal(){sealed=true;}

bool AuthorityActionRegistry::Register(const AuthorityActionDefinition& d){
    if(!SafeToken(d.id,128)||actions.count(d.id))return false;
    if(!d.requiredCapability.empty()&&!SafeToken(d.requiredCapability,128))return false;
    if(!std::isfinite(d.maxRequestsPerSecond)||d.maxRequestsPerSecond<0.1f||d.maxRequestsPerSecond>1000.0f||d.burst<1||d.burst>10000)return false;
    if(d.maxPayloadBytes<1||d.maxPayloadBytes>1024u*1024u||d.maxPayloadDepth<1||d.maxPayloadDepth>64||d.maxPayloadItems<1||d.maxPayloadItems>65536||d.maxStringBytes<1||d.maxStringBytes>1024u*1024u)return false;
    if(d.minNonceBytes<8||d.maxNonceBytes<d.minNonceBytes||d.maxNonceBytes>256)return false;
    actions.emplace(d.id,d);return true;
}
bool AuthorityActionRegistry::RegisterValue(const VekValue& v,std::string* error){
    if(!v.IsMap()){if(error)*error="authority_action_register expects a map";return false;}
    AuthorityActionDefinition d;d.id=v.Get("id").AsString();
    auto b=v.Get("server_authoritative");if(!b.IsNil())d.serverAuthoritative=b.AsBool();
    b=v.Get("allow_client_request");if(!b.IsNil())d.allowClientRequest=b.AsBool();
    b=v.Get("require_sequence");if(!b.IsNil())d.requireSequence=b.AsBool();
    b=v.Get("replay_protected");if(!b.IsNil())d.replayProtected=b.AsBool();
    b=v.Get("require_authenticated_session");if(!b.IsNil())d.requireAuthenticatedSession=b.AsBool();
    d.maxRequestsPerSecond=(float)v.Get("max_requests_per_second").AsNumber(d.maxRequestsPerSecond);
    d.burst=(int)v.Get("burst").AsNumber(d.burst);
    d.maxPayloadBytes=(std::size_t)std::max(1.0,v.Get("max_payload_bytes").AsNumber((double)d.maxPayloadBytes));
    d.maxPayloadDepth=(std::size_t)std::max(1.0,v.Get("max_payload_depth").AsNumber((double)d.maxPayloadDepth));
    d.maxPayloadItems=(std::size_t)std::max(1.0,v.Get("max_payload_items").AsNumber((double)d.maxPayloadItems));
    d.maxStringBytes=(std::size_t)std::max(1.0,v.Get("max_string_bytes").AsNumber((double)d.maxStringBytes));
    d.minNonceBytes=(std::size_t)std::max(1.0,v.Get("min_nonce_bytes").AsNumber((double)d.minNonceBytes));
    d.maxNonceBytes=(std::size_t)std::max(1.0,v.Get("max_nonce_bytes").AsNumber((double)d.maxNonceBytes));
    d.requiredCapability=v.Get("required_capability").AsString();
    if(!Register(d)){if(error)*error="invalid or duplicate authority action definition";return false;}if(error)error->clear();return true;
}
const AuthorityActionDefinition* AuthorityActionRegistry::Find(const std::string& id)const{auto it=actions.find(id);return it==actions.end()?nullptr:&it->second;}
void AuthorityActionRegistry::Clear(){actions.clear();}
void AuthorityActionRegistry::RegisterNatives(VekScriptEngine& e){
    e.RegisterNative("authority_action_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterValue(a[0],&er));});
    e.RegisterNative("authority_action_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString()));});
}

bool ReplicationSchemaRegistry::Register(const ReplicationSchemaDefinition& d){
    if(!SafeToken(d.id,128)||schemas.count(d.id)||d.fields.empty()||d.fields.size()>256||!std::isfinite(d.maxHz)||d.maxHz<0.1f||d.maxHz>240.0f)return false;
    ReplicationSchemaDefinition x=d;x.maxHz=std::clamp(x.maxHz,0.1f,240.0f);std::unordered_set<std::string> names;
    for(auto&f:x.fields){if(!SafeToken(f.name,128)||!names.insert(f.name).second)return false;if(f.type.empty())f.type="number";if(!ValidReplicationType(f.type))return false;}
    schemas.emplace(x.id,std::move(x));return true;
}
bool ReplicationSchemaRegistry::RegisterValue(const VekValue&v,std::string*error){
    if(!v.IsMap()){if(error)*error="replication_schema_register expects a map";return false;}ReplicationSchemaDefinition d;d.id=v.Get("id").AsString();d.maxHz=(float)v.Get("max_hz").AsNumber(d.maxHz);d.reliable=v.Get("reliable").AsBool(false);
    if(auto a=v.Get("fields").AsArray())for(const auto&x:*a)if(x.IsMap()){ReplicationFieldDefinition f;f.name=x.Get("name").AsString();auto t=x.Get("type").AsString();if(!t.empty())f.type=t;auto b=x.Get("server_owned");if(!b.IsNil())f.serverOwned=b.AsBool();b=x.Get("owner_only");if(!b.IsNil())f.ownerOnly=b.AsBool();d.fields.push_back(std::move(f));}
    if(!Register(d)){if(error)*error="invalid or duplicate replication schema";return false;}if(error)error->clear();return true;
}
const ReplicationSchemaDefinition*ReplicationSchemaRegistry::Find(const std::string&id)const{auto it=schemas.find(id);return it==schemas.end()?nullptr:&it->second;}
void ReplicationSchemaRegistry::Clear(){schemas.clear();}
void ReplicationSchemaRegistry::RegisterNatives(VekScriptEngine&e){e.RegisterNative("replication_schema_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterValue(a[0],&er));});e.RegisterNative("replication_schema_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString()));});}

void SecurityAuditBuffer::Push(SecurityAuditEvent e){if(maxEvents==0)return;events.push_back(std::move(e));while(events.size()>maxEvents)events.pop_front();}

ServerAuthoritySystem::ServerAuthoritySystem(HostAuthorityRole r):hostRole(r){}
void ServerAuthoritySystem::ResetActor(const std::string&a){for(auto it=state.begin();it!=state.end();){if(it->first.rfind(a+"\n",0)==0)it=state.erase(it);else ++it;}}
void ServerAuthoritySystem::Reset(){state.clear();audit.Clear();}
AuthorityDecision ServerAuthoritySystem::Deny(const AuthorityRequest&r,AuthorityDecisionCode c,const std::string&reason,float now){audit.Push({now,r.actorId,r.actionId,c,reason});return{c,false,reason};}

AuthorityDecision ServerAuthoritySystem::ValidateClientRequest(const AuthorityRequest&r,const AuthorityActionRegistry&reg,const CapabilityManifest&caps,float now){
    const auto*d=reg.Find(r.actionId);if(!d)return Deny(r,AuthorityDecisionCode::UnknownAction,"unknown action",now);
    if(hostRole!=HostAuthorityRole::ListenServer&&hostRole!=HostAuthorityRole::DedicatedServer)return Deny(r,AuthorityDecisionCode::WrongHostRole,"client requests must be validated by an authoritative server host",now);
    if(!d->allowClientRequest)return Deny(r,AuthorityDecisionCode::ClientRequestDisabled,"client request disabled",now);
    if(!SafeToken(r.actorId,128))return Deny(r,AuthorityDecisionCode::InvalidIdentity,"invalid actor identity",now);
    if(d->requireAuthenticatedSession){
        if(!r.authenticated)return Deny(r,AuthorityDecisionCode::Unauthenticated,"request is not bound to an authenticated session",now);
        if(!SafeToken(r.sessionId,128)||r.sessionId.size()<8)return Deny(r,AuthorityDecisionCode::InvalidSession,"invalid authenticated session id",now);
    }else if(!r.sessionId.empty()&&!SafeToken(r.sessionId,128))return Deny(r,AuthorityDecisionCode::InvalidSession,"invalid session id",now);
    if(!caps.Allows(d->requiredCapability))return Deny(r,AuthorityDecisionCode::MissingCapability,"actor lacks required capability",now);

    PayloadShapeScan scan;std::string payloadReason;
    if(!r.payload.IsNil()&&!ScanPayload(r.payload,*d,0,scan,payloadReason))return Deny(r,AuthorityDecisionCode::InvalidPayload,payloadReason,now);
    std::size_t bytes=0;
    if(!r.payload.IsNil()){
        try{bytes=r.payload.ToJson().size();}catch(...){return Deny(r,AuthorityDecisionCode::InvalidPayload,"payload serialization failed",now);}
    }
    if(bytes>d->maxPayloadBytes)return Deny(r,AuthorityDecisionCode::PayloadTooLarge,"payload exceeds action limit",now);

    if(d->replayProtected){
        if(r.nonce.size()<d->minNonceBytes||r.nonce.size()>d->maxNonceBytes||!SafeToken(r.nonce,d->maxNonceBytes))return Deny(r,AuthorityDecisionCode::InvalidNonce,"replay nonce is missing or malformed",now);
    }

    const std::string sessionKey=d->requireAuthenticatedSession?r.sessionId:std::string{};
    auto key=r.actorId+"\n"+sessionKey+"\n"+r.actionId;
    auto found=state.find(key);
    if(found==state.end()){
        if(state.size()>=maxTrackedStates)return Deny(r,AuthorityDecisionCode::StateCapacity,"authority state capacity reached",now);
        found=state.emplace(key,ActorActionState{}).first;
    }
    auto&s=found->second;

    // Cheap deterministic replay/order checks happen before rate accounting.
    // Invalid duplicates do not drain the bucket, and no state is committed yet.
    if(d->requireSequence&&s.hasSequence&&r.sequence<=s.lastSequence)return Deny(r,AuthorityDecisionCode::OutOfOrder,"sequence is stale or duplicated",now);
    if(d->replayProtected&&s.nonceSet.count(r.nonce))return Deny(r,AuthorityDecisionCode::ReplayDetected,"replayed nonce",now);

    // Token bucket handles normal packet clumping more gracefully than a fixed
    // one-second window, reducing false positives while retaining hard bounds.
    const float safeNow=std::isfinite(now)?std::max(0.0f,now):0.0f;
    if(!s.bucketInitialized){s.tokens=(float)d->burst;s.lastRefill=safeNow;s.bucketInitialized=true;}
    float elapsed=std::clamp(safeNow-s.lastRefill,0.0f,10.0f);
    s.tokens=std::min((float)d->burst,s.tokens+elapsed*d->maxRequestsPerSecond);s.lastRefill=safeNow;
    if(s.tokens<1.0f)return Deny(r,AuthorityDecisionCode::RateLimited,"action rate limit exceeded",now);
    s.tokens-=1.0f;

    // Commit replay/sequence state only after all deterministic request checks
    // have passed, so a rate-limited legitimate request can safely retry later.
    if(d->requireSequence){s.lastSequence=r.sequence;s.hasSequence=true;}
    if(d->replayProtected){s.nonceSet.insert(r.nonce);s.recentNonces.push_back(r.nonce);while(s.recentNonces.size()>512){s.nonceSet.erase(s.recentNonces.front());s.recentNonces.pop_front();}}
    return{AuthorityDecisionCode::Allowed,true,"allowed"};
}

AuthorityDecision ServerAuthoritySystem::ValidateAuthoritativeCommit(const std::string&id,const AuthorityActionRegistry&reg)const{const auto*d=reg.Find(id);if(!d)return{AuthorityDecisionCode::UnknownAction,false,"unknown action"};if(d->serverAuthoritative&&hostRole==HostAuthorityRole::Client)return{AuthorityDecisionCode::ClientCannotCommit,false,"client cannot commit server-authoritative state"};return{AuthorityDecisionCode::Allowed,true,"allowed"};}

void VekRegisterAuthorityLibrary(VekScriptEngine& e,AuthorityActionRegistry* a,ReplicationSchemaRegistry*r){if(a)a->RegisterNatives(e);if(r)r->RegisterNatives(e);e.RegisterNative("security_tier_name",[](const std::vector<VekValue>&x){int v=x.empty()?1:(int)x[0].AsNumber(1);return VekValue(v==2?"hardened_server":(v==0?"development":"hardened_client"));});}

} // namespace vek
