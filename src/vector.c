#include "vector.h"

#include <assert.h>
#include <stdint.h>

#include "common.h"

static const size_t kDefaultVectorCapacity = 16;

void vector_construct(vector* v, size_t data_size, size_t data_alignment) {
  assert(is_power_of_2(data_alignment) && "Invalid alignment");

  v->data_size = data_size;
  v->data_alignment = data_alignment;
  v->size = 0;

  v->capacity =
      kDefaultVectorCapacity < data_size ? data_size : kDefaultVectorCapacity;
  v->capacity = align_up(v->capacity, data_alignment);

  v->data = malloc(v->capacity);
  assert(v->data);
  assert((uintptr_t)v->data % data_alignment == 0);
}

// Each user of a vector should destroy the object in it before calling
// this.
void vector_destroy(vector* v) { free(v->data); }

void vector_reserve(vector* v, size_t new_capacity) {
  if (new_capacity <= v->capacity)
    return;

  // Double the capacity.
  v->capacity = align_up(new_capacity * 2, v->data_alignment);

  v->data = realloc(v->data, v->capacity);
  assert(v->data);
  assert((uintptr_t)v->data % v->data_alignment == 0);
}

void vector_resize(vector* v, size_t new_size) {
  if (v->size < new_size)
    vector_reserve(v, new_size);
  v->size = new_size;
}

// Allocate more storage to the vector and return a pointer to the new
// storage so it can be initialized by the user.
void* vector_append_storage(vector* v) {
  size_t allocation_size = align_up(v->data_size, v->data_alignment);
  vector_reserve(v, (v->size + 1) * allocation_size);
  return (char*)v->data + (allocation_size * (v->size++));
}

// Return a pointer to the allocation of this element.
void* vector_at(const vector* v, size_t i) {
  size_t allocation_size = align_up(v->data_size, v->data_alignment);
  void* ptr = (char*)v->data + allocation_size * i;
  assert((uintptr_t)ptr % v->data_alignment == 0);
  return ptr;
}

void* vector_back(const vector* v) {
  assert(v->size > 0);
  return vector_at(v, v->size - 1);
}

void* vector_begin(const vector* v) { return v->data; }

void* vector_end(const vector* v) { return vector_at(v, v->size); }

void vector_pop_back(vector* v) {
  assert(v->size > 0);
  v->size--;
}

///
/// Start Vector Tests
///

static void TestVectorConstruction() {
  vector v;
  vector_construct(&v, /*data_size=*/4, /*data_alignment=*/4);

  assert(v.data != NULL);
  assert(v.size == 0);
  assert(v.capacity > 0);
  assert(v.capacity % 4 == 0);
  assert((uintptr_t)v.data % 4 == 0);

  vector_destroy(&v);
}

static void TestVectorCapacityReservation() {
  vector v;
  vector_construct(&v, 4, 4);
  size_t old_cap = v.capacity;
  vector_reserve(&v, old_cap * 2);

  assert(v.capacity >= old_cap);
  assert(v.size == 0);

  vector_destroy(&v);
}

static void TestVectorAppend() {
  vector v;
  vector_construct(&v, sizeof(int), alignof(int));

  int* storage = vector_append_storage(&v);
  *storage = 10;
  int* storage2 = vector_append_storage(&v);
  *storage2 = 100;

  assert(*(int*)vector_at(&v, 0) == 10);
  assert((uintptr_t)vector_at(&v, 0) % alignof(int) == 0);

  assert(*(int*)vector_at(&v, 1) == 100);
  assert((uintptr_t)vector_at(&v, 1) % alignof(int) == 0);

  assert(v.size == 2);

  vector_destroy(&v);
}

static void TestVectorResizing() {
  vector v;
  vector_construct(&v, sizeof(int), alignof(int));

  // Keep increasing the vector size until we exceed the capacity.
  // We'll keep adding until the size is twice the original. The
  // vector should automatically resize itself.
  size_t init_cap = v.capacity;
  for (size_t i = 0; i < init_cap * 2; ++i) {
    vector_append_storage(&v);
  }

  assert(v.size == init_cap * 2);
  assert(v.capacity >= init_cap * 2);

  vector_destroy(&v);
}

static void TestVectorResize() {
  vector v;
  vector_construct(&v, sizeof(int), alignof(int));
  assert(v.size == 0);

  vector_resize(&v, 8);
  assert(v.size == 8);

  vector_destroy(&v);
}

static void TestVectorPopBack() {
  vector v;
  vector_construct(&v, sizeof(int), alignof(int));

  vector_append_storage(&v);
  vector_pop_back(&v);
  assert(v.size == 0);

  vector_destroy(&v);
}

void RunVectorTests() {
  TestVectorConstruction();
  TestVectorCapacityReservation();
  TestVectorAppend();
  TestVectorResizing();
  TestVectorResize();
  TestVectorPopBack();
}

///
/// End Vector Tests
///
