#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <windowsx.h>
#include <shobjidl.h>

#include <vek/VekGameSystems.h>
#include <vek/VekScriptEngine.h>

#include <algorithm>
#include <atomic>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

namespace fs = std::filesystem;

namespace {

constexpr UINT WM_VEK_PROGRESS = WM_APP + 31;
constexpr UINT WM_VEK_COMPLETE = WM_APP + 32;
constexpr UINT WM_VEK_FAILED   = WM_APP + 33;
constexpr UINT_PTR TIMER_COUNTDOWN = 1;
constexpr UINT_PTR TIMER_CURSOR = 2;

constexpr COLORREF kBg       = RGB(6, 10, 9);
constexpr COLORREF kPanel    = RGB(10, 18, 14);
constexpr COLORREF kGreen    = RGB(55, 255, 139);
constexpr COLORREF kGreenDim = RGB(36, 156, 92);
constexpr COLORREF kText     = RGB(211, 255, 226);
constexpr COLORREF kMuted    = RGB(112, 156, 127);
constexpr COLORREF kError    = RGB(255, 95, 95);
constexpr COLORREF kBorder   = RGB(29, 89, 57);

std::wstring widenUtf8(const std::string& value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) return std::wstring(value.begin(), value.end());
    std::wstring out(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), out.data(), count);
    return out;
}

std::string narrowUtf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0) return std::string(value.begin(), value.end());
    std::string out(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), out.data(), count, nullptr, nullptr);
    return out;
}

std::wstring win32ErrorMessage(DWORD code) {
    wchar_t* buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD n = FormatMessageW(flags, nullptr, code, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    std::wstring result = n && buffer ? std::wstring(buffer, n) : L"Windows error " + std::to_wstring(code);
    if (buffer) LocalFree(buffer);
    while (!result.empty() && (result.back() == L'\r' || result.back() == L'\n' || result.back() == L' ')) result.pop_back();
    return result;
}

fs::path moduleDirectory() {
    std::wstring buffer(32768, L'\0');
    const DWORD n = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!n || n >= buffer.size()) return fs::current_path();
    buffer.resize(n);
    std::error_code ec;
    fs::path p = fs::weakly_canonical(fs::path(buffer), ec);
    if (ec) p = fs::path(buffer);
    return p.parent_path();
}

std::wstring normalizePathForCompare(fs::path p) {
    std::error_code ec;
    auto canonical = fs::weakly_canonical(p, ec);
    if (!ec) p = canonical;
    auto s = p.wstring();
    std::replace(s.begin(), s.end(), L'/', L'\\');
    while (s.size() > 3 && !s.empty() && s.back() == L'\\') s.pop_back();
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return s;
}

bool samePath(const fs::path& a, const fs::path& b) {
    return normalizePathForCompare(a) == normalizePathForCompare(b);
}

std::wstring trimQuotesAndSpace(std::wstring s) {
    while (!s.empty() && std::iswspace(s.front())) s.erase(s.begin());
    while (!s.empty() && std::iswspace(s.back())) s.pop_back();
    if (s.size() >= 2 && s.front() == L'"' && s.back() == L'"') s = s.substr(1, s.size() - 2);
    return s;
}

std::vector<std::wstring> splitPathList(const std::wstring& value) {
    std::vector<std::wstring> parts;
    std::wstring current;
    for (wchar_t c : value) {
        if (c == L';') {
            auto t = trimQuotesAndSpace(current);
            if (!t.empty()) parts.push_back(std::move(t));
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    auto t = trimQuotesAndSpace(current);
    if (!t.empty()) parts.push_back(std::move(t));
    return parts;
}

std::wstring readUserPath(DWORD& valueType) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        valueType = REG_EXPAND_SZ;
        return L"";
    }
    DWORD type = 0;
    DWORD bytes = 0;
    LONG result = RegQueryValueExW(key, L"Path", nullptr, &type, nullptr, &bytes);
    if (result == ERROR_FILE_NOT_FOUND) {
        RegCloseKey(key);
        valueType = REG_EXPAND_SZ;
        return L"";
    }
    if (result != ERROR_SUCCESS) {
        RegCloseKey(key);
        valueType = REG_EXPAND_SZ;
        return L"";
    }
    std::vector<wchar_t> buffer(bytes / sizeof(wchar_t) + 2, L'\0');
    result = RegQueryValueExW(key, L"Path", nullptr, &type, reinterpret_cast<LPBYTE>(buffer.data()), &bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS) {
        valueType = REG_EXPAND_SZ;
        return L"";
    }
    valueType = (type == REG_SZ || type == REG_EXPAND_SZ) ? type : REG_EXPAND_SZ;
    return std::wstring(buffer.data());
}

bool addUserPath(const fs::path& destination, std::wstring& error) {
    DWORD type = REG_EXPAND_SZ;
    std::wstring current = readUserPath(type);
    const std::wstring wanted = normalizePathForCompare(destination);
    for (const auto& raw : splitPathList(current)) {
        if (normalizePathForCompare(raw) == wanted) return true;
    }

    std::wstring updated = current;
    while (!updated.empty() && (updated.back() == L';' || std::iswspace(updated.back()))) updated.pop_back();
    if (!updated.empty()) updated += L';';
    updated += destination.wstring();
    if (updated.size() >= 32760) {
        error = L"Your Windows User PATH is too long to add VEK safely.";
        return false;
    }

    HKEY key = nullptr;
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Environment", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (result != ERROR_SUCCESS) {
        error = L"Could not open your Windows user environment settings:\n" + win32ErrorMessage(static_cast<DWORD>(result));
        return false;
    }
    result = RegSetValueExW(key, L"Path", 0, type, reinterpret_cast<const BYTE*>(updated.c_str()), static_cast<DWORD>((updated.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    if (result != ERROR_SUCCESS) {
        error = L"Could not update Windows User PATH:\n" + win32ErrorMessage(static_cast<DWORD>(result));
        return false;
    }
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L"Environment"), SMTO_ABORTIFHUNG, 3000, nullptr);
    return true;
}

std::wstring processPath() {
    DWORD size = GetEnvironmentVariableW(L"PATH", nullptr, 0);
    if (!size) return {};
    std::wstring out(size, L'\0');
    const DWORD n = GetEnvironmentVariableW(L"PATH", out.data(), size);
    if (!n) return {};
    out.resize(n);
    return out;
}

std::wstring processEnv(const wchar_t* name) {
    DWORD size = GetEnvironmentVariableW(name, nullptr, 0);
    if (!size) return {};
    std::wstring out(size, L'\0');
    const DWORD n = GetEnvironmentVariableW(name, out.data(), size);
    if (!n) return {};
    out.resize(n);
    return out;
}

std::string readVersion(const fs::path& root) {
    std::ifstream in(root / "VERSION", std::ios::binary);
    std::string s;
    std::getline(in, s);
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t')) s.pop_back();
    return s;
}

bool isVekInstallRoot(const fs::path& root) {
    std::error_code ec;
    return fs::is_regular_file(root / "vek.exe", ec) && fs::is_regular_file(root / "VERSION", ec);
}

struct ExistingInstall {
    bool found = false;
    fs::path root;
    std::string version;
};

ExistingInstall detectExistingInstallation() {
    std::vector<fs::path> candidates;
    candidates.emplace_back(L"C:\\vek");
    const auto vekHome = processEnv(L"VEK_HOME");
    if (!vekHome.empty()) candidates.emplace_back(vekHome);
    for (const auto& p : splitPathList(processPath())) candidates.emplace_back(p);
    DWORD userType = REG_EXPAND_SZ;
    for (const auto& p : splitPathList(readUserPath(userType))) candidates.emplace_back(p);

    std::vector<std::wstring> seen;
    for (const auto& candidate : candidates) {
        if (candidate.empty()) continue;
        const auto key = normalizePathForCompare(candidate);
        if (std::find(seen.begin(), seen.end(), key) != seen.end()) continue;
        seen.push_back(key);
        if (isVekInstallRoot(candidate)) return {true, candidate, readVersion(candidate)};
    }
    return {};
}

bool chooseFolder(HWND owner, fs::path& out) {
    IFileDialog* dialog = nullptr;
    const HRESULT createHr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (FAILED(createHr) || !dialog) return false;
    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    dialog->SetTitle(L"Choose where to install VEK");
    const HRESULT showHr = dialog->Show(owner);
    if (showHr != S_OK) { dialog->Release(); return false; }
    IShellItem* item = nullptr;
    if (FAILED(dialog->GetResult(&item)) || !item) { dialog->Release(); return false; }
    PWSTR raw = nullptr;
    const HRESULT pathHr = item->GetDisplayName(SIGDN_FILESYSPATH, &raw);
    if (SUCCEEDED(pathHr) && raw) out = fs::path(raw);
    if (raw) CoTaskMemFree(raw);
    item->Release();
    dialog->Release();
    return SUCCEEDED(pathHr) && !out.empty();
}

bool packageLooksValid(const fs::path& source) {
    std::error_code ec;
    return fs::is_regular_file(source / "vek.exe", ec) &&
           fs::is_regular_file(source / "VekInstaller.exe", ec) &&
           fs::is_regular_file(source / "VERSION", ec) &&
           fs::is_directory(source / "include", ec) &&
           fs::is_regular_file(source / "share" / "vek" / "installer_ui.vek", ec);
}

bool copyOne(const fs::path& from, const fs::path& to, std::wstring& error) {
    std::error_code ec;
    if (!fs::exists(from, ec)) return true;
    if (fs::is_directory(from, ec)) {
        fs::create_directories(to, ec);
        if (ec) { error = L"Could not create " + to.wstring(); return false; }
        for (fs::recursive_directory_iterator it(from, fs::directory_options::skip_permission_denied, ec), end; it != end; it.increment(ec)) {
            if (ec) { error = L"Could not read " + from.wstring(); return false; }
            const auto rel = fs::relative(it->path(), from, ec);
            if (ec) { error = L"Could not resolve package path."; return false; }
            const auto target = to / rel;
            if (it->is_directory(ec)) {
                fs::create_directories(target, ec);
                if (ec) { error = L"Could not create " + target.wstring(); return false; }
            } else if (it->is_regular_file(ec)) {
                fs::create_directories(target.parent_path(), ec);
                ec.clear();
                fs::copy_file(it->path(), target, fs::copy_options::overwrite_existing, ec);
                if (ec) { error = L"Could not copy " + it->path().wstring() + L"\n\n" + widenUtf8(ec.message()); return false; }
            }
        }
        return true;
    }
    fs::create_directories(to.parent_path(), ec);
    ec.clear();
    fs::copy_file(from, to, fs::copy_options::overwrite_existing, ec);
    if (ec) { error = L"Could not copy " + from.wstring() + L"\n\n" + widenUtf8(ec.message()); return false; }
    return true;
}

bool copyRootMetadata(const fs::path& source, const fs::path& destination, std::wstring& error) {
    std::error_code ec;
    for (fs::directory_iterator it(source, ec), end; it != end; it.increment(ec)) {
        if (ec) { error = L"Could not inspect the VEK package."; return false; }
        if (!it->is_regular_file(ec)) continue;
        const auto name = it->path().filename().wstring();
        if (name == L"vek.exe" || name == L"vek.dll" || name == L"VekInstaller.exe") continue;
        if (!copyOne(it->path(), destination / it->path().filename(), error)) return false;
    }
    return true;
}

struct ProgressPayload { int progress = 0; std::wstring status; };

void postProgress(HWND hwnd, int progress, const std::wstring& status) {
    auto* payload = new ProgressPayload{progress, status};
    PostMessageW(hwnd, WM_VEK_PROGRESS, 0, reinterpret_cast<LPARAM>(payload));
}

void sleepStage(DWORD ms = 130) { Sleep(ms); }

void installWorker(HWND hwnd, fs::path source, fs::path destination, bool addPath) {
    std::wstring error;
    postProgress(hwnd, 1, L"PRE-FLIGHT // scanning package"); sleepStage(150);
    if (!packageLooksValid(source)) {
        auto* message = new std::wstring(L"The VEK package is incomplete. Extract the official Windows ZIP and run the installer from inside it.");
        PostMessageW(hwnd, WM_VEK_FAILED, 0, reinterpret_cast<LPARAM>(message));
        return;
    }

    postProgress(hwnd, 2, L"PACKAGE // VEK runtime + UI script verified"); sleepStage();
    std::error_code ec;
    fs::create_directories(destination, ec);
    if (ec) {
        auto* message = new std::wstring(L"Could not create the install folder:\n" + destination.wstring() + L"\n\n" + widenUtf8(ec.message()));
        PostMessageW(hwnd, WM_VEK_FAILED, 0, reinterpret_cast<LPARAM>(message));
        return;
    }

    postProgress(hwnd, 3, L"TARGET // installation directory ready"); sleepStage();
    if (!samePath(source, destination)) {
        for (const wchar_t* name : {L"vek.exe", L"vek.dll", L"VekInstaller.exe"}) {
            if (!copyOne(source / name, destination / name, error)) {
                auto* message = new std::wstring(error); PostMessageW(hwnd, WM_VEK_FAILED, 0, reinterpret_cast<LPARAM>(message)); return;
            }
        }
    }

    postProgress(hwnd, 4, L"RUNTIME // core binaries deployed"); sleepStage();
    if (!samePath(source, destination)) {
        for (const wchar_t* dir : {L"include", L"lib"}) {
            if (!copyOne(source / dir, destination / dir, error)) {
                auto* message = new std::wstring(error); PostMessageW(hwnd, WM_VEK_FAILED, 0, reinterpret_cast<LPARAM>(message)); return;
            }
        }
    }

    postProgress(hwnd, 5, L"SDK // headers and libraries deployed"); sleepStage();
    if (!samePath(source, destination)) {
        for (const wchar_t* dir : {L"examples", L"docs", L"share"}) {
            if (!copyOne(source / dir, destination / dir, error)) {
                auto* message = new std::wstring(error); PostMessageW(hwnd, WM_VEK_FAILED, 0, reinterpret_cast<LPARAM>(message)); return;
            }
        }
    }

    postProgress(hwnd, 6, L"VEK UI // installer script deployed"); sleepStage();
    if (!samePath(source, destination) && !copyRootMetadata(source, destination, error)) {
        auto* message = new std::wstring(error); PostMessageW(hwnd, WM_VEK_FAILED, 0, reinterpret_cast<LPARAM>(message)); return;
    }

    postProgress(hwnd, 7, L"METADATA // version and integrity files deployed"); sleepStage();
    if (addPath && !addUserPath(destination, error)) {
        auto* message = new std::wstring(error); PostMessageW(hwnd, WM_VEK_FAILED, 0, reinterpret_cast<LPARAM>(message)); return;
    }

    postProgress(hwnd, 8, addPath ? L"WINDOWS // User PATH registered" : L"WINDOWS // PATH change skipped"); sleepStage();
    if (!isVekInstallRoot(destination)) {
        auto* message = new std::wstring(L"VEK verification failed: vek.exe or VERSION is missing from the target folder.");
        PostMessageW(hwnd, WM_VEK_FAILED, 0, reinterpret_cast<LPARAM>(message));
        return;
    }

    postProgress(hwnd, 9, L"VERIFY // executable and version detected"); sleepStage(180);
    postProgress(hwnd, 10, L"COMPLETE // VEK is ready"); sleepStage(100);
    PostMessageW(hwnd, WM_VEK_COMPLETE, 0, 0);
}

HFONT makeFont(int points, int weight = FW_NORMAL) {
    HDC dc = GetDC(nullptr);
    const int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
    if (dc) ReleaseDC(nullptr, dc);
    const int height = -MulDiv(points, dpi, 72);
    return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                       FIXED_PITCH | FF_MODERN, L"Consolas");
}

void fillRectColor(HDC dc, const RECT& r, COLORREF color) {
    HBRUSH b = CreateSolidBrush(color);
    FillRect(dc, &r, b);
    DeleteObject(b);
}

void frameRectColor(HDC dc, const RECT& r, COLORREF color) {
    HBRUSH b = CreateSolidBrush(color);
    FrameRect(dc, &r, b);
    DeleteObject(b);
}

void drawTextLine(HDC dc, HFONT font, COLORREF color, const RECT& r, const std::wstring& text, UINT flags = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS) {
    auto oldFont = SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    RECT copy = r;
    DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &copy, flags);
    SelectObject(dc, oldFont);
}

struct InstallerApp {
    HWND hwnd = nullptr;
    fs::path source;
    fs::path destination;
    ExistingInstall existing;
    bool addPath = true;
    bool installing = false;
    bool complete = false;
    bool failed = false;
    int progress = 0;
    int countdown = 5;
    bool cursorOn = true;
    std::wstring status = L"READY // press INSTALL to continue";
    std::wstring failureText;

    VekScriptEngine vm;
    vek::GuiSystem gui;
    bool uiLoaded = false;
    std::map<std::string, RECT> buttonRects;

    HFONT fontBig = nullptr;
    HFONT fontTitle = nullptr;
    HFONT fontBody = nullptr;
    HFONT fontSmall = nullptr;

    ~InstallerApp() {
        if (fontBig) DeleteObject(fontBig);
        if (fontTitle) DeleteObject(fontTitle);
        if (fontBody) DeleteObject(fontBody);
        if (fontSmall) DeleteObject(fontSmall);
    }

    bool loadVekUi() {
        gui.RegisterNatives(vm);
        vm.RegisterNative("string", [](const std::vector<VekValue>& args) {
            return VekValue(args.empty() ? "nil" : args[0].AsString());
        });
        const auto script = source / "share" / "vek" / "installer_ui.vek";
        std::ifstream in(script, std::ios::binary);
        if (!in) return false;
        std::ostringstream raw;
        raw << in.rdbuf();
        if (!vm.LoadSource(raw.str(), "installer_ui.vek")) return false;
        vm.SealNativeRegistry();
        uiLoaded = vm.HasFunction("draw_installer");
        return uiLoaded;
    }

    std::string buildUi(const std::string& pressed = {}) {
        if (!uiLoaded) return {};
        gui.BeginFrame();
        if (!pressed.empty()) gui.SetPressed(pressed, true);
        std::vector<VekValue> args;
        args.emplace_back(progress);
        args.emplace_back(existing.found);
        args.emplace_back(narrowUtf8(destination.wstring()));
        args.emplace_back(narrowUtf8(status));
        args.emplace_back(complete);
        args.emplace_back(countdown);
        args.emplace_back(addPath);
        args.emplace_back(failed);
        args.emplace_back(existing.version.empty() ? std::string(VEK_VERSION_STRING) : existing.version);
        auto result = vm.Call("draw_installer", args);
        gui.EndFrame();
        if (!vm.LastError().empty()) return "__vek_ui_error__";
        return result.AsString();
    }

    void startInstall() {
        if (installing) return;
        installing = true;
        complete = false;
        failed = false;
        failureText.clear();
        progress = 1;
        countdown = 5;
        status = L"PRE-FLIGHT // scanning package";
        InvalidateRect(hwnd, nullptr, FALSE);
        std::thread(installWorker, hwnd, source, destination, addPath).detach();
    }

    void handleAction(const std::string& action) {
        if (action == "install") {
            startInstall();
        } else if (action == "choose") {
            if (installing) return;
            fs::path selected;
            if (chooseFolder(hwnd, selected)) {
                destination = selected;
                status = L"TARGET CHANGED // ready to install";
                failed = false;
                progress = 0;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        } else if (action == "toggle_path") {
            if (!installing) {
                addPath = !addPath;
                status = addPath ? L"USER PATH // enabled" : L"USER PATH // disabled";
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        } else if (action == "close") {
            if (!installing) DestroyWindow(hwnd);
        }
    }

    void paint(HDC dc, const RECT& client) {
        fillRectColor(dc, client, kBg);
        RECT panel{18, 18, client.right - 18, client.bottom - 18};
        fillRectColor(dc, panel, kPanel);
        frameRectColor(dc, panel, kBorder);

        RECT topLine{18, 18, client.right - 18, 21};
        fillRectColor(dc, topLine, kGreenDim);

        const auto action = buildUi();
        if (action == "__vek_ui_error__") {
            RECT er{42, 45, client.right - 42, client.bottom - 45};
            drawTextLine(dc, fontTitle, kError, er, L"VEK UI SCRIPT ERROR // " + widenUtf8(vm.LastError()), DT_LEFT | DT_WORDBREAK);
            return;
        }

        buttonRects.clear();
        int y = 32;
        const int left = 42;
        const int right = client.right - 42;
        int buttonIndex = 0;

        for (const auto& cmd : gui.Commands()) {
            using T = vek::GuiCommandType;
            if (cmd.type == T::BeginWindow || cmd.type == T::EndWindow) continue;

            if (cmd.type == T::Label) {
                const auto text = widenUtf8(cmd.text);
                if (cmd.text == "VEK") {
                    RECT r{left, y, right, y + 62};
                    drawTextLine(dc, fontBig, kGreen, r, text);
                    y += 58;
                } else {
                    RECT r{left, y, right, y + 28};
                    COLORREF c = (cmd.text.rfind("INSTALLING VEK", 0) == 0 || cmd.text.rfind("PLEASE WAIT", 0) == 0) ? kGreen : kText;
                    drawTextLine(dc, fontBody, c, r, text);
                    y += 30;
                }
                continue;
            }

            if (cmd.type == T::StatusBadge) {
                RECT r{left, y, right, y + 32};
                COLORREF c = (cmd.aux == "error") ? kError : kGreen;
                frameRectColor(dc, r, c);
                RECT tx{r.left + 10, r.top, r.right - 10, r.bottom};
                drawTextLine(dc, fontBody, c, tx, widenUtf8(cmd.text));
                y += 42;
                continue;
            }

            if (cmd.type == T::ProgressBar) {
                RECT outer{left, y, right, y + 28};
                frameRectColor(dc, outer, kGreenDim);
                const int gap = 4;
                const int cells = 10;
                const int innerW = (outer.right - outer.left - 12 - gap * (cells - 1)) / cells;
                const int filled = std::clamp(static_cast<int>(cmd.value), 0, 10);
                int x = outer.left + 6;
                for (int i = 0; i < cells; ++i) {
                    RECT cell{x, outer.top + 6, x + innerW, outer.bottom - 6};
                    fillRectColor(dc, cell, i < filled ? kGreen : RGB(17, 45, 29));
                    x += innerW + gap;
                }
                y += 38;
                continue;
            }

            if (cmd.type == T::Button) {
                const int width = (buttonIndex == 0) ? 260 : 210;
                RECT r{left, y, left + width, y + 38};
                fillRectColor(dc, r, RGB(8, 28, 17));
                frameRectColor(dc, r, kGreenDim);
                RECT tx{r.left + 12, r.top, r.right - 12, r.bottom};
                drawTextLine(dc, fontBody, kGreen, tx, widenUtf8(cmd.text));
                buttonRects[cmd.id] = r;
                y += 46;
                ++buttonIndex;
                continue;
            }
        }

        RECT footer{left, client.bottom - 48, right, client.bottom - 24};
        std::wstring footerText = L"VEK UI // share/vek/installer_ui.vek // ";
        footerText += cursorOn ? L"_" : L" ";
        drawTextLine(dc, fontSmall, kMuted, footer, footerText);
    }
};

InstallerApp* gApp = nullptr;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* app = gApp;
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            RECT client{};
            GetClientRect(hwnd, &client);
            if (app) app->paint(dc, client);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONUP: {
            if (!app) return 0;
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            for (const auto& [id, rect] : app->buttonRects) {
                if (PtInRect(&rect, pt)) {
                    const auto action = app->buildUi(id);
                    app->handleAction(action);
                    return 0;
                }
            }
            return 0;
        }
        case WM_SETCURSOR:
            if (app && LOWORD(lParam) == HTCLIENT) {
                POINT p{}; GetCursorPos(&p); ScreenToClient(hwnd, &p);
                for (const auto& [id, rect] : app->buttonRects) {
                    (void)id;
                    if (PtInRect(&rect, p)) { SetCursor(LoadCursorW(nullptr, IDC_HAND)); return TRUE; }
                }
            }
            break;
        case WM_VEK_PROGRESS: {
            if (!app) break;
            std::unique_ptr<ProgressPayload> payload(reinterpret_cast<ProgressPayload*>(lParam));
            app->progress = payload->progress;
            app->status = payload->status;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_VEK_COMPLETE:
            if (app) {
                app->installing = false;
                app->complete = true;
                app->failed = false;
                app->progress = 10;
                app->countdown = 5;
                app->status = L"COMPLETE // VEK is ready";
                app->existing = detectExistingInstallation();
                SetTimer(hwnd, TIMER_COUNTDOWN, 1000, nullptr);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_VEK_FAILED: {
            if (!app) break;
            std::unique_ptr<std::wstring> message(reinterpret_cast<std::wstring*>(lParam));
            app->installing = false;
            app->complete = false;
            app->failed = true;
            app->status = L"ERROR // " + *message;
            app->failureText = *message;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_TIMER:
            if (!app) return 0;
            if (wParam == TIMER_COUNTDOWN && app->complete) {
                --app->countdown;
                if (app->countdown <= 0) {
                    KillTimer(hwnd, TIMER_COUNTDOWN);
                    DestroyWindow(hwnd);
                } else {
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            } else if (wParam == TIMER_CURSOR) {
                app->cursorOn = !app->cursorOn;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_CLOSE:
            if (app && app->installing) {
                app->status = L"INSTALL ACTIVE // wait for completion before closing";
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int runInstaller(HINSTANCE instance) {
    const HRESULT initHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const fs::path source = moduleDirectory();

    InstallerApp app;
    gApp = &app;
    app.source = source;
    app.existing = detectExistingInstallation();
    app.destination = app.existing.found ? app.existing.root : fs::path(L"C:\\vek");
    app.status = app.existing.found
        ? L"DETECTED // VEK " + widenUtf8(app.existing.version) + L" at " + app.existing.root.wstring()
        : L"READY // no existing VEK install detected";
    app.fontBig = makeFont(40, FW_BOLD);
    app.fontTitle = makeFont(20, FW_BOLD);
    app.fontBody = makeFont(13, FW_NORMAL);
    app.fontSmall = makeFont(10, FW_NORMAL);

    if (!packageLooksValid(source)) {
        MessageBoxW(nullptr,
                    L"This VekInstaller.exe is not inside a complete VEK portable package.\n\nExtract the official VEK Windows ZIP first, then run it again.",
                    L"VEK Installer", MB_OK | MB_ICONERROR | MB_TASKMODAL);
        if (SUCCEEDED(initHr)) CoUninitialize();
        return 2;
    }

    if (!app.loadVekUi()) {
        const auto message = L"VEK could not load its installer UI script:\n\n" + widenUtf8(app.vm.LastError());
        MessageBoxW(nullptr, message.c_str(), L"VEK Installer", MB_OK | MB_ICONERROR | MB_TASKMODAL);
        if (SUCCEEDED(initHr)) CoUninitialize();
        return 3;
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"VEKInstallerWindowClass";
    if (!RegisterClassExW(&wc)) {
        if (SUCCEEDED(initHr)) CoUninitialize();
        return 4;
    }

    constexpr int width = 780;
    constexpr int height = 620;
    RECT desktop{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &desktop, 0);
    const int x = desktop.left + ((desktop.right - desktop.left) - width) / 2;
    const int y = desktop.top + ((desktop.bottom - desktop.top) - height) / 2;

    app.hwnd = CreateWindowExW(
        0, wc.lpszClassName, L"VEK // INSTALLER",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, width, height, nullptr, nullptr, instance, nullptr);
    if (!app.hwnd) {
        if (SUCCEEDED(initHr)) CoUninitialize();
        return 5;
    }

    SetTimer(app.hwnd, TIMER_CURSOR, 500, nullptr);
    ShowWindow(app.hwnd, SW_SHOW);
    UpdateWindow(app.hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    gApp = nullptr;
    if (SUCCEEDED(initHr)) CoUninitialize();
    return static_cast<int>(msg.wParam);
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
    return runInstaller(instance);
}
