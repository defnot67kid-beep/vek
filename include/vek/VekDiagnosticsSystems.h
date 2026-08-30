#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace vek {

enum class DiagnosticSeverity : std::uint8_t { Trace=0, Info, Warning, Error, Fatal };
enum class DiagnosticDomain : std::uint8_t { Language=0, Lexer, Parser, Runtime, Security, Native, Host, Debugger, Physics, GPU, Gameplay, GUI };

struct DiagnosticFrame {
    std::string function;
    std::string source;
    int line = 0;
};

struct DiagnosticRecord {
    std::uint64_t sequence = 0;
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    DiagnosticDomain domain = DiagnosticDomain::Runtime;
    std::string code = "VEK0000";
    std::string message;
    std::string source;
    int line = 0;
    int column = 0;
    std::string function;
    std::vector<DiagnosticFrame> stack;
    std::string utcTimestamp;
};

const char* DiagnosticSeverityName(DiagnosticSeverity severity);
const char* DiagnosticDomainName(DiagnosticDomain domain);
std::string FormatDiagnostic(const DiagnosticRecord& record, bool includeStack=true);
DiagnosticRecord ClassifyDiagnosticMessage(const std::string& message,
                                           const std::string& source={},
                                           int line=0,
                                           std::vector<DiagnosticFrame> stack={});

class DiagnosticHub {
public:
    using Sink = std::function<void(const DiagnosticRecord&)>;

    void SetSink(Sink sink);
    DiagnosticRecord Publish(DiagnosticRecord record);
    std::vector<DiagnosticRecord> Snapshot() const;
    void Clear();
    void SetCapacity(std::size_t maxRecords);

private:
    mutable std::mutex mutex_;
    std::vector<DiagnosticRecord> records_;
    Sink sink_;
    std::size_t capacity_ = 256;
    std::uint64_t nextSequence_ = 1;
};

enum class DebugPauseReason : std::uint8_t { None=0, FunctionBreakpoint, LineBreakpoint, ManualBreak, RuntimeError };

struct DebugTraceEvent {
    std::uint64_t sequence = 0;
    std::string kind;
    std::string function;
    std::string source;
    int line = 0;
    std::string detail;
};

class VekDebugger {
public:
    void Attach(bool enabled=true);
    bool Attached() const;
    void AddFunctionBreakpoint(const std::string& function);
    void RemoveFunctionBreakpoint(const std::string& function);
    void ClearBreakpoints();
    bool HasFunctionBreakpoint(const std::string& function) const;
    void AddLineBreakpoint(const std::string& source,int line);
    void RemoveLineBreakpoint(const std::string& source,int line);
    bool HasLineBreakpoint(const std::string& source,int line) const;
    void RequestBreak();
    void Continue();
    bool Paused() const;
    DebugPauseReason PauseReason() const;
    const std::string& PausedFunction() const;
    void SetTraceCapacity(std::size_t capacity);
    std::vector<DebugTraceEvent> TraceSnapshot() const;

    // VM-facing hooks. These are public so custom embedders can emit the same
    // debugger events for native/host frames.
    bool OnFunctionEnter(const std::string& function,const std::string& source,int line);
    void OnFunctionExit(const std::string& function,const std::string& source,int line);
    bool OnStatement(const std::string& function,const std::string& source,int line);
    void OnRuntimeError(const DiagnosticRecord& diagnostic);

private:
    void PushTrace(const std::string& kind,const std::string& function,const std::string& source,int line,const std::string& detail={});

    bool attached_ = false;
    bool paused_ = false;
    bool manualBreakRequested_ = false;
    DebugPauseReason pauseReason_ = DebugPauseReason::None;
    std::string pausedFunction_;
    std::string pausedSource_;
    int pausedLine_ = 0;
    std::string skipOnceFunction_;
    std::unordered_set<std::string> functionBreakpoints_;
    std::unordered_set<std::string> lineBreakpoints_;
    std::string skipOnceLine_;
    std::vector<DebugTraceEvent> trace_;
    std::size_t traceCapacity_ = 512;
    std::uint64_t nextTraceSequence_ = 1;
};

struct CrashContext {
    std::string product = "VEK";
    std::string version;
    std::string build;
    std::string stage;
    std::string source;
    std::string notes;
};

class VekCrashHandler {
public:
    static VekCrashHandler& Instance();

    // Installs terminate handling and, on Windows, an unhandled SEH filter.
    // The file is append-only so multiple crashes can be preserved.
    bool Install(const std::string& crashLogPath="vek_crashlogs.txt");
    void Uninstall();
    bool Installed() const;
    void SetContext(CrashContext context);
    CrashContext Context() const;
    void SetStage(const std::string& stage);
    void RecordDiagnostic(const DiagnosticRecord& diagnostic);
    bool WriteManualCrash(const std::string& reason,
                          const std::vector<DiagnosticFrame>& stack={});
    bool WritePlatformCrash(const std::string& reason,std::uintptr_t address,std::uint64_t platformCode);

private:
    VekCrashHandler() = default;
    VekCrashHandler(const VekCrashHandler&) = delete;
    VekCrashHandler& operator=(const VekCrashHandler&) = delete;
    bool AppendReport(const std::string& reason,const DiagnosticRecord* diagnostic,const std::vector<DiagnosticFrame>& stack,std::uintptr_t address=0,std::uint64_t platformCode=0);

    mutable std::mutex mutex_;
    bool installed_ = false;
    std::string path_ = "vek_crashlogs.txt";
    CrashContext context_{};
    DiagnosticRecord lastDiagnostic_{};
    bool hasDiagnostic_ = false;
};

} // namespace vek
