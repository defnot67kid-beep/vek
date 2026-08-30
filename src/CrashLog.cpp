#include "CrashLog.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

namespace CrashLog {
namespace {
std::mutex gMutex;
std::filesystem::path gPath;
std::string gBuildName = "unknown";
std::string gStage = "process startup";
bool gInitialized = false;

std::string TimestampUtc() {
    try {
        const auto now = std::chrono::system_clock::now();
        const std::time_t raw = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        gmtime_s(&tm, &raw);
#else
        gmtime_r(&raw, &tm);
#endif
        std::ostringstream out;
        out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S UTC");
        return out.str();
    } catch (...) {
        return "timestamp unavailable";
    }
}

std::filesystem::path ExecutableDirectory() {
#ifdef _WIN32
    try {
        std::wstring buffer(32768, L'\0');
        const DWORD count = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (count > 0 && count < buffer.size()) {
            buffer.resize(count);
            return std::filesystem::path(buffer).parent_path();
        }
    } catch (...) {}
#endif
    try { return std::filesystem::current_path(); }
    catch (...) { return std::filesystem::path("."); }
}

void EnsureHeaderUnlocked() {
    try {
        if (gPath.empty()) return;
        bool needsHeader = true;
        std::error_code ec;
        if (std::filesystem::exists(gPath, ec) && !ec) {
            needsHeader = std::filesystem::file_size(gPath, ec) == 0;
        }
        if (!needsHeader) return;
        std::ofstream out(gPath, std::ios::out | std::ios::app);
        if (!out) return;
        out << "Custom Vehicle Game crash log\n"
            << "Build: " << gBuildName << "\n"
            << "VEK runtime: 2.7.1\n"
            << "Crash reports are appended below. Keep this file when reporting a crash.\n"
            << "======================================================================\n";
        out.flush();
    } catch (...) {}
}

void AppendReportUnlocked(const std::string& kind, const std::string& message,
                          const std::string& extra = {}) {
    try {
        EnsureHeaderUnlocked();
        std::ofstream out(gPath, std::ios::out | std::ios::app);
        if (!out) return;
        out << "\n[CRASH] " << TimestampUtc() << "\n"
            << "Build: " << gBuildName << "\n"
            << "Stage: " << gStage << "\n"
            << "Type: " << kind << "\n"
            << "Message: " << (message.empty() ? "(no message)" : message) << "\n"
            << "Thread: " << std::this_thread::get_id() << "\n";
        if (!extra.empty()) out << extra;
        out << "----------------------------------------------------------------------\n";
        out.flush();
    } catch (...) {}
}

void TerminateHandler() noexcept {
    std::string message = "std::terminate called";
    try {
        if (auto ep = std::current_exception()) {
            try { std::rethrow_exception(ep); }
            catch (const std::exception& e) { message = e.what(); }
            catch (...) { message = "non-standard exception reached std::terminate"; }
        }
    } catch (...) {}
    try {
        std::unique_lock<std::mutex> lock(gMutex, std::try_to_lock);
        // The process is terminating anyway; if another thread crashed while
        // holding the logger mutex, prefer a possibly interleaved report over
        // deadlocking and losing the crash record completely.
        AppendReportUnlocked("C++ terminate", message);
    } catch (...) {}
    std::_Exit(EXIT_FAILURE);
}

#ifdef _WIN32
const char* SehName(DWORD code) {
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
        case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
        case EXCEPTION_DATATYPE_MISALIGNMENT: return "EXCEPTION_DATATYPE_MISALIGNMENT";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_ILLEGAL_INSTRUCTION: return "EXCEPTION_ILLEGAL_INSTRUCTION";
        case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
        case EXCEPTION_STACK_OVERFLOW: return "EXCEPTION_STACK_OVERFLOW";
        default: return "Windows structured exception";
    }
}

LONG WINAPI WindowsUnhandledExceptionFilter(EXCEPTION_POINTERS* info) {
    try {
        DWORD code = 0;
        void* address = nullptr;
        if (info && info->ExceptionRecord) {
            code = info->ExceptionRecord->ExceptionCode;
            address = info->ExceptionRecord->ExceptionAddress;
        }

        std::ostringstream extra;
        extra << "Exception code: 0x" << std::hex << std::uppercase
              << static_cast<unsigned long>(code) << std::dec << "\n"
              << "Exception address: " << address << "\n"
              << "OS thread id: " << GetCurrentThreadId() << "\n";

#if defined(_M_X64) || defined(__x86_64__)
        if (info && info->ContextRecord) {
            extra << "RIP: 0x" << std::hex << info->ContextRecord->Rip
                  << "  RSP: 0x" << info->ContextRecord->Rsp
                  << "  RBP: 0x" << info->ContextRecord->Rbp << std::dec << "\n";
        }
#elif defined(_M_IX86) || defined(__i386__)
        if (info && info->ContextRecord) {
            extra << "EIP: 0x" << std::hex << info->ContextRecord->Eip
                  << "  ESP: 0x" << info->ContextRecord->Esp
                  << "  EBP: 0x" << info->ContextRecord->Ebp << std::dec << "\n";
        }
#endif

        void* frames[32]{};
        const USHORT frameCount = CaptureStackBackTrace(0, 32, frames, nullptr);
        if (frameCount > 0) {
            extra << "Captured stack addresses (newest first):\n";
            for (USHORT i = 0; i < frameCount; ++i)
                extra << "  #" << i << " " << frames[i] << "\n";
        }

        std::unique_lock<std::mutex> lock(gMutex, std::try_to_lock);
        AppendReportUnlocked(SehName(code), "Unhandled Windows exception", extra.str());
    } catch (...) {}
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif
} // namespace

void Initialize(const std::string& buildName) {
    try {
        std::lock_guard<std::mutex> lock(gMutex);
        if (gInitialized) return;
        gBuildName = buildName.empty() ? "unknown" : buildName;
        gPath = ExecutableDirectory() / "crashlogs.txt";
        EnsureHeaderUnlocked();
        std::set_terminate(TerminateHandler);
#ifdef _WIN32
        SetUnhandledExceptionFilter(WindowsUnhandledExceptionFilter);
#endif
        gInitialized = true;
    } catch (...) {}
}

void SetStage(const std::string& stage) {
    try {
        std::lock_guard<std::mutex> lock(gMutex);
        gStage = stage.empty() ? "unknown" : stage;
    } catch (...) {}
}

void WriteFatal(const std::string& kind, const std::string& message) {
    try {
        std::lock_guard<std::mutex> lock(gMutex);
        AppendReportUnlocked(kind, message);
    } catch (...) {}
}

void WriteFatal(const char* kind, const char* message) {
    WriteFatal(kind ? std::string(kind) : std::string("fatal error"),
               message ? std::string(message) : std::string());
}

std::string Path() {
    try {
        std::lock_guard<std::mutex> lock(gMutex);
        return gPath.string();
    } catch (...) { return "crashlogs.txt"; }
}

} // namespace CrashLog
