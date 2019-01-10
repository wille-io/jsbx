#include <jsbx/api.h>


void jsbx_add_js_function(const char *function_name, jerry_external_handler_t function_handler, jerry_value_t parent_object)
{
  int free_parent_object = 0;
  if (parent_object == 0)
  {
    parent_object = jerry_get_global_object();
    free_parent_object = 1;
  }

  jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)function_name);
  jerry_value_t func_val = jerry_create_external_function(function_handler);
  jerry_release_value(jerry_set_property(parent_object, prop_name, func_val));
  jerry_release_value(prop_name);
  jerry_release_value(func_val);

  if (free_parent_object)
    jerry_release_value(parent_object);
}
