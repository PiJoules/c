#ifndef VECTOR_H_
#define VECTOR_H_

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  size_t data_size;
  size_t data_alignment;

  void* data;
  size_t size;
  size_t capacity;  // In bytes
} vector;

void vector_construct(vector* v, size_t data_size, size_t data_alignment);
void vector_destroy(vector* v);
void vector_reserve(vector* v, size_t new_capacity);
void* vector_append_storage(vector* v);
void* vector_at(const vector* v, size_t i);
void* vector_back(const vector* v);
void* vector_begin(const vector* v);
void* vector_end(const vector* v);

void RunVectorTests();

#endif  // VECTOR_H_
