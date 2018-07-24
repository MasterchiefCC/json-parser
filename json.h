#ifndef _JSON_H
#define _JSON_H

#ifndef json_char
#define json_char char
#endif

#ifndef json_int_t
#ifndef _MSC_VER
#include <inttypes.h>
#define json_int_t int64_t
#else
#define json_int_t __int64
#endif
#endif

#include <stdlib.h>

typedef struct {
  size_t len;
  unsigned char* data;
} ngx_str_t;

typedef struct {
  unsigned long max_memory;
  int settings;

  /* Custom allocator support (leave null to use malloc/free)
   */

  void* (*mem_alloc)(size_t, int zero, void* user_data);
  void (*mem_free)(void*, void* user_data);

  void* user_data; /* will be passed to mem_alloc and mem_free */

  size_t value_extra; /* how much extra space to allocate for values? */

} json_settings;


#define json_enable_comments 0x01

typedef enum {
  json_none,
  json_object,
  json_array,
  json_integer,
  json_double,
  json_string,
  json_boolean,
  json_null

} json_type;

extern const struct _json_value json_value_none;

typedef struct _json_object_entry {
  json_char* name;
  unsigned int name_length;

  struct _json_value* value;

} json_object_entry;

typedef struct _json_value {
  struct _json_value* parent;
  //类型
  json_type type;

  union {
    // bool
    int boolean;
    // int
    json_int_t integer;
    // double
    double dbl;

    // string type
    struct {
      unsigned int length;
      json_char* ptr; /* null terminated */

    } string;

    // object type
    struct {
      unsigned int length;

      json_object_entry* values;
    } object;

    // array type
    struct {
      unsigned int length;
      struct _json_value** values;
    } array;

  } u;

  union {
    struct _json_value* next_alloc;
    void* object_mem;

  } _reserved;
} json_value;

json_value* json_parse(const json_char* json, size_t length);

#define json_error_max 128

json_value* json_parse_ex(json_settings* settings, const json_char* json,
                          size_t length, char* error);

void json_value_free(json_value*);

json_value* ngx_find_json_name(json_value* root, ngx_str_t* target);
json_value* ngx_find_json_name_in_object(json_value* root, ngx_str_t* target);
json_value* ngx_find_json_name_in_array(json_value* root, ngx_str_t* target);

/* Not usually necessary, unless you used a custom mem_alloc and now want to
 * use a custom mem_free.
 */
void json_value_free_ex(json_settings* settings, json_value*);
#endif
