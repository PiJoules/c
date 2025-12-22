#ifndef PREPROCESSOR_H_
#define PREPROCESSOR_H_

#include "cstring.h"
#include "ifstream.h"
#include "path.h"
#include "peekable-istream.h"
#include "tree-map.h"
#include "vector.h"

struct PreprocessorInputStream {
  InputStream base;

  const vector* include_paths;  // vector of Path

  // This actually wraps a FileInputStream.
  PeekableInputStream* input_;

  // This is a buffer used for saving chars when handling directives. This needs
  // to have a persistent state in the event we read `#pragma` which isn't
  // handled by the preprocessor directly and needs to be handled by the
  // compiler proper.
  string saved_directive_chars_;

  // If we hit an `include` which we would expand, this nested stream will
  // represent a stream to that included file.
  struct PreprocessorInputStream* included_stream_;

  // Since the PreprocessorInputStream doesn't own the underlying FileInputStream,
  // if we create a new one dynamically via a #include, we need to save a ptr to it
  // so we can destroy it later.
  //FileInputStream *owned_fis;

  // Map of macros (as char pointers) to their expansions (as pointers to
  // `string`).
  TreeMap macros_;

  vector if_levels_;
};
typedef struct PreprocessorInputStream PreprocessorInputStream;

// The `input` is not owned by this preprocessor so it must be created and
// destroyed separately.
void preprocessor_input_stream_construct(PreprocessorInputStream* pp,
                                         FileInputStream* input,
                                         const vector* include_paths);

#endif  // PREPROCESSOR_H_
