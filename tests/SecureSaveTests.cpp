#include "SecureSaveStore.h"
#include <filesystem>
#include <fstream>
#include <iostream>

static int failures=0;
static void Check(bool c,const char*n){if(!c){std::cerr<<"FAIL: "<<n<<"\n";++failures;}}
int main(){
    auto dir=std::filesystem::temp_directory_path()/"vek_guard_save_test";std::filesystem::remove_all(dir);std::filesystem::create_directories(dir);
    auto file=dir/"career.vsave";std::string error,payload;
    Check(SecureSaveStore::Write(file,"1500 20 1\n",&error),"authenticated save writes");
    Check(SecureSaveStore::Read(file,payload,&error),"authenticated save reads");
    Check(payload=="1500 20 1\n","payload round trip");
    {std::fstream f(file,std::ios::in|std::ios::out|std::ios::binary);f.seekp(-2,std::ios::end);char c='9';f.write(&c,1);}
    payload.clear();Check(!SecureSaveStore::Read(file,payload,&error),"edited save rejected");
    std::filesystem::remove_all(dir);
    if(failures==0){std::cout<<"Secure save integrity tests: PASS\n";return 0;}return 1;
}
