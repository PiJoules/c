#ifndef PARSER_H_
#define PARSER_H_

#include "expr.h"
#include "istream.h"
#include "lexer.h"
#include "stmt.h"
#include "top-level-node.h"
#include "tree-map.h"
#include "type.h"
#include "vector.h"

typedef struct {
  Lexer lexer;
  Token lookahead;
  bool has_lookahead;

  // Needed for distinguishing between "(" <expr> ")" and "(" <type> ")".
  // See https://en.wikipedia.org/wiki/Lexer_hack
  // This is really only used as a set rather than a map.
  TreeMap typedef_types;
} Parser;

void parser_construct(Parser* parser, InputStream* input);
void parser_destroy(Parser* parser);
void parser_define_named_type(Parser* parser, const char* name);
bool parser_has_named_type(const Parser* parser, const char* name);

// Callers of this should destroy the token.
Token parser_pop_token(Parser* parser);

// Callers of this should not destroy the token. Instead callers of
// parser_pop_token should destroy it.
const Token* parser_peek_token(Parser* parser);

bool next_token_is(Parser* parser, TokenKind kind);
void parser_skip_next_token(Parser* parser);
void parser_consume_token(Parser* parser, TokenKind kind);

// Similar to `parser_consume_token`, but it will only consume the token if
// it's the one we expect. Otherwise, no token is consumed.
void parser_consume_token_if_matches(Parser* parser, TokenKind kind);

Expr* parse_expr(Parser*);
Expr* parse_cast_expr(Parser*);
Expr* parse_conditional_expr(Parser*);
Expr* parse_assignment_expr(Parser*);
Type* parse_type(Parser* parser);

bool is_next_token_type_token(Parser*);

Qualifiers parse_maybe_qualifiers(Parser* parser, bool* found);
Type* maybe_parse_pointers_and_qualifiers(Parser* parser, Type* base,
                                          Type*** type_usage_addr);

bool is_token_type_token(const Parser* parser, const Token* tok);

typedef struct {
  unsigned int extern_ : 1;
  unsigned int static_ : 1;
  unsigned int auto_ : 1;
  unsigned int register_ : 1;
  unsigned int thread_local_ : 1;
} FoundStorageClasses;

// <SPECIFIER/QUALIFIER MIX> <DECLARATOR>
// DECLARATOR = <POINTER/QUALIFIER MIX> (nested declarators) (parens or sq
// braces)
//
// `name` is an optional pointer to a char*. If provided, and a name is found,
// the string address will be stored in `name. Callers of this are in charge of
// freeing the string.
//
// `storage` is an optional parameter to indicate storage classes found.
Type* parse_type_for_declaration(Parser* parser, char** name,
                                 FoundStorageClasses* storage);

// TODO: Don't handle attributes for now. Just consume them.
void parser_consume_attribute(Parser* parser);
void parser_consume_asm_label(Parser* parser);
void parser_consume_pragma(Parser* parser);

void parse_struct_name_and_members(Parser* parser, char** name,
                                   vector** members);
void parse_union_name_and_members(Parser* parser, char** name,
                                  vector** members);
void parse_enum_name_and_members(Parser* parser, char** name, vector** members);

StructType* parse_struct_type(Parser* parser);
UnionType* parse_union_type(Parser* parser);
EnumType* parse_enum_type(Parser* parser);

Type* parse_specifiers_and_qualifiers_and_storage(Parser* parser,
                                                  FoundStorageClasses* storage,
                                                  bool* found_inline);
Type* parse_pointers_and_qualifiers(Parser* parser, Type* base);
Type* parse_declarator_maybe_type_suffix(Parser* parser, Type* outer_ty,
                                         Type*** type_usage_addr);
Type* parse_array_suffix(Parser* parser, Type* outer_ty,
                         Type*** type_usage_addr);
Type* parse_function_suffix(Parser* parser, Type* outer_ty,
                            Type*** type_usage_addr);

// `type_usage_addr` is used for saving the single location of where the
// `type` is used when parsing the declarator. It is a triple pointer because
// it should point to the location the Type* is used.
Type* parse_declarator(Parser* parser, Type* type, char** name,
                       Type*** type_usage_addr);
//
// <sizeof> = "sizeof" "(" (<expr> | <type>) ")"
//
Expr* parse_sizeof(Parser* parser);

//
// <alignof> = "alignof" "(" (<expr> | <type>) ")"
//
Expr* parse_alignof(Parser* parser);

//
// <primary_expr> = <identifier> | <constant> | <string_literal>
//                | "__PRETTY_FUNCTION__" | "(" <expr> ")"
//
Expr* parse_primary_expr(Parser* parser);

//
// <argument_expr_list> = <assignment_expr> ("," <assignment_expr>)*
//
vector parse_argument_list(Parser* parser);

//
// <postfix_expr> = <primary_expr>
//                | <postfix_expr> "[" <expr> "]"
//                | <postfix_expr> "(" <argument_expr_list>? ")"
//                | <postfix_expr> "." <identifier>
//                | <postfix_expr> "->" <identifier>
//                | <postfix_expr> "++"
//                | <postfix_expr> "--"
//
Expr* parse_postfix_expr(Parser* parser);

//
// <unary_epr> = <postfix_expr>
//             | ("++" | "--") <unary_expr>
//             | ("&" | "*" | "+" | "-" | "~" | "!") <cast_expr>
//             | "sizeof" "(" (<unary_expr> | <type>) ")"
//
Expr* parse_unary_expr(Parser* parser);

//
// <cast_epr> = <unary_expr>
//            | "(" <type> ")" <cast_expr>
//
Expr* parse_cast_expr(Parser* parser);

//
// <multiplicative_epr> = <cast_expr>
//                      | <cast_expr> ("*" | "/" | "%") <multiplicative_expr>
//
Expr* parse_multiplicative_expr(Parser* parser);

//
// <additive_epr> = <multiplicative_expr>
//                | <multiplicative_expr> ("+" | "-") <additive_expr>
//
Expr* parse_additive_expr(Parser* parser);

//
// <shift_epr> = <additive_expr>
//             | <additive_expr> ("<<" | ">>") <shift_expr>
//
Expr* parse_shift_expr(Parser* parser);

//
// <relational_expr> = <shift_expr>
//                   | <shift_expr> ("<" | ">" | ">=" | "<=") <relational_expr>
//
Expr* parse_relational_expr(Parser* parser);

//
// <equality_expr> = <relational_expr>
//                 | <relational_expr> ("==" | "!=") <equality_expr>
//
Expr* parse_equality_expr(Parser* parser);

//
// <and_expr> = <equality_expr>
//            | <equality_expr> "&" <and_expr>
//
Expr* parse_and_expr(Parser* parser);

//
// <exclusive_or_expr> = <and_expr>
//                     | <and_expr> "^" <exclusive_or_expr>
//
Expr* parse_exclusive_or_expr(Parser* parser);

//
// <inclusive_or_expr> = <exclusive_or_expr>
//                     | <exclusive_or_expr> "|" <inclusive_or_expr>
//
Expr* parse_inclusive_or_expr(Parser* parser);

//
// <logical_and_expr> = <inclusive_or_expr>
//                    | <inclusive_or_expr> "&&" <logical_and_expr>
//
Expr* parse_logical_and_expr(Parser* parser);

//
// <logical_or_expr> = <logical_and_expr>
//                   | <logical_and_expr> "||" <logical_or_expr>
//
Expr* parse_logical_or_expr(Parser* parser);

//
// <conditional_expr> = <logical_or_expr>
//                    | <logical_or_expr> "?" <expr> ":" <conditional_expr>
//
Expr* parse_conditional_expr(Parser* parser);

//
// <assignment_expr> = <conditional_expr>
//                   | <conditional_expr> <assignment_op> <assignment_expr>
//
// <assignment_op> = ("*" | "/" | "%" | "+" | "-" | "<<" | ">>" | "&" | "|" |
//                    "^")? "="
//
Expr* parse_assignment_expr(Parser* parser);

//
// <expr> = <assignment_expr> | <assignment_expr> "," <expr>
//
// Operator precedence: https://www.lysator.liu.se/c/ANSI-C-grammar-y.html
Expr* parse_expr(Parser* parser);

//
// <static_assert> = "static_assert" "(" <expr> ")"
//
TopLevelNode* parse_static_assert(Parser* parser);

//
// <typedef> = "typedef" <type_with_declarator>
//
TopLevelNode* parse_typedef(Parser* parser);

// This consumes any trailing semicolons when needed.
Statement* parse_statement(Parser* parser);

Statement* parse_compound_stmt(Parser*);
Statement* parse_expr_statement(Parser* parser);
Statement* parse_declaration(Parser* parser);

// This disambiguates between a top-level tagged type declaration vs a global
// variable declaration.
TopLevelNode* parse_top_level_type_decl(Parser* parser);

TopLevelNode* parse_struct_declaration(Parser* parser);
TopLevelNode* parse_enum_declaration(Parser* parser);
TopLevelNode* parse_union_declaration(Parser* parser);

// NOTE: Callers of this function are in charge of destroying and freeing the
// poiner.
TopLevelNode* parse_top_level_decl(Parser* parser);

void RunParserTests();

#endif  // PARSER_H_
