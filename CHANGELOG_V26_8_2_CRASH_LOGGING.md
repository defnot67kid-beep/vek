# v26.8.2 - Crash Logging Hotfix

- Adds `crashlogs.txt` beside `CustomVehicleGame.exe` automatically.
- Adds a top-level C++ exception boundary around game construction, Init, Run,
  and Shutdown.
- Installs a `std::terminate` handler so fatal C++ termination is logged.
- On Windows, installs an unhandled structured-exception filter so access
  violations, illegal instructions, divide-by-zero and similar hard crashes
  can be written to the log.
- Reports include UTC timestamp, game build, VEK runtime version, last known
  game stage, exception type/message, thread identifiers, and available
  Windows instruction/stack addresses.
- Logging failures are swallowed so the crash reporter does not become a new
  source of crashes.
- Does not modify signed `.vek` gameplay scripts.
