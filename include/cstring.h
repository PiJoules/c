#ifndef CSTRING_H_
#define CSTRING_H_

#include <string.h>

typedef struct {
  char* data;

  // capacity is always greater than size to account for null terminator.
  size_t size;
  size_t capacity;
} string;

void string_construct(string* s);
void string_destroy(string* s);
void string_clear(string* s);
void string_reserve(string* s, size_t new_capacity);
void string_append(string* s, const char* suffix);
void string_append_range(string* s, const char* s2, size_t len);
void string_append_char(string* s, char c);
bool string_equals(const string* s, const char* s2);
char string_pop_front(string* s);
void string_assign(string* this, const string* other);
char string_back(const string* this);

void RunStringTests();

#endif  // CSTRING_H_
