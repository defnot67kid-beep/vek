#pragma once
#include "Vehicle.h"
#include "Types.h"

struct MapSaveData;

class Save {
public:
    static bool SaveBlueprint(const Vehicle& v);
    static bool LoadBlueprint(Vehicle& v);
    static void SaveCareer(const EconomyState& e);
    static void LoadCareer(EconomyState& e);
    static bool SaveMapData(const MapSaveData& data);
    static bool LoadMapData(MapSaveData& data);
};
