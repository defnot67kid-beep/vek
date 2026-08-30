#include "VekGuiTextRenderer.h"
#include <algorithm>
#include <cmath>

namespace VekGuiTextRenderer {

void DrawTextAuto(const std::string& text, Rectangle bounds, const vek::GuiTextPolicy& policy, Color color, bool verticalCenter) {
    if(bounds.width<=1.0f || bounds.height<=1.0f) return;
    auto measure=[](const std::string& value,float size)->float{
        return (float)MeasureText(value.c_str(),std::max(1,(int)std::round(size)));
    };
    auto layout=vek::GuiTextLayoutSystem::Layout(text,bounds.width,bounds.height,policy,measure);
    int fontSize=std::max(1,(int)std::round(layout.fontSize));
    float lineStep=layout.fontSize*std::clamp(policy.lineHeight,0.8f,2.5f);
    float totalHeight=layout.lines.empty()?0.0f:lineStep*(float)layout.lines.size();
    float y=verticalCenter?bounds.y+(bounds.height-totalHeight)*0.5f:bounds.y;

    if(policy.clip) BeginScissorMode((int)std::floor(bounds.x),(int)std::floor(bounds.y),std::max(1,(int)std::ceil(bounds.width)),std::max(1,(int)std::ceil(bounds.height)));
    for(const auto& line:layout.lines){
        float width=measure(line,layout.fontSize);
        float x=bounds.x;
        if(policy.align==vek::GuiTextAlign::Center)x=bounds.x+(bounds.width-width)*0.5f;
        else if(policy.align==vek::GuiTextAlign::Right)x=bounds.x+bounds.width-width;
        DrawText(line.c_str(),(int)std::round(x),(int)std::round(y),fontSize,color);
        y+=lineStep;
    }
    if(policy.clip) EndScissorMode();
}

}
