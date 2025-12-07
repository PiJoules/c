#include "istream.h"

void input_stream_construct(InputStream* is, const InputStreamVtable* vtable) {
  is->vtable = vtable;
}

void input_stream_destroy(InputStream* is) { is->vtable->dtor(is); }
int input_stream_read(InputStream* is) { return is->vtable->read(is); }
bool input_stream_eof(InputStream* is) { return is->vtable->eof(is); }
