# VEK 2.7 Diagnostics, Debugger and Crash Handler

## Structured diagnostics

Every VM load/runtime failure can be represented as `vek::DiagnosticRecord`.
Records include a stable code, severity/domain, source, line, current function,
VEK call stack and UTC timestamp. Use `VekScriptEngine::LastDiagnostic()` for
the latest failure or `Diagnostics()` for the bounded history.

```cpp
VekScriptEngine vm;
vm.LoadFile("game.vek");
if (!vm.LastError().empty()) {
    std::cerr << vek::FormatDiagnostic(vm.LastDiagnostic(), true) << "\n";
}
```

The diagnostic domains separate lexer/parser/runtime/security/native/host/
debugger/physics/GPU/gameplay/GUI concerns so tooling does not have to parse
free-form console text.

## Debugger

```cpp
vek::VekDebugger debugger;
debugger.Attach(true);
debugger.AddFunctionBreakpoint("spawn_vehicle");
debugger.AddLineBreakpoint("game.vek", 42);
vm.SetDebugger(&debugger);

vm.Call("main");
if (debugger.Paused()) {
    // inspect debugger.TraceSnapshot(), then resume
    debugger.Continue();
}
```

The current interpreter is restart-at-call-boundary: a paused top-level `Call`
returns control to the host. `Continue()` suppresses the exact breakpoint once
when the host invokes the call again. This keeps the debugger deterministic and
does not park a native thread inside the VM.

## Crash reports

```cpp
auto& crash = vek::VekCrashHandler::Instance();
crash.SetContext({"MyGame", "1.4.0", "release", "startup"});
crash.Install("vek_crashlogs.txt");
crash.SetStage("loading world");
```

Reports are append-only and contain product/build/stage information and the last
VEK diagnostic. On Windows, unhandled SEH exceptions also record the platform
exception code and fault address.

Crash logging is diagnostic infrastructure, not a substitute for OS minidumps,
symbol servers or a native debugger. Hosts can use both.

## CLI

```text
vek diagnose game.vek
vek trace game.vek
```

`diagnose` prints structured parser/runtime failures. `trace` runs `main()` with
the VEK debugger attached and prints the bounded VM execution trace.
