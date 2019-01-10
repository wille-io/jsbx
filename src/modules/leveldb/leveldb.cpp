#include "leveldb.h"

#include <jerryscript.h>
#include <jsbx/api.h>
#include <leveldb/db.h>
#include <leveldb/status.h>
#include <leveldb/options.h>


struct leveldb_db_t
{
  leveldb::DB *db;
};


void native_leveldb_close(void *ptr) // TODO: generalize
{
  leveldb_db_t *leveldb_db_container = (leveldb_db_t *)ptr;

  printf("native_leveldb_close: closing db: %p\n", leveldb_db_container->db);

  delete leveldb_db_container->db;
  free(leveldb_db_container);
}


static jerry_value_t leveldb_open_handler(const jerry_value_t function_obj,
                                          const jerry_value_t this_val,
                                          const jerry_value_t args[],
                                          const jerry_length_t argc)
{
  puts("leveldb_open_handler called");

  jerry_value_t result = jerry_create_object();

  if (argc != 3)
    return jerry_create_number(0);


  // TODO: options
  leveldb::Options options;


  jerry_value_t db_path_value = args[1];

  if (!jerry_value_is_string(db_path_value))
  {
    puts("leveldb_open_handler: db path must be a string");
    return jerry_create_number(0);
  }

  char *db_path = value_to_string(db_path_value);


  jerry_value_t db_object = args[2];

  if (!jerry_value_is_object(db_object))
  {
    puts("leveldb_open_handler: db not an object");
    return jerry_create_number(0);
  }




  leveldb_db_t *leveldb_db_container = nullptr;
  bool has_leveldb_db_container = jerry_get_object_native_pointer(db_object, (void **)&leveldb_db_container, 0);

  if (!has_leveldb_db_container || leveldb_db_container == nullptr)
  {
    leveldb_db_container = (leveldb_db_t *)malloc(sizeof(leveldb_db_t));
    leveldb_db_container->db = nullptr;

    static const jerry_object_native_info_t native_leveldb_close_native_info =
    {
      .free_cb = native_leveldb_close
    };

    jerry_set_object_native_pointer(db_object, leveldb_db_container, &native_leveldb_close_native_info);
  }

  std::string db_path_string(db_path);
  leveldb::Status status = leveldb::DB::Open(options, db_path_string, &leveldb_db_container->db);

  jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"errorString"); // get with 'ToString()'
  jerry_value_t prop_val = jerry_create_string((jerry_char_t *)status.ToString().c_str());
  jerry_set_property(result, prop_name, prop_val);
  jerry_release_value(prop_name);
  jerry_release_value(prop_val);

  prop_name = jerry_create_string((jerry_char_t *)"okValue"); // get with 'ok()'
  prop_val = jerry_create_boolean(status.ok());
  jerry_set_property(result, prop_name, prop_val);
  jerry_release_value(prop_name);
  jerry_release_value(prop_val);
  return result;
}


void wrapper_init()
{
  jsbx_include("/home/wille/projects/jsbx/leveldb/leveldb.js"); // creates object 'leveldb' on global object

  jerry_value_t glob_obj = jerry_get_global_object();

  jerry_value_t leveldb_obj_name = jerry_create_string((const jerry_char_t *)"leveldb");
  jerry_value_t leveldb_obj = jerry_get_property(glob_obj, leveldb_obj_name);

  if (!jerry_value_is_object(leveldb_obj))
  {
    leveldb_obj = jerry_create_object();
    printf("creating leveldb object on global..");
  }

  jerry_release_value(jerry_set_property(glob_obj, leveldb_obj_name, leveldb_obj));
  //jerry_release_value(leveldb_obj_name);
  jerry_release_value(glob_obj);

  jsbx_add_js_function("Open", leveldb_open_handler, leveldb_obj);
  //jerry_release_value(leveldb_obj);

  printf("leveldb.Open a function? %d\n", jerry_value_is_function(jerry_get_property(leveldb_obj, leveldb_obj_name)));
}
