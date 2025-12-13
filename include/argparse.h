#ifndef ARGPARSE_H_
#define ARGPARSE_H_

#include "tree-map.h"

enum ParseMode {
  // This argument is positional and required.
  PM_RequiredPositional,

  // This argument can be provided more than once and the result will be
  // accumulated in a vector. The default value for this parsed argument
  // is a vector of size 0.
  PM_Multiple,

  // When the argument is provided, it sets the corresponding ParsedArgument
  // to true. This argument can only be accepted once. This has a default
  // value of false.
  PM_StoreTrue,

  // This argument is optional and can store a single value. If it is
  // provided, it is stored as a string.
  PM_Optional,
};

struct Argument {
  // This is a unique single character representing this argument. No other
  // argument must have the same character used for this one. Use a value of
  // `0` to indicate this does not have a short version.
  char short_name;

  // This is a unique string representing this argument. No other
  // argument must have the same string used for this one. This must always
  // be provided.
  const char* long_name;

  const char* help;
  enum ParseMode mode;
};

enum ParsedArgKind {
  PAK_String,        // This represents a `const char *`.
  PAK_StringVector,  // This represents a `vector` of `const char *`.
  PAK_Bool,          // This represents a boolean.
};

struct ParsedArgument {
  enum ParsedArgKind kind;
  void* value;
  bool stored_value;
};

// clang-format off
//
// Parse the command line arguments given `argc` and `argv` and place them into
// an already constructed TreeMap `parsed_args`. `parsed_args` is a map of
// string keys representing the `long_name` in an `Argument` to `struct
// ParsedArgument` pointers. The parsable arguments are provided by `args` which
// should be an array of size `num_args`.
//
// All ParsedArguments are "owned" by the map and do not need to be freed.
// `destroy_parsed_args` will properly destroy and free any allocations created
// by `parse_args`.
//
// Example usage:
//
//   const struct Argument kArguments[] = {
//       ...
//       {'o', "output", "Output file", PM_Optional},
//       ...
//   };
//   const size_t kNumArguments = sizeof(kArguments) / sizeof(struct Argument);
//
//   TreeMap parsed_args;
//   string_tree_map_construct(&parsed_args);
//
//   // Check for a provided value.
//   const char* output = "out.obj";
//   struct ParsedArgument* output_arg;
//   if (tree_map_get(kNumArguments, kArguments, &parsed_args, "output", &output_arg))
//     output = output_arg->value;
//
//   destroy_parsed_args(&parsed_args);
//
// clang-format on
void parse_args(size_t num_args, const struct Argument* args,
                TreeMap* parsed_args, int argc, char** argv);

void destroy_parsed_args(TreeMap* parsed_args);

#endif  // ARGPARSE_H_
