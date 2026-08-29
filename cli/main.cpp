#include <vek/VekScriptEngine.h>
#include <vek/VekGameSystems.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#  define NOMINMAX
#  include <windows.h>
#  include <shellapi.h>
#elif defined(__APPLE__)
#  include <mach-o/dyld.h>
#else
#  include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {

fs::path executablePath(const char* argv0) {
#if defined(_WIN32)
    std::wstring buffer(32768, L'\0');
    DWORD n = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (n > 0 && n < buffer.size()) {
        buffer.resize(n);
        std::error_code ec;
        auto p = fs::weakly_canonical(fs::path(buffer), ec);
        return ec ? fs::path(buffer) : p;
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size + 1, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        std::error_code ec;
        auto p = fs::weakly_canonical(fs::path(buffer.data()), ec);
        return ec ? fs::path(buffer.data()) : p;
    }
#else
    std::array<char, 4096> buffer{};
    const auto n = ::readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (n > 0) {
        buffer[static_cast<std::size_t>(n)] = '\0';
        std::error_code ec;
        auto p = fs::weakly_canonical(fs::path(buffer.data()), ec);
        return ec ? fs::path(buffer.data()) : p;
    }
#endif
    std::error_code ec;
    auto p = fs::absolute(fs::path(argv0 ? argv0 : "vek"), ec);
    return ec ? fs::path(argv0 ? argv0 : "vek") : p;
}

bool looksLikeVekHome(const fs::path& p) {
    std::error_code ec;
    return fs::exists(p / "VERSION", ec) &&
           fs::exists(p / "include" / "vek" / "VekScriptEngine.h", ec);
}

fs::path discoverVekHome(const fs::path& exePath) {
    fs::path p = exePath.parent_path();
    for (int i = 0; i < 6 && !p.empty(); ++i) {
        if (looksLikeVekHome(p)) return p;
        auto parent = p.parent_path();
        if (parent == p) break;
        p = parent;
    }

    if (const char* env = std::getenv("VEK_HOME")) {
        fs::path candidate(env);
        if (looksLikeVekHome(candidate)) return candidate;
    }

    return exePath.parent_path();
}

std::string trim(std::string s) {
    auto notSpace = [](unsigned char c){ return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

std::string readVersionFile(const fs::path& home) {
    std::ifstream in(home / "VERSION", std::ios::binary);
    std::string s;
    if (in) std::getline(in, s);
    return trim(s);
}

std::string normalizedPathString(fs::path p) {
    std::error_code ec;
    p = fs::weakly_canonical(p, ec);
    std::string s = (ec ? p : p).string();
#if defined(_WIN32)
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
#endif
    while (!s.empty() && (s.back() == '/' || s.back() == '\\')) s.pop_back();
    return s;
}

bool pathEnvironmentContains(const fs::path& home) {
    const char* pathVar = std::getenv("PATH");
    if (!pathVar) return false;
#if defined(_WIN32)
    constexpr char sep = ';';
#else
    constexpr char sep = ':';
#endif
    const auto target = normalizedPathString(home);
    std::stringstream ss(pathVar);
    std::string part;
    while (std::getline(ss, part, sep)) {
        part = trim(part);
        if (part.size() >= 2 && part.front() == '"' && part.back() == '"') {
            part = part.substr(1, part.size() - 2);
        }
        if (!part.empty() && normalizedPathString(part) == target) return true;
    }
    return false;
}

static int runFile(const std::string& path) {
    VekScriptEngine vm;
    VekRegisterStandardLibrary(vm);
    vek::VekRegisterGameplayLibrary(vm);
    if (!vm.LoadFile(path)) { std::cerr << vm.LastError() << "\n"; return 2; }
    if (!vm.HasFunction("main")) { std::cerr << "VEK: program has no fn main()\n"; return 3; }
    VekValue result = vm.Call("main");
    if (!vm.LastError().empty()) { std::cerr << vm.LastError() << "\n"; return 4; }
    if (!result.IsNil()) std::cout << result.AsString() << "\n";
    return 0;
}

static int checkFile(const std::string& path) {
    VekScriptEngine vm;
    if (!vm.LoadFile(path)) { std::cerr << vm.LastError() << "\n"; return 2; }
    std::cout << "OK: " << path << "\n";
    return 0;
}

static int evalExpr(const std::string& expr) {
    VekScriptEngine vm;
    VekRegisterStandardLibrary(vm);
    vek::VekRegisterGameplayLibrary(vm);
    std::string src = "fn main(){ return " + expr + "; }";
    if (!vm.LoadSource(src, "<eval>")) { std::cerr << vm.LastError() << "\n"; return 2; }
    auto v = vm.Call("main");
    if (!vm.LastError().empty()) { std::cerr << vm.LastError() << "\n"; return 3; }
    std::cout << v.AsString() << "\n";
    return 0;
}

static int repl() {
    std::cout << "VEK " << VEK_VERSION_STRING << " REPL. :quit to exit. Expressions only.\n";
    std::string line;
    while (true) {
        std::cout << "vek> ";
        if (!std::getline(std::cin, line) || line == ":quit" || line == ":q") break;
        if (line.empty()) continue;
        evalExpr(line);
    }
    return 0;
}

// Small self-contained SHA-256 implementation used only for portable-package integrity checks.
class Sha256 {
public:
    Sha256() { reset(); }

    void update(const std::uint8_t* data, std::size_t len) {
        for (std::size_t i = 0; i < len; ++i) {
            data_[datalen_++] = data[i];
            if (datalen_ == 64) {
                transform();
                bitlen_ += 512;
                datalen_ = 0;
            }
        }
    }

    std::array<std::uint8_t, 32> final() {
        std::size_t i = datalen_;
        data_[i++] = 0x80;
        if (i > 56) {
            while (i < 64) data_[i++] = 0;
            transform();
            i = 0;
        }
        while (i < 56) data_[i++] = 0;
        bitlen_ += static_cast<std::uint64_t>(datalen_) * 8;
        for (int j = 7; j >= 0; --j) data_[56 + (7 - j)] = static_cast<std::uint8_t>((bitlen_ >> (j * 8)) & 0xff);
        transform();

        std::array<std::uint8_t, 32> hash{};
        for (i = 0; i < 4; ++i) {
            for (std::size_t j = 0; j < 8; ++j) {
                hash[i + j * 4] = static_cast<std::uint8_t>((state_[j] >> (24 - i * 8)) & 0xff);
            }
        }
        return hash;
    }

private:
    std::array<std::uint8_t, 64> data_{};
    std::array<std::uint32_t, 8> state_{};
    std::size_t datalen_ = 0;
    std::uint64_t bitlen_ = 0;

    static constexpr std::array<std::uint32_t, 64> k_ = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };

    static std::uint32_t rotr(std::uint32_t x, std::uint32_t n) { return (x >> n) | (x << (32 - n)); }
    static std::uint32_t ch(std::uint32_t x, std::uint32_t y, std::uint32_t z) { return (x & y) ^ (~x & z); }
    static std::uint32_t maj(std::uint32_t x, std::uint32_t y, std::uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
    static std::uint32_t ep0(std::uint32_t x) { return rotr(x,2) ^ rotr(x,13) ^ rotr(x,22); }
    static std::uint32_t ep1(std::uint32_t x) { return rotr(x,6) ^ rotr(x,11) ^ rotr(x,25); }
    static std::uint32_t sig0(std::uint32_t x) { return rotr(x,7) ^ rotr(x,18) ^ (x >> 3); }
    static std::uint32_t sig1(std::uint32_t x) { return rotr(x,17) ^ rotr(x,19) ^ (x >> 10); }

    void reset() {
        datalen_ = 0;
        bitlen_ = 0;
        state_ = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    }

    void transform() {
        std::array<std::uint32_t, 64> m{};
        for (std::size_t i = 0, j = 0; i < 16; ++i, j += 4) {
            m[i] = (static_cast<std::uint32_t>(data_[j]) << 24) |
                   (static_cast<std::uint32_t>(data_[j + 1]) << 16) |
                   (static_cast<std::uint32_t>(data_[j + 2]) << 8) |
                   static_cast<std::uint32_t>(data_[j + 3]);
        }
        for (std::size_t i = 16; i < 64; ++i) m[i] = sig1(m[i-2]) + m[i-7] + sig0(m[i-15]) + m[i-16];

        auto a=state_[0], b=state_[1], c=state_[2], d=state_[3], e=state_[4], f=state_[5], g=state_[6], h=state_[7];
        for (std::size_t i = 0; i < 64; ++i) {
            const auto t1 = h + ep1(e) + ch(e,f,g) + k_[i] + m[i];
            const auto t2 = ep0(a) + maj(a,b,c);
            h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        state_[0]+=a; state_[1]+=b; state_[2]+=c; state_[3]+=d;
        state_[4]+=e; state_[5]+=f; state_[6]+=g; state_[7]+=h;
    }
};

std::string sha256File(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    Sha256 sha;
    std::array<char, 64 * 1024> buffer{};
    while (in) {
        in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto n = in.gcount();
        if (n > 0) sha.update(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::size_t>(n));
    }
    const auto hash = sha.final();
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (auto b : hash) out << std::setw(2) << static_cast<unsigned>(b);
    return out.str();
}

bool isSafeManifestRelativePath(const fs::path& p) {
    if (p.empty() || p.is_absolute() || p.has_root_name() || p.has_root_directory()) return false;
    for (const auto& part : p) if (part == "..") return false;
    return true;
}

int verifyPortable(const fs::path& home) {
    const fs::path manifest = home / "manifest.sha256";
    std::ifstream in(manifest, std::ios::binary);
    if (!in) {
        std::cerr << "VEK verify: manifest.sha256 was not found in " << home.string() << "\n";
        std::cerr << "This command verifies an official portable package after the release workflow creates its manifest.\n";
        return 2;
    }

    std::error_code ec;
    const auto canonicalHome = fs::weakly_canonical(home, ec);
    if (ec) { std::cerr << "VEK verify: cannot resolve VEK home.\n"; return 2; }

    std::size_t checked = 0, failed = 0;
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line.size() < 67) { ++failed; std::cerr << "[FAIL] malformed manifest line\n"; continue; }
        const std::string expected = line.substr(0, 64);
        std::string relText = trim(line.substr(64));
        while (!relText.empty() && (relText.front() == '*' || relText.front() == ' ')) relText.erase(relText.begin());
        fs::path rel = fs::path(relText);
        if (!isSafeManifestRelativePath(rel)) { ++failed; std::cerr << "[FAIL] unsafe manifest path: " << relText << "\n"; continue; }
        const fs::path target = home / rel;
        const auto canonicalTarget = fs::weakly_canonical(target, ec);
        if (ec) { ++failed; std::cerr << "[FAIL] missing/invalid: " << relText << "\n"; continue; }
        const auto insideRel = fs::relative(canonicalTarget, canonicalHome, ec);
        if (ec || !isSafeManifestRelativePath(insideRel) || !fs::is_regular_file(canonicalTarget, ec)) {
            ++failed; std::cerr << "[FAIL] missing/invalid: " << relText << "\n"; continue;
        }
        if (expected.size() != 64 || !std::all_of(expected.begin(), expected.end(), [](unsigned char c){ return std::isxdigit(c) != 0; })) {
            ++failed; std::cerr << "[FAIL] invalid SHA-256 in manifest for: " << relText << "\n"; continue;
        }
        auto normalizedExpected = expected;
        std::transform(normalizedExpected.begin(), normalizedExpected.end(), normalizedExpected.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        const auto actual = sha256File(canonicalTarget);
        ++checked;
        if (actual != normalizedExpected) { ++failed; std::cerr << "[FAIL] " << relText << "\n"; }
        else std::cout << "[OK] " << relText << "\n";
    }

    std::cout << "Checked " << checked << " files; failures: " << failed << "\n";
    if (failed == 0) {
        std::cout << "VEK portable files match the local manifest.\n";
        std::cout << "Note: this detects file changes/corruption; release authenticity still depends on obtaining the manifest from a trusted release.\n";
        return 0;
    }
    return 3;
}

void printInfo(const fs::path& exe, const fs::path& home) {
    std::cout << "VEK Programming Language\n";
    std::cout << "Version: " << VEK_VERSION_STRING << "\n";
#if defined(_WIN32)
    std::cout << "Platform: windows";
#elif defined(__APPLE__)
    std::cout << "Platform: macos";
#else
    std::cout << "Platform: linux";
#endif
#if defined(_M_X64) || defined(__x86_64__)
    std::cout << "-x86_64\n";
#elif defined(_M_ARM64) || defined(__aarch64__)
    std::cout << "-arm64\n";
#else
    std::cout << "\n";
#endif
    std::cout << "Executable: " << exe.string() << "\n";
    std::cout << "VEK_HOME: " << home.string() << "\n";
}


void printInstallIntro() {
    std::cout
        << "\n"
        << " __     __  ______  _  __\n"
        << " \\ \\   / / |  ____|| |/ /\n"
        << "  \\ \\ / /  | |__   | ' / \n"
        << "   \\ V /   |  __|  |  <  \n"
        << "    \\_/    |______||_|\\_\\\n"
        << "\n"
        << "        V E K\n\n";
#if defined(_WIN32)
    std::cout << "1" << std::flush;
    Sleep(220);
    std::cout << " - 2" << std::flush;
    Sleep(220);
    std::cout << " - 3\n" << std::flush;
    Sleep(220);
#else
    std::cout << "1 - 2 - 3\n";
#endif
    std::cout << "Installing VEK...\n" << std::flush;
}

int launchGuiInstaller(const fs::path& home) {
    printInstallIntro();
#if defined(_WIN32)
    const fs::path installer = home / "VekInstaller.exe";
    std::error_code ec;
    if (!fs::is_regular_file(installer, ec)) {
        std::cerr << "VEK install: VekInstaller.exe was not found in " << home.string() << "\n";
        std::cerr << "Download/extract an official VEK 2.2+ Windows portable package, then try again.\n";
        return 2;
    }
    const HINSTANCE result = ShellExecuteW(nullptr, L"open", installer.c_str(), nullptr, home.c_str(), SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        std::cerr << "VEK install: Windows could not open the graphical installer (ShellExecute error "
                  << reinterpret_cast<INT_PTR>(result) << ").\n";
        return 3;
    }
    std::cout << "VEK graphical installer opened. Continue in the Windows installer window.\n";
    return 0;
#else
    (void)home;
    std::cerr << "VEK --install currently provides a native graphical installer on Windows only.\n";
    return 2;
#endif
}

int doctor(const fs::path& exe, const fs::path& home) {
    bool criticalOk = true;
    auto ok = [](const std::string& msg){ std::cout << "[OK] " << msg << "\n"; };
    auto fail = [&](const std::string& msg){ std::cout << "[FAIL] " << msg << "\n"; criticalOk = false; };
    auto warn = [](const std::string& msg){ std::cout << "[WARN] " << msg << "\n"; };

    std::error_code ec;
    fs::is_regular_file(exe, ec) ? ok("VEK executable") : fail("VEK executable could not be resolved");
    looksLikeVekHome(home) ? ok("portable VEK home") : fail("VEK home is missing VERSION/include files");
    const auto diskVersion = readVersionFile(home);
    if (diskVersion == VEK_VERSION_STRING) ok("VERSION matches runtime: " + diskVersion);
    else fail("VERSION mismatch: runtime=" + std::string(VEK_VERSION_STRING) + " disk=" + (diskVersion.empty() ? "<missing>" : diskVersion));
    fs::exists(home / "examples", ec) ? ok("examples directory") : warn("examples directory not present");
    fs::exists(home / "LICENSE", ec) ? ok("license file") : warn("LICENSE not present");
    pathEnvironmentContains(home) ? ok("VEK home is on PATH") : warn("VEK home is not on PATH; run vek --install, VekInstaller.exe, or INSTALL_PATH.cmd on Windows");
    fs::exists(home / "manifest.sha256", ec) ? ok("portable integrity manifest") : warn("manifest.sha256 not present (normal in source/development builds)");

    if (const char* env = std::getenv("VEK_HOME")) {
        std::cout << "[INFO] VEK_HOME environment override: " << env << "\n";
    } else {
        std::cout << "[INFO] VEK_HOME environment variable is not required.\n";
    }

    std::cout << (criticalOk ? "VEK installation is healthy.\n" : "VEK installation has critical problems.\n");
    return criticalOk ? 0 : 2;
}

void usage() {
    std::cout
        << "VEK " << VEK_VERSION_STRING << "\n"
        << "Usage:\n"
        << "  vek --version              Show version\n"
        << "  vek --install              Open the Windows graphical installer\n"
        << "  vek info                   Show portable install information\n"
        << "  vek home                   Print detected VEK home\n"
        << "  vek doctor                 Check installation/PATH\n"
        << "  vek verify                 Verify portable package files\n"
        << "  vek run <file.vek>         Run a VEK program\n"
        << "  vek check <file.vek>       Parse/check a VEK program\n"
        << "  vek eval <expression>      Evaluate an expression\n"
        << "  vek repl                   Start the REPL\n";
}

} // namespace

int main(int argc, char** argv) {
    const fs::path exe = executablePath(argc > 0 ? argv[0] : "vek");
    const fs::path home = discoverVekHome(exe);

    if (argc < 2) { usage(); return 0; }
    const std::string cmd = argv[1];
    if (cmd == "version" || cmd == "--version" || cmd == "-v") { std::cout << "VEK " << VEK_VERSION_STRING << "\n"; return 0; }
    if (cmd == "install" || cmd == "--install") return launchGuiInstaller(home);
    if (cmd == "info") { printInfo(exe, home); return 0; }
    if (cmd == "home") { std::cout << home.string() << "\n"; return 0; }
    if (cmd == "doctor") return doctor(exe, home);
    if (cmd == "verify") return verifyPortable(home);
    if (cmd == "repl") return repl();
    if (cmd == "run" && argc >= 3) return runFile(argv[2]);
    if (cmd == "check" && argc >= 3) return checkFile(argv[2]);
    if (cmd == "eval" && argc >= 3) return evalExpr(argv[2]);
    if (cmd == "help" || cmd == "--help" || cmd == "-h") { usage(); return 0; }
    std::cerr << "Invalid VEK command. Run 'vek --help'.\n";
    return 1;
}
