#ifndef SSTREAM_H_
#define SSTREAM_H_

#include <string.h>

#include "istream.h"

typedef struct {
  InputStream base;
  char* string;

  size_t pos_;
  size_t len_;
} StringInputStream;

void string_input_stream_construct(StringInputStream* sis, const char* string);

#endif  // SSTREAM_H_
