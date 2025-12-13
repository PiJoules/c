#include "argparse.h"

#include <string.h>

#include "common.h"
#include "vector.h"

static bool matches_short_name(const void* it, void* arg) {
  return ((const struct Argument*)it)->short_name == *((char*)arg);
}

static bool matches_long_name(const void* it, void* arg) {
  return strcmp(((const struct Argument*)it)->long_name, arg) == 0;
}

static const struct Argument* get_nth_pos_arg(size_t num_args,
                                              const struct Argument* args,
                                              size_t n) {
  size_t i;
  size_t j;
  for (i = 0, j = 0; i < num_args; ++i) {
    if (args[i].mode == PM_RequiredPositional) {
      if (j == n)
        return &args[i];
      j++;
    }
  }
  return NULL;
}

static const char* get_next_string_argument_and_advance(int* current_arg,
                                                        char** argv) {
  // Get the corresponding argument. Note that if this is in the form
  //
  //   -Iargument
  //
  // where `I` is the short name, then the actual corresponding value
  // is part of the current `arg`. Otherwise, it is in the form
  //
  //   -I argument
  //
  // where the value is the next `arg`.
  const char* res;
  const char* arg = argv[*current_arg];
  if (arg[2] == 0) {
    // Next argument.
    res = argv[*current_arg + 1];

    *current_arg += 2;
  } else {
    // Remainder of this argument.
    res = &arg[2];

    *current_arg += 1;
  }
  return res;
}

void parse_args(size_t num_args, const struct Argument* args,
                TreeMap* parsed_args, int argc, char** argv) {
  const struct Argument* args_end = &args[num_args];
  size_t num_parsed_pos_args = 0;

  // Set then default values for parsed arguments.
  for (const struct Argument* it = args; it != args_end; ++it) {
    switch (it->mode) {
      case PM_RequiredPositional:
      case PM_Optional:
        // None of these modes uses a default value.
        break;
      case PM_StoreTrue: {
        if (!tree_map_get(parsed_args, it->long_name, NULL)) {
          struct ParsedArgument* parsed_arg =
              malloc(sizeof(struct ParsedArgument));
          parsed_arg->kind = PAK_Bool;
          parsed_arg->stored_value = false;
          tree_map_set(parsed_args, it->long_name, parsed_arg);
        }
        break;
      }
      case PM_Multiple: {
        if (!tree_map_get(parsed_args, it->long_name, NULL)) {
          struct ParsedArgument* parsed_arg =
              malloc(sizeof(struct ParsedArgument));
          parsed_arg->kind = PAK_StringVector;
          parsed_arg->value = malloc(sizeof(vector));
          vector_construct(parsed_arg->value, sizeof(char*), alignof(char*));
          tree_map_set(parsed_args, it->long_name, parsed_arg);
        }
        break;
      }
    }
  }

  for (int i = 1; i < argc;) {
    const char* arg = argv[i];

    if (arg[0] == '-') {
      // Optional argument.

      const struct Argument* found_arg;
      if (arg[1] != '-') {
        char short_name = arg[1];
        found_arg = find_if(args, args_end, sizeof(struct Argument),
                            matches_short_name, &short_name);
        ASSERT_MSG(found_arg != args_end, "Unknown argument '%s'", arg);
      } else {
        const char* long_name = &arg[2];
        found_arg = find_if(args, args_end, sizeof(struct Argument),
                            matches_long_name, (void*)long_name);
        ASSERT_MSG(found_arg != args_end, "Unknown argument '%s'", arg);
      }

      switch (found_arg->mode) {
        case PM_RequiredPositional:
          UNREACHABLE_MSG(
              "Required positional argument should not be handled in the "
              "optional argument cases");
        case PM_StoreTrue: {
          // This already has a default value set prior.
          struct ParsedArgument* parsed_arg = NULL;
          tree_map_get(parsed_args, found_arg->long_name, &parsed_arg);
          assert(parsed_arg);

          parsed_arg->kind = PAK_Bool;
          parsed_arg->stored_value = true;

          i += 1;
          break;
        }
        case PM_Optional: {
          ASSERT_MSG(!tree_map_get(parsed_args, found_arg->long_name, NULL),
                     "Duplicate optional argument '%s' was already provided.",
                     found_arg->long_name);
          const char* val = get_next_string_argument_and_advance(&i, argv);

          struct ParsedArgument* parsed_arg =
              malloc(sizeof(struct ParsedArgument));
          parsed_arg->kind = PAK_String;
          parsed_arg->value = (void*)val;
          tree_map_set(parsed_args, found_arg->long_name, parsed_arg);
          break;
        }
        case PM_Multiple: {
          // This already has a default value set prior.
          struct ParsedArgument* parsed_arg = NULL;
          tree_map_get(parsed_args, found_arg->long_name, &parsed_arg);
          ASSERT_MSG(parsed_arg, "Expected default value for '%s'",
                     found_arg->long_name);

          const char** storage = vector_append_storage(parsed_arg->value);
          *storage = get_next_string_argument_and_advance(&i, argv);
          break;
        }
      }
    } else {
      // Positional argument.
      const struct Argument* pos_arg =
          get_nth_pos_arg(num_args, args, num_parsed_pos_args);
      assert(pos_arg);
      ++num_parsed_pos_args;

      struct ParsedArgument* parsed_arg = malloc(sizeof(struct ParsedArgument));
      parsed_arg->kind = PAK_String;
      parsed_arg->value = (void*)arg;

      tree_map_set(parsed_args, pos_arg->long_name, parsed_arg);

      i += 1;
    }
  }
}

static void destroy_parsed_argument(const void* key, void* value, void* arg) {
  struct ParsedArgument* parsed_arg = value;
  switch (parsed_arg->kind) {
    case PAK_Bool:
    case PAK_String:
      // Do nothing since we actually save `const char *` here.
      break;
    case PAK_StringVector:
      vector_destroy(parsed_arg->value);
      free(parsed_arg->value);
      break;
  }
  free(parsed_arg);
}

void destroy_parsed_args(TreeMap* parsed_args) {
  tree_map_iterate(parsed_args, destroy_parsed_argument, /*arg=*/NULL);
  tree_map_destroy(parsed_args);
}
