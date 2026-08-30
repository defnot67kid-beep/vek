#include "Save.h"
#include "MapSystem.h"
#include "SecureSaveStore.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace {
std::filesystem::path Dir(){auto p=std::filesystem::current_path()/"saves";std::filesystem::create_directories(p);return p;}
}

bool Save::SaveBlueprint(const Vehicle&v){
    if(!v.finalized||v.parts.empty())return false;
    std::ostringstream o;
    o<<"VEKBP3\n"<<v.parts.size()<<"\n"<<v.powerMultiplier<<"\n";
    for(const auto&p:v.parts){
        o<<(int)p.type<<" "<<p.legacyType<<" "<<std::quoted(p.partId)<<" "<<std::quoted(p.displayName)<<" "<<std::quoted(p.category)<<" "<<std::quoted(p.visual)<<" "<<std::quoted(p.icon)<<" "<<std::quoted(p.viewModel)<<" "<<std::quoted(p.worldModel)<<" "<<std::quoted(p.material)<<" "<<(p.allowTint?1:0)<<"\n";
        o<<p.viewScale.x<<" "<<p.viewScale.y<<" "<<p.viewScale.z<<" "<<p.viewRotation.x<<" "<<p.viewRotation.y<<" "<<p.viewRotation.z<<" "<<p.viewOffset.x<<" "<<p.viewOffset.y<<" "<<p.viewOffset.z<<"\n";
        o<<p.localPosition.x<<" "<<p.localPosition.y<<" "<<p.localPosition.z<<" "<<p.size.x<<" "<<p.size.y<<" "<<p.size.z<<" "<<p.yaw<<" "<<p.pitch<<" "<<p.roll<<"\n";
        o<<p.mass<<" "<<p.power<<" "<<p.torque<<" "<<p.cost<<" "<<p.durability<<" "<<p.traction<<" "<<p.fuelCapacity<<" "<<p.batteryCapacity<<" "<<p.cargoCapacity<<" "<<p.wingArea<<" "<<p.liftEstimate<<" "<<p.drag<<" "<<p.thrust<<" "<<p.buoyancy<<"\n";
        o<<(p.isWheel?1:0)<<" "<<(p.isStructural?1:0)<<" "<<(p.isPowered?1:0)<<" "<<(p.isSeat?1:0)<<" "<<(p.isMovement?1:0)<<" "<<(int)p.paintR<<" "<<(int)p.paintG<<" "<<(int)p.paintB<<"\n";
    }
    return SecureSaveStore::Write(Dir()/"last_blueprint.vsave",o.str());
}

bool Save::LoadBlueprint(Vehicle&v){
    std::string text,error;if(!SecureSaveStore::Read(Dir()/"last_blueprint.vsave",text,&error))return false;
    std::istringstream i(text);std::string first;if(!(i>>first))return false;
    if(first=="VEKBP3"||first=="VEKBP2"){
        size_t n=0;if(!(i>>n>>v.powerMultiplier)||n>2500)return false;v.parts.clear();v.parts.reserve(n);
        for(size_t k=0;k<n;k++){
            int t=1000,legacy=-1;VehiclePart p;if(!(i>>t>>legacy>>std::quoted(p.partId)>>std::quoted(p.displayName)>>std::quoted(p.category)>>std::quoted(p.visual)))return false;
            if(first=="VEKBP3"){int tint=1;if(!(i>>std::quoted(p.icon)>>std::quoted(p.viewModel)>>std::quoted(p.worldModel)>>std::quoted(p.material)>>tint))return false;p.allowTint=tint!=0;if(!(i>>p.viewScale.x>>p.viewScale.y>>p.viewScale.z>>p.viewRotation.x>>p.viewRotation.y>>p.viewRotation.z>>p.viewOffset.x>>p.viewOffset.y>>p.viewOffset.z))return false;}
            else{p.icon=p.visual;p.viewModel=p.visual;p.worldModel=p.visual;p.material="legacy";p.allowTint=true;}
            p.type=(t>=0&&t<=27)?(PartType)t:PartType::Dynamic;p.legacyType=legacy;
            if(!(i>>p.localPosition.x>>p.localPosition.y>>p.localPosition.z>>p.size.x>>p.size.y>>p.size.z>>p.yaw>>p.pitch>>p.roll))return false;
            if(!(i>>p.mass>>p.power>>p.torque>>p.cost>>p.durability>>p.traction>>p.fuelCapacity>>p.batteryCapacity>>p.cargoCapacity>>p.wingArea>>p.liftEstimate>>p.drag>>p.thrust>>p.buoyancy))return false;
            int iw=0,is=0,ip=0,iseat=0,im=0,r=150,g=160,b=165;if(!(i>>iw>>is>>ip>>iseat>>im>>r>>g>>b))return false;p.isWheel=iw!=0;p.isStructural=is!=0;p.isPowered=ip!=0;p.isSeat=iseat!=0;p.isMovement=im!=0;p.paintR=(unsigned char)std::clamp(r,0,255);p.paintG=(unsigned char)std::clamp(g,0,255);p.paintB=(unsigned char)std::clamp(b,0,255);v.parts.push_back(std::move(p));
        }
        v.finalized=v.ValidBuild();v.Reset();return v.finalized;
    }
    // Legacy v1-v12 format: first token was the part count.
    size_t n=0;try{n=(size_t)std::stoull(first);}catch(...){return false;}if(n>512||!(i>>v.powerMultiplier))return false;v.parts.clear();
    for(size_t k=0;k<n;k++){int t;VehiclePart p;if(!(i>>t>>p.localPosition.x>>p.localPosition.y>>p.localPosition.z>>p.yaw>>p.mass>>p.power>>p.cost))return false;if(t<0||t>27)return false;p.type=(PartType)t;p.legacyType=t;p.partId="legacy."+std::to_string(t);p.displayName="Legacy Part";p.isWheel=PartIsWheel(p.type);p.isStructural=PartIsStructural(p.type);p.isPowered=PartIsPowered(p.type);p.isSeat=p.type==PartType::Seat;p.isMovement=PartIsMovement(p.type);v.parts.push_back(p);}
    v.finalized=v.ValidBuild();v.Reset();return v.finalized;
}

void Save::SaveCareer(const EconomyState&e){
    if(!e.IntegrityOK())return;
    std::ostringstream o;o<<e.money.Get()<<" "<<e.xp.Get()<<" "<<e.reputation.Get()<<"\n";
    SecureSaveStore::Write(Dir()/"career.vsave",o.str());
}

void Save::LoadCareer(EconomyState&e){
    std::string text,error;
    if(!SecureSaveStore::Read(Dir()/"career.vsave",text,&error)) return;
    std::istringstream i(text);int money=1500,xp=0,rep=0;
    if(i>>money>>xp>>rep){
        // Sanity limits catch corrupted/absurd values even after authentication.
        if(money<0||money>100000000||xp<0||xp>100000000||rep<0||rep>1000000)return;
        e.money=money;e.xp=xp;e.reputation=rep;
    }
}

bool Save::SaveMapData(const MapSaveData& d) {
    std::ostringstream o;
    o<<d.version<<"\n";
    o<<(int)d.orientation<<" "<<(d.hasWaypoint?1:0)<<" "<<d.waypoint.x<<" "<<d.waypoint.y<<" "<<d.waypoint.z<<"\n";
    o<<d.worldMapZoom<<" "<<d.worldMapPan.x<<" "<<d.worldMapPan.y<<"\n";
    o<<(d.filters.jobs?1:0)<<" "<<(d.filters.services?1:0)<<" "<<(d.filters.vehicles?1:0)<<" "<<(d.filters.events?1:0)<<"\n";
    o<<d.trackedMarkerId<<"\n";
    o<<d.discoveredMarkerIds.size();for(int id:d.discoveredMarkerIds)o<<" "<<id;o<<"\n";
    o<<d.discoveredRegionIds.size();for(int id:d.discoveredRegionIds)o<<" "<<id;o<<"\n";
    o<<d.fastTravelMarkerIds.size();for(int id:d.fastTravelMarkerIds)o<<" "<<id;o<<"\n";
    return SecureSaveStore::Write(Dir()/"map_state.vsave",o.str());
}

bool Save::LoadMapData(MapSaveData& d) {
    std::string text,error;if(!SecureSaveStore::Read(Dir()/"map_state.vsave",text,&error))return false;
    std::istringstream i(text);
    int orientation=0,waypoint=0,jobs=1,services=1,vehicles=1,events=1;
    i>>d.version;
    i>>orientation>>waypoint>>d.waypoint.x>>d.waypoint.y>>d.waypoint.z;
    i>>d.worldMapZoom>>d.worldMapPan.x>>d.worldMapPan.y;
    i>>jobs>>services>>vehicles>>events;
    i>>d.trackedMarkerId;
    d.orientation=orientation==(int)MinimapOrientation::NorthUp?MinimapOrientation::NorthUp:MinimapOrientation::PlayerUp;
    d.hasWaypoint=waypoint!=0;
    d.filters.jobs=jobs!=0;d.filters.services=services!=0;d.filters.vehicles=vehicles!=0;d.filters.events=events!=0;
    size_t count=0;int id=0;
    i>>count;if(count>10000)return false;d.discoveredMarkerIds.clear();d.discoveredMarkerIds.reserve(count);for(size_t n=0;n<count&&i>>id;++n)d.discoveredMarkerIds.push_back(id);
    i>>count;if(count>1000)return false;d.discoveredRegionIds.clear();d.discoveredRegionIds.reserve(count);for(size_t n=0;n<count&&i>>id;++n)d.discoveredRegionIds.push_back(id);
    i>>count;if(count>10000)return false;d.fastTravelMarkerIds.clear();d.fastTravelMarkerIds.reserve(count);for(size_t n=0;n<count&&i>>id;++n)d.fastTravelMarkerIds.push_back(id);
    return i.good()||i.eof();
}
