#include "CharacterCustomizationSystem.h"
#include "ClothingSystem.h"
#include "raymath.h"
#include <algorithm>

const char* CharacterCustomizationSystem::Label(int row) {
    static const char* labels[]={
        "Presentation","Body size","Height","Shoulders","Arms","Legs","Skin tone","Face shape",
        "Eyes","Brows","Nose","Lips","Jaw","Ears","Freckles","Hair style","Hair colour",
        "Clothing","Clothing colour","Accessory","Gloves"
    };
    return labels[std::clamp(row,0,RowCount-1)];
}

void CharacterCustomizationSystem::Adjust(AvatarAppearance& a,int row,int d,int reputation) {
    auto wrap=[&](int v,int n){v=(v+d)%n;if(v<0)v+=n;return v;};
    switch(row){
        case 0: a.presentation=(Presentation)wrap((int)a.presentation,3);break;
        case 1: a.bodySize=Clamp(a.bodySize+d*0.05f,0.78f,1.28f);break;
        case 2: a.height=Clamp(a.height+d*0.04f,0.86f,1.16f);break;
        case 3: a.shoulderWidth=Clamp(a.shoulderWidth+d*0.05f,0.78f,1.25f);break;
        case 4: a.armSize=Clamp(a.armSize+d*0.04f,0.82f,1.20f);break;
        case 5: a.legSize=Clamp(a.legSize+d*0.04f,0.82f,1.20f);break;
        case 6: a.skinTone=wrap(a.skinTone,8);break;
        case 7: a.faceShape=(FaceShape)wrap((int)a.faceShape,4);break;
        case 8: a.eyeStyle=wrap(a.eyeStyle,4);break;
        case 9: a.eyebrowStyle=wrap(a.eyebrowStyle,4);break;
        case 10: a.noseStyle=wrap(a.noseStyle,4);break;
        case 11: a.lipStyle=wrap(a.lipStyle,4);break;
        case 12: a.jawStyle=wrap(a.jawStyle,4);break;
        case 13: a.earStyle=wrap(a.earStyle,4);break;
        case 14: a.freckles=!a.freckles;break;
        case 15: a.hair=(HairStyle)wrap((int)a.hair,HairStyleCount);break;
        case 16: a.hairColor=wrap(a.hairColor,8);break;
        case 17: {
            int v=(int)a.clothing;
            for(int tries=0;tries<10;tries++){
                v=(v+d)%10;if(v<0)v+=10;
                if(ClothingSystem::IsUnlocked((ClothingStyle)v,reputation)){a.clothing=(ClothingStyle)v;break;}
            }
            break;
        }
        case 18: a.clothingColor=wrap(a.clothingColor,8);break;
        case 19: a.accessory=(AccessoryStyle)wrap((int)a.accessory,6);break;
        case 20: a.gloves=!a.gloves;break;
    }
}
