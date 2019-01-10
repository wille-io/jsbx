#pragma once

#include <jerryscript.h>

#ifdef __cplusplus
extern "C"
{
#endif

void jsbx_init();
void jsbx_event_loop();
void jsbx_stop();

void print_unhandled_exception(jerry_value_t error_value, uint8_t *buffer);
char *value_to_string(const jerry_value_t value);
void print_type(jerry_value_t value);

#ifdef __cplusplus
}
#endif
