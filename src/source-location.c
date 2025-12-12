#include "source-location.h"

#include <string.h>

typedef struct {
  size_t line;
  size_t col;
  const char* filename;
} SourceLocationImpl;

static_assert(sizeof(SourceLocation) == sizeof(SourceLocationImpl));
static_assert(alignof(SourceLocation) == alignof(SourceLocationImpl));

void source_location_construct(SourceLocation* this, size_t line, size_t col,
                               const char* filename) {
  SourceLocationImpl* loc = (SourceLocationImpl*)this->data;
  loc->line = line;
  loc->col = col;
  loc->filename = filename;
}

void source_location_destroy(SourceLocation* this) {}

size_t source_location_line(const SourceLocation* this) {
  return ((const SourceLocationImpl*)this->data)->line;
}

size_t source_location_col(const SourceLocation* this) {
  return ((const SourceLocationImpl*)this->data)->col;
}

const char* source_location_filename(const SourceLocation* this) {
  return ((const SourceLocationImpl*)this->data)->filename;
}
