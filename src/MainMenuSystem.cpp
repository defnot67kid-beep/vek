#include "MainMenuSystem.h"
#include "VekSecuritySystem.h"
#include "VekGuiTextRenderer.h"
#include <algorithm>
#include "VekHostSecurity.h"

Rectangle MainMenuSystem::SurvivalRect() const {
    float w=std::min(440.0f,GetScreenWidth()*0.44f), h=74.0f;
    return {GetScreenWidth()*0.5f-w*0.5f,GetScreenHeight()*0.52f-h,w,h};
}
Rectangle MainMenuSystem::SandboxRect() const {
    Rectangle a=SurvivalRect();a.y+=96.0f;return a;
}

bool MainMenuSystem::Initialize(const std::string& scriptPath) {
    scriptFile=scriptPath;
    VekScriptEngine fresh;
    ApplyGameVekSecurityPolicy(fresh);
    vek::VekRegisterGameplayLibrary(fresh);
    vek::VekRegisterVehicleEditorLibrary(fresh);
    gui.RegisterNatives(fresh);
    fresh.SealNativeRegistry();
    VekVerifiedScript verified=VekSecuritySystem::VerifyScript(scriptFile,scriptFile+".sig");
    if(!verified.ok){error=verified.error;return false;}
    if(!fresh.LoadSource(verified.source,scriptFile)){error=fresh.LastError();return false;}
    if(!fresh.HasFunction("main_menu")){error="VEK main_menu.vek missing fn main_menu";return false;}
    engine=std::move(fresh);error.clear();return true;
}

void MainMenuSystem::Update() {
    if(!open)return;
    Vector2 mouse=GetMousePosition();
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
        if(CheckCollisionPointRec(mouse,SurvivalRect())) gui.SetPressed("survival",true);
        if(CheckCollisionPointRec(mouse,SandboxRect())) gui.SetPressed("sandbox",true);
    }
    gui.BeginFrame();
    VekValue result=engine.Call("main_menu");
    gui.EndFrame();
    if(!engine.LastError().empty()){error=engine.LastError();return;}
    std::string choice=result.AsString();
    if(choice=="survival"){mode=vek::GameMode::Survival;selected=true;open=false;}
    else if(choice=="sandbox"){mode=vek::GameMode::Sandbox;selected=true;open=false;}
}

void MainMenuSystem::Draw() const {
    if(!open)return;
    DrawRectangleGradientV(0,0,GetScreenWidth(),GetScreenHeight(),Color{9,16,24,255},Color{22,31,39,255});
    // Original engineering-grid identity: subtle lines, not copied from another game.
    for(int x=0;x<GetScreenWidth();x+=48)DrawLine(x,0,x,GetScreenHeight(),Color{40,60,70,45});
    for(int y=0;y<GetScreenHeight();y+=48)DrawLine(0,y,GetScreenWidth(),y,Color{40,60,70,45});

    const auto& commands=gui.Commands();
    std::string title="VEK VEHICLE ENGINEERING";
    std::string subtitle="Choose your engineering career";
    std::string survivalText="SURVIVAL";
    std::string sandboxText="SANDBOX";
    for(const auto& c:commands){
        if(c.type==vek::GuiCommandType::BeginWindow&&!c.text.empty())title=c.text;
        else if(c.type==vek::GuiCommandType::Label&&!c.text.empty())subtitle=c.text;
        else if(c.type==vek::GuiCommandType::Button&&c.id=="survival")survivalText=c.text;
        else if(c.type==vek::GuiCommandType::Button&&c.id=="sandbox")sandboxText=c.text;
    }

    vek::GuiTextPolicy titlePolicy;titlePolicy.fontSize=42;titlePolicy.minFontSize=22;titlePolicy.maxFontSize=42;titlePolicy.maxLines=1;titlePolicy.wrap=false;titlePolicy.align=vek::GuiTextAlign::Center;
    vek::GuiTextPolicy subtitlePolicy;subtitlePolicy.fontSize=20;subtitlePolicy.minFontSize=13;subtitlePolicy.maxFontSize=20;subtitlePolicy.maxLines=2;subtitlePolicy.align=vek::GuiTextAlign::Center;
    VekGuiTextRenderer::DrawTextAuto(title,{24.0f,GetScreenHeight()*0.19f,(float)GetScreenWidth()-48.0f,70.0f},titlePolicy,RAYWHITE,true);
    VekGuiTextRenderer::DrawTextAuto(subtitle,{36.0f,GetScreenHeight()*0.30f,(float)GetScreenWidth()-72.0f,46.0f},subtitlePolicy,Color{170,195,202,255},true);

    auto drawButton=[&](Rectangle r,const std::string& text,const char* detail){
        bool hover=CheckCollisionPointRec(GetMousePosition(),r);
        Color fill=hover?Color{45,105,116,255}:Color{29,59,67,245};
        DrawRectangleRounded(r,0.13f,12,fill);DrawRectangleRoundedLinesEx(r,0.13f,12,2.0f,hover?SKYBLUE:Color{70,105,112,255});
        vek::GuiTextPolicy buttonPolicy;buttonPolicy.fontSize=25;buttonPolicy.minFontSize=16;buttonPolicy.maxFontSize=25;buttonPolicy.maxLines=1;buttonPolicy.wrap=false;buttonPolicy.align=vek::GuiTextAlign::Left;
        vek::GuiTextPolicy detailPolicy;detailPolicy.fontSize=14;detailPolicy.minFontSize=10;detailPolicy.maxFontSize=14;detailPolicy.maxLines=2;detailPolicy.wrap=true;detailPolicy.align=vek::GuiTextAlign::Left;
        VekGuiTextRenderer::DrawTextAuto(text,{r.x+24,r.y+7,r.width-48,34},buttonPolicy,RAYWHITE,true);
        VekGuiTextRenderer::DrawTextAuto(detail,{r.x+24,r.y+40,r.width-48,r.height-45},detailPolicy,Color{182,204,209,255},false);
    };
    drawButton(SurvivalRect(),survivalText,"Earn money, unlock parts, pay for builds and repairs");
    drawButton(SandboxRect(),sandboxText,"Unlimited parts, zero build cost, all editor technology unlocked");
    vek::GuiTextPolicy foot;foot.fontSize=14;foot.minFontSize=10;foot.maxLines=1;foot.wrap=false;
    VekGuiTextRenderer::DrawTextAuto("Main-menu layout and mode choice are authored by signed VEK rules.",{24.0f,(float)GetScreenHeight()-42.0f,(float)GetScreenWidth()-48.0f,28.0f},foot,Color{120,145,150,255},true);
    if(!error.empty()){vek::GuiTextPolicy ep=foot;ep.fontSize=16;ep.maxLines=2;ep.wrap=true;VekGuiTextRenderer::DrawTextAuto(error,{24,20,(float)GetScreenWidth()-48,44},ep,RED,false);}
}
