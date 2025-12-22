#ifndef IFSTREAM_H_
#define IFSTREAM_H_

#include <string.h>

#include "istream.h"

typedef struct {
  InputStream base;
  void* input_file;
  const char* input_name;
  size_t line;
  size_t col;
} FileInputStream;

void file_input_stream_construct(FileInputStream* fis, const char* input);

#endif  // IFSTREAM_H_
