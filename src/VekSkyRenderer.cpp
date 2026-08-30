#include "VekSkyRenderer.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
static Color ToColor(const vek::SkyboxDefinition::Color& c){return{(unsigned char)std::clamp(c.r,0.0f,255.0f),(unsigned char)std::clamp(c.g,0.0f,255.0f),(unsigned char)std::clamp(c.b,0.0f,255.0f),(unsigned char)std::clamp(c.a,0.0f,255.0f)};}
void VekSkyRenderer::DrawBackground(const Camera3D& camera,const vek::SkyboxDefinition& sky) const{
    int w=GetScreenWidth(),h=GetScreenHeight();
    Color zen=ToColor(sky.zenith),hor=ToColor(sky.horizon),ground=ToColor(sky.ground);
    int horizonY=(int)std::clamp(sky.horizonHeight*(float)h,0.15f*(float)h,0.88f*(float)h);
    if(horizonY>0)DrawRectangleGradientV(0,0,w,horizonY,zen,hor);
    if(horizonY<h)DrawRectangleGradientV(0,horizonY,w,h-horizonY,hor,ground);
    float yaw=sky.sunYaw*DEG2RAD,pitch=sky.sunPitch*DEG2RAD;
    Vector3 dir{std::sinf(yaw)*std::cosf(pitch),std::sinf(pitch),std::cosf(yaw)*std::cosf(pitch)};
    Vector3 sunWorld=Vector3Add(camera.position,Vector3Scale(dir,350.0f));
    Vector2 sunScreen=GetWorldToScreen(sunWorld,camera);
    if(sunScreen.x>-100&&sunScreen.x<w+100&&sunScreen.y>-100&&sunScreen.y<h+100){
        DrawCircleV(sunScreen,std::clamp(sky.sunSize,2.0f,60.0f),ToColor(sky.sun));
        DrawCircleLines((int)sunScreen.x,(int)sunScreen.y,(int)std::clamp(sky.sunSize*1.55f,3.0f,90.0f),Fade(ToColor(sky.sun),0.35f));
    }
}
