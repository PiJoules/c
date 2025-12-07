#include "sstream.h"

#include <stdlib.h>
#include <string.h>

static void string_input_stream_destroy(InputStream* is);
static int string_input_stream_read(InputStream* is);
static bool string_input_stream_eof(InputStream* is);

static const InputStreamVtable StringInputStreamVtable = {
    .dtor = string_input_stream_destroy,
    .read = string_input_stream_read,
    .eof = string_input_stream_eof,
};

void string_input_stream_construct(StringInputStream* sis, const char* string) {
  input_stream_construct(&sis->base, &StringInputStreamVtable);
  sis->string = strdup(string);
  sis->pos_ = 0;
  sis->len_ = strlen(string);
}

void string_input_stream_destroy(InputStream* is) {
  StringInputStream* sis = (StringInputStream*)is;
  free(sis->string);
}

int string_input_stream_read(InputStream* is) {
  StringInputStream* sis = (StringInputStream*)is;
  if (sis->pos_ >= sis->len_)
    return -1;
  return (int)sis->string[sis->pos_++];
}

bool string_input_stream_eof(InputStream* is) {
  StringInputStream* sis = (StringInputStream*)is;
  return sis->pos_ >= sis->len_;
}
