#include "parser.h"

#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "istream.h"
#include "lexer.h"
#include "sstream.h"
#include "stmt.h"
#include "top-level-node.h"
#include "tree-map.h"

void parser_construct(Parser* parser, InputStream* input) {
  lexer_construct(&parser->lexer, input);
  parser->has_lookahead = false;
  string_tree_map_construct(&parser->typedef_types);
}

void parser_destroy(Parser* parser) {
  lexer_destroy(&parser->lexer);
  if (parser->has_lookahead)
    token_destroy(&parser->lookahead);
  tree_map_destroy(&parser->typedef_types);
}

void parser_define_named_type(Parser* parser, const char* name) {
  tree_map_set(&parser->typedef_types, name, /*val=*/NULL);
}

bool parser_has_named_type(const Parser* parser, const char* name) {
  return tree_map_get(&parser->typedef_types, name, /*val=*/NULL);
}

static void expect_token(const Token* tok, TokenKind expected) {
  ASSERT_MSG(tok->kind == expected,
             "%zu:%zu: Expected token %d but found %d: '%s'", tok->line,
             tok->col, expected, tok->kind, tok->chars.data);
}

Token parser_pop_token(Parser* parser) {
  if (parser->has_lookahead) {
    parser->has_lookahead = false;
    return parser->lookahead;
  }
  return lex(&parser->lexer);
}

const Token* parser_peek_token(Parser* parser) {
  if (!parser->has_lookahead) {
    parser->lookahead = lex(&parser->lexer);
    parser->has_lookahead = true;
  }
  return &parser->lookahead;
}

bool next_token_is(Parser* parser, TokenKind kind) {
  return parser_peek_token(parser)->kind == kind;
}

static void expect_next_token(Parser* parser, TokenKind expected) {
  expect_token(parser_peek_token(parser), expected);
}

void parser_skip_next_token(Parser* parser) {
  Token token = parser_pop_token(parser);
  token_destroy(&token);
}

void parser_consume_token(Parser* parser, TokenKind kind) {
  Token next = parser_pop_token(parser);
  ASSERT_MSG(next.kind == kind,
             "%zu:%zu: Expected token kind %d but found %d: '%s'", next.line,
             next.col, kind, next.kind, next.chars.data);
  token_destroy(&next);
}

void parser_consume_token_if_matches(Parser* parser, TokenKind kind) {
  if (next_token_is(parser, kind))
    parser_consume_token(parser, kind);
}

Qualifiers parse_maybe_qualifiers(Parser* parser, bool* found) {
  Qualifiers quals = 0;
  if (found)
    *found = true;
  while (true) {
    switch (parser_peek_token(parser)->kind) {
      case TK_Const:
        quals |= kConstMask;
        parser_skip_next_token(parser);
        continue;
      case TK_Volatile:
        quals |= kVolatileMask;
        parser_skip_next_token(parser);
        continue;
      case TK_Restrict:
        quals |= kRestrictMask;
        parser_skip_next_token(parser);
        continue;
      default:
        if (found)
          *found = false;
    }
    break;
  }
  return quals;
}

Type* maybe_parse_pointers_and_qualifiers(Parser* parser, Type* base,
                                          Type*** type_usage_addr) {
  for (; next_token_is(parser, TK_Star);) {
    parser_consume_token(parser, TK_Star);

    PointerType* ptr = create_pointer_to(base);
    if (type_usage_addr && *type_usage_addr == NULL) {
      // Check NULL to capture the very first usage.
      Type** pointee = &ptr->pointee;
      *type_usage_addr = pointee;
    }
    base = &ptr->type;

    // May be more '*'s or qualifiers.
    base->qualifiers |= parse_maybe_qualifiers(parser, /*found=*/NULL);
  }
  return base;
}

bool is_token_type_token(const Parser* parser, const Token* tok) {
  bool is_type = is_builtin_type_token(tok->kind) ||
                 is_qualifier_token(tok->kind) || tok->kind == TK_Enum ||
                 tok->kind == TK_Struct || tok->kind == TK_Union;

  // Need to check for typedefs
  if (!is_type && tok->kind == TK_Identifier)
    is_type = parser_has_named_type(parser, tok->chars.data);

  return is_type;
}

static Type* parse_type_for_declaration_impl(Parser* parser, char** name,
                                             FoundStorageClasses* storage,
                                             Type* base_type) {
  Type* type = maybe_parse_pointers_and_qualifiers(parser, base_type,
                                                   /*type_usage_addr=*/NULL);
  Type* ret = parse_declarator(parser, type, name, /*type_usage_addr=*/NULL);

  // https://gcc.gnu.org/onlinedocs/gcc/Asm-Labels.html
  //
  // Parse an asm label after a declarator. The compiler should replace all
  // references to this variable with the name specified in the asm label.
  //
  // TODO: Handle this. For now, we just consume it.
  if (next_token_is(parser, TK_Asm))
    parser_consume_asm_label(parser);

  // https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html
  //
  // An attribute specifier list may appear immediately before the comma, ‘=’,
  // or semicolon terminating the declaration of an identifier other than a
  // function definition. Such attribute specifiers apply to the declared object
  // or function. Where an assembler name for an object or function is specified
  // (see Controlling Names Used in Assembler Code), the attribute must follow
  // the asm specification.
  bool found_attr = next_token_is(parser, TK_Attribute);

  for (; parser_peek_token(parser)->kind == TK_Attribute;)
    parser_consume_attribute(parser);

  if (found_attr) {
    assert(parser_peek_token(parser)->kind == TK_Semicolon ||
           parser_peek_token(parser)->kind == TK_Comma ||
           parser_peek_token(parser)->kind == TK_Assign);
  }

  return ret;
}

Type* parse_type_for_declaration(Parser* parser, char** name,
                                 FoundStorageClasses* storage) {
  // TODO: Handle inlines?
  Type* type = parse_specifiers_and_qualifiers_and_storage(
      parser, storage, /*found_inline=*/NULL);
  return parse_type_for_declaration_impl(parser, name, storage, type);
}

// TODO: Don't handle attributes for now. Just consume them.
void parser_consume_attribute(Parser* parser) {
  parser_consume_token(parser, TK_Attribute);
  parser_consume_token(parser, TK_LPar);
  parser_consume_token(parser, TK_LPar);

  // This is a cheeky way of just consuming the whole attribute.
  // Consume tokens until we match the opening left parenthesis.
  int par_count = 2;
  for (; par_count;) {
    Token tok = parser_pop_token(parser);

    if (tok.kind == TK_RPar) {
      par_count--;
    } else if (tok.kind == TK_LPar) {
      par_count++;
    }

    token_destroy(&tok);
  }
}

void parser_consume_asm_label(Parser* parser) {
  parser_consume_token(parser, TK_Asm);
  parser_consume_token(parser, TK_LPar);

  // This is a cheeky way of just consuming the whole attribute.
  // Consume tokens until we match the opening left parenthesis.
  int par_count = 1;
  for (; par_count;) {
    Token tok = parser_pop_token(parser);

    if (tok.kind == TK_RPar) {
      par_count--;
    } else if (tok.kind == TK_LPar) {
      par_count++;
    }

    token_destroy(&tok);
  }
}

void parser_consume_pragma(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  parser_consume_token(parser, TK_Hash);

  assert(parser_peek_token(parser)->line == line &&
         "# and pragma must be on same line");
  parser_consume_token(parser, TK_Pragma);

  // Pragmas take token strings which generally end at the end of a line,
  // also accounting for escaped newlines. For now, we can just collect
  // tokens until we see the next line.
  //
  // TODO: Even if this is for consumptions, we need to account for escaped
  // newlines.
  while (1) {
    const Token* peek = parser_peek_token(parser);
    if (peek->line != line)
      break;
    parser_skip_next_token(parser);
  }
}

static void parse_struct_or_union_name_and_members_impl(Parser* parser,
                                                        char** name,
                                                        vector** members,
                                                        bool is_struct) {
  assert(name);
  assert(members);

  parser_consume_token(parser, is_struct ? TK_Struct : TK_Union);

  const Token* peek = parser_peek_token(parser);
  if (peek->kind == TK_Identifier) {
    *name = strdup(peek->chars.data);
    parser_consume_token(parser, TK_Identifier);
  } else {
    *name = NULL;
  }

  if (!next_token_is(parser, TK_LCurlyBrace))
    return;

  parser_consume_token(parser, TK_LCurlyBrace);

  *members = malloc(sizeof(vector));
  vector_construct(*members, sizeof(Member), alignof(Member));
  for (; !next_token_is(parser, TK_RCurlyBrace);) {
    // https://gcc.gnu.org/onlinedocs/gcc/Alternate-Keywords.html
    //
    // Writing __extension__ before an expression prevents warnings about
    // extensions within that expression.
    if (next_token_is(parser, TK_Extension))
      parser_consume_token(parser, TK_Extension);

    char* member_name = NULL;
    Type* member_ty =
        parse_type_for_declaration(parser, &member_name, /*storage=*/NULL);
    assert(member_name);

    Expr* bitfield = NULL;
    if (next_token_is(parser, TK_Colon)) {
      parser_consume_token(parser, TK_Colon);
      bitfield = parse_expr(parser);
    }

    Member* member = vector_append_storage(*members);
    member->type = member_ty;
    member->name = member_name;
    member->bitfield = bitfield;

    if (parser_peek_token(parser)->kind == TK_Attribute) {
      parser_consume_attribute(parser);
    }

    parser_consume_token(parser, TK_Semicolon);
  }

  parser_consume_token(parser, TK_RCurlyBrace);
}

void parse_struct_name_and_members(Parser* parser, char** name,
                                   vector** members) {
  parse_struct_or_union_name_and_members_impl(parser, name, members,
                                              /*is_struct=*/true);
}

void parse_union_name_and_members(Parser* parser, char** name,
                                  vector** members) {
  parse_struct_or_union_name_and_members_impl(parser, name, members,
                                              /*is_struct=*/false);
}

StructType* parse_struct_type(Parser* parser) {
  char* name = NULL;
  vector* members = NULL;
  parse_struct_name_and_members(parser, &name, &members);

  StructType* struct_ty = malloc(sizeof(StructType));
  struct_type_construct(struct_ty, name, members);
  return struct_ty;
}

UnionType* parse_union_type(Parser* parser) {
  char* name = NULL;
  vector* members = NULL;
  parse_union_name_and_members(parser, &name, &members);

  UnionType* union_ty = malloc(sizeof(UnionType));
  union_type_construct(union_ty, name, members);
  return union_ty;
}

void parse_enum_name_and_members(Parser* parser, char** name,
                                 vector** members) {
  assert(name);
  assert(members);

  parser_consume_token(parser, TK_Enum);

  const Token* peek = parser_peek_token(parser);
  if (peek->kind == TK_Identifier) {
    *name = strdup(peek->chars.data);
    parser_consume_token(parser, TK_Identifier);
  } else {
    *name = NULL;
  }

  if (!next_token_is(parser, TK_LCurlyBrace))
    return;

  parser_consume_token(parser, TK_LCurlyBrace);

  *members = malloc(sizeof(vector));
  vector_construct(*members, sizeof(EnumMember), alignof(EnumMember));
  for (; !next_token_is(parser, TK_RCurlyBrace);) {
    Token next = parser_pop_token(parser);
    assert(next.kind == TK_Identifier);
    char* member_name = strdup(next.chars.data);
    token_destroy(&next);

    Expr* member_val;
    if (next_token_is(parser, TK_Assign)) {
      parser_consume_token(parser, TK_Assign);
      // The RHS can only be a `constant_expression` which can only be a
      // `conditional_expression. This prevents enums which assign values from
      // looking like comma-separated expressions.
      member_val = parse_conditional_expr(parser);
    } else {
      member_val = NULL;
    }

    EnumMember* member = vector_append_storage(*members);
    member->name = member_name;
    member->value = member_val;

    parser_consume_token_if_matches(parser, TK_Comma);
  }

  parser_consume_token(parser, TK_RCurlyBrace);
}

EnumType* parse_enum_type(Parser* parser) {
  char* name = NULL;
  vector* members = NULL;
  parse_enum_name_and_members(parser, &name, &members);

  EnumType* enum_ty = malloc(sizeof(EnumType));
  enum_type_construct(enum_ty, name, members);
  return enum_ty;
}

struct TypeSpecifier {
  unsigned int char_ : 1;
  unsigned int short_ : 1;
  unsigned int int_ : 1;
  unsigned int signed_ : 1;
  unsigned int unsigned_ : 1;
  unsigned int long_ : 2;  // long can be specified up to 2 times
  unsigned int float_ : 1;
  unsigned int float128_ : 1;
  unsigned int double_ : 1;
  unsigned int complex_ : 1;
  unsigned int void_ : 1;
  unsigned int bool_ : 1;
  unsigned int named_ : 1;
  unsigned int struct_ : 1;
  unsigned int enum_ : 1;
  unsigned int union_ : 1;
  unsigned int builtin_va_list_ : 1;
};

Type* parse_specifiers_and_qualifiers_and_storage(Parser* parser,
                                                  FoundStorageClasses* storage,
                                                  bool* found_inline) {
  Qualifiers quals = 0;

  struct TypeSpecifier spec = {};

  if (found_inline)
    *found_inline = false;

  StructType* struct_ty;
  EnumType* enum_ty;
  UnionType* union_ty;
  bool should_stop = false;
  char* name;
  for (; !should_stop;) {
    bool consume_next_token = true;
    const Token* peek = parser_peek_token(parser);
    switch (peek->kind) {
      case TK_Struct:
        assert(!spec.struct_);
        assert(!spec.named_);
        assert(!spec.enum_);
        assert(!spec.union_);
        spec.struct_ = 1;
        consume_next_token = false;
        struct_ty = parse_struct_type(parser);
        break;
      case TK_Enum:
        assert(!spec.struct_);
        assert(!spec.named_);
        assert(!spec.enum_);
        assert(!spec.union_);
        spec.enum_ = 1;
        consume_next_token = false;
        enum_ty = parse_enum_type(parser);
        break;
      case TK_Union:
        assert(!spec.struct_);
        assert(!spec.named_);
        assert(!spec.enum_);
        assert(!spec.union_);
        spec.union_ = 1;
        consume_next_token = false;
        union_ty = parse_union_type(parser);
        break;
      case TK_Char:
        spec.char_ = 1;
        break;
      case TK_Short:
        spec.short_ = 1;
        break;
      case TK_Int:
        spec.int_ = 1;
        break;
      case TK_Unsigned:
        spec.unsigned_ = 1;
        break;
      case TK_Signed:
        spec.unsigned_ = 0;
        break;
      case TK_Long:
        spec.long_++;
        assert(spec.long_ < 3);
        break;
      case TK_BuiltinVAList:
        spec.builtin_va_list_ = 1;
        break;
      case TK_Float:
        spec.float_ = 1;
        break;
      case TK_Float128:
        spec.float128_ = 1;
        break;
      case TK_Double:
        spec.double_ = 1;
        break;
      case TK_Complex:
        spec.complex_ = 1;
        break;
      case TK_Void:
        spec.void_ = 1;
        break;
      case TK_Bool:
        spec.bool_ = 1;
        break;
      case TK_Const:
        quals |= kConstMask;
        break;
      case TK_Volatile:
        quals |= kVolatileMask;
        break;
      case TK_Restrict:
        quals |= kRestrictMask;
        break;
      case TK_Extern:
        if (storage)
          storage->extern_ = 1;
        break;
      case TK_Static:
        if (storage)
          storage->static_ = 1;
        break;
      case TK_Inline:
        if (found_inline)
          *found_inline = true;
        break;
      case TK_Auto:
        if (storage)
          storage->auto_ = 1;
        break;
      case TK_Register:
        if (storage)
          storage->register_ = 1;
        break;
      case TK_ThreadLocal:
        if (storage)
          storage->thread_local_ = 1;
        break;
      case TK_Identifier:
        if (is_token_type_token(parser, peek)) {
          assert(!spec.struct_);
          assert(!spec.named_);
          assert(!spec.enum_);
          spec.named_ = 1;
          name = strdup(peek->chars.data);
          break;
        }
        [[fallthrough]];
      default:
        consume_next_token = false;
        should_stop = true;
        break;
    }

    if (consume_next_token)
      parser_skip_next_token(parser);
  }

  if (spec.named_) {
    NamedType* nt = create_named_type(name);
    nt->type.qualifiers = quals;
    free(name);
    return &nt->type;
  }

  if (spec.struct_) {
    struct_ty->type.qualifiers = quals;
    return &struct_ty->type;
  }

  if (spec.enum_) {
    enum_ty->type.qualifiers = quals;
    return &enum_ty->type;
  }

  if (spec.union_) {
    union_ty->type.qualifiers = quals;
    return &union_ty->type;
  }

  BuiltinTypeKind kind;
  if (spec.char_) {
    if (spec.signed_) {
      kind = BTK_SignedChar;
    } else if (spec.unsigned_) {
      kind = BTK_UnsignedChar;
    } else {
      kind = BTK_Char;
    }
  } else if (spec.short_) {
    if (spec.unsigned_) {
      kind = BTK_UnsignedShort;
    } else {
      kind = BTK_Short;
    }
  } else if (spec.long_) {
    assert(spec.long_ < 3);
    if (spec.long_ == 2) {
      if (spec.unsigned_) {
        kind = BTK_UnsignedLongLong;
      } else {
        kind = BTK_LongLong;
      }
    } else {
      if (spec.double_) {
        if (spec.complex_) {
          kind = BTK_ComplexLongDouble;
        } else {
          kind = BTK_LongDouble;
        }
      } else if (spec.unsigned_) {
        kind = BTK_UnsignedLong;
      } else {
        kind = BTK_Long;
      }
    }
  } else if (spec.complex_) {
    if (spec.float_) {
      kind = BTK_ComplexFloat;
    } else {
      assert(spec.double_);
      kind = BTK_ComplexDouble;
    }
    // Note `_Complex long double` should be handled in the top level
    // `spec.long_` branch.
  } else if (spec.int_) {
    if (spec.unsigned_) {
      kind = BTK_UnsignedInt;
    } else {
      kind = BTK_Int;
    }
  } else if (spec.unsigned_) {
    kind = BTK_UnsignedInt;
  } else if (spec.float_) {
    kind = BTK_Float;
  } else if (spec.double_) {
    kind = BTK_Double;
  } else if (spec.float128_) {
    kind = BTK_Float128;
  } else if (spec.bool_) {
    kind = BTK_Bool;
  } else if (spec.void_) {
    kind = BTK_Void;
  } else if (spec.builtin_va_list_) {
    kind = BTK_BuiltinVAList;
  } else {
    const Token* tok = parser_peek_token(parser);
    UNREACHABLE_MSG(
        "%zu:%zu: parse_specifiers_and_qualifiers: Unhandled token (%d): '%s'",
        tok->line, tok->col, tok->kind, tok->chars.data);
  }

  BuiltinType* bt = malloc(sizeof(BuiltinType));
  builtin_type_construct(bt, kind);
  bt->type.qualifiers = quals;
  return &bt->type;
}

//
// <pointer> = "*" <typequalifier>*
//           | "*" <pointer>
//           | "*" <typequalifier>* <pointer>
//
Type* parse_pointers_and_qualifiers(Parser* parser, Type* base) {
  expect_next_token(parser, TK_Star);

  PointerType* ptr = malloc(sizeof(PointerType));
  pointer_type_construct(ptr, base);
  base = &ptr->type;

  parser_consume_token(parser, TK_Star);

  // May be more '*'s or qualifiers.
  base->qualifiers |= parse_maybe_qualifiers(parser, /*found=*/NULL);

  if (next_token_is(parser, TK_Star))
    return parse_pointers_and_qualifiers(parser, base);

  return base;
}

// outer_ty is everything that came before the trailing part of the declarator.
Type* parse_declarator_maybe_type_suffix(Parser* parser, Type* outer_ty,
                                         Type*** type_usage_addr) {
  // There may be optional () or [] after.
  switch (parser_peek_token(parser)->kind) {
    case TK_LSquareBrace:
      return parse_array_suffix(parser, outer_ty, type_usage_addr);
    case TK_LPar:
      return parse_function_suffix(parser, outer_ty, type_usage_addr);
    default:
      return outer_ty;
  }
}

Type* parse_array_suffix(Parser* parser, Type* outer_ty,
                         Type*** type_usage_addr) {
  parser_consume_token(parser, TK_LSquareBrace);

  Expr* expr;
  if (!next_token_is(parser, TK_RSquareBrace)) {
    expr = parse_expr(parser);
  } else {
    expr = NULL;
  }

  parser_consume_token(parser, TK_RSquareBrace);

  // <blank> is an array (of size ...) of <remaining>
  Type* remaining =
      parse_declarator_maybe_type_suffix(parser, outer_ty, type_usage_addr);

  ArrayType* arr = create_array_of(remaining, expr);
  if (remaining == outer_ty && type_usage_addr && *type_usage_addr == NULL) {
    Type** elem = &arr->elem_type;
    *type_usage_addr = elem;
  }
  return &arr->type;
}

Type* parse_function_suffix(Parser* parser, Type* outer_ty,
                            Type*** type_usage_addr) {
  parser_consume_token(parser, TK_LPar);

  vector param_tys;
  vector_construct(&param_tys, sizeof(FunctionArg), alignof(FunctionArg));

  bool has_var_args = false;
  while (!next_token_is(parser, TK_RPar)) {
    // Handle varags.
    if (next_token_is(parser, TK_Ellipsis)) {
      parser_consume_token(parser, TK_Ellipsis);
      has_var_args = true;

      const Token* peek = parser_peek_token(parser);
      ASSERT_MSG(
          next_token_is(parser, TK_RPar),
          "%zu:%zu: parse_function_suffix: '...' must be the last in the "
          "parameter list; instead found '%s'.",
          peek->line, peek->col, peek->chars.data);
      continue;
    }

    char* name = NULL;
    Type* param_ty =
        parse_type_for_declaration(parser, &name, /*storage=*/NULL);
    FunctionArg* storage = vector_append_storage(&param_tys);
    storage->name = name;
    storage->type = param_ty;

    if (!next_token_is(parser, TK_Comma))
      break;

    parser_consume_token(parser, TK_Comma);
  }

  parser_consume_token(parser, TK_RPar);

  // <blank> is a function (with params ...) returning <outer_ty>
  FunctionType* func = malloc(sizeof(FunctionType));
  function_type_construct(func, outer_ty, param_tys);
  func->has_var_args = has_var_args;

  if (type_usage_addr && *type_usage_addr == NULL) {
    Type** ret = &func->return_type;
    *type_usage_addr = ret;
  }

  return &func->type;
}

// `type_usage_addr` is used for saving the single location of where the
// `type` is used when parsing the declarator. It is a triple pointer because
// it should point to the location the Type* is used.
Type* parse_declarator(Parser* parser, Type* type, char** name,
                       Type*** type_usage_addr) {
  type = maybe_parse_pointers_and_qualifiers(parser, type, type_usage_addr);

  const Token* tok = parser_peek_token(parser);

  switch (tok->kind) {
    case TK_Identifier:
      if (name)
        *name = strdup(tok->chars.data);
      parser_consume_token(parser, TK_Identifier);
      break;
    case TK_LPar:
      parser_consume_token(parser, TK_LPar);

      // Parse the inner declarator which kinda "wraps" the outer type.
      // We will pass a sentinel type that will be replaced once we finish
      // parsing any type suffixes.
      ReplacementSentinelType dummy = GetSentinel();
      Type** this_sentinel_addr = NULL;
      Type* inner_type =
          parse_declarator(parser, &dummy.type, name, &this_sentinel_addr);
      parser_consume_token(parser, TK_RPar);

      // Now parse any remaining suffix.
      type = parse_declarator_maybe_type_suffix(parser, type, type_usage_addr);

      if (this_sentinel_addr) {
        // Replace the sentinel with the outer type.
        *this_sentinel_addr = type;

        // The inner type represents the actual type.
        return inner_type;
      } else {
        // This sentinel was not used at all for the inner declarator. In that
        // case, the inner_type must be the sentinel itself. In this case, the
        // inner_type isn't needed and we can just return the outer type.
        assert(inner_type == &dummy.type);
        return type;
      }
    default:
      break;
  }

  return parse_declarator_maybe_type_suffix(parser, type, type_usage_addr);
}

Type* parse_type(Parser* parser) {
  return parse_type_for_declaration(parser, /*name=*/NULL, /*storage=*/NULL);
}

//
// <sizeof> = "sizeof" "(" (<expr> | <type>) ")"
//
Expr* parse_sizeof(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  parser_consume_token(parser, TK_SizeOf);
  parser_consume_token(parser, TK_LPar);

  SizeOf* so = malloc(sizeof(SizeOf));

  void* expr_or_type;
  bool is_expr;
  if (is_next_token_type_token(parser)) {
    expr_or_type = parse_type(parser);
    is_expr = false;
  } else {
    expr_or_type = parse_expr(parser);
    is_expr = true;
  }

  parser_consume_token(parser, TK_RPar);

  sizeof_construct(so, expr_or_type, is_expr);
  so->expr.line = line;
  so->expr.col = col;
  return &so->expr;
}

//
// <alignof> = "alignof" "(" (<expr> | <type>) ")"
//
Expr* parse_alignof(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  parser_consume_token(parser, TK_AlignOf);
  parser_consume_token(parser, TK_LPar);

  AlignOf* ao = malloc(sizeof(AlignOf));

  void* expr_or_type;
  bool is_expr;
  if (is_next_token_type_token(parser)) {
    expr_or_type = parse_type(parser);
    is_expr = false;
  } else {
    expr_or_type = parse_expr(parser);
    is_expr = true;
  }

  parser_consume_token(parser, TK_RPar);

  alignof_construct(ao, expr_or_type, is_expr);
  ao->expr.line = line;
  ao->expr.col = col;
  return &ao->expr;
}

// For parsing "(" <expr> ")" but the initial opening LPar is consumed.
static Expr* parse_parentheses_expr_tail(Parser* parser) {
  Expr* expr = parse_expr(parser);
  parser_consume_token(parser, TK_RPar);
  return expr;
}

//
// <primary_expr> = <identifier> | <constant> | <string_literal>
//                | "__PRETTY_FUNCTION__" | "(" <expr> ")"
//
Expr* parse_primary_expr(Parser* parser) {
  const Token* tok = parser_peek_token(parser);

  if (tok->kind == TK_LPar) {
    parser_consume_token(parser, TK_LPar);
    return parse_parentheses_expr_tail(parser);
  }

  if (tok->kind == TK_PrettyFunction) {
    PrettyFunction* pf = malloc(sizeof(PrettyFunction));
    pretty_function_construct(pf);
    parser_consume_token(parser, TK_PrettyFunction);
    return &pf->expr;
  }

  if (tok->kind == TK_Identifier) {
    DeclRef* ref = malloc(sizeof(DeclRef));
    declref_construct(ref, tok->chars.data);
    parser_consume_token(parser, TK_Identifier);
    return &ref->expr;
  }

  if (tok->kind == TK_IntLiteral) {
    // TODO: Rather than using strtoull, we should probably parse this
    // ourselves for better error handling. This is simpler for now.

    int base = (tok->chars.size >= 2 &&
                (tok->chars.data[1] == 'x' || tok->chars.data[1] == 'X'))
                   ? 16
                   : 10;
    unsigned long long val = strtoull(tok->chars.data, NULL, base);
    Int* i = malloc(sizeof(Int));
    // FIXME: This doesn't account for suffixes.
    int_construct(i, val, BTK_Int);
    parser_consume_token(parser, TK_IntLiteral);
    return &i->expr;
  }

  if (tok->kind == TK_StringLiteral) {
    size_t size = tok->chars.size;
    assert(size >= 2 &&
           "String literals from the lexer should have the start and end "
           "double quotes");

    string str;
    string_construct(&str);
    string_append_range(&str, tok->chars.data + 1, size - 2);
    parser_consume_token(parser, TK_StringLiteral);

    for (; next_token_is(parser, TK_StringLiteral);) {
      // Merge all adjacent string literals.
      const Token* tok = parser_peek_token(parser);
      assert(tok->chars.size >= 2 &&
             "String literals from the lexer should have the start and end "
             "double quotes");
      string_append_range(&str, tok->chars.data + 1, tok->chars.size - 2);
      parser_consume_token(parser, TK_StringLiteral);
    }

    StringLiteral* s = malloc(sizeof(StringLiteral));
    string_literal_construct(s, str.data, str.size);

    string_destroy(&str);

    return &s->expr;
  }

  if (tok->kind == TK_True || tok->kind == TK_False) {
    Bool* b = malloc(sizeof(Bool));
    bool_construct(b, tok->kind == TK_True);
    parser_skip_next_token(parser);
    return &b->expr;
  }

  if (tok->kind == TK_CharLiteral) {
    size_t size = tok->chars.size;
    assert(size == 3 && tok->chars.data[0] == '\'' &&
           tok->chars.data[2] == '\'' &&
           "Char literals from the lexer should have the start and end "
           "single quotes");

    Char* c = malloc(sizeof(Char));
    char_construct(c, tok->chars.data[1]);
    parser_consume_token(parser, TK_CharLiteral);
    return &c->expr;
  }

  vector elems;
  vector_construct(&elems, sizeof(InitializerListElem),
                   alignof(InitializerListElem));

  if (tok->kind == TK_LCurlyBrace) {
    parser_consume_token(parser, TK_LCurlyBrace);

    for (; !next_token_is(parser, TK_RCurlyBrace);) {
      char* name = NULL;

      if (next_token_is(parser, TK_Dot)) {
        // Designated initializer.
        parser_consume_token(parser, TK_Dot);

        const Token* next = parser_peek_token(parser);
        name = strdup(next->chars.data);

        parser_consume_token(parser, TK_Identifier);
        parser_consume_token(parser, TK_Assign);
      }

      // TODO: Should this be just `parse_expr`? Using `parse_expr` may consume
      // the following comma in a comma expression.
      Expr* expr = parse_assignment_expr(parser);
      InitializerListElem* elem = vector_append_storage(&elems);
      elem->name = name;
      elem->expr = expr;

      // Consume optional `,`.
      if (next_token_is(parser, TK_Comma))
        parser_consume_token(parser, TK_Comma);
      else
        break;
    }
    parser_consume_token(parser, TK_RCurlyBrace);

    InitializerList* init = malloc(sizeof(InitializerList));
    initializer_list_construct(init, elems);
    return &init->expr;
  }

  UNREACHABLE_MSG("%zu:%zu: parse_primary_expr: Unhandled token (%d): '%s'\n",
                  tok->line, tok->col, tok->kind, tok->chars.data);
  return NULL;
}

//
// <argument_expr_list> = <assignment_expr> ("," <assignment_expr>)*
//
vector parse_argument_list(Parser* parser) {
  vector v;
  vector_construct(&v, sizeof(Expr*), alignof(Expr*));

  while (true) {
    // https://gcc.gnu.org/onlinedocs/gcc/Alternate-Keywords.html
    //
    // Writing __extension__ before an expression prevents warnings about
    // extensions within that expression.
    //
    // NOTE: Ideally we'd only be handling __extension__ in the start
    // of parse_expr, but an argument_expr_list can only accept assignment_exprs
    // since it helps disambiguate between arguments and comma expressions.
    if (next_token_is(parser, TK_Extension))
      parser_consume_token(parser, TK_Extension);

    Expr* expr = parse_assignment_expr(parser);
    Expr** storage = vector_append_storage(&v);
    *storage = expr;

    if (next_token_is(parser, TK_Comma))
      parser_consume_token(parser, TK_Comma);
    else
      break;
  }

  return v;
}

//
// <postfix_expr> = <primary_expr>
//                | <postfix_expr> "[" <expr> "]"
//                | <postfix_expr> "(" <argument_expr_list>? ")"
//                | <postfix_expr> "." <identifier>
//                | <postfix_expr> "->" <identifier>
//                | <postfix_expr> "++"
//                | <postfix_expr> "--"
//
static Expr* parse_postfix_expr_with_primary(Parser* parser, Expr* expr) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  bool should_continue = true;
  for (; should_continue;) {
    const Token* tok = parser_peek_token(parser);

    switch (tok->kind) {
      case TK_LSquareBrace: {
        parser_consume_token(parser, TK_LSquareBrace);
        Expr* idx = parse_expr(parser);
        parser_consume_token(parser, TK_RSquareBrace);

        Index* index = malloc(sizeof(Index));
        index_construct(index, expr, idx);
        expr = &index->expr;
        break;
      }

      case TK_LPar: {
        parser_consume_token(parser, TK_LPar);

        vector v;
        if (parser_peek_token(parser)->kind != TK_RPar)
          v = parse_argument_list(parser);
        else
          vector_construct(&v, sizeof(Expr*), alignof(Expr*));

        parser_consume_token(parser, TK_RPar);

        Call* call = malloc(sizeof(Call));
        call_construct(call, expr, v);
        expr = &call->expr;
        break;
      }

      case TK_Dot:
      case TK_Arrow: {
        bool is_arrow = tok->kind == TK_Arrow;

        parser_consume_token(parser, tok->kind);

        Token id = parser_pop_token(parser);
        assert(id.kind == TK_Identifier);

        MemberAccess* member_access = malloc(sizeof(MemberAccess));
        member_access_construct(member_access, expr, id.chars.data, is_arrow);
        expr = &member_access->expr;

        token_destroy(&id);
        break;
      }

      case TK_Inc: {
        parser_consume_token(parser, TK_Inc);
        UnOp* unop = malloc(sizeof(UnOp));
        unop_construct(unop, expr, UOK_PostInc);
        expr = &unop->expr;
        break;
      }

      case TK_Dec: {
        parser_consume_token(parser, TK_Dec);
        UnOp* unop = malloc(sizeof(UnOp));
        unop_construct(unop, expr, UOK_PostDec);
        expr = &unop->expr;
        break;
      }

      default:
        should_continue = false;
    }
  }
  expr->line = line;
  expr->col = col;
  return expr;
}

Expr* parse_postfix_expr(Parser* parser) {
  Expr* expr = parse_primary_expr(parser);
  return parse_postfix_expr_with_primary(parser, expr);
}

//
// <unary_epr> = <postfix_expr>
//             | ("++" | "--") <unary_expr>
//             | ("&" | "*" | "+" | "-" | "~" | "!") <cast_expr>
//             | "sizeof" "(" (<unary_expr> | <type>) ")"
//
Expr* parse_unary_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  const Token* tok = parser_peek_token(parser);

  UnOpKind op;
  switch (tok->kind) {
    case TK_Inc:
      op = UOK_PreInc;
      break;
    case TK_Dec:
      op = UOK_PreDec;
      break;
    case TK_Ampersand:
      op = UOK_AddrOf;
      break;
    case TK_Star:
      op = UOK_Deref;
      break;
    case TK_Add:
      op = UOK_Plus;
      break;
    case TK_Sub:
      op = UOK_Negate;
      break;
    case TK_BitNot:
      op = UOK_BitNot;
      break;
    case TK_Not:
      op = UOK_Not;
      break;
    case TK_SizeOf:
      return parse_sizeof(parser);
    case TK_AlignOf:
      return parse_alignof(parser);
    default:
      return parse_postfix_expr(parser);
  }

  parser_consume_token(parser, tok->kind);

  Expr* expr;
  if (tok->kind == TK_Inc || tok->kind == TK_Dec) {
    expr = parse_unary_expr(parser);
  } else {
    expr = parse_cast_expr(parser);
  }

  UnOp* unop = malloc(sizeof(UnOp));
  unop_construct(unop, expr, op);
  unop->expr.line = line;
  unop->expr.col = col;
  return &unop->expr;
}

bool is_next_token_type_token(Parser* parser) {
  return is_token_type_token(parser, parser_peek_token(parser));
}

// This only exists so we can dispatch to either a cast expression or a normal
// expression surrounded by parenthesis since we need to know if the contents
// after the LPar is a type or not.
static Expr* parse_cast_or_paren_or_stmt_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  parser_consume_token(parser, TK_LPar);

  // https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
  //
  // ({ ... }) is an expression statement provided as a GCC extension.
  if (next_token_is(parser, TK_LCurlyBrace)) {
    Statement* stmt = parse_compound_stmt(parser);
    StmtExpr* se = malloc(sizeof(StmtExpr));
    stmt_expr_construct(se, (CompoundStmt*)stmt);
    se->expr.line = line;
    se->expr.col = col;
    parser_consume_token(parser, TK_RPar);
    return &se->expr;
  }

  if (!is_next_token_type_token(parser)) {
    Expr* expr = parse_parentheses_expr_tail(parser);
    return parse_postfix_expr_with_primary(parser, expr);
  }

  // Definitely a type case.
  // "(" <type> ")" <cast_expr>
  Type* type = parse_type(parser);
  parser_consume_token(parser, TK_RPar);

  Expr* base = parse_cast_expr(parser);

  Cast* cast = malloc(sizeof(Cast));
  cast_construct(cast, base, type);
  cast->expr.line = line;
  cast->expr.col = col;
  return &cast->expr;
}

//
// <cast_epr> = <unary_expr>
//            | "(" <type> ")" <cast_expr>
//
Expr* parse_cast_expr(Parser* parser) {
  const Token* tok = parser_peek_token(parser);

  if (tok->kind == TK_LPar) {
    Expr* e = parse_cast_or_paren_or_stmt_expr(parser);
    assert(e->line && e->col);
    return e;
  }

  Expr* e = parse_unary_expr(parser);
  assert(e->line && e->col);
  return e;
}

//
// <multiplicative_epr> = <cast_expr>
//                      | <cast_expr> ("*" | "/" | "%") <multiplicative_expr>
//
Expr* parse_multiplicative_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr* expr = parse_cast_expr(parser);
  assert(expr);
  if (!(expr->line && expr->col)) {
    printf("line: %zu, col: %zu\n", expr->line, expr->col);
  }
  assert(expr->line && expr->col);

  const Token* token = parser_peek_token(parser);
  BinOpKind op;
  switch (token->kind) {
    case TK_Star:
      op = BOK_Mul;
      break;
    case TK_Div:
      op = BOK_Div;
      break;
    case TK_Mod:
      op = BOK_Mod;
      break;
    default:
      return expr;
  }

  parser_consume_token(parser, token->kind);

  Expr* rhs = parse_multiplicative_expr(parser);
  assert(rhs);

  BinOp* binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, op);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <additive_epr> = <multiplicative_expr>
//                | <multiplicative_expr> ("+" | "-") <additive_expr>
//
Expr* parse_additive_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr* expr = parse_multiplicative_expr(parser);
  assert(expr);
  assert(expr->line && expr->col);

  const Token* token = parser_peek_token(parser);
  BinOpKind op;
  switch (token->kind) {
    case TK_Add:
      op = BOK_Add;
      break;
    case TK_Sub:
      op = BOK_Sub;
      break;
    default:
      return expr;
  }

  parser_consume_token(parser, token->kind);

  Expr* rhs = parse_additive_expr(parser);
  assert(rhs);

  BinOp* binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, op);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <shift_epr> = <additive_expr>
//             | <additive_expr> ("<<" | ">>") <shift_expr>
//
Expr* parse_shift_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;
  assert(line && col);

  Expr* expr = parse_additive_expr(parser);
  assert(expr);
  assert(expr->line && expr->col);

  const Token* token = parser_peek_token(parser);
  BinOpKind op;
  switch (token->kind) {
    case TK_LShift:
      op = BOK_LShift;
      break;
    case TK_RShift:
      op = BOK_RShift;
      break;
    default:
      return expr;
  }

  parser_consume_token(parser, token->kind);

  Expr* rhs = parse_shift_expr(parser);
  assert(rhs);

  BinOp* binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, op);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <relational_expr> = <shift_expr>
//                   | <shift_expr> ("<" | ">" | ">=" | "<=") <relational_expr>
//
Expr* parse_relational_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr* expr = parse_shift_expr(parser);
  assert(expr);

  const Token* token = parser_peek_token(parser);
  BinOpKind op;
  switch (token->kind) {
    case TK_Lt:
      op = BOK_Lt;
      break;
    case TK_Gt:
      op = BOK_Gt;
      break;
    case TK_Le:
      op = BOK_Le;
      break;
    case TK_Ge:
      op = BOK_Ge;
      break;
    default:
      return expr;
  }

  parser_consume_token(parser, token->kind);

  Expr* rhs = parse_relational_expr(parser);
  assert(rhs);

  BinOp* binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, op);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <equality_expr> = <relational_expr>
//                 | <relational_expr> ("==" | "!=") <equality_expr>
//
Expr* parse_equality_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr* expr = parse_relational_expr(parser);
  assert(expr);

  const Token* token = parser_peek_token(parser);
  BinOpKind op;
  switch (token->kind) {
    case TK_Eq:
      op = BOK_Eq;
      break;
    case TK_Ne:
      op = BOK_Ne;
      break;
    default:
      return expr;
  }

  parser_consume_token(parser, token->kind);

  Expr* rhs = parse_equality_expr(parser);
  assert(rhs);

  BinOp* binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, op);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <and_expr> = <equality_expr>
//            | <equality_expr> "&" <and_expr>
//
Expr* parse_and_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr* expr = parse_equality_expr(parser);
  assert(expr);

  const Token* token = parser_peek_token(parser);
  if (token->kind != TK_Ampersand)
    return expr;

  parser_consume_token(parser, TK_Ampersand);

  Expr* rhs = parse_and_expr(parser);
  assert(rhs);

  BinOp* binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, BOK_BitwiseAnd);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <exclusive_or_expr> = <and_expr>
//                     | <and_expr> "^" <exclusive_or_expr>
//
Expr* parse_exclusive_or_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr* expr = parse_and_expr(parser);
  assert(expr);

  const Token* token = parser_peek_token(parser);
  if (token->kind != TK_Xor)
    return expr;

  parser_consume_token(parser, TK_Xor);

  Expr* rhs = parse_exclusive_or_expr(parser);
  assert(rhs);

  BinOp* binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, BOK_Xor);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <inclusive_or_expr> = <exclusive_or_expr>
//                     | <exclusive_or_expr> "|" <inclusive_or_expr>
//
Expr* parse_inclusive_or_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr* expr = parse_exclusive_or_expr(parser);
  assert(expr);

  const Token* token = parser_peek_token(parser);
  if (token->kind != TK_Or)
    return expr;

  parser_consume_token(parser, TK_Or);

  Expr* rhs = parse_inclusive_or_expr(parser);
  assert(rhs);

  BinOp* binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, BOK_BitwiseOr);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <logical_and_expr> = <inclusive_or_expr>
//                    | <inclusive_or_expr> "&&" <logical_and_expr>
//
Expr* parse_logical_and_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr* expr = parse_inclusive_or_expr(parser);
  assert(expr);

  const Token* token = parser_peek_token(parser);
  if (token->kind != TK_LogicalAnd)
    return expr;

  parser_consume_token(parser, TK_LogicalAnd);

  Expr* rhs = parse_logical_and_expr(parser);
  assert(rhs);

  BinOp* binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, BOK_LogicalAnd);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <logical_or_expr> = <logical_and_expr>
//                   | <logical_and_expr> "||" <logical_or_expr>
//
Expr* parse_logical_or_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr* expr = parse_logical_and_expr(parser);
  assert(expr);

  const Token* token = parser_peek_token(parser);
  if (token->kind != TK_LogicalOr)
    return expr;

  parser_consume_token(parser, TK_LogicalOr);

  Expr* rhs = parse_logical_or_expr(parser);
  assert(rhs);

  BinOp* binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, BOK_LogicalOr);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <conditional_expr> = <logical_or_expr>
//                    | <logical_or_expr> "?" <expr> ":" <conditional_expr>
//
Expr* parse_conditional_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr* expr = parse_logical_or_expr(parser);
  assert(expr);

  const Token* token = parser_peek_token(parser);
  if (token->kind != TK_Question)
    return expr;

  parser_consume_token(parser, TK_Question);

  Expr* true_expr = parse_expr(parser);
  assert(true_expr);

  parser_consume_token(parser, TK_Colon);

  Expr* false_expr = parse_conditional_expr(parser);
  assert(false_expr);

  Conditional* cond = malloc(sizeof(Conditional));
  conditional_construct(cond, expr, true_expr, false_expr);
  cond->expr.line = line;
  cond->expr.col = col;
  return &cond->expr;
}

//
// <assignment_expr> = <conditional_expr>
//                   | <conditional_expr> <assignment_op> <assignment_expr>
//
// <assignment_op> = ("*" | "/" | "%" | "+" | "-" | "<<" | ">>" | "&" | "|" |
//                    "^")? "="
//
Expr* parse_assignment_expr(Parser* parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr* expr = parse_conditional_expr(parser);
  assert(expr);
  assert(expr->line && expr->col);

  BinOpKind op;
  const Token* token = parser_peek_token(parser);
  switch (token->kind) {
    case TK_Assign:
      op = BOK_Assign;
      break;
    case TK_MulAssign:
      op = BOK_MulAssign;
      break;
    case TK_DivAssign:
      op = BOK_DivAssign;
      break;
    case TK_AddAssign:
      op = BOK_AddAssign;
      break;
    case TK_SubAssign:
      op = BOK_SubAssign;
      break;
    case TK_LShiftAssign:
      op = BOK_LShiftAssign;
      break;
    case TK_RShiftAssign:
      op = BOK_RShiftAssign;
      break;
    case TK_AndAssign:
      op = BOK_AndAssign;
      break;
    case TK_OrAssign:
      op = BOK_OrAssign;
      break;
    case TK_XorAssign:
      op = BOK_XorAssign;
      break;
    default:
      return expr;
  }

  parser_consume_token(parser, token->kind);

  Expr* rhs = parse_assignment_expr(parser);
  assert(expr);

  BinOp* binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, op);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <expr> = <assignment_expr> | <assignment_expr> "," <expr>
//
// Operator precedence: https://www.lysator.liu.se/c/ANSI-C-grammar-y.html
Expr* parse_expr(Parser* parser) {
  // https://gcc.gnu.org/onlinedocs/gcc/Alternate-Keywords.html
  //
  // Writing __extension__ before an expression prevents warnings about
  // extensions within that expression.
  if (next_token_is(parser, TK_Extension))
    parser_consume_token(parser, TK_Extension);

  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr* expr = parse_assignment_expr(parser);
  assert(expr);

  const Token* token = parser_peek_token(parser);
  if (token->kind != TK_Comma)
    return expr;

  parser_consume_token(parser, TK_Comma);
  Expr* rhs = parse_expr(parser);
  assert(rhs);

  BinOp* binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, BOK_Comma);
  binop->expr.line = line;
  binop->expr.col = col;

  return &binop->expr;
}

//
// <static_assert> = "static_assert" "(" <expr> ")"
//
TopLevelNode* parse_static_assert(Parser* parser) {
  parser_consume_token(parser, TK_StaticAssert);
  parser_consume_token(parser, TK_LPar);

  Expr* expr = parse_expr(parser);

  parser_consume_token(parser, TK_RPar);
  parser_consume_token(parser, TK_Semicolon);

  StaticAssert* sa = malloc(sizeof(StaticAssert));
  static_assert_construct(sa, expr);

  return &sa->node;
}

//
// <typedef> = "typedef" <type_with_declarator>
//
TopLevelNode* parse_typedef(Parser* parser) {
  parser_consume_token(parser, TK_Typedef);

  Typedef* td = malloc(sizeof(Typedef));
  typedef_construct(td);

  td->type = parse_type_for_declaration(parser, &td->name, /*storage=*/NULL);

  // We don't care about the value.
  assert(!parser_has_named_type(parser, td->name) && "Duplicate named typedef");
  parser_define_named_type(parser, td->name);

  parser_consume_token(parser, TK_Semicolon);

  return &td->node;
}

Statement* parse_expr_statement(Parser* parser) {
  Expr* expr = parse_expr(parser);
  parser_consume_token(parser, TK_Semicolon);

  ExprStmt* stmt = malloc(sizeof(ExprStmt));
  expr_stmt_construct(stmt, expr);
  return &stmt->base;
}

Statement* parse_declaration(Parser* parser) {
  FoundStorageClasses storage;
  char* name = NULL;
  Type* type = parse_type_for_declaration(parser, &name, &storage);
  assert(name);

  Expr* init = NULL;
  if (next_token_is(parser, TK_Assign)) {
    parser_consume_token(parser, TK_Assign);
    init = parse_expr(parser);
  }

  parser_consume_token(parser, TK_Semicolon);

  Declaration* decl = malloc(sizeof(Declaration));
  declaration_construct(decl, name, type, init);
  free(name);
  return &decl->base;
}

static Statement* parse_statement_impl(Parser* parser) {
  const Token* peek = parser_peek_token(parser);

  if (peek->kind == TK_LCurlyBrace)
    return parse_compound_stmt(parser);

  if (peek->kind == TK_If) {
    parser_consume_token(parser, TK_If);
    parser_consume_token(parser, TK_LPar);

    Expr* cond = parse_expr(parser);

    parser_consume_token(parser, TK_RPar);

    Statement* body = NULL;
    if (next_token_is(parser, TK_Semicolon)) {
      // Empty if stmt body.
      parser_consume_token(parser, TK_Semicolon);
    } else {
      body = parse_statement(parser);
    }

    IfStmt* ifstmt = malloc(sizeof(IfStmt));
    if_stmt_construct(ifstmt, cond, body);

    if (next_token_is(parser, TK_Else)) {
      parser_consume_token(parser, TK_Else);
      ifstmt->else_stmt = parse_statement(parser);
    }

    return &ifstmt->base;
  }

  if (peek->kind == TK_While) {
    parser_consume_token(parser, TK_While);
    parser_consume_token(parser, TK_LPar);

    Expr* cond = parse_expr(parser);

    parser_consume_token(parser, TK_RPar);

    Statement* body = NULL;
    if (next_token_is(parser, TK_Semicolon)) {
      // Empty while stmt body.
      parser_consume_token(parser, TK_Semicolon);
    } else {
      body = parse_statement(parser);
    }

    WhileStmt* while_stmt = malloc(sizeof(WhileStmt));
    while_stmt_construct(while_stmt, cond, body);
    return &while_stmt->base;
  }

  if (peek->kind == TK_Return) {
    parser_consume_token(parser, TK_Return);

    Expr* expr;
    if (!next_token_is(parser, TK_Semicolon)) {
      expr = parse_expr(parser);
    } else {
      expr = NULL;
    }
    parser_consume_token(parser, TK_Semicolon);

    ReturnStmt* ret = malloc(sizeof(ReturnStmt));
    return_stmt_construct(ret, expr);
    return &ret->base;
  }

  if (peek->kind == TK_Switch) {
    parser_consume_token(parser, TK_Switch);

    parser_consume_token(parser, TK_LPar);
    Expr* cond = parse_expr(parser);
    parser_consume_token(parser, TK_RPar);

    parser_consume_token(parser, TK_LCurlyBrace);

    // Vector of SwitchCases - same as SwitchStmt::cases.
    vector cases;
    vector_construct(&cases, sizeof(SwitchCase), alignof(SwitchCase));

    vector* default_stmts = NULL;

    while (true) {
      if (next_token_is(parser, TK_RCurlyBrace))
        break;

      if (next_token_is(parser, TK_LSquareBrace)) {
        // Expect a C23 attribute like [[fallthrough]].
        parser_consume_token(parser, TK_LSquareBrace);
        parser_consume_token(parser, TK_LSquareBrace);

        const Token* attr = parser_peek_token(parser);
        assert(attr->kind == TK_Identifier);
        if (string_equals(&attr->chars, "fallthrough")) {
          // TODO: If this project is ever working, come back and potentially
          // warn on missing fallthroughs.
          parser_consume_token(parser, TK_Identifier);
        } else {
          UNREACHABLE_MSG("%zu:%zu: Unknown attribute '%s'", attr->line,
                          attr->col, attr->chars.data);
        }

        parser_consume_token(parser, TK_RSquareBrace);
        parser_consume_token(parser, TK_RSquareBrace);
        parser_consume_token(parser, TK_Semicolon);
      }

      if (next_token_is(parser, TK_Case)) {
        parser_consume_token(parser, TK_Case);

        Expr* case_expr = parse_expr(parser);

        parser_consume_token(parser, TK_Colon);

        vector case_stmts;
        vector_construct(&case_stmts, sizeof(Statement*), alignof(Statement*));
        for (; !(next_token_is(parser, TK_Case) ||
                 next_token_is(parser, TK_Default) ||
                 next_token_is(parser, TK_RCurlyBrace) ||
                 next_token_is(parser, TK_LSquareBrace));) {
          Statement* stmt = parse_statement(parser);
          *(Statement**)vector_append_storage(&case_stmts) = stmt;
        }

        SwitchCase* switch_case = vector_append_storage(&cases);
        switch_case->cond = case_expr;
        switch_case->stmts = case_stmts;
      } else if (next_token_is(parser, TK_Default)) {
        parser_consume_token(parser, TK_Default);
        parser_consume_token(parser, TK_Colon);

        assert(default_stmts == NULL);
        default_stmts = malloc(sizeof(vector));
        vector_construct(default_stmts, sizeof(Statement*),
                         alignof(Statement*));
        for (; !(next_token_is(parser, TK_Case) ||
                 next_token_is(parser, TK_Default) ||
                 next_token_is(parser, TK_RCurlyBrace) ||
                 next_token_is(parser, TK_LSquareBrace));) {
          Statement* stmt = parse_statement(parser);
          *(Statement**)vector_append_storage(default_stmts) = stmt;
        }
      } else {
        const Token* peek = parser_peek_token(parser);
        UNREACHABLE_MSG("Neither case nor default: '%s'", peek->chars.data);
      }
    }
    parser_consume_token(parser, TK_RCurlyBrace);

    SwitchStmt* switch_stmt = malloc(sizeof(SwitchStmt));
    switch_stmt_construct(switch_stmt, cond, cases, default_stmts);
    return &switch_stmt->base;
  }

  if (peek->kind == TK_Continue) {
    parser_consume_token(parser, TK_Continue);
    parser_consume_token(parser, TK_Semicolon);

    ContinueStmt* cnt = malloc(sizeof(ContinueStmt));
    continue_stmt_construct(cnt);
    return &cnt->base;
  }

  if (peek->kind == TK_Break) {
    parser_consume_token(parser, TK_Break);
    parser_consume_token(parser, TK_Semicolon);

    BreakStmt* brk = malloc(sizeof(BreakStmt));
    break_stmt_construct(brk);
    return &brk->base;
  }

  if (peek->kind == TK_For) {
    parser_consume_token(parser, TK_For);
    parser_consume_token(parser, TK_LPar);

    Statement* init = NULL;
    Expr* cond = NULL;
    Expr* iter = NULL;
    Statement* body = NULL;

    // Parse optional initializer expr.
    if (!next_token_is(parser, TK_Semicolon)) {
      if (is_next_token_type_token(parser)) {
        init = parse_declaration(parser);
      } else {
        init = parse_expr_statement(parser);
      }
    } else {
      parser_consume_token(parser, TK_Semicolon);
    }

    // Parse optional condition expr.
    if (!next_token_is(parser, TK_Semicolon))
      cond = parse_expr(parser);
    parser_consume_token(parser, TK_Semicolon);

    // Parse optional iterating expr.
    if (!next_token_is(parser, TK_RPar))
      iter = parse_expr(parser);

    parser_consume_token(parser, TK_RPar);

    // Parse optional body.
    if (!next_token_is(parser, TK_Semicolon)) {
      body = parse_statement(parser);
    } else {
      parser_consume_token(parser, TK_Semicolon);
    }

    ForStmt* for_stmt = malloc(sizeof(ForStmt));
    for_stmt_construct(for_stmt, init, cond, iter, body);
    return &for_stmt->base;
  }

  if (is_token_type_token(parser, peek)) {
    // This is a declaration.
    return parse_declaration(parser);
  }

  // Default is an expression statement.
  return parse_expr_statement(parser);
}

// This consumes any trailing semicolons when needed.
Statement* parse_statement(Parser* parser) {
  Statement* stmt = parse_statement_impl(parser);

  while (next_token_is(parser, TK_Semicolon))
    parser_consume_token(parser, TK_Semicolon);

  return stmt;
}

Statement* parse_compound_stmt(Parser* parser) {
  parser_consume_token(parser, TK_LCurlyBrace);

  vector body;
  vector_construct(&body, sizeof(Statement*), alignof(Statement*));

  while (!next_token_is(parser, TK_RCurlyBrace)) {
    Statement** storage = vector_append_storage(&body);
    *storage = parse_statement(parser);
  }

  parser_consume_token(parser, TK_RCurlyBrace);

  CompoundStmt* cmpd = malloc(sizeof(CompoundStmt));
  compound_stmt_construct(cmpd, body);
  return &cmpd->base;
}

// This disambiguates between a top-level tagged type declaration vs a global
// variable declaration.
TopLevelNode* parse_top_level_type_decl(Parser* parser) {
  const Token* peek = parser_peek_token(parser);
  assert(is_token_type_token(parser, peek) ||
         is_storage_class_specifier_token(peek->kind));

  FoundStorageClasses storage = {};
  // TODO: Handle inlines?
  Type* type = parse_specifiers_and_qualifiers_and_storage(
      parser, &storage, /*found_inline=*/NULL);

  // If the next token is a semicolon, then we know for a fact this is a tagged
  // type declaration.
  if (next_token_is(parser, TK_Semicolon)) {
    parser_skip_next_token(parser);
    switch (type->vtable->kind) {
      case TK_UnionType: {
        UnionDeclaration* decl = malloc(sizeof(UnionDeclaration));
        union_declaration_construct_from_type(decl, (UnionType*)type);
        return &decl->node;
      }
      case TK_EnumType: {
        EnumDeclaration* decl = malloc(sizeof(EnumDeclaration));
        enum_declaration_construct_from_type(decl, (EnumType*)type);
        return &decl->node;
      }
      case TK_StructType: {
        StructDeclaration* decl = malloc(sizeof(StructDeclaration));
        struct_declaration_construct_from_type(decl, (StructType*)type);
        return &decl->node;
      }
      default:
        UNREACHABLE_MSG(
            "Expected a ';' for the end of a tagged type declaration");
    }
  }

  // Otherwise, continue parsing as if this were a type for a variable
  // declaration.
  char* name = NULL;
  type = parse_type_for_declaration_impl(parser, &name, &storage, type);
  assert(name);

  const Token* tok = parser_peek_token(parser);
  ASSERT_MSG(!storage.auto_, "%zu:%zu: 'auto' can only be used at block scope",
             tok->line, tok->col);

  if (type->vtable->kind == TK_FunctionType &&
      next_token_is(parser, TK_LCurlyBrace)) {
    Statement* cmpd = parse_compound_stmt(parser);

    FunctionDefinition* func_def = malloc(sizeof(FunctionDefinition));
    function_definition_construct(func_def, name, type, (CompoundStmt*)cmpd);

    if (storage.static_)
      func_def->is_extern = false;

    free(name);

    return &func_def->node;
  }

  GlobalVariable* gv = malloc(sizeof(GlobalVariable));
  global_variable_construct(gv, name, type);

  if (storage.static_)
    gv->is_extern = false;

  if (storage.thread_local_)
    gv->is_thread_local = true;

  if (next_token_is(parser, TK_Assign)) {
    parser_consume_token(parser, TK_Assign);
    Expr* init = parse_expr(parser);
    gv->initializer = init;

    // If the type is an array type which is unsized, infer the size from the
    // expression.
    if (is_array_type(type)) {
      ArrayType* arr_ty = (ArrayType*)type;
      if (!arr_ty->size && init->vtable->kind == EK_InitializerList) {
        const InitializerList* il = (const InitializerList*)init;
        arr_ty->size = (Expr*)malloc(sizeof(Int));
        int_construct((Int*)arr_ty->size, il->elems.size, BTK_Int);
      }
    }
  }

  free(name);

  parser_consume_token(parser, TK_Semicolon);

  return &gv->node;
}

TopLevelNode* parse_struct_declaration(Parser* parser) {
  char* name = NULL;
  vector* members = NULL;
  parse_struct_name_and_members(parser, &name, &members);

  StructDeclaration* decl = malloc(sizeof(StructDeclaration));
  struct_declaration_construct(decl, name, members);
  return &decl->node;
}

TopLevelNode* parse_enum_declaration(Parser* parser) {
  char* name = NULL;
  vector* members = NULL;
  parse_enum_name_and_members(parser, &name, &members);

  EnumDeclaration* decl = malloc(sizeof(EnumDeclaration));
  enum_declaration_construct(decl, name, members);
  return &decl->node;
}

TopLevelNode* parse_union_declaration(Parser* parser) {
  char* name = NULL;
  vector* members = NULL;
  parse_union_name_and_members(parser, &name, &members);

  UnionDeclaration* decl = malloc(sizeof(UnionDeclaration));
  union_declaration_construct(decl, name, members);
  return &decl->node;
}

// NOTE: Callers of this function are in charge of destroying and freeing the
// poiner.
TopLevelNode* parse_top_level_decl(Parser* parser) {
  const Token* token = parser_peek_token(parser);
  assert(token->kind != TK_Eof);

  // https://gcc.gnu.org/onlinedocs/gcc/Alternate-Keywords.html
  //
  // Writing __extension__ before an expression prevents warnings about
  // extensions within that expression.
  if (token->kind == TK_Extension) {
    // TODO: Eventually handle it.
    parser_consume_token(parser, TK_Extension);
    token = parser_peek_token(parser);
  }

  // Module-level pragmas.
  while (token->kind == TK_Hash) {
    // TODO: Eventually handle it.
    parser_consume_pragma(parser);
    token = parser_peek_token(parser);
  }

  TopLevelNode* node = NULL;
  switch (token->kind) {
    case TK_Typedef:
      node = parse_typedef(parser);
      break;
    case TK_StaticAssert:
      node = parse_static_assert(parser);
      break;
    default:
      if (is_token_type_token(parser, token) ||
          is_storage_class_specifier_token(token->kind)) {
        node = parse_top_level_type_decl(parser);
      }
  }

  if (node)
    return node;

  UNREACHABLE_MSG("%zu:%zu: parse_top_level_decl: Unhandled token (%d): '%s'\n",
                  token->line, token->col, token->kind, token->chars.data);
  return NULL;
}

///
/// Start Parser Tests
///

static Type* ParseTypeString(const char* str, char** name) {
  StringInputStream ss;
  string_input_stream_construct(&ss, str);

  Parser p;
  parser_construct(&p, &ss.base);
  parser_define_named_type(&p, "size_t");  // Just let some tests use this.

  Type* type = parse_type_for_declaration(&p, name, /*storage=*/NULL);

  parser_destroy(&p);
  input_stream_destroy(&ss.base);

  return type;
}

static void TestSimpleDeclarationParse() {
  char* name;
  Type* type = ParseTypeString("int x", &name);

  assert(strcmp(name, "x") == 0);
  assert(type->vtable->kind == TK_BuiltinType);
  assert(((BuiltinType*)type)->kind == BTK_Int);
  assert(type->qualifiers == 0);

  type_destroy(type);
  free(type);
  free(name);
}

static void TestArrayParsing() {
  char* name;
  Type* type = ParseTypeString("int x[5]", &name);

  assert(strcmp(name, "x") == 0);
  assert(type->vtable->kind == TK_ArrayType);
  assert(type->qualifiers == 0);

  Type* elem = ((ArrayType*)type)->elem_type;
  assert(elem->vtable->kind == TK_BuiltinType);
  assert(((BuiltinType*)elem)->kind == BTK_Int);

  Expr* size = ((ArrayType*)type)->size;
  assert(size);
  assert(size->vtable->kind == EK_Int);
  assert(((Int*)size)->val == 5);

  type_destroy(type);
  free(type);
  free(name);
}

static void TestPointerParsing() {
  char* name;
  Type* type = ParseTypeString("int *x", &name);

  assert(strcmp(name, "x") == 0);
  assert(type->vtable->kind == TK_PointerType);
  assert(type->qualifiers == 0);

  Type* pointee = ((PointerType*)type)->pointee;
  assert(pointee->vtable->kind == TK_BuiltinType);
  assert(((BuiltinType*)pointee)->kind == BTK_Int);

  type_destroy(type);
  free(type);
  free(name);
}

static void TestArrayPointerParsing() {
  char* name;
  Type* type = ParseTypeString("int **x[][10]", &name);

  assert(strcmp(name, "x") == 0);

  // `x` is an array...
  assert(type->vtable->kind == TK_ArrayType);
  ArrayType* outer_arr = (ArrayType*)type;

  // of no specified size...
  Expr* size = outer_arr->size;
  assert(size == NULL);

  // to an array...
  Type* elem = outer_arr->elem_type;
  assert(elem->vtable->kind == TK_ArrayType);
  ArrayType* inner_arr = (ArrayType*)elem;

  // of size 10...
  Expr* inner_size = inner_arr->size;
  assert(inner_size != NULL);
  assert(inner_size->vtable->kind == EK_Int);
  assert(((Int*)inner_size)->val == 10);

  // of pointer...
  Type* inner_elem = ((ArrayType*)elem)->elem_type;
  assert(inner_elem->vtable->kind == TK_PointerType);
  PointerType* ptrptr = (PointerType*)inner_elem;

  // to pointer...
  assert(ptrptr->pointee->vtable->kind == TK_PointerType);
  PointerType* ptr = (PointerType*)ptrptr->pointee;

  // to ints...
  assert(ptr->pointee->vtable->kind == TK_BuiltinType);
  assert(((BuiltinType*)ptr->pointee)->kind == BTK_Int);

  type_destroy(type);
  free(type);
  free(name);
}

static void TestPointerQualifierParsing() {
  char* name;
  Type* type = ParseTypeString("const int * volatile x", &name);

  assert(strcmp(name, "x") == 0);
  assert(type->vtable->kind == TK_PointerType);
  assert(type->qualifiers == kVolatileMask);

  Type* pointee = ((PointerType*)type)->pointee;
  assert(pointee->vtable->kind == TK_BuiltinType);
  assert(((BuiltinType*)pointee)->kind == BTK_Int);
  assert(pointee->qualifiers == kConstMask);

  type_destroy(type);
  free(type);
  free(name);
}

static void TestFunctionParsing() {
  char* name;
  Type* type = ParseTypeString("void *realloc(void *, size_t)", &name);

  assert(strcmp(name, "realloc") == 0);
  assert(type->vtable->kind == TK_FunctionType);
  FunctionType* func = (FunctionType*)type;
  assert(func->pos_args.size == 2);
  assert(!func->has_var_args);

  Type* arg0 = get_arg_type(func, 0);
  assert(arg0->vtable->kind == TK_PointerType);
  assert(is_builtin_type(((PointerType*)arg0)->pointee, BTK_Void));

  Type* arg1 = get_arg_type(func, 1);
  assert(arg1->vtable->kind == TK_NamedType);
  assert(strcmp(((NamedType*)arg1)->name, "size_t") == 0);

  Type* ret = func->return_type;
  assert(ret->vtable->kind == TK_PointerType);
  assert(is_builtin_type(((PointerType*)ret)->pointee, BTK_Void));

  type_destroy(type);
  free(type);
  free(name);
}

static void TestFunctionParsing2() {
  char* name;
  Type* type = ParseTypeString("size_t strlen(const char *)", &name);

  assert(strcmp(name, "strlen") == 0);
  assert(type->vtable->kind == TK_FunctionType);
  FunctionType* func = (FunctionType*)type;
  assert(func->pos_args.size == 1);
  assert(!func->has_var_args);

  Type* arg0 = get_arg_type(func, 0);
  assert(arg0->vtable->kind == TK_PointerType);
  Type* pointee = ((PointerType*)arg0)->pointee;
  assert(is_builtin_type(pointee, BTK_Char));
  assert(type_is_const(pointee));

  Type* ret = func->return_type;
  assert(ret->vtable->kind == TK_NamedType);
  assert(strcmp(((NamedType*)ret)->name, "size_t") == 0);

  type_destroy(type);
  free(type);
  free(name);
}

static void TestFunctionPointerParsing() {
  char* name;
  Type* type = ParseTypeString("int (*fptr)(void)", &name);

  assert(strcmp(name, "fptr") == 0);
  assert(type->vtable->kind == TK_PointerType);
  Type* nested = ((PointerType*)type)->pointee;

  assert(nested->vtable->kind == TK_FunctionType);
  FunctionType* func = (FunctionType*)nested;
  assert(func->pos_args.size == 1);
  assert(!func->has_var_args);

  assert(is_builtin_type(get_arg_type(func, 0), BTK_Void));
  assert(is_builtin_type(func->return_type, BTK_Int));

  type_destroy(type);
  free(type);
  free(name);
}

static void TestFunctionParsingVarArgs() {
  char* name;
  Type* type = ParseTypeString("int printf(const char *, ...)", &name);

  assert(strcmp(name, "printf") == 0);
  assert(type->vtable->kind == TK_FunctionType);
  FunctionType* func = (FunctionType*)type;
  assert(func->pos_args.size == 1);
  assert(func->has_var_args);

  Type* arg0 = get_arg_type(func, 0);
  assert(arg0->vtable->kind == TK_PointerType);
  Type* pointee = ((PointerType*)arg0)->pointee;
  assert(is_builtin_type(pointee, BTK_Char));
  assert(type_is_const(pointee));

  Type* ret = func->return_type;
  assert(is_builtin_type(ret, BTK_Int));

  type_destroy(type);
  free(type);
  free(name);
}

void RunParserTests() {
  TestSimpleDeclarationParse();
  TestArrayParsing();
  TestPointerParsing();
  TestArrayPointerParsing();
  TestPointerQualifierParsing();
  TestFunctionParsing();
  TestFunctionParsing2();
  TestFunctionPointerParsing();
  TestFunctionParsingVarArgs();
}

///
/// End Parser Tests
///
