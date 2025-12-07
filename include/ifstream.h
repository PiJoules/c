#ifndef IFSTREAM_H_
#define IFSTREAM_H_

#include "istream.h"

typedef struct {
  InputStream base;
  void* input_file;
} FileInputStream;

void file_input_stream_construct(FileInputStream* fis, const char* input);

#endif  // IFSTREAM_H_
