// v26 note: animated door interaction, richer character rig, fuller hair, day/night cycle, and dev cheat panel groundwork.
#pragma once
#include <VekGameSystems.h>
#include <VekScriptEngine.h>
#include <memory>
#include <string>
#include <vector>

struct HangarDoorSettings {
    float offsetX=-21.0f;
    float width=2.4f;
    float height=3.0f;
    float openAngle=95.0f;
    float openSpeed=1.6f;
    float autoCloseDelay=0.8f;
};

struct HangarAccessSettings {
    float passlockOffsetX=-18.4f;
    float passlockHeight=1.45f;
    std::string garageId="hangar.main_garage";
    std::string passlockId="hangar.builder_access";
    std::string keypadClip="hangar.use_passlock";
    std::string garageOpenClip="hangar.garage_open";
    std::string garageCloseClip="hangar.garage_close";
};

class VekHangarInteractionSystem {
public:
    bool Initialize(const std::string& path);
    bool Reload();
    const std::string& Error() const { return error; }
    const HangarDoorSettings& Door() const { return door; }
    const HangarAccessSettings& Access() const { return access; }
    const vek::GarageDoorDefinition* Garage() const { return garages.Find(access.garageId); }
    const vek::PasslockDefinition* Passlock() const { return passlocks.Find(access.passlockId); }
    const vek::AnimationDefinition* Animation(const std::string& id) const { return animations.Find(id); }
    float AnimationDuration(const std::string& id,float fallback) const;

    bool BuildPasslockGui(const std::string& maskedInput,const std::string& status);
    const std::vector<vek::GuiCommand>& PasslockGuiCommands() const { return gui.Commands(); }
private:
    std::string file,error;
    HangarDoorSettings door;
    HangarAccessSettings access;
    vek::AnimationLibrary animations;
    vek::GarageDoorRegistry garages;
    vek::PasslockRegistry passlocks;
    vek::GuiSystem gui;
    std::unique_ptr<VekScriptEngine> vm;
    bool Load();
};
