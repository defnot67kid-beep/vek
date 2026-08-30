#pragma once
#include "raylib.h"

enum class EscapeMenuAction {
    None,
    Resume,
    Reset,
    Leave
};

// Small modal pause menu. It owns only UI/input state; Game performs the
// actual reset/leave operations so gameplay systems stay modular.
class EscapeMenuSystem {
public:
    void Open();
    void Close();
    void Toggle();
    bool IsOpen() const { return open; }

    EscapeMenuAction Update();
    // First Alt+F4 opens the menu; another fresh Alt+F4 while open requests Leave.
    EscapeMenuAction HandleAltF4();
    void Draw() const;

private:
    bool open = false;

    Rectangle PanelRect() const;
    Rectangle ButtonRect(int index) const;
    void DrawButton(Rectangle rect, const char* label, const char* subtitle) const;
};
