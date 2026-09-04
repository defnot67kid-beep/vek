#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <commctrl.h>
#include <shobjidl.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace fs = std::filesystem;

namespace {

std::wstring widen(const std::string& value) { return std::wstring(value.begin(), value.end()); }

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
    p = fs::weakly_canonical(p, ec);
    auto s = (ec ? p : p).wstring();
    std::replace(s.begin(), s.end(), L'/', L'\\');
    while (s.size() > 3 && !s.empty() && s.back() == L'\\') s.pop_back();
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return s;
}

bool samePath(const fs::path& a, const fs::path& b) {
    return normalizePathForCompare(a) == normalizePathForCompare(b);
}

int showTaskDialog(const wchar_t* title,
                   const wchar_t* instruction,
                   const std::wstring& content,
                   const std::vector<TASKDIALOG_BUTTON>& buttons,
                   int defaultButton,
                   const wchar_t* verificationText = nullptr,
                   bool verificationChecked = false,
                   bool* verificationResult = nullptr,
                   TASKDIALOG_COMMON_BUTTON_FLAGS commonButtons = TDCBF_CANCEL_BUTTON) {
    TASKDIALOGCONFIG cfg{};
    cfg.cbSize = sizeof(cfg);
    cfg.hInstance = GetModuleHandleW(nullptr);
    cfg.dwFlags = TDF_USE_COMMAND_LINKS | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT;
    if (verificationChecked) cfg.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;
    cfg.dwCommonButtons = commonButtons;
    cfg.pszWindowTitle = title;
    cfg.pszMainInstruction = instruction;
    cfg.pszContent = content.c_str();
    cfg.cButtons = static_cast<UINT>(buttons.size());
    cfg.pButtons = buttons.empty() ? nullptr : buttons.data();
    cfg.nDefaultButton = defaultButton;
    cfg.pszVerificationText = verificationText;
    int pressed = IDCANCEL;
    BOOL checked = FALSE;
    const HRESULT hr = TaskDialogIndirect(&cfg, &pressed, nullptr, verificationText ? &checked : nullptr);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, content.c_str(), instruction, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
        return IDCANCEL;
    }
    if (verificationResult) *verificationResult = checked == TRUE;
    return pressed;
}

bool chooseFolder(fs::path& out) {
    IFileDialog* dialog = nullptr;
    const HRESULT createHr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (FAILED(createHr) || !dialog) return false;

    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    dialog->SetTitle(L"Choose where to install VEK");

    const HRESULT showHr = dialog->Show(nullptr);
    if (showHr != S_OK) {
        dialog->Release();
        return false;
    }

    IShellItem* item = nullptr;
    const HRESULT resultHr = dialog->GetResult(&item);
    if (FAILED(resultHr) || !item) {
        dialog->Release();
        return false;
    }

    PWSTR raw = nullptr;
    const HRESULT pathHr = item->GetDisplayName(SIGDN_FILESYSPATH, &raw);
    if (SUCCEEDED(pathHr) && raw) out = fs::path(raw);
    if (raw) CoTaskMemFree(raw);
    item->Release();
    dialog->Release();
    return SUCCEEDED(pathHr) && !out.empty();
}

bool shouldSkipRelative(const fs::path& rel) {
    if (rel.empty()) return false;
    const auto first = *rel.begin();
    const auto firstText = first.wstring();
    if (firstText == L".git" || firstText == L"build" || firstText == L"build-portable" || firstText == L"stage" || firstText == L"stage-portable") return true;
    return false;
}

bool copyPortableTree(const fs::path& source, const fs::path& destination, std::wstring& error) {
    if (samePath(source, destination)) return true;

    std::error_code ec;
    fs::create_directories(destination, ec);
    if (ec) {
        error = L"Could not create the installation folder:\n\n" + destination.wstring() + L"\n\n" + widen(ec.message());
        return false;
    }

    for (fs::recursive_directory_iterator it(source, fs::directory_options::skip_permission_denied, ec), end; it != end; it.increment(ec)) {
        if (ec) {
            error = L"Could not read the VEK package while installing.";
            return false;
        }
        const fs::path rel = fs::relative(it->path(), source, ec);
        if (ec || shouldSkipRelative(rel)) {
            if (it->is_directory()) it.disable_recursion_pending();
            continue;
        }
        const fs::path target = destination / rel;
        if (it->is_directory(ec)) {
            fs::create_directories(target, ec);
            if (ec) {
                error = L"Could not create:\n\n" + target.wstring();
                return false;
            }
        } else if (it->is_regular_file(ec)) {
            fs::create_directories(target.parent_path(), ec);
            ec.clear();
            fs::copy_file(it->path(), target, fs::copy_options::overwrite_existing, ec);
            if (ec) {
                error = L"Could not copy:\n\n" + it->path().wstring() + L"\n\nto:\n\n" + target.wstring() + L"\n\n" + widen(ec.message());
                return false;
            }
        }
    }
    return true;
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

std::vector<std::wstring> splitPathList(const std::wstring& value) {
    std::vector<std::wstring> parts;
    std::wstring current;
    for (wchar_t c : value) {
        if (c == L';') {
            if (!current.empty()) parts.push_back(current);
            current.clear();
        } else current.push_back(c);
    }
    if (!current.empty()) parts.push_back(current);
    return parts;
}

std::wstring trimQuotesAndSpace(std::wstring s) {
    auto isSpace = [](wchar_t c) { return std::iswspace(c) != 0; };
    while (!s.empty() && isSpace(s.front())) s.erase(s.begin());
    while (!s.empty() && isSpace(s.back())) s.pop_back();
    if (s.size() >= 2 && s.front() == L'"' && s.back() == L'"') s = s.substr(1, s.size() - 2);
    return s;
}

bool addUserPath(const fs::path& destination, std::wstring& error) {
    DWORD type = REG_EXPAND_SZ;
    std::wstring current = readUserPath(type);
    const std::wstring wanted = normalizePathForCompare(destination);
    for (const auto& raw : splitPathList(current)) {
        if (normalizePathForCompare(trimQuotesAndSpace(raw)) == wanted) return true;
    }

    std::wstring updated = current;
    while (!updated.empty() && (updated.back() == L';' || std::iswspace(updated.back()))) updated.pop_back();
    if (!updated.empty()) updated += L';';
    updated += destination.wstring();
    if (updated.size() >= 32760) {
        error = L"Your User PATH is too long to add VEK safely. Choose manual PATH setup instead.";
        return false;
    }

    HKEY key = nullptr;
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Environment", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (result != ERROR_SUCCESS) {
        error = L"Could not open your Windows user environment settings:\n\n" + win32ErrorMessage(static_cast<DWORD>(result));
        return false;
    }
    result = RegSetValueExW(key, L"Path", 0, type, reinterpret_cast<const BYTE*>(updated.c_str()), static_cast<DWORD>((updated.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    if (result != ERROR_SUCCESS) {
        error = L"Could not update your Windows User PATH:\n\n" + win32ErrorMessage(static_cast<DWORD>(result));
        return false;
    }

    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L"Environment"), SMTO_ABORTIFHUNG, 3000, nullptr);
    return true;
}

bool packageLooksValid(const fs::path& source) {
    std::error_code ec;
    return fs::is_regular_file(source / "vek.exe", ec) &&
           fs::is_regular_file(source / "VERSION", ec) &&
           fs::is_directory(source / "include", ec);
}

int installWizard() {
    const HRESULT initHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool coReady = SUCCEEDED(initHr) || initHr == RPC_E_CHANGED_MODE;

    const fs::path source = moduleDirectory();
    if (!packageLooksValid(source)) {
        MessageBoxW(nullptr,
                    L"This installer is not inside a complete VEK portable package.\n\nExtract the official VEK ZIP first, then run VekInstaller.exe again.",
                    L"VEK Installer",
                    MB_OK | MB_ICONERROR | MB_TASKMODAL);
        if (SUCCEEDED(initHr)) CoUninitialize();
        return 2;
    }

    const TASKDIALOG_BUTTON modeButtons[] = {
        {100, L"Quick install\nInstall or repair VEK at C:\\vek"},
        {101, L"Choose a folder\nInstall VEK somewhere else"},
        {102, L"Use this folder\nKeep this extracted copy where it is and register it with Windows"}
    };
    const std::vector<TASKDIALOG_BUTTON> modes(std::begin(modeButtons), std::end(modeButtons));
    const int mode = showTaskDialog(
        L"VEK Installer",
        L"Install VEK Programming Language",
        L"Choose how you want VEK to be installed. No Git clone or traditional setup program is required.",
        modes,
        100);
    if (mode == IDCANCEL) {
        if (SUCCEEDED(initHr)) CoUninitialize();
        return 0;
    }

    fs::path destination;
    if (mode == 100) destination = fs::path(L"C:\\vek");
    else if (mode == 102) destination = source;
    else if (mode == 101) {
        if (!coReady || !chooseFolder(destination)) {
            if (SUCCEEDED(initHr)) CoUninitialize();
            return 0;
        }
    } else {
        if (SUCCEEDED(initHr)) CoUninitialize();
        return 0;
    }

    bool addPath = true;
    const TASKDIALOG_BUTTON confirmButtons[] = {{200, L"Install VEK\nCopy/register the language and finish setup"}};
    const std::vector<TASKDIALOG_BUTTON> confirm(std::begin(confirmButtons), std::end(confirmButtons));
    std::wstringstream details;
    details << L"VEK will use:\n" << destination.wstring();
    if (!samePath(source, destination)) details << L"\n\nFiles will be copied from:\n" << source.wstring();
    const int confirmed = showTaskDialog(
        L"VEK Installer",
        L"Ready to install VEK",
        details.str(),
        confirm,
        200,
        L"Add VEK to my Windows User PATH",
        true,
        &addPath);
    if (confirmed != 200) {
        if (SUCCEEDED(initHr)) CoUninitialize();
        return 0;
    }

    std::wstring error;
    if (!copyPortableTree(source, destination, error)) {
        std::wstring content = error;
        if (mode == 100) content += L"\n\nIf C:\\vek is protected on this PC, choose 'Choose a folder' and select a folder inside your user profile.";
        MessageBoxW(nullptr, content.c_str(), L"VEK installation failed", MB_OK | MB_ICONERROR | MB_TASKMODAL);
        if (SUCCEEDED(initHr)) CoUninitialize();
        return 3;
    }

    if (addPath && !addUserPath(destination, error)) {
        MessageBoxW(nullptr, error.c_str(), L"VEK PATH setup failed", MB_OK | MB_ICONWARNING | MB_TASKMODAL);
        if (SUCCEEDED(initHr)) CoUninitialize();
        return 4;
    }

    std::wstringstream done;
    done << L"VEK is ready at:\n" << destination.wstring();
    if (addPath) done << L"\n\nVEK was added to your User PATH. Open a NEW Command Prompt before typing 'vek --version'.";
    else done << L"\n\nPATH was not changed. You can run vek.exe directly or add this folder to PATH later.";
    done << L"\n\nTry:\nvek --version\nvek doctor\nvek verify";
    TaskDialog(nullptr, GetModuleHandleW(nullptr), L"VEK Installer", L"VEK installation complete", done.str().c_str(), TDCBF_OK_BUTTON, TD_INFORMATION_ICON, nullptr);

    if (SUCCEEDED(initHr)) CoUninitialize();
    return 0;
}

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return installWizard();
}
