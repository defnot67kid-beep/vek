#include "VekScriptEngine.h"
#include "VekSecuritySystem.h"
#include "SecurityHash.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#ifndef VEK_TEST_SCRIPT_PATH
#define VEK_TEST_SCRIPT_PATH "scripts/collision.vek"
#endif

static int failures=0;
static void Check(bool c,const char* n){if(!c){std::cerr<<"FAIL: "<<n<<"\n";++failures;}}

int main(){
    Check(SecurityHash::Hex(SecurityHash::Sha256("abc"))=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "SHA-256 known vector");

    std::string script=VEK_TEST_SCRIPT_PATH;
    auto verified=VekSecuritySystem::VerifyScript(script,script+".sig");
    Check(verified.ok,"signed collision.vek accepted");
    Check(verified.signatureVerified,"signature/integrity marked verified");

    auto temp=std::filesystem::temp_directory_path()/"vek_tamper_test.vek";
    {std::ofstream o(temp,std::ios::binary);o<<verified.source<<"\n# tampered";}
    std::filesystem::copy_file(script+".sig",temp.string()+".sig",std::filesystem::copy_options::overwrite_existing);
    auto tampered=VekSecuritySystem::VerifyScript(temp.string(),temp.string()+".sig");
    Check(!tampered.ok,"modified signed script rejected");
    std::error_code ec;std::filesystem::remove(temp,ec);std::filesystem::remove(temp.string()+".sig",ec);

    VekScriptEngine vm;
    vm.RegisterNative("noop",[](const std::vector<VekValue>&)->VekValue{return {};});
    vm.SealNativeRegistry();
    Check(!vm.RegisterNative("late",[](const std::vector<VekValue>&)->VekValue{return {};}),"sealed native registry rejects new capability");

    VekSecurityPolicy p; p.maxCallDepth=6; p.maxInstructionsPerCall=100;
    VekScriptEngine recursive; recursive.SetSecurityPolicy(p);
    Check(recursive.LoadSource("fn dive(x){ return dive(x+1); }","recursive.vek"),"recursive script parses");
    recursive.Call("dive",{0});
    Check(!recursive.LastError().empty(),"recursive script stopped by sandbox limit");

    VekSecurityPolicy tiny; tiny.maxSourceBytes=8; tiny.maxModuleCount=1;
    VekScriptEngine limited; limited.SetSecurityPolicy(tiny);
    Check(!limited.LoadSource("fn hello(){ return 1; }","too-large.vek"),"source-size sandbox enforced");

    if(failures==0){std::cout<<"VEK Guard security tests: PASS\n";return 0;}
    std::cout<<"VEK Guard security tests: FAIL ("<<failures<<")\n";return 1;
}
