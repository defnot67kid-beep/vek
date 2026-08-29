#include <vek/vek_c.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>

static vek_value native_42(vek_runtime* r,const vek_value* args,size_t count,void* user){
    (void)r;(void)args;(void)count;(void)user;
    vek_value v={VEK_NUMBER,42,0,0};return v;
}
static vek_value native_99(vek_runtime* r,const vek_value* args,size_t count,void* user){
    (void)r;(void)args;(void)count;(void)user;
    vek_value v={VEK_NUMBER,99,0,0};return v;
}

int main(void){
    vek_runtime*r=vek_create();assert(r);
    assert(vek_set_security_tier(r,2));
    assert(vek_set_authority_role(r,3));

    // Duplicate native registration must fail without replacing the original
    // host callback. This closes the old C-ABI callback-table overwrite path.
    assert(vek_register_native(r,"host_value",native_42,0));
    assert(!vek_register_native(r,"host_value",native_99,0));

    assert(vek_load_source(r,
        "fn add(a,b){return a+b;} event Ping(x){return x+1;} fn native_value(){return host_value();} "
        "fn secure_setup(){ authority_action_register({id:\"shop.buy\",server_authoritative:true,allow_client_request:true,require_sequence:true,replay_protected:true,require_authenticated_session:true,min_nonce_bytes:12,max_nonce_bytes:64,max_requests_per_second:3,burst:3,max_payload_bytes:256,required_capability:\"shop.buy\"}); return true; }",
        "<c-test>"));
    vek_value a[2]={{VEK_NUMBER,2,0,0},{VEK_NUMBER,5,0,0}};vek_value z=vek_call(r,"add",a,2);assert(z.type==VEK_NUMBER&&z.number==7);
    z=vek_call(r,"native_value",0,0);assert(z.type==VEK_NUMBER&&z.number==42);
    assert(vek_has_event(r,"Ping"));vek_value p={VEK_NUMBER,9,0,0};z=vek_emit_event(r,"Ping",&p,1);assert(z.number==10);
    z=vek_call(r,"secure_setup",0,0);assert(z.type==VEK_BOOL&&z.boolean);assert(vek_authority_action_count(r)==1);
    assert(vek_authority_validate_request_v2(r,"shop.buy","p1","session-01",1,1,"nonce-000001","{item:wheel}","shop.buy",1.0));
    assert(!vek_authority_validate_request_v2(r,"shop.buy","p1","session-01",1,1,"nonce-000001","{item:wheel}","shop.buy",1.1));
    assert(!vek_authority_validate_request(r,"shop.buy","p1",2,"nonce-000002","{item:wheel}","shop.buy",1.2));
    printf("VEK C ABI tests: PASS\n");vek_destroy(r);return 0;
}
