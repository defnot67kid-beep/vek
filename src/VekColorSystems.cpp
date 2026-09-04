#include <vek/VekColorSystems.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <unordered_map>

namespace vek {
namespace {
float C(float v){ return std::clamp(std::isfinite(v)?v:0.0f, 0.0f, 1.0f); }
int HexNibble(char c){ if(c>='0'&&c<='9')return c-'0'; if(c>='a'&&c<='f')return 10+c-'a'; if(c>='A'&&c<='F')return 10+c-'A'; return -1; }
int HexByte(const std::string&s,std::size_t i){ if(i+1>=s.size())return -1;int a=HexNibble(s[i]),b=HexNibble(s[i+1]);return a<0||b<0?-1:a*16+b; }
float Hue(float p,float q,float t){ if(t<0)t+=1;if(t>1)t-=1;if(t<1.0f/6)return p+(q-p)*6*t;if(t<.5f)return q;if(t<2.0f/3)return p+(q-p)*(2.0f/3-t)*6;return p; }
std::string Lower(std::string s){ for(char&c:s){c=(char)std::tolower((unsigned char)c);if(c=='-')c='_';}return s; }
}

VekColor ParseColorHex(const std::string& text,VekColor fallback){
    std::string s=text;if(!s.empty()&&s[0]=='#')s.erase(s.begin());
    if(s.size()==3||s.size()==4){std::string e;for(char c:s){e.push_back(c);e.push_back(c);}s=e;}
    if(s.size()!=6&&s.size()!=8)return fallback;int r=HexByte(s,0),g=HexByte(s,2),b=HexByte(s,4),a=s.size()==8?HexByte(s,6):255;if(r<0||g<0||b<0||a<0)return fallback;
    return {r/255.0f,g/255.0f,b/255.0f,a/255.0f};
}

VekColor NamedVekColor(const std::string& name,VekColor fallback){
    static const std::unordered_map<std::string,VekColor> colors={
        {"vek_blue",ParseColorHex("#69A9FF")},{"vek_cyan",ParseColorHex("#59D6E8")},{"vek_teal",ParseColorHex("#55C8B3")},
        {"vek_green",ParseColorHex("#59BE80")},{"vek_lime",ParseColorHex("#A3D65C")},{"vek_yellow",ParseColorHex("#E8C75B")},
        {"vek_orange",ParseColorHex("#F29A5B")},{"vek_red",ParseColorHex("#E05D65")},{"vek_pink",ParseColorHex("#E879B7")},
        {"vek_violet",ParseColorHex("#A78BFA")},{"vek_white",ParseColorHex("#F7F8FA")},{"vek_black",ParseColorHex("#111316")},
        {"vek_grey_50",ParseColorHex("#F5F6F7")},{"vek_grey_100",ParseColorHex("#E4E6E9")},{"vek_grey_200",ParseColorHex("#C9CDD2")},
        {"vek_grey_300",ParseColorHex("#AEB4BC")},{"vek_grey_400",ParseColorHex("#8A929D")},{"vek_grey_500",ParseColorHex("#6B737E")},
        {"vek_grey_600",ParseColorHex("#525963")},{"vek_grey_700",ParseColorHex("#3E444C")},{"vek_grey_800",ParseColorHex("#2B2F35")},
        {"vek_grey_900",ParseColorHex("#1B1E22")}
    };auto it=colors.find(Lower(name));return it==colors.end()?fallback:it->second;
}

VekColor MixColor(VekColor a,VekColor b,float t){t=C(t);return{a.r+(b.r-a.r)*t,a.g+(b.g-a.g)*t,a.b+(b.b-a.b)*t,a.a+(b.a-a.a)*t};}
VekColor WithColorAlpha(VekColor c,float alpha){c.a=C(alpha);return c;}
VekColor LightenColor(VekColor c,float amount){return MixColor(c,{1,1,1,c.a},C(amount));}
VekColor DarkenColor(VekColor c,float amount){return MixColor(c,{0,0,0,c.a},C(amount));}
VekColor HslColor(float h,float s,float l,float a){h=std::fmod(h,360.0f);if(h<0)h+=360;h/=360.0f;s=C(s);l=C(l);a=C(a);if(s<=0)return{l,l,l,a};float q=l<.5f?l*(1+s):l+s-l*s,p=2*l-q;return{Hue(p,q,h+1.0f/3),Hue(p,q,h),Hue(p,q,h-1.0f/3),a};}
float RelativeLuminance(VekColor c){auto f=[](float v){v=C(v);return v<=.04045f?v/12.92f:std::pow((v+.055f)/1.055f,2.4f);};return .2126f*f(c.r)+.7152f*f(c.g)+.0722f*f(c.b);}
float ContrastRatio(VekColor a,VekColor b){float x=RelativeLuminance(a),y=RelativeLuminance(b);if(x<y)std::swap(x,y);return(x+.05f)/(y+.05f);}
std::string ColorToHex(VekColor c,bool alpha){char out[10]{};int r=(int)std::lround(C(c.r)*255),g=(int)std::lround(C(c.g)*255),b=(int)std::lround(C(c.b)*255),a=(int)std::lround(C(c.a)*255);if(alpha)std::snprintf(out,sizeof(out),"#%02X%02X%02X%02X",r,g,b,a);else std::snprintf(out,sizeof(out),"#%02X%02X%02X",r,g,b);return out;}
VekValue ColorToValue(VekColor c){VekMap m;m["r"]=(double)std::lround(C(c.r)*255);m["g"]=(double)std::lround(C(c.g)*255);m["b"]=(double)std::lround(C(c.b)*255);m["a"]=(double)std::lround(C(c.a)*255);m["hex"]=ColorToHex(c,true);return VekValue(std::move(m));}
VekColor ColorFromValue(const VekValue&v,VekColor fallback){if(v.IsString()){auto named=NamedVekColor(v.AsString(),{-1,-1,-1,-1});if(named.r>=0)return named;return ParseColorHex(v.AsString(),fallback);}if(!v.IsMap())return fallback;auto ch=[&](const char*k,float d){return C((float)v.Get(k).AsNumber(d*255.0)/255.0f);};return{ch("r",fallback.r),ch("g",fallback.g),ch("b",fallback.b),ch("a",fallback.a)};}

void VekRegisterColorLibrary(VekScriptEngine&e){
    e.RegisterNative("color_hex",[](const std::vector<VekValue>&a){return ColorToValue(ParseColorHex(a.empty()?"#000000":a[0].AsString()));});
    e.RegisterNative("color_named",[](const std::vector<VekValue>&a){return ColorToValue(NamedVekColor(a.empty()?"vek_blue":a[0].AsString(),NamedVekColor("vek_blue")));});
    e.RegisterNative("color_mix",[](const std::vector<VekValue>&a){if(a.size()<3)return VekValue();return ColorToValue(MixColor(ColorFromValue(a[0]),ColorFromValue(a[1]),(float)a[2].AsNumber()));});
    e.RegisterNative("color_alpha",[](const std::vector<VekValue>&a){if(a.size()<2)return VekValue();double x=a[1].AsNumber(1);if(x>1)x/=255.0;return ColorToValue(WithColorAlpha(ColorFromValue(a[0]),(float)x));});
    e.RegisterNative("color_lighten",[](const std::vector<VekValue>&a){if(a.size()<2)return VekValue();return ColorToValue(LightenColor(ColorFromValue(a[0]),(float)a[1].AsNumber()));});
    e.RegisterNative("color_darken",[](const std::vector<VekValue>&a){if(a.size()<2)return VekValue();return ColorToValue(DarkenColor(ColorFromValue(a[0]),(float)a[1].AsNumber()));});
    e.RegisterNative("color_hsl",[](const std::vector<VekValue>&a){return ColorToValue(HslColor((float)(a.size()>0?a[0].AsNumber():0),(float)(a.size()>1?a[1].AsNumber():1),(float)(a.size()>2?a[2].AsNumber():.5),(float)(a.size()>3?a[3].AsNumber():1)));});
    e.RegisterNative("color_luminance",[](const std::vector<VekValue>&a){return VekValue(a.empty()?0.0:RelativeLuminance(ColorFromValue(a[0])));});
    e.RegisterNative("color_contrast",[](const std::vector<VekValue>&a){return VekValue(a.size()<2?1.0:ContrastRatio(ColorFromValue(a[0]),ColorFromValue(a[1])));});
}

} // namespace vek
