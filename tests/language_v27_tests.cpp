#include <vek/VekScriptEngine.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>

int main(){
    VekScriptEngine vm;VekRegisterStandardLibrary(vm);
    const char* src=R"VEK(
/* VEK 2.7 block comments */
fn sum(values){
  let total=0;
  for item in values { total=total+item; }
  return total;
}
fn caught(){
  try { throw "custom-problem"; }
  catch(err) { return "caught:"+err; }
}
fn typed_error(){
  try { throw error("E_SPEED","bad speed",{speed:-1}); }
  catch(err) { return is_error(err) && err.code == "E_SPEED" && err.data.speed == -1; }
}
fn strings(){ return upper("vek")+":"+substring("language",0,4); }
fn assert_ok(){ assert(2+2==4,"math failed"); return true; }
)VEK";
    assert(vm.LoadSource(src,"language_v27.vek"));
    assert(vm.Call("sum",{VekValue(VekArray{1,2,3,4})}).AsNumber()==10);
    assert(vm.Call("caught").AsString()=="caught:custom-problem");
    assert(vm.Call("typed_error").AsBool());
    assert(vm.Call("strings").AsString()=="VEK:lang");
    assert(vm.Call("assert_ok").AsBool());
    std::cout<<"VEK 2.7 language tests: PASS\n";
}
