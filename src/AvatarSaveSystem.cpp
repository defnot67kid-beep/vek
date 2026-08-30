#include "AvatarSaveSystem.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

bool AvatarSaveSystem::Save(const AvatarAppearance& a) {
    std::filesystem::path dir=std::filesystem::current_path()/"saves";
    std::filesystem::create_directories(dir);
    std::ofstream out(dir/"avatar.txt");
    if(!out)return false;
    out<<(int)a.presentation<<' '<<a.bodySize<<' '<<a.height<<' '<<a.shoulderWidth<<' '<<a.armSize<<' '<<a.legSize<<' '
       <<a.skinTone<<' '<<(int)a.faceShape<<' '<<a.eyeStyle<<' '<<a.eyebrowStyle<<' '<<a.noseStyle<<' '<<a.lipStyle<<' '
       <<a.jawStyle<<' '<<a.earStyle<<' '<<a.freckles<<' '<<(int)a.hair<<' '<<a.hairColor<<' '<<(int)a.clothing<<' '
       <<a.clothingColor<<' '<<(int)a.accessory<<' '<<a.gloves<<'\n';
    return true;
}

bool AvatarSaveSystem::Load(AvatarAppearance& a) {
    std::ifstream in(std::filesystem::current_path()/"saves"/"avatar.txt");
    if(!in)return false;
    int presentation,face,hair,clothing,accessory;
    in>>presentation>>a.bodySize>>a.height>>a.shoulderWidth>>a.armSize>>a.legSize
      >>a.skinTone>>face>>a.eyeStyle>>a.eyebrowStyle>>a.noseStyle>>a.lipStyle
      >>a.jawStyle>>a.earStyle>>a.freckles>>hair>>a.hairColor>>clothing
      >>a.clothingColor>>accessory>>a.gloves;
    if(!in)return false;
    a.presentation=(Presentation)std::clamp(presentation,0,2);
    a.faceShape=(FaceShape)std::clamp(face,0,3);
    a.hair=(HairStyle)std::clamp(hair,0,6);
    a.clothing=(ClothingStyle)std::clamp(clothing,0,9);
    a.accessory=(AccessoryStyle)std::clamp(accessory,0,5);
    a.skinTone=std::clamp(a.skinTone,0,7);
    a.hairColor=std::clamp(a.hairColor,0,7);
    a.clothingColor=std::clamp(a.clothingColor,0,7);
    return true;
}
