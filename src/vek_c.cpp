#include <vek/vek_c.h>
#include <vek/VekScriptEngine.h>
#include <vek/VekAuthoritySystems.h>
#include <vek/VekDiagnosticsSystems.h>
#include <string>
#include <unordered_map>
#include <vector>

struct vek_runtime {
    VekScriptEngine engine;
    std::vector<std::string> roots;
    std::string scratch;
    std::string authority_reason;
    vek::AuthorityActionRegistry actions;
    vek::ReplicationSchemaRegistry replication;
    vek::ServerAuthoritySystem authority{vek::HostAuthorityRole::Standalone};
    vek::VekDebugger debugger;
    struct Native { vek_native_fn fn=nullptr; void* user=nullptr; };
    std::unordered_map<std::string,Native> native;
    vek_runtime(){VekRegisterStandardLibrary(engine);vek::VekRegisterAuthorityLibrary(engine,&actions,&replication);}
};
static VekValue ToCpp(const vek_value&v){switch(v.type){case VEK_NUMBER:return VekValue(v.number);case VEK_BOOL:return VekValue(v.boolean!=0);case VEK_STRING:return VekValue(v.string_value?v.string_value:"");case VEK_JSON:return VekValue(v.string_value?v.string_value:"");default:return VekValue();}}
static vek_value ToC(vek_runtime*r,const VekValue&v){vek_value out{};if(v.IsNumber()){out.type=VEK_NUMBER;out.number=v.AsNumber();}else if(v.IsBool()){out.type=VEK_BOOL;out.boolean=v.AsBool()?1:0;}else if(v.IsString()){out.type=VEK_STRING;r->scratch=v.AsString();out.string_value=r->scratch.c_str();}else if(v.IsArray()||v.IsMap()){out.type=VEK_JSON;r->scratch=v.ToJson();out.string_value=r->scratch.c_str();}else out.type=VEK_NIL;return out;}
extern "C" {
vek_runtime* vek_create(void){try{return new vek_runtime();}catch(...){return nullptr;}}
void vek_destroy(vek_runtime*r){delete r;}
int vek_load_file(vek_runtime*r,const char*p){return r&&p&&r->engine.LoadFile(p)?1:0;}
int vek_load_source(vek_runtime*r,const char*s,const char*n){return r&&s&&r->engine.LoadSource(s,n?n:"<c-api>")?1:0;}
int vek_add_module_root(vek_runtime*r,const char*p){if(!r||!p)return 0;r->roots.push_back(p);r->engine.SetModuleRoots(r->roots);return 1;}
int vek_register_native(vek_runtime*r,const char*n,vek_native_fn fn,void*u){
    if(!r||!n||!fn)return 0;
    std::string key(n);
    // Do not update the host callback table until the engine accepts the name.
    // Older code overwrote r->native first, so a rejected duplicate could still
    // replace the callback used by the already-registered native.
    if(r->native.count(key))return 0;
    bool registered=r->engine.RegisterNative(key,[r,key](const std::vector<VekValue>&args){
        auto it=r->native.find(key);if(it==r->native.end())return VekValue();
        std::vector<vek_value> ca;ca.reserve(args.size());std::vector<std::string> storage;storage.reserve(args.size());
        for(auto&v:args){vek_value cv{};if(v.IsNumber()){cv.type=VEK_NUMBER;cv.number=v.AsNumber();}else if(v.IsBool()){cv.type=VEK_BOOL;cv.boolean=v.AsBool()?1:0;}else{storage.push_back(v.IsString()?v.AsString():v.ToJson());cv.type=v.IsString()?VEK_STRING:VEK_JSON;cv.string_value=storage.back().c_str();}ca.push_back(cv);}
        vek_value ret=it->second.fn(r,ca.data(),ca.size(),it->second.user);return ToCpp(ret);
    });
    if(!registered)return 0;
    r->native.emplace(std::move(key),vek_runtime::Native{fn,u});
    return 1;
}
void vek_seal_natives(vek_runtime*r){if(r)r->engine.SealNativeRegistry();}
int vek_set_security_tier(vek_runtime*r,int tier){if(!r)return 0;vek::SecurityTier t=tier<=0?vek::SecurityTier::Development:(tier>=2?vek::SecurityTier::HardenedServer:vek::SecurityTier::HardenedClient);vek::SecurityPolicyFactory::Apply(r->engine,t);return 1;}
int vek_set_authority_role(vek_runtime*r,int role){if(!r)return 0;vek::HostAuthorityRole v=vek::HostAuthorityRole::Standalone;if(role==1)v=vek::HostAuthorityRole::Client;else if(role==2)v=vek::HostAuthorityRole::ListenServer;else if(role>=3)v=vek::HostAuthorityRole::DedicatedServer;r->authority.SetRole(v);return 1;}
size_t vek_authority_action_count(vek_runtime*r){return r?r->actions.Size():0;}
size_t vek_replication_schema_count(vek_runtime*r){return r?r->replication.Size():0;}
int vek_authority_validate_request_v2(vek_runtime*r,const char*action,const char*actor,const char*session,int authenticated,uint64_t sequence,const char*nonce,const char*payload,const char*cap,double now){if(!r||!action||!actor)return 0;vek::CapabilityManifest caps;if(cap&&cap[0])caps.Grant(cap);caps.Seal();vek::AuthorityRequest q;q.actionId=action;q.actorId=actor;q.sessionId=session?session:"";q.authenticated=authenticated!=0;q.sequence=sequence;q.nonce=nonce?nonce:"";q.payload=VekValue(payload?payload:"");auto d=r->authority.ValidateClientRequest(q,r->actions,caps,(float)now);r->authority_reason=d.reason;return d.allowed?1:0;}
int vek_authority_validate_request(vek_runtime*r,const char*action,const char*actor,uint64_t sequence,const char*nonce,const char*payload,const char*cap,double now){return vek_authority_validate_request_v2(r,action,actor,"",0,sequence,nonce,payload,cap,now);}
const char* vek_authority_last_reason(vek_runtime*r){return r?r->authority_reason.c_str():"null runtime";}
vek_value vek_call(vek_runtime*r,const char*n,const vek_value*a,size_t c){if(!r||!n)return {};std::vector<VekValue>args;for(size_t i=0;i<c;++i)args.push_back(ToCpp(a[i]));return ToC(r,r->engine.Call(n,args));}
vek_value vek_emit_event(vek_runtime*r,const char*n,const vek_value*a,size_t c){if(!r||!n)return {};std::vector<VekValue>args;for(size_t i=0;i<c;++i)args.push_back(ToCpp(a[i]));return ToC(r,r->engine.EmitEvent(n,args));}
int vek_has_function(vek_runtime*r,const char*n){return r&&n&&r->engine.HasFunction(n);}
int vek_has_event(vek_runtime*r,const char*n){return r&&n&&r->engine.HasEvent(n);}
const char* vek_last_error(vek_runtime*r){return r?r->engine.LastError().c_str():"VEK: null runtime";}
const char* vek_last_diagnostic_code(vek_runtime*r){if(!r)return "VEK0000";r->scratch=r->engine.LastDiagnostic().code;return r->scratch.c_str();}
const char* vek_last_diagnostic_text(vek_runtime*r){if(!r)return "VEK: null runtime";r->scratch=vek::FormatDiagnostic(r->engine.LastDiagnostic(),true);return r->scratch.c_str();}
size_t vek_diagnostic_count(vek_runtime*r){return r?r->engine.Diagnostics().size():0;}
int vek_debugger_attach(vek_runtime*r,int enabled){if(!r)return 0;r->debugger.Attach(enabled!=0);r->engine.SetDebugger(enabled?&r->debugger:nullptr);return 1;}
int vek_debugger_add_function_breakpoint(vek_runtime*r,const char*n){if(!r||!n||!*n)return 0;r->debugger.AddFunctionBreakpoint(n);return 1;}
int vek_debugger_remove_function_breakpoint(vek_runtime*r,const char*n){if(!r||!n||!*n)return 0;r->debugger.RemoveFunctionBreakpoint(n);return 1;}
int vek_debugger_add_line_breakpoint(vek_runtime*r,const char*s,int line){if(!r||!s||!*s||line<=0)return 0;r->debugger.AddLineBreakpoint(s,line);return 1;}
int vek_debugger_remove_line_breakpoint(vek_runtime*r,const char*s,int line){if(!r||!s||!*s||line<=0)return 0;r->debugger.RemoveLineBreakpoint(s,line);return 1;}
void vek_debugger_continue(vek_runtime*r){if(r)r->debugger.Continue();}
int vek_debugger_is_paused(vek_runtime*r){return r&&r->debugger.Paused()?1:0;}
const char* vek_debugger_paused_function(vek_runtime*r){if(!r)return "";r->scratch=r->debugger.PausedFunction();return r->scratch.c_str();}
const char* vek_debugger_trace_json(vek_runtime*r){if(!r)return "[]";VekArray rows;for(auto&e:r->debugger.TraceSnapshot()){VekMap m;m["sequence"]=(double)e.sequence;m["kind"]=e.kind;m["function"]=e.function;m["source"]=e.source;m["line"]=(double)e.line;m["detail"]=e.detail;rows.emplace_back(std::move(m));}r->scratch=VekValue(std::move(rows)).ToJson();return r->scratch.c_str();}
int vek_install_crash_handler(vek_runtime*r,const char*path,const char*product,const char*build){if(!r)return 0;auto&h=vek::VekCrashHandler::Instance();vek::CrashContext c;c.product=product&&*product?product:"VEK Host";c.version=VEK_VERSION_STRING;c.build=build?build:"";h.SetContext(std::move(c));return h.Install(path&&*path?path:"vek_crashlogs.txt")?1:0;}
void vek_set_crash_stage(vek_runtime*r,const char*stage){if(r)vek::VekCrashHandler::Instance().SetStage(stage?stage:"");}
int vek_write_crash_report(vek_runtime*r,const char*reason){if(!r)return 0;return vek::VekCrashHandler::Instance().WriteManualCrash(reason?reason:"manual crash report")?1:0;}
const char* vek_version(void){return VEK_VERSION_STRING;}
}
