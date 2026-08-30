#pragma once

#include <string>

namespace CrashLog {

// Creates crashlogs.txt beside the game executable (if it does not already
// exist) and installs process-level crash/terminate handlers.
void Initialize(const std::string& buildName);

// Records the high-level area the game is currently executing. The value is
// included in a crash report to make reports useful even without a debugger.
void SetStage(const std::string& stage);

// Appends a fatal report. These functions never intentionally throw.
void WriteFatal(const std::string& kind, const std::string& message);
void WriteFatal(const char* kind, const char* message);

// Full path used by this process.
std::string Path();

} // namespace CrashLog
