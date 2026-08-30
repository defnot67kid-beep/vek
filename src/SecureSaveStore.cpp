#include "SecureSaveStore.h"
#include "SecurityHash.h"

#include <array>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#include <bcrypt.h>
#endif

namespace {
std::filesystem::path KeyPath(const std::filesystem::path& saveFile) {
    return saveFile.parent_path() / ".vek_guard_key";
}

bool ReadAllBytes(const std::filesystem::path& p, std::vector<std::uint8_t>& out) {
    std::ifstream in(p, std::ios::binary);
    if(!in) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

bool WriteAllBytes(const std::filesystem::path& p, const std::vector<std::uint8_t>& data) {
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    if(!out) return false;
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return out.good();
}

std::vector<std::uint8_t> GenerateKey() {
    std::vector<std::uint8_t> key(32);
#ifdef _WIN32
    if(BCryptGenRandom(nullptr,key.data(),static_cast<ULONG>(key.size()),BCRYPT_USE_SYSTEM_PREFERRED_RNG)>=0) return key;
#endif
    std::random_device rd;
    for(auto& b:key) b=static_cast<std::uint8_t>(rd());
    return key;
}

bool LoadOrCreateKey(const std::filesystem::path& saveFile, std::vector<std::uint8_t>& key, std::string* error) {
    std::filesystem::create_directories(saveFile.parent_path());
    const auto kp=KeyPath(saveFile);
    std::vector<std::uint8_t> stored;
    if(ReadAllBytes(kp,stored)) {
#ifdef _WIN32
        DATA_BLOB in{static_cast<DWORD>(stored.size()), stored.data()}, out{};
        if(!CryptUnprotectData(&in,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out)) {
            if(error)*error="VEK Guard: could not unlock local save key"; return false;
        }
        key.assign(out.pbData,out.pbData+out.cbData); LocalFree(out.pbData);
#else
        key=stored;
#endif
        if(key.size()!=32){if(error)*error="VEK Guard: local save key is invalid";return false;}
        return true;
    }

    key=GenerateKey();
#ifdef _WIN32
    DATA_BLOB in{static_cast<DWORD>(key.size()), key.data()}, out{};
    if(!CryptProtectData(&in,L"VEK Guard local save key",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out)) {
        if(error)*error="VEK Guard: could not protect local save key";return false;
    }
    std::vector<std::uint8_t> protectedData(out.pbData,out.pbData+out.cbData); LocalFree(out.pbData);
    if(!WriteAllBytes(kp,protectedData)){if(error)*error="VEK Guard: could not store local save key";return false;}
#else
    if(!WriteAllBytes(kp,key)){if(error)*error="VEK Guard: could not store local save key";return false;}
#endif
    return true;
}

bool ParseDigest(const std::string& hex, SecurityHash::Sha256Digest& out) {
    if(hex.size()!=64)return false;
    auto cv=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return 10+c-'a';if(c>='A'&&c<='F')return 10+c-'A';return -1;};
    for(size_t i=0;i<32;++i){int h=cv(hex[i*2]),l=cv(hex[i*2+1]);if(h<0||l<0)return false;out[i]=static_cast<std::uint8_t>((h<<4)|l);}return true;
}
}

bool SecureSaveStore::Write(const std::filesystem::path& path,const std::string& payload,std::string* error){
    std::vector<std::uint8_t> key;if(!LoadOrCreateKey(path,key,error))return false;
    auto tag=SecurityHash::HmacSha256(key,payload);
    std::ofstream out(path,std::ios::binary|std::ios::trunc);if(!out){if(error)*error="VEK Guard: could not open secure save for writing";return false;}
    out<<"VEKSEC1\n"<<SecurityHash::Hex(tag)<<"\n";out.write(payload.data(),static_cast<std::streamsize>(payload.size()));
    return out.good();
}

bool SecureSaveStore::Read(const std::filesystem::path& path,std::string& payload,std::string* error){
    std::ifstream in(path,std::ios::binary);if(!in){if(error)*error="missing";return false;}
    std::string magic,hex;std::getline(in,magic);std::getline(in,hex);if(magic!="VEKSEC1"){if(error)*error="VEK Guard: unsigned/legacy save rejected";return false;}
    std::ostringstream ss;ss<<in.rdbuf();payload=ss.str();
    std::vector<std::uint8_t> key;if(!LoadOrCreateKey(path,key,error))return false;
    SecurityHash::Sha256Digest expected{};if(!ParseDigest(hex,expected)){if(error)*error="VEK Guard: malformed save integrity tag";return false;}
    auto actual=SecurityHash::HmacSha256(key,payload);if(!SecurityHash::ConstantTimeEqual(actual,expected)){if(error)*error="VEK Guard: save integrity check failed";payload.clear();return false;}
    return true;
}
