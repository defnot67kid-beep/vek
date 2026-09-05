#pragma once
// VekRemoteSystems — minimal remote (TCP) + local connection layer (VEK 3.7).
//
// This is a small, dependency-free request/response protocol: a client sends
// one line "COMMAND {json-args}\n", the server dispatches to a registered
// handler and replies with one line "{json-result}\n". It works the same way
// over a real network socket or over 127.0.0.1 ("local connection"), which is
// the point — one implementation, two deployment modes. A worked example
// (AltitudeService) is included: a host can serve its current altitude and a
// client/GUI can poll it and render it live.
//
// This module intentionally does not depend on any other VEK subsystem
// beyond VekScriptEngine's VekValue, so it can be dropped into non-game hosts
// too.

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <vek/VekScriptEngine.h>

namespace vek::remote {

// Parses a compact JSON document into a VekValue (numbers, strings, bools,
// null, arrays, objects). Not a general-purpose validator — malformed input
// yields VekValue() (nil) rather than throwing, so a bad remote payload can
// never crash the caller.
VekValue ParseJson(const std::string& text);

using RemoteHandler = std::function<VekValue(const VekMap& args)>;

// A TCP (or loopback/"local") server. One handler dispatch table, one
// connection handled at a time per accepted socket, each on its own thread.
class RemoteServer {
public:
    RemoteServer();
    ~RemoteServer();

    // host: "0.0.0.0" for all interfaces, "127.0.0.1" for local-only.
    bool Start(const std::string& host, std::uint16_t port, std::string* error = nullptr);
    void Stop();
    bool Running() const { return running_.load(); }

    void RegisterHandler(const std::string& command, RemoteHandler handler);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::atomic<bool> running_{false};
};

// A blocking TCP (or loopback) client for the same protocol.
class RemoteClient {
public:
    RemoteClient();
    ~RemoteClient();

    bool Connect(const std::string& host, std::uint16_t port, std::string* error = nullptr);
    void Disconnect();
    bool Connected() const;

    // Sends `command` with `args`, blocks for one line of response, and
    // returns the parsed VekValue (nil on any I/O or parse failure; check
    // `error` for details when non-null).
    VekValue Request(const std::string& command, const VekMap& args = {}, std::string* error = nullptr);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// --- Worked example: a live altitude feed -----------------------------------
// A tiny thread-safe value the host updates locally (e.g. from
// mechanics::RocketAscentState.position.y) and exposes to remote/local
// clients via one handler, "get_altitude".
class AltitudeService {
public:
    void SetAltitudeMeters(double altitude) { altitude_.store(altitude); }
    double AltitudeMeters() const { return altitude_.load(); }

    // Registers "get_altitude" (and "set_altitude" for test/debug hosts) on
    // the given server.
    void Attach(RemoteServer& server);

private:
    std::atomic<double> altitude_{0.0};
};

// Convenience for clients/GUIs: requests "get_altitude" and extracts the
// numeric "altitude" field, or `fallback` on any failure.
double RequestAltitudeMeters(RemoteClient& client, double fallback = 0.0);

} // namespace vek::remote
