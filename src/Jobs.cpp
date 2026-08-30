#include "Jobs.h"
#include "raymath.h"
#include "VekJobRules.h"
#include <cmath>
void Jobs::Configure(const JobDefinition&d,float multiplier){jobName=d.name;objective=d.objective;difficulty=d.difficulty;cargoMass=d.cargoMass;reward=(int)std::lround(d.reward*multiplier);xpReward=d.xp;}
void Jobs::Accept(){if(Active())return;state=State::Pickup;toast=jobName+": drive to the yellow cargo marker.";toastTime=4;}
void Jobs::Update(Vector3 pos, EconomyState& economy){
    Vector3 p{0,0.4f,88},d{78,0.4f,85};
    if(state==State::Pickup&&Vector3Distance(pos,p)<5.5f){state=State::Dropoff;toast="Cargo loaded! Drive to the green delivery marker.";toastTime=4;}
    else if(state==State::Dropoff&&Vector3Distance(pos,d)<5.5f){state=State::Complete;economy.money+=reward;economy.xp+=xpReward;toast=TextFormat("Mission complete! +$%d +%d XP",reward,xpReward);toastTime=5;}
    if(toastTime>0)toastTime-=GetFrameTime();
}
Vector3 Jobs::Target(Vector3 p,Vector3 d)const{return state==State::Pickup?p:(state==State::Dropoff?d:Vector3{0,0,0});}
const char* Jobs::Label()const{return state==State::Pickup?"Cargo Pickup":(state==State::Dropoff?"Delivery Point":"");}
const char* Jobs::Task()const{return state==State::None?"Press J to accept a delivery job":(state==State::Pickup?"Travel to cargo pickup":(state==State::Dropoff?"Deliver cargo":"Mission complete - return to workshop"));}
bool Jobs::Active()const{return state==State::Pickup||state==State::Dropoff;}
