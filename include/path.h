#ifndef PATH_H_
#define PATH_H_

#include "cstring.h"

typedef struct {
  // TODO: Rename this.
  string path;
} Path;

void path_construct(Path* path, const string* other);
void path_construct_from_c_str(Path* path, const char* other);
void path_construct_with_current_dir(Path* path);
void path_construct_from_path(Path* path, const Path* other);

void path_construct_with_dirname(Path* this, const Path* other);
void path_construct_with_dirname_c_str(Path* this, const char* other);

void path_destroy(Path* path);
bool path_is_abs(const Path* this);
bool path_exists(const Path* this);
void path_append(Path* this, const Path* other);
bool path_is_file(const Path* this);
bool path_is_dir(const Path* this);

#endif  // PATH_H_
