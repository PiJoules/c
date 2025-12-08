#ifndef ISTREAM_H_
#define ISTREAM_H_

struct InputStream;

typedef struct {
  void (*dtor)(struct InputStream*);
  int (*read)(struct InputStream*);
  bool (*eof)(struct InputStream*);
} InputStreamVtable;

struct InputStream {
  const InputStreamVtable* vtable;
};
typedef struct InputStream InputStream;

void input_stream_construct(InputStream* is, const InputStreamVtable* vtable);
void input_stream_destroy(InputStream* is);
int input_stream_read(InputStream* is);
bool input_stream_eof(InputStream* is);

#endif  // ISTREAM_H_
