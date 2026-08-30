#include "Game.h"
#include "CrashLog.h"

#include <exception>

int main() {
    CrashLog::Initialize("CustomVehicleGame v26.8.6 - VEK Part Icons + Viewmodels");

    try {
        CrashLog::SetStage("constructing Game");
        Game game;

        CrashLog::SetStage("Game::Init");
        game.Init();

        CrashLog::SetStage("Game::Run / gameplay loop");
        game.Run();

        CrashLog::SetStage("Game::Shutdown");
        game.Shutdown();

        CrashLog::SetStage("clean exit");
        return 0;
    } catch (const std::exception& e) {
        CrashLog::WriteFatal("Unhandled C++ exception", e.what());
    } catch (...) {
        CrashLog::WriteFatal("Unhandled C++ exception", "unknown non-standard exception");
    }

    return 1;
}
