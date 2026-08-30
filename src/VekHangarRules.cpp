#include "VekHangarRules.h"
#include "VekSecuritySystem.h"
#include <VekScriptEngine.h>
#include <VekEditorSystems.h>
#include "VekHostSecurity.h"
static vek::VekVec3 V3(const VekValue&v,vek::VekVec3 f){if(!v.IsMap())return f;return{(float)v.Get("x").AsNumber(f.x),(float)v.Get("y").AsNumber(f.y),(float)v.Get("z").AsNumber(f.z)};}
bool VekHangarRules::Initialize(const std::string&p){file=p;return Load();}bool VekHangarRules::Reload(){return Load();}
bool VekHangarRules::Load(){auto verified=VekSecuritySystem::VerifyScript(file,file+".sig");if(!verified.ok){error=verified.error;return false;}VekScriptEngine vm;ApplyGameVekSecurityPolicy(vm);VekRegisterStandardLibrary(vm);vek::VekRegisterVehicleEditorLibrary(vm);vm.SealNativeRegistry();if(!vm.LoadSource(verified.source,file)){error=vm.LastError();return false;}auto v=vm.Call("hangar_config");if(!v.IsMap()){error="hangar.vek must return a map from hangar_config()";return false;}vek::HangarBuildArea a;a.center=V3(v.Get("center"),a.center);a.size=V3(v.Get("size"),a.size);a.gridSize=(float)v.Get("grid").AsNumber(a.gridSize);a.maxBuildHeight=(float)v.Get("max_build_height").AsNumber(a.maxBuildHeight);a.maxParts=(int)v.Get("max_parts").AsNumber(a.maxParts);a.allowLargeVehicles=v.Get("allow_large_vehicles").AsBool(a.allowLargeVehicles);a.allowAircraft=v.Get("allow_aircraft").AsBool(a.allowAircraft);if(a.size.x<20||a.size.z<20||a.maxBuildHeight<5){error="VEK hangar dimensions rejected by native safety limits";return false;}area=a;error.clear();return true;}
