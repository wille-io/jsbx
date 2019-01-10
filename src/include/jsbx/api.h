#pragma once

#include <jerryscript.h>


#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*event_handler_fn)(void);

// if parent_object == 0 then the global object will be used
void jsbx_add_js_function(const char *function_name, jerry_external_handler_t function_handler, jerry_value_t parent_object);
bool jsbx_include(const char *filename);
void jsbx_register_worker_event_handler(event_handler_fn event_handler);
void jsbx_register_script_event_handler(event_handler_fn event_handler);

char *value_to_string(const jerry_value_t value);

struct handle_container_t
{
  int handle;
};

void native_close_cb(void *ptr);

#ifdef __cplusplus
}
#endif