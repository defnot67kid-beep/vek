package dev.vek;

import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.foreign.ValueLayout;
import java.lang.invoke.MethodHandle;
import java.nio.file.Path;

/** Java 22+ Foreign Function & Memory binding for the stable VEK C ABI. */
public final class VekRuntime implements AutoCloseable {
    private final Arena arena = Arena.ofShared();
    private final Linker linker = Linker.nativeLinker();
    private final SymbolLookup symbols;
    private MemorySegment runtime;

    private final MethodHandle create, destroy, loadFile, lastError, lastDiagnostic,
            debuggerAttach, addBreakpoint, debuggerContinue, debuggerTrace, installCrashHandler;

    public VekRuntime(Path vekLibrary) throws Throwable {
        symbols = SymbolLookup.libraryLookup(vekLibrary, arena);
        create = down("vek_create", FunctionDescriptor.of(ValueLayout.ADDRESS));
        destroy = down("vek_destroy", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
        loadFile = down("vek_load_file", FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        lastError = down("vek_last_error", FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        lastDiagnostic = down("vek_last_diagnostic_text", FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        debuggerAttach = down("vek_debugger_attach", FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS, ValueLayout.JAVA_INT));
        addBreakpoint = down("vek_debugger_add_function_breakpoint", FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        debuggerContinue = down("vek_debugger_continue", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
        debuggerTrace = down("vek_debugger_trace_json", FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        installCrashHandler = down("vek_install_crash_handler", FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS));
        runtime = (MemorySegment) create.invokeExact();
        if (runtime.equals(MemorySegment.NULL)) throw new IllegalStateException("VEK runtime creation failed");
    }

    private MethodHandle down(String name, FunctionDescriptor descriptor) {
        return linker.downcallHandle(symbols.find(name).orElseThrow(() -> new IllegalStateException("Missing VEK symbol: " + name)), descriptor);
    }
    private MemorySegment cString(String value) { return arena.allocateFrom(value == null ? "" : value); }
    private String readString(MethodHandle fn) throws Throwable {
        MemorySegment p=(MemorySegment)fn.invokeExact(runtime);
        return p.equals(MemorySegment.NULL)?"":p.reinterpret(Long.MAX_VALUE).getString(0);
    }

    public void loadFile(String path) throws Throwable {
        int ok=(int)loadFile.invokeExact(runtime,cString(path));
        if(ok==0)throw new IllegalStateException(lastError());
    }
    public String lastError() throws Throwable { return readString(lastError); }
    public String lastDiagnostic() throws Throwable { return readString(lastDiagnostic); }
    public void attachDebugger(boolean enabled) throws Throwable { debuggerAttach.invokeExact(runtime,enabled?1:0); }
    public void addBreakpoint(String function) throws Throwable { addBreakpoint.invokeExact(runtime,cString(function)); }
    public void continueExecution() throws Throwable { debuggerContinue.invokeExact(runtime); }
    public String debuggerTraceJson() throws Throwable { return readString(debuggerTrace); }
    public boolean installCrashHandler(String path,String product,String build) throws Throwable {
        return (int)installCrashHandler.invokeExact(runtime,cString(path),cString(product),cString(build))!=0;
    }
    @Override public void close() {
        if(!runtime.equals(MemorySegment.NULL)){
            try{destroy.invokeExact(runtime);}catch(Throwable ignored){}
            runtime=MemorySegment.NULL;
        }
        arena.close();
    }
}
