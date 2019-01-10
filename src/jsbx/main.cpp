#include "jsbx.h"

#include <jerryscript.h>

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <signal.h>
#include <string.h>


volatile bool run = true;


void int_handler(int )
{
  if (!run)
  {
    puts(" Okay, exiting immediately!"); // dangerous, as we are in an interrupt handler !
    exit(0);
  }

  puts(" Okay, exiting event loop.."); // dangerous, as we are in an interrupt handler !
  run = false;
}


int main(int argc, char **args)
{
  signal(SIGINT, int_handler);

  if (argc != 2)
  {
    std::cerr << "usage: " << args[0] << " <js script file>" << std::endl;
    return -1;
  }


  jsbx_init();


  std::ifstream scriptFile;

  const char *filename = args[1];
  printf("loading %s\n", filename);
  scriptFile.open(filename, std::ios_base::in);

  if (!scriptFile.is_open())
  {
    std::cerr << "cannot open file " << filename << ": " << strerror(errno) << std::endl;
    return -1;
  }

  std::string scriptString((std::istreambuf_iterator<char>(scriptFile)),
                            std::istreambuf_iterator<char>());


  jerry_value_t parsed_script_code = jerry_parse((const jerry_char_t *)filename, strlen(filename), (jerry_char_t *)scriptString.c_str(), scriptString.size(), JERRY_PARSE_NO_OPTS);
//jerry_value_t parsed_script_code = jerry_eval((jerry_char_t *)scriptString.c_str(), scriptString.size(), false /* non-strict */);
  //std::cout << "script: " << scriptString << std::endl;








  if (!jerry_value_is_error(parsed_script_code))
  {
//    /* Print return value */

//    char *result = value_to_string(parsed_script_code);

//    if (result != nullptr)
//    {
//      printf("Error: %s\n", result);

//      jerry_release_value(parsed_script_code);
//      parsed_script_code = jerry_run_all_enqueued_jobs();

//      if (jerry_value_is_error(parsed_script_code))
//      {
//        parsed_script_code = jerry_get_value_from_error(parsed_script_code, true);
//        print_unhandled_exception(parsed_script_code, (uint8_t *)scriptString.c_str());
//      }
//    }
  }
  else
  {
    parsed_script_code = jerry_get_value_from_error(parsed_script_code, true);
    print_unhandled_exception(parsed_script_code, (uint8_t *)scriptString.c_str());
    return -1;
  }




  printf("initial run.. ");
  jerry_value_t run_result = jerry_run(parsed_script_code);

  if (jerry_value_is_error(run_result))
  {
    run_result = jerry_get_value_from_error(run_result, true);
    print_unhandled_exception(run_result, (uint8_t *)scriptString.c_str());
    return -4;
  }
  printf("done!\n");







  jerry_value_t glob_obj = jerry_get_global_object();
  jerry_value_t main_function_name = jerry_create_string((const jerry_char_t *)"main");
  jerry_value_t main_function = jerry_get_property(glob_obj, main_function_name);

  if (jerry_value_is_error(main_function))
  {
    puts("dayum1");
    main_function = jerry_get_value_from_error(main_function, true);
    print_unhandled_exception(main_function, (uint8_t *)scriptString.c_str());
    return -2;
  }

  //printf("result? %d\n", jerry_value_is_function(jerry_get_property(jerry_get_global_object(), jerry_create_string((const jerry_char_t *)"main"))));

  if (!jerry_value_is_function(main_function))
  {
    puts("main function not found!");

    printf("script: %s\n", scriptString.c_str());

    return -2;
  }

  jerry_value_t this_val = jerry_create_undefined();
  jerry_value_t *jargs = nullptr;
  jerry_value_t result = jerry_call_function(main_function, this_val, jargs, 0);

  if (jerry_value_is_error(result))
  {
    puts("dayum2");
    result = jerry_get_value_from_error(result, true);
    print_unhandled_exception(result, (uint8_t *)scriptString.c_str());
    return -3;
  }







  // if (!jerry_value_is_error(parsed_script_code))
  // {
  //   // jerry_value_t ret_value = jerry_run(parsed_script_code);
  //   // jerry_release_value(ret_value);
  // }
  // else
  // {
  //   puts("script: error!");

  //   //printf("script was: %s\n", scriptString.c_str());

  //   jerry_error_t err = jerry_get_error_type(parsed_script_code);
  //   printf("script: error type: %d\n", err);

  //   //jerry_value_clear_error_flag(&parsed_script_code);
  //   //print_unhandled_exception(parsed_script_code);

  //   //print_value(err);
  // }

  jerry_release_value(parsed_script_code);

  while (run) // script event loop
  {
    jsbx_event_loop();
  }

  puts("cleaning up..");
  jsbx_stop();

  return 0;
}
