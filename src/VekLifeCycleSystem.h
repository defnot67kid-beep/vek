#pragma once
#include <VekGameSystems.h>
#include <VekScriptEngine.h>
#include <memory>
#include <string>

class VekLifeCycleSystem {
public:
    bool Initialize(const std::string& path);
    bool Reload();
    const std::string& Error() const { return error; }

    const vek::DeathSequenceDefinition* ResetDeath() const { return deaths.Find(resetDeathId); }
    const vek::ScreenEffectDefinition* ResetScreenEffect() const {
        auto* d=ResetDeath(); return d?effects.Find(d->screenEffectId):nullptr;
    }
    const vek::AudioCueDefinition* ResetAudioCue() const {
        auto* d=ResetDeath(); return d?audio.Find(d->audioCueId):nullptr;
    }
    const vek::GroundingProfile* PlayerGrounding() const { return grounding.Find(groundingId); }
    float GroundRootY(float surfaceY,float avatarHeight) const;

private:
    bool Load();
    std::string file,error;
    std::string resetDeathId="player.reset";
    std::string groundingId="player.default";
    vek::AudioCueRegistry audio;
    vek::ScreenEffectRegistry effects;
    vek::DeathSequenceRegistry deaths;
    vek::GroundingRegistry grounding;
    std::unique_ptr<VekScriptEngine> vm;
};
