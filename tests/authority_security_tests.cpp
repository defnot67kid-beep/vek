#include <vek/VekAuthoritySystems.h>
#include <vek/VekScriptEngine.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>

int main(){
    using namespace vek;
    auto serverPolicy=SecurityPolicyFactory::ForTier(SecurityTier::HardenedServer);
    auto devPolicy=SecurityPolicyFactory::ForTier(SecurityTier::Development);
    assert(serverPolicy.maxInstructionsPerCall < devPolicy.maxInstructionsPerCall);

    VekScriptEngine vm;
    AuthorityActionRegistry actions;
    ReplicationSchemaRegistry replication;
    VekRegisterStandardLibrary(vm);
    VekRegisterAuthorityLibrary(vm,&actions,&replication);
    vm.SealNativeRegistry();
    const char* src=R"VEK(
        fn setup(){
            authority_action_register({id:"economy.purchase",server_authoritative:true,allow_client_request:true,require_sequence:true,replay_protected:true,require_authenticated_session:true,max_requests_per_second:2,burst:2,max_payload_bytes:128,max_payload_depth:5,max_payload_items:16,max_string_bytes:64,min_nonce_bytes:12,max_nonce_bytes:64,required_capability:"shop.buy"});
            replication_schema_register({id:"economy",max_hz:4,reliable:true,fields:[{name:"money",type:"int",server_owned:true},{name:"xp",type:"int",server_owned:true}]});
            return true;
        }
    )VEK";
    assert(vm.LoadSource(src,"authority-test"));
    assert(vm.Call("setup").AsBool());
    assert(actions.Find("economy.purchase"));
    assert(replication.Find("economy"));

    CapabilityManifest caps;assert(caps.Grant("shop.buy"));caps.Seal();assert(!caps.Grant("admin"));
    ServerAuthoritySystem server(HostAuthorityRole::DedicatedServer);
    AuthorityRequest r;r.actionId="economy.purchase";r.actorId="p1";r.sessionId="session-01";r.authenticated=true;r.sequence=1;r.nonce="nonce-000001";r.payload=VekValue(VekMap{{"item",VekValue("wheel")}});
    assert(server.ValidateClientRequest(r,actions,caps,1.0f).allowed);
    assert(!server.ValidateClientRequest(r,actions,caps,1.1f).allowed); // stale sequence / replay
    r.sequence=2;r.nonce="nonce-000002";assert(server.ValidateClientRequest(r,actions,caps,1.2f).allowed);
    r.sequence=3;r.nonce="nonce-000003";assert(!server.ValidateClientRequest(r,actions,caps,1.3f).allowed); // token bucket exhausted
    // A rate-limited request does not burn sequence/nonce state; it can retry after refill.
    assert(server.ValidateClientRequest(r,actions,caps,2.3f).allowed);

    AuthorityRequest unauth=r;unauth.sequence=4;unauth.nonce="nonce-000004";unauth.authenticated=false;
    assert(!server.ValidateClientRequest(unauth,actions,caps,3.0f).allowed);

    AuthorityRequest malformed=r;malformed.sequence=4;malformed.nonce="short";
    assert(!server.ValidateClientRequest(malformed,actions,caps,3.0f).allowed);

    ServerAuthoritySystem client(HostAuthorityRole::Client);
    assert(!client.ValidateAuthoritativeCommit("economy.purchase",actions).allowed);
    assert(server.ValidateAuthoritativeCommit("economy.purchase",actions).allowed);
    std::cout << "VEK 2.0 authority/security tests: PASS\n";
    return 0;
}
