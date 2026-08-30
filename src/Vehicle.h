#pragma once
#include "Types.h"

class Vehicle {
public:
    std::vector<VehiclePart> parts;
    Vector3 position{0.0f,0.7f,10.0f};
    Vector3 velocity{0,0,0};
    float heading = 0.0f;
    float fuel = 100.0f;
    float health = 100.0f;
    float powerMultiplier = 1.0f;
    bool finalized = false;

    float TotalMass() const;
    float TotalPower() const;
    int WheelCount() const;
    bool HasEngine() const;
    bool HasSeat() const;
    bool ValidBuild() const;
    float SpeedKph() const;
    void UpdateDriving(float dt);
    void Draw() const;
    void Reset();
    void Upgrade();
    Vector3 SeatWorldPosition() const;
    Vector3 EntryWorldPosition() const;
    Vector3 ForwardWorld() const;
    Vector3 RightWorld() const;
private:
    Vector3 Forward() const;
};
