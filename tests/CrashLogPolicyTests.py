from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
impl = (ROOT / "src" / "CrashLog.cpp").read_text(encoding="utf-8")
cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

assert 'CrashLog::Initialize("CustomVehicleGame v26.8.6 - VEK Part Icons + Viewmodels")' in main
assert 'CrashLog::SetStage("Game::Run / gameplay loop")' in main
assert 'CrashLog::WriteFatal("Unhandled C++ exception"' in main
assert '"crashlogs.txt"' in impl
assert 'std::set_terminate(TerminateHandler)' in impl
assert 'SetUnhandledExceptionFilter(WindowsUnhandledExceptionFilter)' in impl
assert 'EXCEPTION_ACCESS_VIOLATION' in impl
assert 'CaptureStackBackTrace' in impl
assert 'src/CrashLog.cpp' in cmake
print("CrashLogPolicyTests: PASS")
