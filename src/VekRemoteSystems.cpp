#include <vek/VekRemoteSystems.h>

#include <cctype>
#include <cstring>
#include <thread>
#include <vector>

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  using SocketHandle = SOCKET;
  static constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <sys/select.h>
  #include <sys/socket.h>
  #include <unistd.h>
  using SocketHandle = int;
  static constexpr SocketHandle kInvalidSocket = -1;
#endif

namespace vek::remote {

namespace {

void CloseSocket(SocketHandle s) {
#if defined(_WIN32)
    closesocket(s);
#else
    close(s);
#endif
}

bool SendAll(SocketHandle s, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
#if defined(_WIN32)
        int n = send(s, data.data() + sent, static_cast<int>(data.size() - sent), 0);
#else
        ssize_t n = send(s, data.data() + sent, data.size() - sent, 0);
#endif
        if (n <= 0) return false;
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

// Reads bytes until '\n' (exclusive) or EOF/error. Returns false on
// immediate error with nothing read.
bool ReadLine(SocketHandle s, std::string& out) {
    out.clear();
    char buf[1];
    for (;;) {
#if defined(_WIN32)
        int n = recv(s, buf, 1, 0);
#else
        ssize_t n = recv(s, buf, 1, 0);
#endif
        if (n <= 0) return !out.empty();
        if (buf[0] == '\n') return true;
        out.push_back(buf[0]);
        if (out.size() > (1u << 20)) return false; // 1MB line safety cap
    }
}

struct WinsockGuard {
    WinsockGuard() {
#if defined(_WIN32)
        WSADATA data;
        WSAStartup(MAKEWORD(2, 2), &data);
#endif
    }
    ~WinsockGuard() {
#if defined(_WIN32)
        WSACleanup();
#endif
    }
};
WinsockGuard& EnsureWinsock() {
    static WinsockGuard guard;
    return guard;
}

// --- tiny JSON parser --------------------------------------------------------
void SkipWs(const std::string& s, std::size_t& i) {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
}

VekValue ParseValue(const std::string& s, std::size_t& i);

VekValue ParseString(const std::string& s, std::size_t& i) {
    std::string out;
    ++i; // opening quote
    while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            switch (s[i]) {
                case 'n': out.push_back('\n'); break;
                case 't': out.push_back('\t'); break;
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                default: out.push_back(s[i]); break;
            }
        } else {
            out.push_back(s[i]);
        }
        ++i;
    }
    if (i < s.size()) ++i; // closing quote
    return VekValue(out);
}

VekValue ParseNumber(const std::string& s, std::size_t& i) {
    std::size_t start = i;
    if (i < s.size() && (s[i] == '-' || s[i] == '+')) ++i;
    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.' || s[i] == 'e' || s[i] == 'E' || s[i] == '+' || s[i] == '-')) ++i;
    try {
        return VekValue(std::stod(s.substr(start, i - start)));
    } catch (...) {
        return VekValue(0.0);
    }
}

VekValue ParseArray(const std::string& s, std::size_t& i) {
    VekArray arr;
    ++i; // '['
    SkipWs(s, i);
    if (i < s.size() && s[i] == ']') { ++i; return VekValue(std::move(arr)); }
    while (i < s.size()) {
        SkipWs(s, i);
        arr.push_back(ParseValue(s, i));
        SkipWs(s, i);
        if (i < s.size() && s[i] == ',') { ++i; continue; }
        if (i < s.size() && s[i] == ']') { ++i; break; }
        break;
    }
    return VekValue(std::move(arr));
}

VekValue ParseObject(const std::string& s, std::size_t& i) {
    VekMap map;
    ++i; // '{'
    SkipWs(s, i);
    if (i < s.size() && s[i] == '}') { ++i; return VekValue(std::move(map)); }
    while (i < s.size()) {
        SkipWs(s, i);
        if (i >= s.size() || s[i] != '"') break;
        VekValue key = ParseString(s, i);
        SkipWs(s, i);
        if (i < s.size() && s[i] == ':') ++i;
        SkipWs(s, i);
        VekValue value = ParseValue(s, i);
        map[key.AsString()] = value;
        SkipWs(s, i);
        if (i < s.size() && s[i] == ',') { ++i; continue; }
        if (i < s.size() && s[i] == '}') { ++i; break; }
        break;
    }
    return VekValue(std::move(map));
}

VekValue ParseValue(const std::string& s, std::size_t& i) {
    SkipWs(s, i);
    if (i >= s.size()) return VekValue();
    char c = s[i];
    if (c == '"') return ParseString(s, i);
    if (c == '{') return ParseObject(s, i);
    if (c == '[') return ParseArray(s, i);
    if (c == 't' && s.compare(i, 4, "true") == 0) { i += 4; return VekValue(true); }
    if (c == 'f' && s.compare(i, 5, "false") == 0) { i += 5; return VekValue(false); }
    if (c == 'n' && s.compare(i, 4, "null") == 0) { i += 4; return VekValue(); }
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return ParseNumber(s, i);
    return VekValue();
}

} // namespace

VekValue ParseJson(const std::string& text) {
    std::size_t i = 0;
    return ParseValue(text, i);
}

// --- RemoteServer -------------------------------------------------------------
struct RemoteServer::Impl {
    SocketHandle listenSocket = kInvalidSocket;
    std::thread acceptThread;
    std::mutex handlersMutex;
    std::unordered_map<std::string, RemoteHandler> handlers;
    std::atomic<bool>* runningFlag = nullptr;

    void HandleConnection(SocketHandle client) {
        std::string line;
        if (ReadLine(client, line)) {
            std::size_t space = line.find(' ');
            std::string command = (space == std::string::npos) ? line : line.substr(0, space);
            std::string payload = (space == std::string::npos) ? "{}" : line.substr(space + 1);
            VekValue argsValue = ParseJson(payload);
            VekMap args = argsValue.IsMap() ? *argsValue.AsMap() : VekMap{};

            RemoteHandler handler;
            {
                std::lock_guard<std::mutex> lock(handlersMutex);
                auto it = handlers.find(command);
                if (it != handlers.end()) handler = it->second;
            }

            VekValue result;
            if (handler) {
                try {
                    result = handler(args);
                } catch (const std::exception& e) {
                    VekMap err;
                    err["error"] = std::string(e.what());
                    result = VekValue(std::move(err));
                }
            } else {
                VekMap err;
                err["error"] = std::string("unknown command: ") + command;
                result = VekValue(std::move(err));
            }
            SendAll(client, result.ToJson() + "\n");
        }
        CloseSocket(client);
    }

    void AcceptLoop() {
        while (runningFlag && runningFlag->load()) {
            // Poll with a short timeout instead of blocking forever in accept():
            // some sandboxed/virtualized network stacks don't reliably wake a
            // blocked accept() when the listening socket is closed from
            // another thread, which would otherwise hang Stop() forever.
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(listenSocket, &readSet);
            timeval timeout{0, 200000}; // 200ms
#if defined(_WIN32)
            int ready = select(0, &readSet, nullptr, nullptr, &timeout);
#else
            int ready = select(listenSocket + 1, &readSet, nullptr, nullptr, &timeout);
#endif
            if (ready <= 0) continue; // timeout or interrupted; re-check runningFlag
            if (!runningFlag->load()) break;

            sockaddr_in clientAddr{};
#if defined(_WIN32)
            int addrLen = sizeof(clientAddr);
#else
            socklen_t addrLen = sizeof(clientAddr);
#endif
            SocketHandle client = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
            if (client == kInvalidSocket) continue;
            std::thread(&Impl::HandleConnection, this, client).detach();
        }
    }
};

RemoteServer::RemoteServer() : impl_(std::make_unique<Impl>()) {}
RemoteServer::~RemoteServer() { Stop(); }

bool RemoteServer::Start(const std::string& host, std::uint16_t port, std::string* error) {
    EnsureWinsock();
    impl_->listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (impl_->listenSocket == kInvalidSocket) {
        if (error) *error = "failed to create socket";
        return false;
    }

    int reuse = 1;
#if defined(_WIN32)
    setsockopt(impl_->listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
#else
    setsockopt(impl_->listenSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = (host.empty() || host == "0.0.0.0") ? INADDR_ANY : inet_addr(host.c_str());

    if (bind(impl_->listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        if (error) *error = "failed to bind to " + host + ":" + std::to_string(port);
        CloseSocket(impl_->listenSocket);
        impl_->listenSocket = kInvalidSocket;
        return false;
    }
    if (listen(impl_->listenSocket, 16) != 0) {
        if (error) *error = "failed to listen";
        CloseSocket(impl_->listenSocket);
        impl_->listenSocket = kInvalidSocket;
        return false;
    }

    running_.store(true);
    impl_->runningFlag = &running_;
    impl_->acceptThread = std::thread(&Impl::AcceptLoop, impl_.get());
    return true;
}

void RemoteServer::Stop() {
    if (!running_.load()) return;
    running_.store(false);
    if (impl_->listenSocket != kInvalidSocket) {
#if !defined(_WIN32)
        shutdown(impl_->listenSocket, SHUT_RDWR);
#endif
        CloseSocket(impl_->listenSocket);
        impl_->listenSocket = kInvalidSocket;
    }
    if (impl_->acceptThread.joinable()) impl_->acceptThread.join();
}

void RemoteServer::RegisterHandler(const std::string& command, RemoteHandler handler) {
    std::lock_guard<std::mutex> lock(impl_->handlersMutex);
    impl_->handlers[command] = std::move(handler);
}

// --- RemoteClient ---------------------------------------------------------
struct RemoteClient::Impl {
    SocketHandle socketHandle = kInvalidSocket;
};

RemoteClient::RemoteClient() : impl_(std::make_unique<Impl>()) {}
RemoteClient::~RemoteClient() { Disconnect(); }

bool RemoteClient::Connect(const std::string& host, std::uint16_t port, std::string* error) {
    EnsureWinsock();
    Disconnect();
    impl_->socketHandle = socket(AF_INET, SOCK_STREAM, 0);
    if (impl_->socketHandle == kInvalidSocket) {
        if (error) *error = "failed to create socket";
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // Accept both dotted IPs and hostnames (falls back to getaddrinfo).
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* resolved = nullptr;
        if (getaddrinfo(host.c_str(), nullptr, &hints, &resolved) != 0 || !resolved) {
            if (error) *error = "failed to resolve host: " + host;
            CloseSocket(impl_->socketHandle);
            impl_->socketHandle = kInvalidSocket;
            return false;
        }
        addr.sin_addr = reinterpret_cast<sockaddr_in*>(resolved->ai_addr)->sin_addr;
        freeaddrinfo(resolved);
    }

    if (connect(impl_->socketHandle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        if (error) *error = "failed to connect to " + host + ":" + std::to_string(port);
        CloseSocket(impl_->socketHandle);
        impl_->socketHandle = kInvalidSocket;
        return false;
    }
    return true;
}

void RemoteClient::Disconnect() {
    if (impl_->socketHandle != kInvalidSocket) {
        CloseSocket(impl_->socketHandle);
        impl_->socketHandle = kInvalidSocket;
    }
}

bool RemoteClient::Connected() const { return impl_->socketHandle != kInvalidSocket; }

VekValue RemoteClient::Request(const std::string& command, const VekMap& args, std::string* error) {
    if (!Connected()) {
        if (error) *error = "not connected";
        return VekValue();
    }
    VekValue argsValue{args};
    std::string line = command + " " + argsValue.ToJson() + "\n";
    if (!SendAll(impl_->socketHandle, line)) {
        if (error) *error = "send failed";
        return VekValue();
    }
    std::string response;
    if (!ReadLine(impl_->socketHandle, response)) {
        if (error) *error = "no response";
        return VekValue();
    }
    return ParseJson(response);
}

// --- AltitudeService --------------------------------------------------------
void AltitudeService::Attach(RemoteServer& server) {
    server.RegisterHandler("get_altitude", [this](const VekMap&) {
        VekMap out;
        out["altitude"] = AltitudeMeters();
        return VekValue(std::move(out));
    });
    server.RegisterHandler("set_altitude", [this](const VekMap& args) {
        auto it = args.find("altitude");
        if (it != args.end()) SetAltitudeMeters(it->second.AsNumber(0.0));
        VekMap out;
        out["ok"] = true;
        out["altitude"] = AltitudeMeters();
        return VekValue(std::move(out));
    });
}

double RequestAltitudeMeters(RemoteClient& client, double fallback) {
    VekValue result = client.Request("get_altitude", {});
    if (!result.IsMap()) return fallback;
    VekValue altitude = result.Get("altitude");
    return altitude.IsNumber() ? altitude.AsNumber(fallback) : fallback;
}

} // namespace vek::remote
