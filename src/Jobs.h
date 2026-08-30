#pragma once
#include "raylib.h"
#include <string>
#include "Types.h"
struct JobDefinition;
class Jobs {
public:
    enum class State{None,Pickup,Dropoff,Complete}; State state=State::None;
    int reward=500,xpReward=100,difficulty=2;float cargoMass=450;std::string jobName="Workshop Supplies";std::string objective="Deliver workshop supplies"; std::string toast; float toastTime=0;
    void Configure(const JobDefinition& definition,float rewardMultiplier=1.0f);void Accept(); void Update(Vector3 pos, EconomyState& economy);
    Vector3 Target(Vector3 pickup,Vector3 dropoff) const;
    const char* Label() const; const char* Task() const; bool Active() const;
};
