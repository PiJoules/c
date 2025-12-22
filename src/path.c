#include "path.h"

#include <assert.h>
#include <libgen.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "cstring.h"

static const char kPathSeparator = '/';

void path_construct(Path* path, const string* other) {
  string_construct(&path->path);
  string_assign(&path->path, other);
}

void path_construct_from_path(Path* path, const Path* other) {
  string_construct(&path->path);
  string_append(&path->path, other->path.data);
}

void path_construct_from_c_str(Path* path, const char* other) {
  string_construct(&path->path);
  string_append(&path->path, other);
}

void path_construct_with_current_dir(Path* path) {
  char cwd[PATH_MAX];
  ASSERT_MSG(getcwd(cwd, sizeof(cwd)) != NULL, "Couldn't get CWD");
  string_construct(&path->path);
  string_append(&path->path, cwd);
}

void path_construct_with_dirname(Path* this, const Path* other) {
  string_construct(&this->path);
  string_append(&this->path, dirname(other->path.data));
}

void path_construct_with_dirname_c_str(Path* this, const char* other) {
  string_construct(&this->path);
  string_append(&this->path, other);
}

void path_destroy(Path* path) { string_destroy(&path->path); }

bool path_is_abs(const Path* this) {
  // FIXME: Only works on *nix systems.
  return this->path.data[0] == '/';
}

bool path_exists(const Path* this) {
  return access(this->path.data, F_OK) == 0;
}

void path_append(Path* this, const Path* other) {
  // Do not append an absolute path to anything.
  assert(!path_is_abs(other));

  if (string_back(&this->path) != kPathSeparator)
    string_append_char(&this->path, kPathSeparator);

  string_append(&this->path, other->path.data);
}

bool path_is_file(const Path* this) {
  struct stat path_stat;
  stat(this->path.data, &path_stat);
  return S_ISREG(path_stat.st_mode);
}

bool path_is_dir(const Path* this) {
  struct stat path_stat;
  stat(this->path.data, &path_stat);
  return S_ISDIR(path_stat.st_mode);
}
