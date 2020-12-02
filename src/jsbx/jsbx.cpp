// TODO: script/worker loops should only remove events that they process themself
// TODO: release every jerryscript value that was created in C

#include <jsbx/api.h>
#include <jsbx/event.h>
#include "jsbx.h"

#include <jerryscript.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <inttypes.h>
#include <dlfcn.h>
#include <iostream>
#include <list>
#include <string>
#include <fstream>
#include <streambuf>
#include <list>
#include <map>
#include <algorithm>


static jerry_value_t print(const jerry_value_t function_obj,
                           const jerry_value_t this_val,
                           const jerry_value_t args_p[],
                           const jerry_length_t args_cnt)
{
  if (args_cnt != 1)
    return jerry_create_undefined();

  jerry_value_t str = args_p[0];

  if (!jerry_value_is_string(str))
    return jerry_create_undefined();

  size_t str_size = jerry_get_string_size(str);
  jerry_char_t str_buf[str_size + 1];

  jerry_string_to_char_buffer(str, str_buf, str_size);
  str_buf[str_size] = 0;

  printf("> %s\n", str_buf);

  return jerry_create_undefined();
}


char *value_to_string(const jerry_value_t value)
{
  if (!jerry_value_is_string(value))
    return 0;

  size_t str_size = jerry_get_string_size(value);

  if (str_size < 1)
    return 0;

  jerry_char_t *str_buf = (jerry_char_t *)malloc(str_size+1); // TODO: static, otherwise returning string must be free'd every time

  jerry_string_to_char_buffer(value, str_buf, str_size);

  str_buf[str_size] = 0;

  return (char *)str_buf;
}



























void print_unhandled_exception(jerry_value_t error_value, uint8_t *buffer)
{
  #define SYNTAX_ERROR_CONTEXT_SIZE 2

  if (!(!jerry_value_is_error(error_value)))
  {
    printf("assert(!jerry_value_is_error(error_value)\n");
    return;
  }

  jerry_char_t err_str_buf[256];

  if (jerry_value_is_object(error_value))
  {
    jerry_value_t stack_str = jerry_create_string((const jerry_char_t *)"stack");
    jerry_value_t backtrace_val = jerry_get_property(error_value, stack_str);
    jerry_release_value(stack_str);

    if (!jerry_value_is_error(backtrace_val) && jerry_value_is_array(backtrace_val))
    {
      printf("Exception backtrace:\n");

      uint32_t length = jerry_get_array_length(backtrace_val);

      /* This length should be enough. */
      if (length > 32)
      {
        length = 32;
      }

      for (uint32_t i = 0; i < length; i++)
      {
        jerry_value_t item_val = jerry_get_property_by_index(backtrace_val, i);

        if (/*!jerry_value_is_error (item_val) && */jerry_value_is_string (item_val))
        {
          jerry_size_t str_size = jerry_get_string_size(item_val);

          if (str_size >= 256)
          {
            printf ("%3u: [Backtrace string too long]\n", i);
          }
          else
          {
            jerry_size_t string_end = jerry_string_to_char_buffer(item_val, err_str_buf, str_size);
            if (!(string_end == str_size))
            {
              printf("assert (string_end == str_size)\n");
              return;
            }

            err_str_buf[string_end] = 0;

            printf ("%3u: %s\n", i, err_str_buf);
          }
        }

        jerry_release_value (item_val);
      }
    }

    jerry_release_value(backtrace_val);
  }

  jerry_value_t err_str_val = jerry_value_to_string(error_value);
  jerry_size_t err_str_size = jerry_get_string_size(err_str_val);

  if (err_str_size >= 256)
  {
    const char msg[] = "[Error message too long]";
    err_str_size = sizeof (msg) / sizeof (char) - 1;
    memcpy (err_str_buf, msg, err_str_size + 1);
  }
  else
  {
    jerry_size_t string_end = jerry_string_to_char_buffer(err_str_val, err_str_buf, err_str_size);
    if (!(string_end == err_str_size))
    {
      printf("assert (string_end (%d) == err_str_size (%d))\n", string_end, err_str_size);
      return;
    }
    err_str_buf[string_end] = 0;

    if (jerry_is_feature_enabled(JERRY_FEATURE_ERROR_MESSAGES) && jerry_get_error_type(error_value) == JERRY_ERROR_SYNTAX)
    {
      unsigned int err_line = 0;
      unsigned int err_col = 0;

      /* 1. parse column and line information */
      for (jerry_size_t i = 0; i < string_end; i++)
      {
        if (!strncmp ((char *) (err_str_buf + i), "[line: ", 7))
        {
          i += 7;

          char num_str[8];
          unsigned int j = 0;

          while (i < string_end && err_str_buf[i] != ',')
          {
            num_str[j] = (char) err_str_buf[i];
            j++;
            i++;
          }
          num_str[j] = '\0';

          err_line = (unsigned int) strtol (num_str, NULL, 10);

          if (strncmp ((char *) (err_str_buf + i), ", column: ", 10))
          {
            break; /* wrong position info format */
          }

          i += 10;
          j = 0;

          while (i < string_end && err_str_buf[i] != ']')
          {
            num_str[j] = (char) err_str_buf[i];
            j++;
            i++;
          }
          num_str[j] = '\0';

          err_col = (unsigned int) strtol (num_str, NULL, 10);
          break;
        }
      } /* for */

      if (err_line != 0 && err_col != 0)
      {
        unsigned int curr_line = 1;

        bool is_printing_context = false;
        unsigned int pos = 0;

        /* 2. seek and print */
        while (buffer[pos] != '\0')
        {
          if (buffer[pos] == '\n')
          {
            curr_line++;
          }

          if (err_line < SYNTAX_ERROR_CONTEXT_SIZE
              || (err_line >= curr_line
                  && (err_line - curr_line) <= SYNTAX_ERROR_CONTEXT_SIZE))
          {
            /* context must be printed */
            is_printing_context = true;
          }

          if (curr_line > err_line)
          {
            break;
          }

          if (is_printing_context)
          {
            printf("%c", buffer[pos]);
          }

          pos++;
        }

        printf("\n");

        while (--err_col)
        {
          printf("~");
        }

        printf("^\n");
      }
    }
  }

  printf("Script Error: %s\n", err_str_buf);
  jerry_release_value (err_str_val);
} /* print_unhandled_exception */





























void native_close_cb(void *ptr)
{
  handle_container_t *handle_container = (handle_container_t *)ptr;

  printf("native_close_cb: closing handle: %d\n", handle_container->handle);
  close(handle_container->handle);
  free(handle_container);
}










// CAUTION: can open *any* file on the fs at the moment !
static jerry_value_t global_include_handler(const jerry_value_t function_obj,
                                            const jerry_value_t this_val,
                                            const jerry_value_t args[],
                                            const jerry_length_t argc)
{
  puts("global_include_handler called");

  if (argc != 1)
  {
    puts("global_include_handler: invalid arg count");
    return jerry_create_boolean(false);
  }


  char *filename = value_to_string(args[0]);

  if (!filename)
  {
    puts("global_include_handler: invalid file name");
    return jerry_create_boolean(false);
  }


  jsbx_include(filename);


  return jerry_create_boolean(true);
}


// TODO: return a handler object, which unloads a lib if it's deleted (via jerryscript's native obj handler)
static jerry_value_t global_load_handler(const jerry_value_t function_obj,
                                         const jerry_value_t this_val,
                                         const jerry_value_t args[],
                                         const jerry_length_t argc)
{
  puts("global_load_handler called");

  if (argc != 1)
  {
    puts("global_load_handler: invalid arg count");
    return jerry_create_undefined();
  }


  char *filename = value_to_string(args[0]);

  if (!filename)
  {
    puts("global_load_handler: invalid file name");
    return jerry_create_undefined();
  }

  printf("global_load_handler: loading %s\n", filename);


  void *module = dlopen(filename, RTLD_LAZY);

  if (!module)
  {
    printf("global_load_handler: dlopen for %s failed: %s\n", filename, dlerror());
    return jerry_create_undefined();
  }


  typedef void (*wrapper_init_fn)(void);
  wrapper_init_fn wrapper_init = (wrapper_init_fn)dlsym(module, "wrapper_init");

  if (!wrapper_init)
  {
    puts("global_load_handler: module's 'wrapper_init' is missing");
    return jerry_create_undefined();
  }


  puts("global_load_handler: calling wrapper_init now..");
  wrapper_init(); // !!!
  puts("global_load_handler: done!");


  return jerry_create_undefined(); // TODO: handler object
}





static jerry_value_t internal_errno_handler(const jerry_value_t function_obj,
                                            const jerry_value_t this_val,
                                            const jerry_value_t args[],
                                            const jerry_length_t argc)
{
  int _errno = errno; // TODO: check if jerryscript calls some stdlib functions after calling Internal.errno() and before this point

  puts("internal_errno_handler called");

  return jerry_create_number(_errno);
}


static jerry_value_t internal_strerror_handler(const jerry_value_t function_obj,
                                               const jerry_value_t this_val,
                                               const jerry_value_t args[],
                                               const jerry_length_t argc)
{
  puts("internal_strerror_handler called");

  if (argc != 1)
  {
    puts("internal_strerror_handler: invalid arg count");
    return jerry_create_undefined();
  }

  int errorCodeValue = args[0];

  if (!jerry_value_is_number(errorCodeValue))
  {
    puts("internal_strerror_handler: argument is not a number");
    return jerry_create_undefined();
  }

  int errorCode = jerry_value_to_number(errorCodeValue);

  return jerry_create_string((const jerry_char_t *)strerror(errorCode)); // TODO: thread-safe !!
}







volatile bool run = true;




pthread_mutex_t *scriptEventHandlerListMutex;
std::list<event_handler_fn> scriptEventHandlerList;


void jsbx_register_worker_event_handler(event_handler_fn event_handler)
{
  scriptEventHandlerList.push_back(event_handler);
}


pthread_mutex_t *workerEventHandlerListMutex;
std::list<event_handler_fn> workerEventHandlerList;


void jsbx_register_script_event_handler(event_handler_fn event_handler)
{
  workerEventHandlerList.push_back(event_handler);
}



void *worker(void *arg)
{
  (void)arg;

  while (run)
  {
    pthread_mutex_lock(workerEventHandlerListMutex);

    for (auto i1 = workerEventHandlerList.begin(); i1 != workerEventHandlerList.end(); ++i1)
    {
      event_handler_fn event_handler = (*i1);

      event_handler();
    }

    pthread_mutex_unlock(workerEventHandlerListMutex);

    usleep(10 * 1000);
  }


  puts("worker: thread end");
  return nullptr;
}


void print_type(jerry_value_t value)
{

}









pthread_t *thread1;


void jsbx_init()
{
  printf("jsbx init..\n");

  scriptEventHandlerListMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(scriptEventHandlerListMutex, 0);

  workerEventHandlerListMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(workerEventHandlerListMutex, 0);

  thread1 = (pthread_t *)malloc(sizeof(pthread_t));
  pthread_create(thread1, 0, worker, 0);


  jerry_init(JERRY_INIT_EMPTY);


  bool loadMain = true;

  loadMain = false;

  if (loadMain)
  {
    std::ifstream mainFile;

    mainFile.open("../main.js", std::ios_base::in); // debug

    if (!mainFile.is_open())
    {
      mainFile.open("main.js", std::ios_base::in);
    }

    if (!mainFile.is_open())
    {
      std::cerr << "jsbx_init: cannot open main.js" << std::endl;
      return;
    }


    std::string mainScriptString((std::istreambuf_iterator<char>(mainFile)),
                                  std::istreambuf_iterator<char>());

    const char *mainFilename = "main.js";
    jerry_value_t parsed_main_code = jerry_parse((const jerry_char_t *)mainFilename, strlen(mainFilename), (jerry_char_t *)mainScriptString.c_str(), mainScriptString.size(), JERRY_PARSE_NO_OPTS);
    //std::cout << "main script: " << mainScriptString << std::endl;

    if (!jerry_value_is_error(parsed_main_code))
    {
      /* Print return value */

      char *result = value_to_string(parsed_main_code);

      if (result != nullptr)
      {
        printf("Error: %s\n", result);

        jerry_release_value(parsed_main_code);
        parsed_main_code = jerry_run_all_enqueued_jobs();

        if (jerry_value_is_error(parsed_main_code))
        {
          parsed_main_code = jerry_get_value_from_error(parsed_main_code, true);
          print_unhandled_exception(parsed_main_code, (uint8_t *)mainScriptString.c_str());
        }
      }
    }
    else
    {
      parsed_main_code = jerry_get_value_from_error(parsed_main_code, true);
      print_unhandled_exception(parsed_main_code, (uint8_t *)mainScriptString.c_str());
    }

    jerry_release_value(parsed_main_code);
  }



  jerry_value_t glob_obj = jerry_get_global_object();


  jerry_value_t internal_obj = jerry_create_object();
  jerry_value_t internal_obj_name = jerry_create_string((const jerry_char_t *)"Internal");
  jerry_release_value(jerry_set_property(glob_obj, internal_obj_name, internal_obj));
  jerry_release_value(internal_obj_name);


{
  jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"errno");
  jerry_value_t func_val = jerry_create_external_function(internal_errno_handler);
  jerry_release_value(jerry_set_property(internal_obj, prop_name, func_val));
  jerry_release_value(prop_name);
  jerry_release_value(func_val);
}
{
  jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"strerror");
  jerry_value_t func_val = jerry_create_external_function(internal_strerror_handler);
  jerry_release_value(jerry_set_property(internal_obj, prop_name, func_val));
  jerry_release_value(prop_name);
  jerry_release_value(func_val);
}

  jerry_release_value(internal_obj);



// global:
{
  jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"print");
  jerry_value_t func_val = jerry_create_external_function(print);
  jerry_release_value(jerry_set_property(glob_obj, prop_name, func_val));
  jerry_release_value(prop_name);
  jerry_release_value(func_val);
}
{
  jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"load");
  jerry_value_t func_val = jerry_create_external_function(global_load_handler);
  jerry_release_value(jerry_set_property(glob_obj, prop_name, func_val));
  jerry_release_value(prop_name);
  jerry_release_value(func_val);
}
{
  jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"include");
  jerry_value_t func_val = jerry_create_external_function(global_include_handler);
  jerry_release_value(jerry_set_property(glob_obj, prop_name, func_val));
  jerry_release_value(prop_name);
  jerry_release_value(func_val);
}

  jerry_release_value(glob_obj);
}


void jsbx_event_loop()
{
  usleep(10000); // TODO: use pthread conditions instead of sleeping !

  // feed to javascript:

  // check Events for Script
  pthread_mutex_lock(scriptEventHandlerListMutex);

  for (auto i1 = scriptEventHandlerList.begin(); i1 != scriptEventHandlerList.end(); ++i1)
  {
    event_handler_fn event_handler = *i1;

    event_handler();
  }

  pthread_mutex_unlock(scriptEventHandlerListMutex);


  // run promises, etc. everything that is left for javascript's event loop to do
  jerry_release_value(jerry_run_all_enqueued_jobs());
}


bool jsbx_include(const char *filename)
{
  std::ifstream includeFile;
  includeFile.open(filename, std::ios_base::in); // debug

  printf("jsbx_include: file => %s\n", filename);

  if (!includeFile.is_open())
  {
    printf("jsbx_include: cannot open file: %s\n", strerror(errno));
    return false;
  }


  std::string includeFileString((std::istreambuf_iterator<char>(includeFile)),
                                 std::istreambuf_iterator<char>());


  //jerry_value_t parse = jerry_parse((const jerry_char_t *)filename, strlen(filename), (const jerry_char_t *)includeFileString.c_str(), includeFileString.size(), JERRY_PARSE_NO_OPTS);
  printf("jsbx_include: parsing.. \n");
  jerry_value_t parse = jerry_parse((const jerry_char_t *)filename, strlen(filename), (const jerry_char_t *)includeFileString.c_str(), includeFileString.size(), JERRY_PARSE_STRICT_MODE);

  if (!jerry_value_is_error(parse))
  {
    // jerry_value_t ret_value = jerry_run(parse);
    // jerry_release_value(ret_value);
    puts("jsbx_include: seems to have worked");
  }
  else
  {
    jerry_error_t err = jerry_get_error_type(parse);
    printf("jsbx_include: error parsing script. Error type: %d\n", err);

    parse = jerry_get_value_from_error(parse, true);
    print_unhandled_exception(parse, (uint8_t *)nullptr);
    return false;
  }
  printf("jsbx_include: done!\n");



  printf("jsbx_include: initial run.. \n");
  jerry_value_t run = jerry_run(parse);

  if (jerry_value_is_error(run))
  {
    run = jerry_get_value_from_error(run, true);
    print_unhandled_exception(run, (uint8_t *)nullptr);
    return false;
  }
  printf("jsbx_include: done!\n");



  jerry_release_value(parse);

  return true;
}


void jsbx_stop()
{
  puts("jsbx_stop called");

  run = false;
  jerry_cleanup();

  pthread_join(*thread1, 0);
  pthread_mutex_destroy(scriptEventHandlerListMutex);
  pthread_mutex_destroy(workerEventHandlerListMutex);
}
