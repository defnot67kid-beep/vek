#include "WindowInputSystem.h"
#include "WindowsPlatformHooks.h"
#include "raylib.h"

void WindowInputSystem::Initialize() {
    if (initialized) return;
    initialized = true;

    // Install the native Alt+F4 interception AFTER raylib/GLFW creates its window.
    PlatformInstallWindowHooks(GetWindowHandle());
    altF4HookInstalled = PlatformAltF4HookInstalled();

    // Explicitly choose normal cursor mode ONCE. From this point onward gameplay
    // deliberately leaves the OS pointer alone.
    SetNormalCursorOnce();
}

void WindowInputSystem::Shutdown() {
    PlatformRemoveWindowHooks();
    altF4HookInstalled = false;
    initialized = false;
}

void WindowInputSystem::SetNormalCursorOnce() {
    EnableCursor();
    ShowCursor();
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

bool WindowInputSystem::ConsumeAltF4() {
    if (PlatformConsumeAltF4()) return true;

#ifndef _WIN32
    const bool altHeld = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
    return altHeld && IsKeyPressed(KEY_F4);
#else
    return false;
#endif
}
