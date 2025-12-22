#include "preprocessor.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

#include "common.h"
#include "cstring.h"
#include "ifstream.h"
#include "istream.h"
#include "path.h"
#include "peekable-istream.h"
#include "vector.h"

static void preprocessor_input_stream_destroy(InputStream*);
static int preprocessor_input_stream_read(InputStream*);
static bool preprocessor_input_stream_eof(InputStream*);

static const InputStreamVtable PreprocessorInputStreamVtable = {
    .dtor = preprocessor_input_stream_destroy,
    .read = preprocessor_input_stream_read,
    .eof = preprocessor_input_stream_eof,
};

void preprocessor_input_stream_construct(PreprocessorInputStream* pp,
                                         FileInputStream* input,
                                         const vector* include_paths) {
  input_stream_construct(&pp->base, &PreprocessorInputStreamVtable);

  assert(input);
  pp->input_ = malloc(sizeof(PeekableInputStream));
  peekable_input_stream_construct(pp->input_, &input->base);

  string_construct(&pp->saved_directive_chars_);
  pp->included_stream_ = NULL;
  pp->include_paths = include_paths;
  string_tree_map_construct(&pp->macros_);

  pp->owned_fis = NULL;

  vector_construct(&pp->if_levels_, sizeof(int), alignof(int));
}

static void destroy_macro_string(const void*, void* value, void*) {
  string_destroy(value);
}

void preprocessor_input_stream_destroy(InputStream* input) {
  PreprocessorInputStream* pp = (PreprocessorInputStream*)input;

  string_destroy(&pp->saved_directive_chars_);

  input_stream_destroy(&pp->input_->base);
  free(pp->input_);

  if (pp->included_stream_) {
    input_stream_destroy(&pp->included_stream_->base);
    free(pp->included_stream_);

    InputStream *inner = pp->included_stream_;
  }

  tree_map_iterate(&pp->macros_, destroy_macro_string, /*arg=*/NULL);
  tree_map_destroy(&pp->macros_);

  vector_destroy(&pp->if_levels_);
}

static void preprocessor_find_included_file(PreprocessorInputStream* this,
                                            const Path* included_path,
                                            Path* dst_storage,
                                            bool check_includes_dir) {
  if (path_is_abs(included_path)) {
    path_construct(dst_storage, &included_path->path);
    return;
  }

  // Check include directories.

  if (check_includes_dir) {
    // First check the dir of the file we're looking at.
    FileInputStream* f = (FileInputStream*)this->input_->inner_;
    path_construct_with_dirname_c_str(dst_storage, f->input_name);

    path_append(dst_storage, included_path);
    if (path_exists(dst_storage))
      return;  // Found it. Return early.

    path_destroy(dst_storage);  // Couldn't find it. Destroy the constructed
                                // path and continue.
  }

  // Now check via the includes.
  for (const Path* it = vector_begin(this->include_paths);
       it != vector_end(this->include_paths); ++it) {
    path_construct_from_path(dst_storage, it);
    path_append(dst_storage, included_path);

    if (path_exists(dst_storage))
      return;

    path_destroy(dst_storage);
  }

  UNREACHABLE_MSG("Could not find include '%s'", included_path->path.data);
}

static void skip_to_next_line(PreprocessorInputStream* pp) {
  while (input_stream_read(&pp->input_->base) != '\n');
}

static string* get_identifier(PreprocessorInputStream* pp) {
  string* id = malloc(sizeof(string));
  string_construct(id);

  while (1) {
    int c = input_stream_read(&pp->input_->base);
    assert(c != EOF);

    if (isalnum(c) || c == '_')
      string_append_char(id, (char)c);
    else
      break;
  }

  return id;
}

static void handle_conditional_directive(PreprocessorInputStream* pp,
                                         string* directive) {
  assert(pp->saved_directive_chars_.size == 0);
  assert(!pp->included_stream_);

  FileInputStream* f = (FileInputStream*)pp->input_->inner_;
  size_t* line = &f->line;
  size_t* col = &f->col;
  const char* input_name = f->input_name;
  (void)input_name;
  (void)col;
  (void)line;
  //printf("appended at %s:%zu:%zu\n", input_name, *line, *col);

  int* back = vector_append_storage(&pp->if_levels_);
  *back = 1;

  if (string_equals(directive, "ifndef")) {
    // Found a `#ifndef`. Read an identifier.
    string* macro = get_identifier(pp);

    if (!tree_map_has(&pp->macros_, macro->data)) {
      // Continue to the next line. Any characters after the macro are ignored.
      skip_to_next_line(pp);

      string_destroy(macro);
      free(macro);
      return;
    }

    string_destroy(macro);
    free(macro);
  } else if (string_equals(directive, "ifdef")) {
    // Found a `#ifdef`. Read an identifier.
    string* macro = get_identifier(pp);

    if (tree_map_has(&pp->macros_, macro->data)) {
      // Continue to the next line. Any characters after the macro are ignored.
      skip_to_next_line(pp);

      string_destroy(macro);
      free(macro);
      return;
    }

    string_destroy(macro);
    free(macro);
  } else {
    UNREACHABLE_MSG("Handle conditional directive '%s'", directive->data);
  }

  // Continue to the next `else` or `elif` or `endif`.

  while (1) {
    bool in_string = false;
    int starting_char;
    while (1) {
      int c = input_stream_read(&pp->input_->base);
      if (c == EOF)
        return;

      if (in_string) {
        if (c == starting_char) {
          in_string = false;
        } else if (c == '\\') {
          // Consume next char.
          input_stream_read(&pp->input_->base);
        }
        continue;
      }

      if (c == '\'' || c == '"') {
        in_string = true;
        starting_char = c;
        continue;
      }

      if (c == '#')
        break;
    }

    // Found the start of a macro. Look for the next keyword.
    string *directive = get_identifier(pp);

    if (string_equals(directive, "if") || string_equals(directive, "ifdef") ||
        string_equals(directive, "ifndef")) {
      skip_to_next_line(pp);
      (*back)++;
    } else if (string_equals(directive, "else")) {
      assert(*back > 0);

      if (*back == 1) {
        // This is the right block. Jump into it.
        skip_to_next_line(pp);
        string_destroy_and_free(directive);
        return;
      }

      (*back)--;
    } else if (string_equals(directive, "elif")) {
      UNREACHABLE_MSG("TODO: handle this");
    } else if (string_equals(directive, "endif")) {
      assert(*back > 0);

      if (*back == 1) {
        // Found no prior block we could jump into.
        vector_pop_back(&pp->if_levels_);
        skip_to_next_line(pp);
        string_destroy_and_free(directive);
        return;
      }

      (*back)--;
    }

    string_destroy_and_free(directive);
  }
}

static string* get_define_value(PreprocessorInputStream* pp) {
  string* def = malloc(sizeof(string));
  string_construct(def);

  while (1) {
    int c = input_stream_read(&pp->input_->base);
    assert(c != EOF);

    if (c == '\\') {
      c = input_stream_read(&pp->input_->base);
      string_append_char(def, (char)c);
    } else if (c == '\n') {
      break;
    } else {
      string_append_char(def, (char)c);
    }
  }

  return def;
}

int preprocessor_input_stream_read(InputStream* input) {
  PreprocessorInputStream* pp = (PreprocessorInputStream*)input;

  FileInputStream* f = (FileInputStream*)pp->input_->inner_;
  size_t* line = &f->line;
  size_t* col = &f->col;
  const char* input_name = f->input_name;

  if (pp->saved_directive_chars_.size > 0) {
    // FIXME: We should be adding an implicit cast in the AST so we don't need
    // to explicitly cast here.
    return (int)string_pop_front(&pp->saved_directive_chars_);
  }

  if (!pp->included_stream_) {
    // Handle preprocessor expansions.
    int c = input_stream_read(&pp->input_->base);

    // Take into account string literals where we cannot treat `# <directive>`
    // with normal directive-handlign logic.
    if (c == '"' || c == '\'') {
      char starting_char = (char)c;
      string_append_char(&pp->saved_directive_chars_, starting_char);

      // Keep reading chars until the closing char, accounting for escaped
      // chars.
      while (1) {
        c = input_stream_read(&pp->input_->base);
        string_append_char(&pp->saved_directive_chars_, (char)c);

        if (c == '\\') {
          c = input_stream_read(&pp->input_->base);
          string_append_char(&pp->saved_directive_chars_, (char)c);
        } else if (c == starting_char) {
          break;
        }
      }

      return (int)string_pop_front(&pp->saved_directive_chars_);
    }

    // Also ignore comments.
    if (c == '/') {
      string_append_char(&pp->saved_directive_chars_, (char)c);

      c = input_stream_read(&pp->input_->base);
      string_append_char(&pp->saved_directive_chars_, (char)c);

      if (c == '/') {
        // Read the entire line into the buffer then return early.
        while (1) {
          c = input_stream_read(&pp->input_->base);
          string_append_char(&pp->saved_directive_chars_, (char)c);
          if (c == '\n')
            break;
        }
      } else if (c == '*') {
        bool last_char_was_star = false;
        while (1) {
          c = input_stream_read(&pp->input_->base);
          string_append_char(&pp->saved_directive_chars_, (char)c);

          if (last_char_was_star && c == '/') {
            // End the multiline comment.
            break;
          }

          last_char_was_star = c == '*';
        }
      }

      return (int)string_pop_front(&pp->saved_directive_chars_);
    }

    if (c != '#')
      return c;

    // Need to save it in case this is a #pragma.
    string_append_char(&pp->saved_directive_chars_, '#');

    // Skip WS.
    while (isspace(c = input_stream_read(&pp->input_->base))) {
      assert(c != EOF);
      string_append_char(&pp->saved_directive_chars_, (char)c);
    }
    assert(c != EOF);

    string directive;
    string_construct(&directive);
    string_append_char(&directive, (char)c);
    string_append_char(&pp->saved_directive_chars_, (char)c);

    while (!isspace(c = input_stream_read(&pp->input_->base))) {
      string_append_char(&directive, (char)c);
      string_append_char(&pp->saved_directive_chars_, (char)c);
    }

    //printf("Handling directive '%s' at %s:%zu:%zu\n", directive.data, input_name, *line, *col);

    if (string_equals(&directive, "include")) {
      // Found a `#include`. Read either <path> or "path".
      while (isspace(c)) c = input_stream_read(&pp->input_->base);

      assert(c == '<' || c == '"');
      int closing_c = c == '<' ? '>' : '"';

      string include_path;
      string_construct(&include_path);

      // Now read the path. Note the path can have spaces in it.
      while ((c = input_stream_read(&pp->input_->base)) != closing_c) {
        assert(c != EOF);
        string_append_char(&include_path, (char)c);
      }

      Path path;
      path_construct(&path, &include_path);

      Path full_included_path;
      preprocessor_find_included_file(pp, &path, &full_included_path,
                                      closing_c == '"');

      // Open the file and assign it to a new nested preprocessor.
      FileInputStream* include_file = malloc(sizeof(FileInputStream));
      file_input_stream_construct(include_file, full_included_path.path.data);
      pp->included_stream_ = malloc(sizeof(PreprocessorInputStream));
      preprocessor_input_stream_construct(pp->included_stream_, include_file,
                                          pp->include_paths);

      string_destroy(&include_path);
      path_destroy(&path);
      path_destroy(&full_included_path);
    } else if (string_equals(&directive, "pragma")) {
      // #pragmas are the one exception where the preprocessor doesn't handle
      // them, so we need to save the characters for passing them to the
      // compiler proper for handling.
      //
      // Just add the last read char back to `saved_directive_chars_` so it can
      // eventually be popped later.
      assert(c != EOF);
      string_append_char(&pp->saved_directive_chars_, (char)c);
      string_destroy(&directive);
      return (int)string_pop_front(&pp->saved_directive_chars_);
    } else if (string_equals(&directive, "ifndef") || string_equals(&directive, "ifdef")) {
      string_clear(&pp->saved_directive_chars_);

      handle_conditional_directive(pp, &directive);
      string_destroy(&directive);

      // We should've jumped to the next valid line to lex, so read normally.
      return preprocessor_input_stream_read(&pp->base);
    } else if (string_equals(&directive, "endif")) {
      ASSERT_MSG(pp->if_levels_.size,
                 "Found #endif with no corresponding #if at %s:%zu:%zu",
                 input_name, *line, *col);

      int back = *(int*)vector_back(&pp->if_levels_);
      ASSERT_MSG(back == 1, "num if_levels: %zu, back: %d", pp->if_levels_.size,
                 back);
      //printf("popped at %s:%zu:%zu\n", input_name, *line, *col);
      vector_pop_back(&pp->if_levels_);

      // Just continue to next line.
      skip_to_next_line(pp);

      string_destroy(&directive);
      string_clear(&pp->saved_directive_chars_);

      return preprocessor_input_stream_read(&pp->base);
    } else if (string_equals(&directive, "else")) {
      // We should've already jumped prior to the appropriate conditional block
      // via `handle_conditional_directive`. If we ran into a #else, then it
      // should only have been after handling all conditions from that block.
      // Just continue to next line.
      skip_to_next_line(pp);

      string_destroy(&directive);
      string_clear(&pp->saved_directive_chars_);

      return preprocessor_input_stream_read(&pp->base);
    } else if (string_equals(&directive, "define")) {
      string* id = get_identifier(pp);
      string* def = get_define_value(pp);

      // TODO: Warn on overriding a macro.
      tree_map_set(&pp->macros_, id->data, def);

      // The last character should've been a newline, so no need to skip.
      string_destroy(id);
      free(id);
      string_destroy(&directive);
      string_clear(&pp->saved_directive_chars_);

      return preprocessor_input_stream_read(&pp->base);
    } else if (string_equals(&directive, "undef")) {
      string* id = get_identifier(pp);

      assert(tree_map_has(&pp->macros_, id->data));
      tree_map_erase(&pp->macros_, id->data);

      string_destroy(id);
      free(id);
      string_destroy(&directive);
      string_clear(&pp->saved_directive_chars_);

      // The last character should've been a newline, so no need to skip.
      return preprocessor_input_stream_read(&pp->base);
    } else {
      UNREACHABLE_MSG("TODO: Handle preprocessor directive '%s'",
                      directive.data);
    }

    string_destroy(&directive);
    string_clear(&pp->saved_directive_chars_);

    assert(pp->included_stream_);
  }

  if (preprocessor_input_stream_eof(&pp->included_stream_->base)) {
    input_stream_destroy(&pp->included_stream_->base);
    free(pp->included_stream_);
    pp->included_stream_ = NULL;
    return preprocessor_input_stream_read(&pp->base);
  }

  return input_stream_read(&pp->included_stream_->base);
}

bool preprocessor_input_stream_eof(InputStream* input) {
  PreprocessorInputStream* pp = (PreprocessorInputStream*)input;
  if (pp->included_stream_)
    return input_stream_eof(&pp->included_stream_->base);
  return input_stream_eof(&pp->input_->base);
}
