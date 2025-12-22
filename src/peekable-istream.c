#include "peekable-istream.h"

#include "istream.h"

static void peekable_input_stream_destroy(InputStream* is);
static int peekable_input_stream_read(InputStream* is);
static bool peekable_input_stream_eof(InputStream* is);

static const InputStreamVtable PeekableInputStreamVtable = {
    .dtor = peekable_input_stream_destroy,
    .read = peekable_input_stream_read,
    .eof = peekable_input_stream_eof,
};

void peekable_input_stream_construct(PeekableInputStream* is,
                                     InputStream* inner) {
  input_stream_construct(&is->base, &PeekableInputStreamVtable);
  is->inner_ = inner;
  is->has_lookahead_ = false;
}

void peekable_input_stream_destroy(InputStream*) {}

int peekable_input_stream_read(InputStream* is) {
  PeekableInputStream* p = (PeekableInputStream*)is;
  if (p->has_lookahead_) {
    p->has_lookahead_ = false;
    return p->lookahead_;
  }

  return input_stream_read(p->inner_);
}

int peekable_input_stream_peek(PeekableInputStream* is) {
  PeekableInputStream* p = (PeekableInputStream*)is;
  if (!p->has_lookahead_) {
    p->has_lookahead_ = true;
    p->lookahead_ = input_stream_read(p->inner_);
  }
  return p->lookahead_;
}

bool peekable_input_stream_eof(InputStream* is) {
  PeekableInputStream* p = (PeekableInputStream*)is;
  if (p->has_lookahead_)
    return false;
  return input_stream_eof(p->inner_);
}
