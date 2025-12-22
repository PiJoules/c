#include "ifstream.h"

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "istream.h"

static void file_input_stream_destroy(InputStream* is);
static int file_input_stream_read(InputStream* is);
static bool file_input_stream_eof(InputStream* is);

static const InputStreamVtable FileInputStreamVtable = {
    .dtor = file_input_stream_destroy,
    .read = file_input_stream_read,
    .eof = file_input_stream_eof,
};

void file_input_stream_construct(FileInputStream* fis, const char* input) {
  input_stream_construct(&fis->base, &FileInputStreamVtable);
  fis->input_file = fopen(input, "r");
  ASSERT_MSG(fis->input_file, "Could not open file '%s'", input);
  fis->input_name = strdup(input);
  fis->line = 1;
  fis->col = 0;
}

void file_input_stream_destroy(InputStream* is) {
  FileInputStream* fis = (FileInputStream*)is;
  fclose(fis->input_file);
  free((char*)fis->input_name);
}

int file_input_stream_read(InputStream* is) {
  FileInputStream* fis = (FileInputStream*)is;
  int c = fgetc(fis->input_file);

  if (c == '\n') {
    fis->line++;
    fis->col = 0;
  } else {
    fis->col++;
  }

  return c;
}

bool file_input_stream_eof(InputStream* is) {
  FileInputStream* fis = (FileInputStream*)is;
  return (bool)feof(fis->input_file);
}
