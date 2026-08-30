# VEK host-language bindings

VEK has one canonical C++ runtime and a stable C ABI. Bindings call that runtime;
they do not fork or reimplement the VEK language.

Included foundations:

- C / C++ (native runtime and stable ABI)
- Python (`ctypes`)
- Rust (FFI)
- Node.js (N-API/native addon foundation)
- C# / .NET (P/Invoke)
- Go (cgo, added in 2.7)
- Java 22+ (Foreign Function & Memory API, added in 2.7)

VEK 2.7's C ABI also exposes structured diagnostics, debugger controls/trace,
and the crash-handler entry points so host languages can use the same error and
debugging model.
