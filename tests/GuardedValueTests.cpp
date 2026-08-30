#include "GuardedValue.h"
#include <iostream>

static int failures=0;
static void Check(bool c,const char*n){if(!c){std::cerr<<"FAIL: "<<n<<"\n";++failures;}}
int main(){
    GuardedInt v=1500; Check(v.Get()==1500,"guarded value round trip"); Check(v.IntegrityOK(),"initial integrity");
    v-=300; Check(v.Get()==1200,"guarded subtraction"); v+=50; Check(v.Get()==1250,"guarded addition");
    unsigned char* bytes=reinterpret_cast<unsigned char*>(&v);bytes[0]^=0x40;Check(!v.IntegrityOK(),"single-address style tamper detected");
    if(failures==0){std::cout<<"Guarded runtime value tests: PASS\n";return 0;}return 1;
}
