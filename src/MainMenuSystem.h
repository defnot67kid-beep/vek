#pragma once
#include "raylib.h"
#include <VekScriptEngine.h>
#include <VekGameSystems.h>
#include <VekEditorSystems.h>
#include <string>

class MainMenuSystem {
public:
    bool Initialize(const std::string& scriptPath);
    void Update();
    void Draw() const;

    bool IsOpen() const { return open; }
    bool HasSelection() const { return selected; }
    vek::GameMode SelectedMode() const { return mode; }
    const std::string& ScriptError() const { return error; }

private:
    VekScriptEngine engine;
    vek::GuiSystem gui;
    bool open = true;
    bool selected = false;
    vek::GameMode mode = vek::GameMode::Survival;
    std::string scriptFile;
    std::string error;

    Rectangle SurvivalRect() const;
    Rectangle SandboxRect() const;
};
