#include "EscapeMenuSystem.h"

void EscapeMenuSystem::Open() { open = true; }
void EscapeMenuSystem::Close() { open = false; }
void EscapeMenuSystem::Toggle() { open = !open; }

EscapeMenuAction EscapeMenuSystem::HandleAltF4() {
    if (!open) {
        Open();
        return EscapeMenuAction::None;
    }
    return EscapeMenuAction::Leave;
}

Rectangle EscapeMenuSystem::PanelRect() const {
    const float width = 420.0f;
    const float height = 430.0f;
    return {
        GetScreenWidth()*0.5f - width*0.5f,
        GetScreenHeight()*0.5f - height*0.5f,
        width,
        height
    };
}

Rectangle EscapeMenuSystem::ButtonRect(int index) const {
    Rectangle panel = PanelRect();
    return { panel.x + 55.0f, panel.y + 135.0f + index*86.0f, panel.width - 110.0f, 66.0f };
}

EscapeMenuAction EscapeMenuSystem::Update() {
    if (!open) return EscapeMenuAction::None;

    Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, ButtonRect(0))) return EscapeMenuAction::Resume;
        if (CheckCollisionPointRec(mouse, ButtonRect(1))) return EscapeMenuAction::Reset;
        if (CheckCollisionPointRec(mouse, ButtonRect(2))) return EscapeMenuAction::Leave;
    }

    return EscapeMenuAction::None;
}

void EscapeMenuSystem::DrawButton(Rectangle rect, const char* label, const char* subtitle) const {
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, rect);
    Color fill = hover ? Color{48,78,91,245} : Color{30,42,49,240};
    Color border = hover ? SKYBLUE : Color{87,105,112,255};

    DrawRectangleRec(rect, fill);
    DrawRectangleLinesEx(rect, 2.0f, border);

    int labelWidth = MeasureText(label, 24);
    DrawText(label, (int)(rect.x + rect.width*0.5f - labelWidth*0.5f), (int)rect.y + 10, 24,
             hover ? RAYWHITE : LIGHTGRAY);

    int subtitleWidth = MeasureText(subtitle, 14);
    DrawText(subtitle, (int)(rect.x + rect.width*0.5f - subtitleWidth*0.5f), (int)rect.y + 42, 14,
             Color{165,177,182,255});
}

void EscapeMenuSystem::Draw() const {
    if (!open) return;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.68f));

    Rectangle panel = PanelRect();
    DrawRectangleRec(panel, Color{17,24,29,250});
    DrawRectangleLinesEx(panel, 2.0f, SKYBLUE);

    const char* title = "ESCAPE MENU";
    int titleWidth = MeasureText(title, 32);
    DrawText(title, (int)(panel.x + panel.width*0.5f - titleWidth*0.5f), (int)panel.y + 32, 32, RAYWHITE);

    const char* hint = "Game paused";
    int hintWidth = MeasureText(hint, 17);
    DrawText(hint, (int)(panel.x + panel.width*0.5f - hintWidth*0.5f), (int)panel.y + 78, 17, SKYBLUE);

    DrawButton(ButtonRect(0), "RESUME", "Return to the game");
    DrawButton(ButtonRect(1), "RESET", "Ragdoll, death effect, then respawn");
    DrawButton(ButtonRect(2), "LEAVE", "Exit to desktop");

    const char* footer = "ESC resumes  |  Alt+F4 again leaves";
    int footerWidth = MeasureText(footer, 14);
    DrawText(footer, (int)(panel.x + panel.width*0.5f - footerWidth*0.5f),
             (int)(panel.y + panel.height - 30.0f), 14, Color{140,151,156,255});
}
