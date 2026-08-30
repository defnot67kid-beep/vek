#include "VekSecuritySystem.h"
#include "SecurityHash.h"
#include "generated/VekPublicKey.h"

#include <array>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <vector>
#include <algorithm>

#ifndef VEK_DEVELOPMENT_MODE
#define VEK_DEVELOPMENT_MODE 0
#endif

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#endif

namespace {
namespace fs = std::filesystem;

bool PathContainsParentTraversal(const fs::path& p){for(const auto& part:p.lexically_normal())if(part=="..")return true;return false;}

std::string ManifestKeyFor(const std::string& path){
    fs::path p=fs::path(path).lexically_normal();
    std::vector<std::string> parts;
    for(const auto& part:p){auto s=part.generic_string();if(!s.empty()&&s!="/"&&s!="\\")parts.push_back(s);}
    for(std::size_t i=0;i<parts.size();++i){
        if(parts[i]=="scripts"){
            std::string key="scripts";
            for(std::size_t j=i+1;j<parts.size();++j)key+="/"+parts[j];
            return key;
        }
    }
    return {};
}

bool SafeScriptPath(const std::string& path, std::string& error) {
    fs::path p(path);
    if(p.extension() != ".vek") { error = "VEK Guard: only .vek script files may be loaded"; return false; }
    if(PathContainsParentTraversal(p)){error="VEK Guard: parent-path traversal is not allowed";return false;}
    std::string key=ManifestKeyFor(path);
#if !VEK_DEVELOPMENT_MODE
    if(key.empty()){error="VEK Guard: secure-release scripts must live under scripts/";return false;}
#endif
    std::error_code ec;
    fs::path normalized=p.lexically_normal();
    if(fs::exists(normalized,ec) && fs::is_symlink(normalized,ec)) {
        error = "VEK Guard: symbolic-link script paths are rejected in the game host";
        return false;
    }
    return true;
}

bool SafeSignaturePath(const std::string& scriptPath,const std::string& signaturePath,std::string& error){
    if(signaturePath!=scriptPath+".sig"){error="VEK Guard: signature path must be paired with its script";return false;}
    fs::path p(signaturePath);
    if(PathContainsParentTraversal(p)){error="VEK Guard: signature parent-path traversal is not allowed";return false;}
    std::error_code ec;if(fs::exists(p,ec)&&fs::is_symlink(p,ec)){error="VEK Guard: symbolic-link signature paths are rejected";return false;}
    return true;
}

bool ReadBinary(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);if(!in)return false;std::ostringstream ss;ss<<in.rdbuf();out=ss.str();return true;
}
bool ReadText(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);if(!in)return false;std::ostringstream ss;ss<<in.rdbuf();out=ss.str();while(!out.empty()&&(out.back()=='\r'||out.back()=='\n'||out.back()==' '||out.back()=='\t'))out.pop_back();return true;
}

const std::array<std::uint8_t,32>* ManifestDigest(const std::string& scriptPath){
    std::string key=ManifestKeyFor(scriptPath);if(key.empty())return nullptr;
    for(const auto& e:VekGeneratedKey::ScriptHashes)if(e.path==key)return &e.sha256;
    return nullptr;
}

#ifdef _WIN32
bool VerifyEcdsaP256(const std::string& source, const std::array<std::uint8_t,64>& signature, std::string& error) {
    BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_KEY_HANDLE key=nullptr;
    NTSTATUS status=BCryptOpenAlgorithmProvider(&alg,BCRYPT_ECDSA_P256_ALGORITHM,nullptr,0);
    if(status<0){error="VEK Guard: BCryptOpenAlgorithmProvider failed";return false;}
    struct PublicBlob{BCRYPT_ECCKEY_BLOB header;UCHAR x[32];UCHAR y[32];}blob{};
    blob.header.dwMagic=BCRYPT_ECDSA_PUBLIC_P256_MAGIC;blob.header.cbKey=32;
    for(size_t i=0;i<32;++i){blob.x[i]=VekGeneratedKey::PublicX[i];blob.y[i]=VekGeneratedKey::PublicY[i];}
    status=BCryptImportKeyPair(alg,nullptr,BCRYPT_ECCPUBLIC_BLOB,&key,reinterpret_cast<PUCHAR>(&blob),sizeof(blob),0);
    if(status<0){BCryptCloseAlgorithmProvider(alg,0);error="VEK Guard: public signing key import failed";return false;}
    auto digest=SecurityHash::Sha256(reinterpret_cast<const std::uint8_t*>(source.data()),source.size());
    status=BCryptVerifySignature(key,nullptr,const_cast<PUCHAR>(digest.data()),static_cast<ULONG>(digest.size()),const_cast<PUCHAR>(signature.data()),static_cast<ULONG>(signature.size()),0);
    BCryptDestroyKey(key);BCryptCloseAlgorithmProvider(alg,0);
    if(status<0){error="VEK Guard: script signature is invalid or the script was modified";return false;}return true;
}
#endif
}

VekVerifiedScript VekSecuritySystem::VerifyScript(const std::string& scriptPath,const std::string& signaturePath) {
    VekVerifiedScript result;
    if(!SafeScriptPath(scriptPath,result.error))return result;
    if(!SafeSignaturePath(scriptPath,signaturePath,result.error))return result;
    if(!ReadBinary(scriptPath,result.source)){result.error="VEK Guard: could not read script: "+scriptPath;return result;}
    if(result.source.size()>256u*1024u){result.error="VEK Guard: script exceeds the secure 256 KiB source limit";result.source.clear();return result;}
    if(result.source.find('\0')!=std::string::npos){result.error="VEK Guard: embedded NUL bytes are not allowed in source";result.source.clear();return result;}

#if VEK_DEVELOPMENT_MODE
    result.ok=true;result.signatureVerified=false;return result;
#else
    // First pin every shipped script to the exact digest compiled into the EXE.
    // This remains effective even if a signing key is stolen later: an attacker
    // cannot substitute a newly signed script without rebuilding the executable.
    const auto* expected=ManifestDigest(scriptPath);
    if(!expected){result.error="VEK Guard: script is not present in the compiled allow-list";result.source.clear();return result;}
    auto digest=SecurityHash::Sha256(reinterpret_cast<const std::uint8_t*>(result.source.data()),result.source.size());
    if(!SecurityHash::ConstantTimeEqual(digest,*expected)){result.error="VEK Guard: script digest does not match the compiled release manifest";result.source.clear();return result;}

    std::string sigText;if(!ReadText(signaturePath,sigText)){result.error="VEK Guard: missing signature file: "+signaturePath;result.source.clear();return result;}
    std::array<std::uint8_t,64> signature{};if(!SecurityHash::ParseHex64(sigText,signature)){result.error="VEK Guard: signature file is malformed";result.source.clear();return result;}
#ifdef _WIN32
    if(!VerifyEcdsaP256(result.source,signature,result.error)){result.source.clear();return result;}
#endif
    result.signatureVerified=true;result.ok=true;return result;
#endif
}

const char* VekSecuritySystem::SecurityModeName(){
#if VEK_DEVELOPMENT_MODE
    return "DEVELOPMENT";
#else
    return "SECURE RELEASE + PINNED MANIFEST";
#endif
}
bool VekSecuritySystem::IsDevelopmentMode(){
#if VEK_DEVELOPMENT_MODE
    return true;
#else
    return false;
#endif
}
