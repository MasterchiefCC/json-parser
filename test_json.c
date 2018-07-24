#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "json.h"

#define ngx_string(str) \
  { sizeof(str) - 1, (char*)str }

static void print_depth_shift(int depth) {
  int j;
  for (j = 0; j < depth; j++) {
    printf(" ");
  }
}

static void process_value(json_value* value, int depth);

static void process_object(json_value* value, int depth) {
  int length, x;
  if (value == NULL) {
    return;
  }
  length = value->u.object.length;
  for (x = 0; x < length; x++) {
    print_depth_shift(depth);
    printf("object[%d].name = %s\n", x, value->u.object.values[x].name);
    process_value(value->u.object.values[x].value, depth + 1);
  }
}

static void process_array(json_value* value, int depth) {
  int length, x;
  if (value == NULL) {
    return;
  }
  length = value->u.array.length;
  printf("array\n");
  for (x = 0; x < length; x++) {
    process_value(value->u.array.values[x], depth);
  }
}

static void process_value(json_value* value, int depth) {
  int j;
  if (value == NULL) {
    return;
  }
  if (value->type != json_object) {
    print_depth_shift(depth);
  }
  switch (value->type) {
    case json_none:
      printf("none\n");
      break;
    case json_object:
      process_object(value, depth + 1);
      break;
    case json_array:
      process_array(value, depth + 1);
      break;
    case json_integer:
      printf("int: %10" PRId64 "\n", value->u.integer);
      break;
    case json_double:
      printf("double: %f\n", value->u.dbl);
      break;
    case json_string:
      printf("string: %s\n", value->u.string.ptr);
      break;
    case json_boolean:
      printf("bool: %d\n", value->u.boolean);
      break;
  }
}

int ngx_strncasecmp(unsigned char* s1, unsigned char* s2, size_t n) {
  unsigned int c1, c2;

  while (n) {
    c1 = (unsigned int)*s1++;
    c2 = (unsigned int)*s2++;

    //小写
    c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
    c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;

    if (c1 == c2) {
      if (c1) {
        n--;
        continue;
      }
      return 0;
    }

    return c1 - c2;
  }

  return 0;
}

json_value* ngx_find_json_name(json_value* root, ngx_str_t* target) {
  if (root == NULL || target == NULL) return NULL;
  switch (root->type) {
    case json_object:
      return ngx_find_json_name_in_object(root, target);
    case json_array:
      return ngx_find_json_name_in_array(root, target);
    default:
      return NULL;
  }
  return NULL;
}

json_value* ngx_find_json_name_in_object(json_value* root, ngx_str_t* target) {
  int length, i;
  json_object_entry temp;
  json_value* ret;
  if (root == NULL || target == NULL || root->type != json_object) return NULL;

  length = root->u.object.length;
  // root->u.object.values[x].name||value;

  for (i = 0; i < length; ++i) {
    temp = root->u.object.values[i];
    if (ngx_strncasecmp(temp.name, target->data, target->len) == 0) {
      return temp.value;
    }

    switch (temp.value->type) {
      case json_object:
        ret = ngx_find_json_name_in_object(temp.value, target);
        if (ret != NULL) return ret;
        break;
      case json_array:
        ret = ngx_find_json_name_in_array(temp.value, target);
        if (ret != NULL) return ret;
        break;
      default:
        break;
    }
  }

  return NULL;
}

json_value* ngx_find_json_name_in_array(json_value* root, ngx_str_t* target) {
  int length, i;
  struct _json_value* temp;
  json_value* ret;
  if (root == NULL || target == NULL || root->type != json_array) return NULL;

  length = root->u.array.length;

  for (i = 0; i < length; ++i) {
    temp = root->u.array.values[i];
    if (temp->type != json_object) continue;

    ret = ngx_find_json_name_in_object(temp, target);
    if (ret != NULL) return ret;
  }
  return NULL;
}

int main(int argc, char** argv) {
  char* filename;
  FILE* fp;
  struct stat filestatus;
  int file_size;
  char* file_contents;
  json_char* json;
  json_value *value, *ret;

  if (argc != 2) {
    fprintf(stderr, "%s <file_json>\n", argv[0]);
    return 1;
  }
  filename = argv[1];

  if (stat(filename, &filestatus) != 0) {
    fprintf(stderr, "File %s not found\n", filename);
    return 1;
  }
  file_size = filestatus.st_size;
  file_contents = (char*)malloc(filestatus.st_size);
  if (file_contents == NULL) {
    fprintf(stderr, "Memory error: unable to allocate %d bytes\n", file_size);
    return 1;
  }

  fp = fopen(filename, "rt");
  if (fp == NULL) {
    fprintf(stderr, "Unable to open %s\n", filename);
    fclose(fp);
    free(file_contents);
    return 1;
  }
  if (fread(file_contents, file_size, 1, fp) != 1) {
    fprintf(stderr, "Unable t read content of %s\n", filename);
    fclose(fp);
    free(file_contents);
    return 1;
  }
  fclose(fp);

  printf("%s\n", file_contents);

  printf("--------------------------------\n\n");

  json = (json_char*)file_contents;

  //   char buffer[]={"
  //   {
  //     \"name\": \"网站\",
  //     \"num\": 3,
  //     \"sites\": [
  //         \"Google\",
  //         \"Runoob\",
  //         \"Taobao\"
  //     ]
  //   }"
  // };
  // json = (json_char*)buffer;
  // file_size = sizeof(buffer);

  value = json_parse(json, file_size);

  if (value == NULL) {
    fprintf(stderr, "Unable to parse data\n");

    exit(1);
  }

  process_value(value, 0);

  printf("--------------------------------\n\n");

  ngx_str_t servlet = ngx_string("servlet");

  ret = ngx_find_json_name(value, &servlet);

  ngx_str_t target = ngx_string("servlet-class");

  ret = ngx_find_json_name(ret, &target);

  process_value(ret, 0);

  json_value_free(value);
  return 0;
}
