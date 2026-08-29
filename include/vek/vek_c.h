#ifndef VEK_C_H
#define VEK_C_H
#include <stddef.h>
#include <stdint.h>
#ifdef _WIN32
  #ifdef VEK_C_EXPORTS
    #define VEK_C_API __declspec(dllexport)
  #else
    #define VEK_C_API __declspec(dllimport)
  #endif
#else
  #define VEK_C_API
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct vek_runtime vek_runtime;
typedef enum vek_value_type { VEK_NIL=0, VEK_NUMBER=1, VEK_BOOL=2, VEK_STRING=3, VEK_JSON=4 } vek_value_type;
typedef struct vek_value { vek_value_type type; double number; int boolean; const char* string_value; } vek_value;
typedef vek_value (*vek_native_fn)(vek_runtime*, const vek_value*, size_t, void*);

VEK_C_API vek_runtime* vek_create(void);
VEK_C_API void vek_destroy(vek_runtime* runtime);
VEK_C_API int vek_load_file(vek_runtime* runtime, const char* path);
VEK_C_API int vek_load_source(vek_runtime* runtime, const char* source, const char* source_name);
VEK_C_API int vek_add_module_root(vek_runtime* runtime, const char* path);
VEK_C_API int vek_register_native(vek_runtime* runtime, const char* name, vek_native_fn fn, void* user_data);
VEK_C_API void vek_seal_natives(vek_runtime* runtime);
VEK_C_API int vek_set_security_tier(vek_runtime* runtime, int tier);
VEK_C_API int vek_set_authority_role(vek_runtime* runtime, int role);
VEK_C_API size_t vek_authority_action_count(vek_runtime* runtime);
VEK_C_API size_t vek_replication_schema_count(vek_runtime* runtime);
VEK_C_API int vek_authority_validate_request(vek_runtime* runtime, const char* action_id, const char* actor_id, uint64_t sequence, const char* nonce, const char* payload_text, const char* granted_capability, double now_seconds);
VEK_C_API int vek_authority_validate_request_v2(vek_runtime* runtime, const char* action_id, const char* actor_id, const char* session_id, int authenticated, uint64_t sequence, const char* nonce, const char* payload_text, const char* granted_capability, double now_seconds);
VEK_C_API const char* vek_authority_last_reason(vek_runtime* runtime);
VEK_C_API vek_value vek_call(vek_runtime* runtime, const char* function_name, const vek_value* args, size_t arg_count);
VEK_C_API vek_value vek_emit_event(vek_runtime* runtime, const char* event_name, const vek_value* args, size_t arg_count);
VEK_C_API int vek_has_function(vek_runtime* runtime, const char* function_name);
VEK_C_API int vek_has_event(vek_runtime* runtime, const char* event_name);
VEK_C_API const char* vek_last_error(vek_runtime* runtime);
VEK_C_API const char* vek_version(void);

#ifdef __cplusplus
}
#endif
#endif
