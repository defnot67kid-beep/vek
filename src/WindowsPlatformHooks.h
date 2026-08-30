#pragma once

// Tiny platform bridge kept separate from raylib headers to avoid Win32 API
// name collisions (for example raylib CloseWindow/ShowCursor).
void PlatformInstallWindowHooks(void* nativeWindowHandle);
void PlatformRemoveWindowHooks();
bool PlatformConsumeAltF4();
bool PlatformAltF4HookInstalled();
