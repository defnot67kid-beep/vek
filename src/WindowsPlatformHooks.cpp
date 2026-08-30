#include "WindowsPlatformHooks.h"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <atomic>

namespace {
    HWND gWindowHandle = nullptr;
    WNDPROC gPreviousWndProc = nullptr;
    std::atomic<bool> gAltF4Pending{false};
    bool gInstalled = false;

    LRESULT CALLBACK GameWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        // Alt+F4 normally reaches DefWindowProc and becomes SC_CLOSE/WM_CLOSE.
        // Swallow the initial keydown and all auto-repeat keydowns so holding
        // the keys cannot open and immediately close the game.
        if (message == WM_SYSKEYDOWN && wParam == VK_F4) {
            const bool altHeld = (lParam & (1LL << 29)) != 0;
            const bool isRepeat = (lParam & (1LL << 30)) != 0;
            if (altHeld) {
                if (!isRepeat) gAltF4Pending.store(true, std::memory_order_release);
                return 0;
            }
        }

        // Keyboard-initiated system close is a fallback path used by some
        // Windows/GLFW combinations. A title-bar X click carries mouse-position
        // data in lParam and is intentionally allowed to close normally.
        if (message == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_CLOSE && lParam == 0) {
            gAltF4Pending.store(true, std::memory_order_release);
            return 0;
        }

        return gPreviousWndProc
            ? CallWindowProcW(gPreviousWndProc, hwnd, message, wParam, lParam)
            : DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

void PlatformInstallWindowHooks(void* nativeWindowHandle) {
    if (gInstalled) return;
    gWindowHandle = static_cast<HWND>(nativeWindowHandle);
    if (!gWindowHandle) return;

    SetLastError(0);
    LONG_PTR previous = SetWindowLongPtrW(
        gWindowHandle,
        GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(GameWindowProc));

    if (previous != 0 || GetLastError() == 0) {
        gPreviousWndProc = reinterpret_cast<WNDPROC>(previous);
        gInstalled = true;
    }
}

void PlatformRemoveWindowHooks() {
    if (gInstalled && gWindowHandle && gPreviousWndProc) {
        SetWindowLongPtrW(
            gWindowHandle,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(gPreviousWndProc));
    }
    gPreviousWndProc = nullptr;
    gWindowHandle = nullptr;
    gAltF4Pending.store(false, std::memory_order_release);
    gInstalled = false;
}


bool PlatformConsumeAltF4() {
    return gAltF4Pending.exchange(false, std::memory_order_acq_rel);
}

bool PlatformAltF4HookInstalled() { return gInstalled; }

#else

void PlatformInstallWindowHooks(void*) {}
void PlatformRemoveWindowHooks() {}
bool PlatformConsumeAltF4() { return false; }
bool PlatformAltF4HookInstalled() { return true; }

#endif
