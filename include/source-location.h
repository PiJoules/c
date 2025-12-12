#ifndef SOURCE_LOCATION_
#define SOURCE_LOCATION_

#include <string.h>

// Keep the struct opaque.
typedef struct {
  alignas(8) char data[24];
} SourceLocation;

void source_location_construct(SourceLocation* this, size_t line, size_t col,
                               const char* filename);
void source_location_destroy(SourceLocation* this);
size_t source_location_line(const SourceLocation* this);
size_t source_location_col(const SourceLocation* this);
const char* source_location_filename(const SourceLocation* this);

#endif  // SOURCE_LOCATION_
