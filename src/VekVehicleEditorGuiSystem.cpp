#include "VekVehicleEditorGuiSystem.h"
#include "VekSecuritySystem.h"
#include "VekGuiTextRenderer.h"
#include "PartPresentationRenderer.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include "VekHostSecurity.h"

bool VekVehicleEditorGuiSystem::Initialize(const std::string&p){file=p;return Load();}
bool VekVehicleEditorGuiSystem::Reload(){return Load();}
bool VekVehicleEditorGuiSystem::Load(){VekScriptEngine fresh;ApplyGameVekSecurityPolicy(fresh);VekRegisterStandardLibrary(fresh);gui.RegisterNatives(fresh);fresh.SealNativeRegistry();auto verified=VekSecuritySystem::VerifyScript(file,file+".sig");if(!verified.ok){error=verified.error;return false;}if(!fresh.LoadSource(verified.source,file)){error=fresh.LastError();return false;}if(!fresh.HasFunction("build_editor_ui")){error="vehicle_editor_gui.vek missing build_editor_ui";return false;}engine=std::move(fresh);error.clear();cacheKey.clear();return true;}
static VekValue V3(float x,float y,float z){VekMap m;m["x"]=x;m["y"]=y;m["z"]=z;return VekValue(std::move(m));}
VekValue VekVehicleEditorGuiSystem::BuildPartValue(const vek::PartDefinition&p,int level,vek::GameMode mode)const{VekMap m;m["id"]=p.id;m["display_name"]=p.displayName;m["description"]=p.description;m["category"]=p.category;m["subcategory"]=p.subcategory;m["mass"]=p.mass;m["price"]=mode==vek::GameMode::Sandbox?0:p.price;m["durability"]=p.durability;m["unlock_level"]=p.unlockLevel;m["technology"]=p.requiredTechnology;m["locked"]=mode!=vek::GameMode::Sandbox&&(p.sandboxOnly||level<p.unlockLevel);m["size"]=V3(p.size.x,p.size.y,p.size.z);m["visual"]=p.visual;m["icon"]=p.presentation.icon;m["view_model"]=p.presentation.viewModel;m["world_model"]=p.presentation.worldModel;m["material"]=p.presentation.material;return VekValue(std::move(m));}
VehicleEditorGuiActions VekVehicleEditorGuiSystem::UpdateAndDraw(vek::GameMode mode,const std::string&search,const std::string&category,const std::string&selected,const std::vector<const vek::PartDefinition*>&parts,int level){
 std::string key=category+"|"+search+"|"+std::to_string(level)+"|"+std::to_string((int)mode)+"|"+std::to_string(parts.size());for(auto*p:parts)key+=p->id+"|"+p->presentation.icon+"|"+p->presentation.viewModel+"|"+p->presentation.worldModel;
 if(key!=cacheKey){VekArray a;a.reserve(parts.size());for(auto*p:parts)a.push_back(BuildPartValue(*p,level,mode));cachedParts=VekValue(std::move(a));cacheKey=key;}
 gui.BeginFrame();auto r=engine.Call("build_editor_ui",{mode==vek::GameMode::Sandbox?"sandbox":"survival",search,category,selected,cachedParts});if(!engine.LastError().empty())error=engine.LastError();gui.EndFrame();auto act=Render(search);if(r.IsString())act.search=r.AsString();return act;
}
bool VekVehicleEditorGuiSystem::MouseOverUI(Vector2 m)const{return m.x>=8&&m.x<=458&&m.y>=8&&m.y<=GetScreenHeight()-8;}
VehicleEditorGuiActions VekVehicleEditorGuiSystem::Render(const std::string&currentSearch){
 VehicleEditorGuiActions action;action.search=currentSearch;Vector2 mouse=GetMousePosition();bool click=IsMouseButtonPressed(MOUSE_BUTTON_LEFT);float wheel=GetMouseWheelMove();if(MouseOverUI(mouse)&&fabsf(wheel)>0.01f)scrollOffset=std::clamp(scrollOffset-wheel*42.0f,0.0f,6000.0f);
 Rectangle window{10,10,440.0f,(float)GetScreenHeight()-20};DrawRectangleRounded(window,0.025f,8,Color{10,17,22,247});DrawRectangleRoundedLinesEx(window,0.025f,8,1.5f,Color{65,108,118,255});
 float x=24,y=24;int catIndex=0;int nonCardButton=0;int cardIndex=0;int cardLine=0;bool inCard=false,inScroll=false;Rectangle card{};std::unordered_map<std::string,Rectangle> hitRects;std::string tooltip;
 float scrollTop=150.0f,scrollBottom=window.y+window.height-155.0f;
 for(const auto&c:gui.Commands()){
  switch(c.type){
   case vek::GuiCommandType::BeginWindow:{auto tp=c.textPolicy;tp.fontSize=23;tp.maxFontSize=23;tp.minFontSize=14;tp.maxLines=1;tp.wrap=false;VekGuiTextRenderer::DrawTextAuto(c.text,{x,y,405,30},tp,RAYWHITE,true);y+=36;break;}
   case vek::GuiCommandType::BeginVertical:case vek::GuiCommandType::EndVertical:break;
   case vek::GuiCommandType::BeginHorizontal:catIndex=0;nonCardButton=0;break;
   case vek::GuiCommandType::EndHorizontal:y+=36;break;
   case vek::GuiCommandType::BeginScrollPanel:inScroll=true;scrollTop=y;scrollBottom=window.y+window.height-150;BeginScissorMode((int)window.x+8,(int)scrollTop,(int)window.width-16,(int)(scrollBottom-scrollTop));break;
   case vek::GuiCommandType::EndScrollPanel:if(inScroll){EndScissorMode();inScroll=false;}y=scrollBottom+8;break;
   case vek::GuiCommandType::BeginGrid:cardIndex=0;break;
   case vek::GuiCommandType::EndGrid:break;
   case vek::GuiCommandType::BeginPanel:{if(c.id.rfind("card_",0)==0){int col=cardIndex%2,row=cardIndex/2;card={x+col*198.0f,scrollTop+row*154.0f-scrollOffset,190,146};cardIndex++;cardLine=0;inCard=true;if(card.y+card.height>=scrollTop&&card.y<=scrollBottom){DrawRectangleRounded(card,0.08f,6,Color{20,31,37,245});DrawRectangleRoundedLinesEx(card,0.08f,6,1,Color{52,81,90,255});}}break;}
   case vek::GuiCommandType::EndPanel:inCard=false;break;
   case vek::GuiCommandType::SearchBox:{Rectangle r{x,y,405,34};DrawRectangleRounded(r,0.18f,6,Color{22,34,40,255});DrawRectangleRoundedLinesEx(r,0.18f,6,1,Color{65,108,118,255});std::string txt=c.text;if(activeTextId==c.id){int ch=0;while((ch=GetCharPressed())>0)if(ch>=32&&ch<127&&txt.size()<64)txt.push_back((char)ch);if(IsKeyPressed(KEY_BACKSPACE)&&!txt.empty())txt.pop_back();gui.SetText(c.id,txt);action.search=txt;}auto tp=c.textPolicy;tp.fontSize=16;tp.maxFontSize=16;tp.minFontSize=10;tp.maxLines=1;tp.wrap=false;VekGuiTextRenderer::DrawTextAuto(txt.empty()?"SEARCH PARTS...":txt,{r.x+10,r.y+4,r.width-20,r.height-8},tp,txt.empty()?GRAY:RAYWHITE,true);if(click&&CheckCollisionPointRec(mouse,r))activeTextId=c.id;hitRects[c.id]=r;y+=42;break;}
   case vek::GuiCommandType::ImageButton:{if(inCard){Rectangle r{card.x+8,card.y+8,42,42};if(r.y+42>=scrollTop&&r.y<=scrollBottom)PartPresentationRenderer::DrawCatalogIcon(r,c.aux,c.text,false);hitRects[c.id]=r;}break;}
   case vek::GuiCommandType::Label:{if(inCard){float lx=card.x+58,ly=card.y+8+cardLine*20;Color col=cardLine==0?RAYWHITE:LIGHTGRAY;if(ly>=scrollTop&&ly<scrollBottom){auto tp=c.textPolicy;tp.fontSize=cardLine==0?13:11;tp.maxFontSize=tp.fontSize;tp.minFontSize=8;tp.maxLines=1;tp.wrap=false;tp.clip=false;VekGuiTextRenderer::DrawTextAuto(c.text,{lx,ly,card.x+card.width-8-lx,18},tp,col,true);}cardLine++;}else{auto tp=c.textPolicy;tp.fontSize=14;tp.maxFontSize=14;tp.minFontSize=9;tp.maxLines=2;tp.clip=false;VekGuiTextRenderer::DrawTextAuto(c.text,{x,y,405,36},tp,LIGHTGRAY,false);y+=std::max(19.0f,tp.maxLines>1?32.0f:19.0f);}break;}
   case vek::GuiCommandType::Button:{Rectangle r;if(c.id.rfind("cat_",0)==0){int col=catIndex%5,row=catIndex/5;r={x+col*79.0f,y+row*31.0f,74,27};catIndex++;}else if(inCard){r={card.x+8,card.y+108,174,28};}else{r={x+nonCardButton*100.0f,y,94,30};nonCardButton++;}if(!inScroll||r.y+30>=scrollTop&&r.y<=scrollBottom){Color bc=c.id.rfind("construct",0)==0?Color{43,126,91,255}:Color{35,80,90,255};DrawRectangleRounded(r,0.14f,6,bc);auto tp=c.textPolicy;tp.fontSize=12;tp.maxFontSize=12;tp.minFontSize=8;tp.maxLines=1;tp.wrap=false;tp.align=vek::GuiTextAlign::Center;tp.clip=!inScroll;VekGuiTextRenderer::DrawTextAuto(c.text,{r.x+5,r.y+3,r.width-10,r.height-6},tp,RAYWHITE,true);if(click&&CheckCollisionPointRec(mouse,r)){gui.SetPressed(c.id,true);if(c.id.rfind("part_",0)==0)action.selectedPartId=c.id.substr(5);else if(c.id.rfind("cat_",0)==0)action.category=c.id.substr(4);else action.command=c.id;}}hitRects[c.id]=r;break;}
   case vek::GuiCommandType::Tooltip:{auto it=hitRects.find(c.id);if(it!=hitRects.end()&&CheckCollisionPointRec(mouse,it->second))tooltip=c.text;break;}
   case vek::GuiCommandType::Separator:if(!inScroll){DrawLine((int)x,(int)y,(int)(x+405),(int)y,Color{58,78,84,255});y+=8;}break;
   case vek::GuiCommandType::Spacer:if(!inScroll)y+=c.value;break;
   default:break;
  }
 }
 if(inScroll)EndScissorMode();
 if(!tooltip.empty()){float w=std::min(360.0f,std::max(180.0f,(float)MeasureText(tooltip.c_str(),13)+20.0f));Rectangle tr{std::min(mouse.x+14,(float)GetScreenWidth()-w-8),std::min(mouse.y+14,(float)GetScreenHeight()-74),w,62};DrawRectangleRounded(tr,0.12f,5,Color{5,10,13,245});vek::GuiTextPolicy tp;tp.fontSize=13;tp.minFontSize=9;tp.maxFontSize=13;tp.maxLines=3;tp.wrap=true;tp.ellipsis=true;tp.clip=true;VekGuiTextRenderer::DrawTextAuto(tooltip,{tr.x+8,tr.y+6,tr.width-16,tr.height-12},tp,RAYWHITE,false);}
 return action;
}

