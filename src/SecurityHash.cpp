#include "SecurityHash.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace {
constexpr std::uint32_t K[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

inline std::uint32_t Ror(std::uint32_t v, unsigned n) { return (v >> n) | (v << (32 - n)); }
inline std::uint32_t Ch(std::uint32_t x, std::uint32_t y, std::uint32_t z) { return (x & y) ^ (~x & z); }
inline std::uint32_t Maj(std::uint32_t x, std::uint32_t y, std::uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
inline std::uint32_t BS0(std::uint32_t x) { return Ror(x,2) ^ Ror(x,13) ^ Ror(x,22); }
inline std::uint32_t BS1(std::uint32_t x) { return Ror(x,6) ^ Ror(x,11) ^ Ror(x,25); }
inline std::uint32_t SS0(std::uint32_t x) { return Ror(x,7) ^ Ror(x,18) ^ (x >> 3); }
inline std::uint32_t SS1(std::uint32_t x) { return Ror(x,17) ^ Ror(x,19) ^ (x >> 10); }

void Transform(std::uint32_t state[8], const std::uint8_t block[64]) {
    std::uint32_t w[64]{};
    for (int i=0;i<16;++i) {
        int j=i*4;
        w[i]=(std::uint32_t(block[j])<<24)|(std::uint32_t(block[j+1])<<16)|(std::uint32_t(block[j+2])<<8)|std::uint32_t(block[j+3]);
    }
    for (int i=16;i<64;++i) w[i]=SS1(w[i-2])+w[i-7]+SS0(w[i-15])+w[i-16];
    std::uint32_t a=state[0],b=state[1],c=state[2],d=state[3],e=state[4],f=state[5],g=state[6],h=state[7];
    for (int i=0;i<64;++i) {
        std::uint32_t t1=h+BS1(e)+Ch(e,f,g)+K[i]+w[i];
        std::uint32_t t2=BS0(a)+Maj(a,b,c);
        h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    state[0]+=a;state[1]+=b;state[2]+=c;state[3]+=d;state[4]+=e;state[5]+=f;state[6]+=g;state[7]+=h;
}

int HexVal(char c) {
    if(c>='0'&&c<='9') return c-'0';
    if(c>='a'&&c<='f') return 10+c-'a';
    if(c>='A'&&c<='F') return 10+c-'A';
    return -1;
}
}

namespace SecurityHash {
Sha256Digest Sha256(const std::uint8_t* data, std::size_t size) {
    std::uint32_t state[8]={0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    std::size_t full=size/64;
    for(std::size_t i=0;i<full;++i) Transform(state,data+i*64);
    std::uint8_t tail[128]{};
    std::size_t rem=size%64;
    if(rem) std::memcpy(tail,data+full*64,rem);
    tail[rem]=0x80;
    std::uint64_t bits=static_cast<std::uint64_t>(size)*8u;
    std::size_t padBlock=(rem<56)?64:128;
    for(int i=0;i<8;++i) tail[padBlock-1-i]=static_cast<std::uint8_t>((bits>>(8*i))&0xffu);
    Transform(state,tail);
    if(padBlock==128) Transform(state,tail+64);
    Sha256Digest out{};
    for(int i=0;i<8;++i){out[i*4]=(state[i]>>24)&0xff;out[i*4+1]=(state[i]>>16)&0xff;out[i*4+2]=(state[i]>>8)&0xff;out[i*4+3]=state[i]&0xff;}
    return out;
}
Sha256Digest Sha256(std::string_view text) { return Sha256(reinterpret_cast<const std::uint8_t*>(text.data()), text.size()); }
Sha256Digest HmacSha256(const std::vector<std::uint8_t>& key, std::string_view data) {
    std::array<std::uint8_t,64> block{};
    if(key.size()>64){auto kh=Sha256(key.data(),key.size());std::copy(kh.begin(),kh.end(),block.begin());}
    else std::copy(key.begin(),key.end(),block.begin());
    std::vector<std::uint8_t> inner(64+data.size());
    std::vector<std::uint8_t> outer(64+32);
    for(size_t i=0;i<64;++i){inner[i]=block[i]^0x36u;outer[i]=block[i]^0x5cu;}
    if(!data.empty()) std::memcpy(inner.data()+64,data.data(),data.size());
    auto ih=Sha256(inner.data(),inner.size());
    std::copy(ih.begin(),ih.end(),outer.begin()+64);
    return Sha256(outer.data(),outer.size());
}
std::string Hex(const Sha256Digest& d){std::ostringstream o;o<<std::hex<<std::setfill('0');for(auto b:d)o<<std::setw(2)<<int(b);return o.str();}
bool ConstantTimeEqual(const Sha256Digest&a,const Sha256Digest&b){std::uint8_t diff=0;for(size_t i=0;i<a.size();++i)diff|=a[i]^b[i];return diff==0;}
bool ParseHex64(std::string_view hex,std::array<std::uint8_t,64>&out){if(hex.size()!=128)return false;for(size_t i=0;i<64;++i){int h=HexVal(hex[i*2]),l=HexVal(hex[i*2+1]);if(h<0||l<0)return false;out[i]=std::uint8_t((h<<4)|l);}return true;}
}
