#pragma once

// Owns desktop/window-specific input behavior that should not leak into Game.cpp.
// The cursor policy is deliberately simple: initialize one normal visible OS
// pointer and then DO NOT continuously capture, release, hide, clip, re-center,
// or otherwise manipulate it during normal gameplay.
//
// Alt+F4 interception remains Windows-specific so the first press can open the
// Escape Menu instead of immediately closing the process.
class WindowInputSystem {
public:
    void Initialize();
    void Shutdown();

    // One-time safety reset used at startup only. This does not move the mouse.
    // Normal gameplay should not call this every frame.
    void SetNormalCursorOnce();

    // Returns true once for each fresh Alt+F4 press.
    bool ConsumeAltF4();

    bool AltF4HookInstalled() const { return altF4HookInstalled; }

private:
    bool initialized = false;
    bool altF4HookInstalled = false;
};
