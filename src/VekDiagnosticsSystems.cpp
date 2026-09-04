#include <vek/VekDiagnosticsSystems.h>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>

#if defined(_WIN32)
#  define NOMINMAX
#  include <windows.h>
#endif

namespace vek {
namespace {
std::string UtcNow() {
    const auto now = std::chrono::system_clock::now();
    const auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm,&t);
#else
    gmtime_r(&t,&tm);
#endif
    std::ostringstream out;
    out << std::put_time(&tm,"%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

std::terminate_handler gPreviousTerminate = nullptr;
#if defined(_WIN32)
LPTOP_LEVEL_EXCEPTION_FILTER gPreviousExceptionFilter = nullptr;
#endif

[[noreturn]] void VekTerminateHandler() {
    std::string reason = "std::terminate";
    try {
        if (auto ep = std::current_exception()) std::rethrow_exception(ep);
    } catch (const std::exception& e) {
        reason += std::string(": ") + e.what();
    } catch (...) {
        reason += ": unknown exception";
    }
    VekCrashHandler::Instance().WriteManualCrash(reason);
    if (gPreviousTerminate && gPreviousTerminate != VekTerminateHandler) gPreviousTerminate();
    std::_Exit(EXIT_FAILURE);
}

#if defined(_WIN32)
LONG WINAPI VekUnhandledExceptionFilter(EXCEPTION_POINTERS* info) {
    std::uint64_t code = 0;
    std::uintptr_t address = 0;
    if (info && info->ExceptionRecord) {
        code = static_cast<std::uint64_t>(info->ExceptionRecord->ExceptionCode);
        address = reinterpret_cast<std::uintptr_t>(info->ExceptionRecord->ExceptionAddress);
    }
    std::ostringstream reason;
    reason << "Windows unhandled exception 0x" << std::hex << code;
    VekCrashHandler::Instance().WritePlatformCrash(reason.str(),address,code);
    if (gPreviousExceptionFilter && gPreviousExceptionFilter != VekUnhandledExceptionFilter) return gPreviousExceptionFilter(info);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif
}

const char* DiagnosticSeverityName(DiagnosticSeverity severity) {
    switch (severity) {
        case DiagnosticSeverity::Trace:return "trace"; case DiagnosticSeverity::Info:return "info";
        case DiagnosticSeverity::Warning:return "warning"; case DiagnosticSeverity::Error:return "error";
        case DiagnosticSeverity::Fatal:return "fatal";
    }
    return "error";
}
const char* DiagnosticDomainName(DiagnosticDomain domain) {
    switch (domain) {
        case DiagnosticDomain::Language:return "language"; case DiagnosticDomain::Lexer:return "lexer";
        case DiagnosticDomain::Parser:return "parser"; case DiagnosticDomain::Runtime:return "runtime";
        case DiagnosticDomain::Security:return "security"; case DiagnosticDomain::Native:return "native";
        case DiagnosticDomain::Host:return "host"; case DiagnosticDomain::Debugger:return "debugger";
        case DiagnosticDomain::Physics:return "physics"; case DiagnosticDomain::GPU:return "gpu";
        case DiagnosticDomain::Gameplay:return "gameplay"; case DiagnosticDomain::GUI:return "gui";
    }
    return "runtime";
}

std::string FormatDiagnostic(const DiagnosticRecord& r,bool includeStack) {
    std::ostringstream out;
    out << r.code << " [" << DiagnosticSeverityName(r.severity) << "/" << DiagnosticDomainName(r.domain) << "] " << r.message;
    if (!r.source.empty()) {
        out << "\n  at " << r.source;
        if (r.line > 0) out << ':' << r.line;
        if (!r.function.empty()) out << " in " << r.function;
    }
    if (includeStack && !r.stack.empty()) {
        out << "\nVEK stack:";
        for (auto it=r.stack.rbegin();it!=r.stack.rend();++it) {
            out << "\n  " << it->function;
            if (!it->source.empty()) out << " (" << it->source << (it->line>0?":"+std::to_string(it->line):"") << ')';
        }
    }
    return out.str();
}

DiagnosticRecord ClassifyDiagnosticMessage(const std::string& message,const std::string& source,int line,std::vector<DiagnosticFrame> stack) {
    DiagnosticRecord r;
    r.message=message;r.source=source;r.line=line;r.stack=std::move(stack);r.utcTimestamp=UtcNow();
    if(r.line<=0){auto pos=message.find(" line ");if(pos!=std::string::npos){pos+=6;int parsed=0;while(pos<message.size()&&message[pos]>='0'&&message[pos]<='9'){parsed=parsed*10+(message[pos]-'0');++pos;}if(parsed>0)r.line=parsed;}}
    if (message.find("VEK lexer") != std::string::npos) { r.domain=DiagnosticDomain::Lexer;r.code="VEK1001"; }
    else if (message.find("VEK parser") != std::string::npos) { r.domain=DiagnosticDomain::Parser;r.code="VEK1101"; }
    else if (message.find("VEK security") != std::string::npos) { r.domain=DiagnosticDomain::Security;r.code="VEK3001"; }
    else if (message.find("VEK debugger") != std::string::npos) { r.domain=DiagnosticDomain::Debugger;r.code="VEK4001"; }
    else if (message.find("native") != std::string::npos) { r.domain=DiagnosticDomain::Native;r.code="VEK2201"; }
    else { r.domain=DiagnosticDomain::Runtime;r.code="VEK2001"; }
    return r;
}

void DiagnosticHub::SetSink(Sink sink){std::lock_guard<std::mutex>l(mutex_);sink_=std::move(sink);} 
DiagnosticRecord DiagnosticHub::Publish(DiagnosticRecord r){Sink sink;{std::lock_guard<std::mutex>l(mutex_);r.sequence=nextSequence_++;if(r.utcTimestamp.empty())r.utcTimestamp=UtcNow();records_.push_back(r);if(records_.size()>capacity_)records_.erase(records_.begin(),records_.begin()+(records_.size()-capacity_));sink=sink_;}if(sink)sink(r);return r;}
std::vector<DiagnosticRecord> DiagnosticHub::Snapshot()const{std::lock_guard<std::mutex>l(mutex_);return records_;}
void DiagnosticHub::Clear(){std::lock_guard<std::mutex>l(mutex_);records_.clear();}
void DiagnosticHub::SetCapacity(std::size_t n){std::lock_guard<std::mutex>l(mutex_);capacity_=std::clamp<std::size_t>(n,8,8192);if(records_.size()>capacity_)records_.erase(records_.begin(),records_.begin()+(records_.size()-capacity_));}

void VekDebugger::Attach(bool enabled){attached_=enabled;if(!enabled)Continue();}
bool VekDebugger::Attached()const{return attached_;}
void VekDebugger::AddFunctionBreakpoint(const std::string&f){if(!f.empty())functionBreakpoints_.insert(f);}
void VekDebugger::RemoveFunctionBreakpoint(const std::string&f){functionBreakpoints_.erase(f);}void VekDebugger::ClearBreakpoints(){functionBreakpoints_.clear();lineBreakpoints_.clear();}
bool VekDebugger::HasFunctionBreakpoint(const std::string&f)const{return functionBreakpoints_.count(f)!=0;}
static std::string DebugLineKey(const std::string&s,int line){return s+"#"+std::to_string(line);}
void VekDebugger::AddLineBreakpoint(const std::string&s,int line){if(!s.empty()&&line>0)lineBreakpoints_.insert(DebugLineKey(s,line));}
void VekDebugger::RemoveLineBreakpoint(const std::string&s,int line){lineBreakpoints_.erase(DebugLineKey(s,line));}
bool VekDebugger::HasLineBreakpoint(const std::string&s,int line)const{return lineBreakpoints_.count(DebugLineKey(s,line))!=0;}
void VekDebugger::RequestBreak(){manualBreakRequested_=true;}void VekDebugger::Continue(){skipOnceFunction_=pauseReason_==DebugPauseReason::FunctionBreakpoint?pausedFunction_:"";skipOnceLine_=pauseReason_==DebugPauseReason::LineBreakpoint?DebugLineKey(pausedSource_,pausedLine_):"";paused_=false;manualBreakRequested_=false;pauseReason_=DebugPauseReason::None;pausedFunction_.clear();pausedSource_.clear();pausedLine_=0;}
bool VekDebugger::Paused()const{return paused_;}DebugPauseReason VekDebugger::PauseReason()const{return pauseReason_;}const std::string&VekDebugger::PausedFunction()const{return pausedFunction_;}
void VekDebugger::SetTraceCapacity(std::size_t n){traceCapacity_=std::clamp<std::size_t>(n,16,65536);if(trace_.size()>traceCapacity_)trace_.erase(trace_.begin(),trace_.begin()+(trace_.size()-traceCapacity_));}
std::vector<DebugTraceEvent> VekDebugger::TraceSnapshot()const{return trace_;}
void VekDebugger::PushTrace(const std::string&k,const std::string&f,const std::string&s,int line,const std::string&d){if(!attached_)return;trace_.push_back({nextTraceSequence_++,k,f,s,line,d});if(trace_.size()>traceCapacity_)trace_.erase(trace_.begin(),trace_.begin()+(trace_.size()-traceCapacity_));}
bool VekDebugger::OnFunctionEnter(const std::string&f,const std::string&s,int line){PushTrace("enter",f,s,line);if(!attached_)return false;if(!skipOnceFunction_.empty()&&skipOnceFunction_==f){skipOnceFunction_.clear();return false;}if(manualBreakRequested_||HasFunctionBreakpoint(f)){paused_=true;pauseReason_=manualBreakRequested_?DebugPauseReason::ManualBreak:DebugPauseReason::FunctionBreakpoint;pausedFunction_=f;pausedSource_=s;pausedLine_=line;manualBreakRequested_=false;PushTrace("break",f,s,line);return true;}return false;}
void VekDebugger::OnFunctionExit(const std::string&f,const std::string&s,int line){PushTrace("exit",f,s,line);}
bool VekDebugger::OnStatement(const std::string&f,const std::string&s,int line){PushTrace("line",f,s,line);if(!attached_)return false;const auto key=DebugLineKey(s,line);if(!skipOnceLine_.empty()&&skipOnceLine_==key){skipOnceLine_.clear();return false;}const bool manual=manualBreakRequested_;const bool lineBreak=HasLineBreakpoint(s,line);if(manual||lineBreak){paused_=true;manualBreakRequested_=false;pauseReason_=manual?DebugPauseReason::ManualBreak:DebugPauseReason::LineBreakpoint;pausedFunction_=f;pausedSource_=s;pausedLine_=line;PushTrace("break",f,s,line,lineBreak?"line breakpoint":"manual break");return true;}return false;}
void VekDebugger::OnRuntimeError(const DiagnosticRecord&d){if(!attached_)return;paused_=true;pauseReason_=DebugPauseReason::RuntimeError;pausedFunction_=d.function;pausedSource_=d.source;pausedLine_=d.line;PushTrace("error",d.function,d.source,d.line,d.code+": "+d.message);}

VekCrashHandler& VekCrashHandler::Instance(){static VekCrashHandler h;return h;}
bool VekCrashHandler::Install(const std::string&path){std::lock_guard<std::mutex>l(mutex_);if(installed_)return true;path_=path.empty()?"vek_crashlogs.txt":path;gPreviousTerminate=std::set_terminate(VekTerminateHandler);
#if defined(_WIN32)
    gPreviousExceptionFilter=SetUnhandledExceptionFilter(VekUnhandledExceptionFilter);
#endif
    installed_=true;return true;}
void VekCrashHandler::Uninstall(){std::lock_guard<std::mutex>l(mutex_);if(!installed_)return;if(gPreviousTerminate)std::set_terminate(gPreviousTerminate);
#if defined(_WIN32)
    SetUnhandledExceptionFilter(gPreviousExceptionFilter);
#endif
    installed_=false;}
bool VekCrashHandler::Installed()const{std::lock_guard<std::mutex>l(mutex_);return installed_;}
void VekCrashHandler::SetContext(CrashContext c){std::lock_guard<std::mutex>l(mutex_);context_=std::move(c);}CrashContext VekCrashHandler::Context()const{std::lock_guard<std::mutex>l(mutex_);return context_;}void VekCrashHandler::SetStage(const std::string&s){std::lock_guard<std::mutex>l(mutex_);context_.stage=s;}void VekCrashHandler::RecordDiagnostic(const DiagnosticRecord&d){std::lock_guard<std::mutex>l(mutex_);lastDiagnostic_=d;hasDiagnostic_=true;}
bool VekCrashHandler::WriteManualCrash(const std::string&reason,const std::vector<DiagnosticFrame>&stack){return AppendReport(reason,nullptr,stack);}
bool VekCrashHandler::WritePlatformCrash(const std::string&reason,std::uintptr_t address,std::uint64_t code){return AppendReport(reason,nullptr,{},address,code);}
bool VekCrashHandler::AppendReport(const std::string&reason,const DiagnosticRecord* diagnostic,const std::vector<DiagnosticFrame>&stack,std::uintptr_t address,std::uint64_t platformCode){CrashContext ctx;DiagnosticRecord last;bool has=false;std::string path;{std::lock_guard<std::mutex>l(mutex_);ctx=context_;last=lastDiagnostic_;has=hasDiagnostic_;path=path_;}std::ofstream out(path,std::ios::app|std::ios::binary);if(!out)return false;out<<"\n============================================================\n";out<<"VEK CRASH REPORT\n";out<<"UTC: "<<UtcNow()<<"\nProduct: "<<ctx.product<<"\nVersion: "<<ctx.version<<"\nBuild: "<<ctx.build<<"\nStage: "<<ctx.stage<<"\nThread: "<<std::this_thread::get_id()<<"\nReason: "<<reason<<"\n";if(platformCode)out<<"Platform code: 0x"<<std::hex<<platformCode<<std::dec<<"\n";if(address)out<<"Address: 0x"<<std::hex<<address<<std::dec<<"\n";const DiagnosticRecord* d=diagnostic?diagnostic:(has?&last:nullptr);if(d)out<<"Last diagnostic:\n"<<FormatDiagnostic(*d,true)<<"\n";if(!stack.empty()){out<<"Host/VEK stack:\n";for(auto it=stack.rbegin();it!=stack.rend();++it)out<<"  "<<it->function<<" ("<<it->source<<(it->line>0?":"+std::to_string(it->line):"")<<")\n";}if(!ctx.source.empty())out<<"Source: "<<ctx.source<<"\n";if(!ctx.notes.empty())out<<"Notes: "<<ctx.notes<<"\n";out<<"============================================================\n";return static_cast<bool>(out);}

} // namespace vek
