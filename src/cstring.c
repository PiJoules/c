#include "cstring.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static const size_t kDefaultStringCapacity = 16;

void string_construct(string* s) {
  s->data = malloc(kDefaultStringCapacity);
  assert(s->data);
  s->data[0] = 0;  // NULL terminator

  s->size = 0;
  s->capacity = kDefaultStringCapacity;
}

void string_destroy(string* s) { free(s->data); }

void string_clear(string* s) {
  s->size = 0;
  s->data[0] = 0;
}

void string_reserve(string* s, size_t new_capacity) {
  if (new_capacity <= s->capacity)
    return;

  // Double the capacity.
  s->capacity = new_capacity * 2;
  s->data = realloc(s->data, s->capacity);
  assert(s->data);
}

void string_append(string* s, const char* suffix) {
  size_t len = strlen(suffix);
  string_reserve(s, s->size + len + 1);  // +1 for null terminator
  memcpy(s->data + s->size, suffix, len);
  s->size += len;
  s->data[s->size] = 0;
}

void string_append_range(string* s, const char* s2, size_t len) {
  string_reserve(s, s->size + len + 1);  // +1 for null terminator
  memcpy(s->data + s->size, s2, len);
  s->size += len;
  s->data[s->size] = 0;
}

void string_append_char(string* s, char c) {
  string_reserve(s, s->size + 1 /*char size*/ + 1 /*Null terminator*/);
  s->data[s->size] = c;
  s->size++;
  s->data[s->size] = 0;
}

bool string_equals(const string* s, const char* s2) {
  return strcmp(s->data, s2) == 0;
}

char string_pop_front(string* s) {
  assert(s->size > 0);
  char front = s->data[0];
  memmove(s->data, s->data + 1,
          s->size);  // This also moves the null terminator.
  s->size--;
  return front;
}

void string_assign(string* this, const string* other) {
  string_clear(this);
  string_append(this, other->data);
}

char string_back(const string* this) {
  assert(this->size > 0);
  return this->data[this->size - 1];
}

///
/// Start String tests
///

static void TestStringConstruction() {
  string s;
  string_construct(&s);

  assert(s.data != NULL);
  assert(s.size == 0);
  assert(s.capacity > 0);
  assert(string_equals(&s, ""));

  string_destroy(&s);
}

static void TestStringCapacityReservation() {
  string s;
  string_construct(&s);
  size_t old_cap = s.capacity;
  string_reserve(&s, old_cap * 2);

  assert(s.capacity >= old_cap);
  assert(string_equals(&s, ""));
  assert(s.size == 0);
  assert(s.data[0] == 0 && "An empty string should be null-terminated.");

  string_destroy(&s);
}

static void TestStringAppend() {
  string s;
  string_construct(&s);
  string_append(&s, "abc");

  assert(s.size == 3);
  assert(string_equals(&s, "abc"));
  assert(s.data[s.size] == 0);

  string_append(&s, "xyz");

  assert(s.size == 6);
  assert(string_equals(&s, "abcxyz"));
  assert(s.data[s.size] == 0);

  string_destroy(&s);
}

static void TestStringAppendChar() {
  string s;
  string_construct(&s);
  string_append_char(&s, 'a');
  string_append_char(&s, 'b');
  string_append_char(&s, 'c');

  assert(s.size == 3);
  assert(string_equals(&s, "abc"));
  assert(s.data[s.size] == 0);

  string_append_char(&s, 'x');
  string_append_char(&s, 'y');
  string_append_char(&s, 'z');

  assert(s.size == 6);
  assert(string_equals(&s, "abcxyz"));
  assert(s.data[s.size] == 0);

  string_destroy(&s);
}

static void TestStringResizing() {
  string s;
  string_construct(&s);

  // Keep increasing the string size until we exceed the capacity.
  // We'll keep adding until the size is twice the original. The
  // string should automatically resize itself.
  size_t init_cap = s.capacity;
  for (size_t i = 0; i < init_cap * 2; ++i) {
    string_append_char(&s, 'a');
  }

  assert(s.size == init_cap * 2);

  for (size_t i = 0; i < init_cap * 2; ++i) {
    assert(s.data[i] == 'a');
  }

  assert(s.capacity >= init_cap * 2);
  assert(s.data[s.size] == 0);

  string_destroy(&s);
}

static void TestStringClear() {
  string s;
  string_construct(&s);

  string_append(&s, "abc");
  assert(s.size == 3);
  assert(string_equals(&s, "abc"));

  string_clear(&s);
  assert(s.size == 0);
  assert(string_equals(&s, ""));

  string_destroy(&s);
}

static void TestStringPopFront() {
  string s;
  string_construct(&s);

  string_append(&s, "abc");
  assert(s.size == 3);

  assert(string_pop_front(&s) == 'a');
  assert(s.size == 2);
  assert(string_equals(&s, "bc"));

  assert(string_pop_front(&s) == 'b');
  assert(s.size == 1);
  assert(string_equals(&s, "c"));

  assert(string_pop_front(&s) == 'c');
  assert(s.size == 0);
  assert(string_equals(&s, ""));

  string_destroy(&s);
}

void RunStringTests() {
  TestStringConstruction();
  TestStringCapacityReservation();
  TestStringAppend();
  TestStringAppendChar();
  TestStringResizing();
  TestStringClear();
  TestStringPopFront();
}

///
/// End String tests
///
