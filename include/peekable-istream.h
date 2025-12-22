#ifndef PEEKABLE_ISTREAM_H_
#define PEEKABLE_ISTREAM_H_

#include "istream.h"

typedef struct {
  InputStream base;

  InputStream* inner_;
  int lookahead_;
  bool has_lookahead_;
} PeekableInputStream;

void peekable_input_stream_construct(PeekableInputStream* is,
                                     InputStream* inner);
int peekable_input_stream_peek(PeekableInputStream* is);

#endif  // PEEKABLE_ISTREAM_H_
