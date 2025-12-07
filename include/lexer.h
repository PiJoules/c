#ifndef LEXER_H_
#define LEXER_H_

#include <string.h>

#include "cstring.h"
#include "istream.h"

typedef enum {
  TK_BuiltinTypesFirst,
  TK_Char = TK_BuiltinTypesFirst,
  TK_Short,
  TK_Int,
  TK_Signed,
  TK_Unsigned,
  TK_Long,
  TK_Float,
  TK_Double,
  TK_Complex,
  TK_Float128,
  TK_BuiltinVAList,
  TK_Void,
  TK_Bool,
  TK_BuiltinTypesLast = TK_Bool,

  // Qualifiers
  TK_QualifiersFirst,
  TK_Const = TK_QualifiersFirst,
  TK_Volatile,
  TK_Restrict,
  TK_QualifiersLast = TK_Restrict,

  // Storage specifiers
  TK_StorageClassSpecifiersFirst,
  TK_Extern = TK_StorageClassSpecifiersFirst,
  TK_Static,
  TK_Auto,
  TK_Register,
  TK_ThreadLocal,
  TK_StorageClassSpecifiersLast = TK_ThreadLocal,

  // Operations
  TK_LogicalAnd,  // &&
  TK_LogicalOr,   // ||
  TK_Arrow,       // ->, Dereference
  TK_Star,        // *, Pointer, dereference, multiplication
  TK_Ampersand,   // &, Address-of, bitwise-and
  TK_Not,         // !
  TK_Eq,          // ==
  TK_Ne,          // !=
  TK_Lt,          // <
  TK_Gt,          // >
  TK_Le,          // <=
  TK_Ge,          // >=
  TK_Div,         // /
  TK_Mod,         // %
  TK_Add,         // +
  TK_Sub,         // -
  TK_LShift,      // <<
  TK_RShift,      // >>
  TK_Or,          // |
  TK_Xor,         // ^
  TK_BitNot,      // ~

  TK_Inc,  // ++
  TK_Dec,  // --

  // Assignment ops
  TK_Assign,        // =
  TK_MulAssign,     // *-
  TK_DivAssign,     // /=
  TK_ModAssign,     // %=
  TK_AddAssign,     // +=
  TK_SubAssign,     // -=
  TK_LShiftAssign,  // <<=
  TK_RShiftAssign,  // >>+
  TK_AndAssign,     // &=
  TK_OrAssign,      // |=
  TK_XorAssign,     // ^=

  // Other Keywords
  TK_Enum,
  TK_Union,
  TK_Typedef,
  TK_Struct,
  TK_Return,
  TK_StaticAssert,
  TK_SizeOf,
  TK_AlignOf,
  TK_If,
  TK_Else,
  TK_While,
  TK_For,
  TK_Switch,
  TK_Break,
  TK_Continue,
  TK_Case,
  TK_Default,
  TK_True,  // TODO: I believe these are macros instead of actual keywords.
  TK_False,
  TK_Attribute,
  TK_Extension,
  TK_Asm,
  TK_Inline,
  TK_Pragma,

  TK_PrettyFunction,
  TK_Identifier,

  // Literals
  TK_IntLiteral,
  TK_StringLiteral,
  TK_CharLiteral,

  // Single-char tokens
  TK_LPar,          // (
  TK_RPar,          // )
  TK_LCurlyBrace,   // {
  TK_RCurlyBrace,   // }
  TK_LSquareBrace,  // [
  TK_RSquareBrace,  // ]
  TK_Semicolon,     // ;
  TK_Colon,         // :
  TK_Comma,         // ,
  TK_Question,      // ?
  TK_Hash,          // #

  TK_Dot,       // .
  TK_Ellipsis,  // ...

  // This can be returned at the end if there's whitespace at the EOF.
  TK_Eof = -1,
} TokenKind;

static bool is_builtin_type_token(TokenKind kind) {
  return TK_BuiltinTypesFirst <= kind && kind <= TK_BuiltinTypesLast;
}

static bool is_qualifier_token(TokenKind kind) {
  return TK_QualifiersFirst <= kind && kind <= TK_QualifiersLast;
}

static bool is_storage_class_specifier_token(TokenKind kind) {
  return TK_StorageClassSpecifiersFirst <= kind &&
         kind <= TK_StorageClassSpecifiersLast;
}

typedef struct {
  string chars;
  TokenKind kind;
  size_t line;
  size_t col;
} Token;

void token_construct(Token* tok);
void token_destroy(Token* tok);

typedef struct {
  InputStream* input;

  // 0 indicates no lookahead. Non-zero means we have a lookahead.
  int lookahead;

  // Lexer users should never access these directly. Instead refer to the line
  // and col returned in the Token.
  size_t line_;
  size_t col_;
} Lexer;

void lexer_construct(Lexer* lexer, InputStream* input);
void lexer_destroy(Lexer* lexer);
bool lexer_has_lookahead(const Lexer* lexer);
bool lexer_can_get_char(Lexer* lexer);
int lexer_peek_char(Lexer* lexer);
int lexer_get_char(Lexer* lexer);
bool lexer_peek_then_consume_char(Lexer* lexer, char c);

// Callers of this function should destroy the token.
Token lex(Lexer* lexer);

#endif  // LEXER_H_
