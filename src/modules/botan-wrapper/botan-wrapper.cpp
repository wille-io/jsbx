#include "botan-wrapper.h"

#include <stdio.h>
#include <jsbx/api.h>
#include <botan/hash.h>
#include <jerryscript.h>


static jerry_value_t test123_handler(const jerry_value_t function_obj,
                                     const jerry_value_t this_val,
                                     const jerry_value_t args_p[],
                                     const jerry_length_t args_cnt)
{
  puts("TEST123 CALLED!");
  return jerry_create_number(7);
}


void wrapper_init()
{
  puts("BotanWrapper: wrapper_init called");

  jsbx_include("../botan-wrapper/botan-wrapper.js");
  jsbx_add_js_function("test123", test123_handler, 0);
}
