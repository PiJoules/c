#include "lexer.h"

#include <ctype.h>
#include <stdio.h>

#include "common.h"
#include "cstring.h"
#include "istream.h"

void lexer_construct(Lexer* lexer, InputStream* input) {
  lexer->input = input;
  lexer->lookahead = 0;
  lexer->line_ = 1;
  // Start at zero since this is incremented for each char read.
  lexer->col_ = 0;
}

void lexer_destroy(Lexer* lexer) {}

bool lexer_has_lookahead(const Lexer* lexer) { return lexer->lookahead != 0; }

// Return true if the lexer can read a more token. This means there's at least
// a lookahead or something we can read from the file stream.
bool lexer_can_get_char(Lexer* lexer) {
  if (lexer_has_lookahead(lexer))
    return true;

  if (input_stream_eof(lexer->input))
    return false;

  // It's possible that we have no lookahead but have recently popped the very
  // last char in a stream with fgetc. In that case, the very next call to
  // fgetc will return EOF and calls to feof will return true. To account for
  // this, let's read a character then set it as the lookahead. If the fgetc
  // call hit EOF, then feof will always return true from now on. If not, then
  // we can set this valid char as the lookahead.
  int peek = input_stream_read(lexer->input);
  if (peek == EOF)
    return false;

  if (peek == '\n') {
    lexer->line_++;
    lexer->col_ = 0;
  } else {
    lexer->col_++;
  }

  lexer->lookahead = peek;
  return true;
}

// Peek a character from the lexer. If the lexer reached EOF then the
// returned value is an EOF value but it's not stored as a lookahead.
int lexer_peek_char(Lexer* lexer) {
  if (!lexer_has_lookahead(lexer)) {
    if (input_stream_eof(lexer->input))
      return -1;

    int res = input_stream_read(lexer->input);
    if (res == EOF)
      return -1;

    if (res == '\n') {
      lexer->line_++;
      lexer->col_ = 0;
    } else {
      lexer->col_++;
    }

    lexer->lookahead = res;
    assert(lexer->lookahead != 0 && "Invalid lookahead");
    assert(lexer->lookahead != EOF);
  }

  return lexer->lookahead;
}

int lexer_get_char(Lexer* lexer) {
  if (lexer_has_lookahead(lexer)) {
    int c = lexer->lookahead;
    lexer->lookahead = 0;
    return c;
  }

  int res = input_stream_read(lexer->input);
  if (res == '\n') {
    lexer->line_++;
    lexer->col_ = 0;
  } else if (res != EOF) {
    lexer->col_++;
  }
  return res;
}

void token_construct(Token* tok) { string_construct(&tok->chars); }

void token_destroy(Token* tok) { string_destroy(&tok->chars); }

static bool is_kw_char(int c) { return c == '_' || isalnum(c); }

// Skip whitespace and ensure the cursor is on a character that isn't ws,
// unless it's EOF.
static void skip_ws(Lexer* lexer) {
  int c;
  while (true) {
    c = lexer_peek_char(lexer);
    if (c == EOF)
      return;

    if (!isspace(c))
      return;

    // Continue and keep popping from the stream until we hit EOF or a
    // non-space character.
    lexer_get_char(lexer);
  }
}

// Peek a char. If it matches the expected char, consume it and return true.
// Otherwise, don't consume it and return false.
bool lexer_peek_then_consume_char(Lexer* lexer, char c) {
  if (lexer_peek_char(lexer) != c)
    return false;
  lexer_get_char(lexer);
  return true;
}

static char handle_escape_char(int c) {
  switch (c) {
    case 'n':
      return '\n';
    case 't':
      return '\t';
    case '\'':
      return '\'';
    case '\\':
      return '\\';
    case '"':
      return '"';
    default:
      UNREACHABLE_MSG("Unhandled escape character '%c'\n", c);
  }
}

Token lex(Lexer* lexer) {
  Token tok;
  token_construct(&tok);
  tok.kind = TK_Eof;  // Default value

  int c;
  while (true) {
    skip_ws(lexer);

    if (lexer_peek_char(lexer) == EOF) {
      tok.line = lexer->line_;
      tok.col = lexer->col_;
      return tok;
    }

    c = lexer_get_char(lexer);
    assert(c != EOF && !isspace(c));

    string_append_char(&tok.chars, (char)c);

    // First handle potential comments before anything else.
    if (c == '/') {
      tok.line = lexer->line_;
      tok.col = lexer->col_;

      if (lexer_peek_char(lexer) == '/') {
        // This is a comment. Consume all characters until the newline.
        for (; lexer_get_char(lexer) != '\n';);
        string_clear(&tok.chars);
        continue;
      }

      if (lexer_peek_char(lexer) == '*') {
        // This is a comment. Consume all characters until the final `*/`.
        while (true) {
          if (lexer_get_char(lexer) == '*') {
            if (lexer_peek_then_consume_char(lexer, '/'))
              break;
          }
        }
        string_clear(&tok.chars);
        continue;
      }

      // Normal division.
      tok.kind = TK_Div;
      return tok;
    }

    break;
  }

  tok.line = lexer->line_;
  tok.col = lexer->col_;

  // Handle elipsis.
  if (c == '.') {
    if (lexer_peek_then_consume_char(lexer, '.')) {
      ASSERT_MSG(lexer_peek_then_consume_char(lexer, '.'),
                 "%zu:%zu: Expected 3 '.' for elipses but found 2.", tok.line,
                 tok.col);
      string_append(&tok.chars, "..");
      tok.kind = TK_Ellipsis;
    } else {
      tok.kind = TK_Dot;
    }
    return tok;
  }

  // Non-single char tokens starting with special characters.
  if (c == '+') {
    if (lexer_peek_then_consume_char(lexer, '+')) {
      tok.kind = TK_Inc;
      string_append_char(&tok.chars, '+');
    } else if (lexer_peek_then_consume_char(lexer, '=')) {
      tok.kind = TK_AddAssign;
      string_append_char(&tok.chars, '=');
    } else {
      tok.kind = TK_Add;
    }
    return tok;
  }

  if (c == '-') {
    if (lexer_peek_then_consume_char(lexer, '-')) {
      tok.kind = TK_Dec;
      string_append_char(&tok.chars, '-');
    } else if (lexer_peek_then_consume_char(lexer, '=')) {
      tok.kind = TK_SubAssign;
      string_append_char(&tok.chars, '=');
    } else if (lexer_peek_then_consume_char(lexer, '>')) {
      tok.kind = TK_Arrow;
      string_append_char(&tok.chars, '>');
    } else {
      tok.kind = TK_Sub;
    }
    return tok;
  }

  if (c == '!') {
    if (lexer_peek_then_consume_char(lexer, '=')) {
      tok.kind = TK_Ne;
      string_append_char(&tok.chars, '=');
    } else {
      tok.kind = TK_Not;
    }
    return tok;
  }

  if (c == '=') {
    if (lexer_peek_then_consume_char(lexer, '=')) {
      string_append_char(&tok.chars, '=');
      tok.kind = TK_Eq;
    } else {
      tok.kind = TK_Assign;
    }
    return tok;
  }

  if (c == '&') {
    // Disambiguate between bitwise-and/address-of vs logical-and.
    if (lexer_peek_then_consume_char(lexer, '&')) {
      string_append_char(&tok.chars, '&');
      tok.kind = TK_LogicalAnd;
    } else {
      tok.kind = TK_Ampersand;
    }
    return tok;
  }

  if (c == '|') {
    // Disambiguate between bitwise-or and logical-or.
    if (lexer_peek_then_consume_char(lexer, '|')) {
      string_append_char(&tok.chars, '|');
      tok.kind = TK_LogicalOr;
    } else if (lexer_peek_then_consume_char(lexer, '=')) {
      string_append_char(&tok.chars, '=');
      tok.kind = TK_OrAssign;
    } else {
      tok.kind = TK_Or;
    }
    return tok;
  }

  if (c == '%') {
    if (lexer_peek_then_consume_char(lexer, '=')) {
      string_append_char(&tok.chars, '=');
      tok.kind = TK_ModAssign;
    } else {
      tok.kind = TK_Mod;
    }
    return tok;
  }

  if (c == '<') {
    if (lexer_peek_then_consume_char(lexer, '=')) {
      string_append_char(&tok.chars, '=');
      tok.kind = TK_Le;
    } else if (lexer_peek_then_consume_char(lexer, '<')) {
      string_append_char(&tok.chars, '<');
      if (lexer_peek_then_consume_char(lexer, '=')) {
        string_append_char(&tok.chars, '=');
        tok.kind = TK_LShiftAssign;
      } else {
        tok.kind = TK_LShift;
      }
    } else {
      tok.kind = TK_Lt;
    }
    return tok;
  }

  if (c == '>') {
    if (lexer_peek_then_consume_char(lexer, '=')) {
      string_append_char(&tok.chars, '=');
      tok.kind = TK_Ge;
    } else if (lexer_peek_then_consume_char(lexer, '>')) {
      string_append_char(&tok.chars, '>');
      if (lexer_peek_then_consume_char(lexer, '=')) {
        string_append_char(&tok.chars, '=');
        tok.kind = TK_RShiftAssign;
      } else {
        tok.kind = TK_RShift;
      }
    } else {
      tok.kind = TK_Gt;
    }
    return tok;
  }

  if (c == '"') {
    // TODO: Handle escape chars.
    for (; !lexer_peek_then_consume_char(lexer, '"');) {
      int c = lexer_get_char(lexer);
      ASSERT_MSG(c != EOF, "Got EOF before finishing string parsing");

      if (c == '\\')
        c = handle_escape_char(lexer_get_char(lexer));

      string_append_char(&tok.chars, (char)c);
    }

    string_append_char(&tok.chars, '"');
    tok.kind = TK_StringLiteral;
    return tok;
  }

  if (c == '\'') {
    tok.kind = TK_CharLiteral;
    int c = lexer_get_char(lexer);
    assert(c != EOF);

    if (c == '\\')
      c = handle_escape_char(lexer_get_char(lexer));

    string_append_char(&tok.chars, (char)c);

    ASSERT_MSG(lexer_peek_then_consume_char(lexer, '\''),
               "%zu:%zu: Expected `'` for ending char.\n", tok.line, tok.col);

    string_append_char(&tok.chars, '\'');
    return tok;
  }

  // Handle special single character tokens.
  switch (c) {
    case '(':
      tok.kind = TK_LPar;
      return tok;
    case ')':
      tok.kind = TK_RPar;
      return tok;
    case '{':
      tok.kind = TK_LCurlyBrace;
      return tok;
    case '}':
      tok.kind = TK_RCurlyBrace;
      return tok;
    case '[':
      tok.kind = TK_LSquareBrace;
      return tok;
    case ']':
      tok.kind = TK_RSquareBrace;
      return tok;
    case '*':
      tok.kind = TK_Star;
      return tok;
    case ';':
      tok.kind = TK_Semicolon;
      return tok;
    case ':':
      tok.kind = TK_Colon;
      return tok;
    case ',':
      tok.kind = TK_Comma;
      return tok;
    case '?':
      tok.kind = TK_Question;
      return tok;
    case '#':
      tok.kind = TK_Hash;
      return tok;
    case '~':
      tok.kind = TK_BitNot;
      return tok;
  }

  // Integers
  if (isdigit(c)) {
    tok.kind = TK_IntLiteral;

    if (lexer_peek_char(lexer) == 'x' || lexer_peek_char(lexer) == 'X') {
      string_append_char(&tok.chars, (char)lexer_get_char(lexer));
      while (true) {
        char c = (char)lexer_peek_char(lexer);
        if (isdigit(c) || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F'))
          string_append_char(&tok.chars, (char)lexer_get_char(lexer));
        else
          break;
      }
    } else {
      for (; isdigit(lexer_peek_char(lexer));)
        string_append_char(&tok.chars, (char)lexer_get_char(lexer));
    }

    if (lexer_peek_char(lexer) == 'u')
      string_append_char(&tok.chars, (char)lexer_get_char(lexer));

    if (lexer_peek_char(lexer) == 'l') {
      string_append_char(&tok.chars, (char)lexer_get_char(lexer));
      if (lexer_peek_char(lexer) == 'l')
        string_append_char(&tok.chars, (char)lexer_get_char(lexer));
    }

    return tok;
  }

  // Keywords
  for (; is_kw_char(lexer_peek_char(lexer));) {
    string_append_char(&tok.chars, (char)lexer_get_char(lexer));
  }

  if (string_equals(&tok.chars, "char")) {
    tok.kind = TK_Char;
  } else if (string_equals(&tok.chars, "bool")) {
    tok.kind = TK_Bool;
  } else if (string_equals(&tok.chars, "short")) {
    tok.kind = TK_Short;
  } else if (string_equals(&tok.chars, "int")) {
    tok.kind = TK_Int;
  } else if (string_equals(&tok.chars, "unsigned")) {
    tok.kind = TK_Unsigned;
  } else if (string_equals(&tok.chars, "signed")) {
    tok.kind = TK_Signed;
  } else if (string_equals(&tok.chars, "long")) {
    tok.kind = TK_Long;
  } else if (string_equals(&tok.chars, "float")) {
    tok.kind = TK_Float;
  } else if (string_equals(&tok.chars, "double")) {
    tok.kind = TK_Double;
  } else if (string_equals(&tok.chars, "_Complex")) {
    tok.kind = TK_Complex;
  } else if (string_equals(&tok.chars, "__float128")) {
    tok.kind = TK_Float128;
  } else if (string_equals(&tok.chars, "void")) {
    tok.kind = TK_Void;
  } else if (string_equals(&tok.chars, "const")) {
    tok.kind = TK_Const;
  } else if (string_equals(&tok.chars, "volatile")) {
    tok.kind = TK_Volatile;
  } else if (string_equals(&tok.chars, "restrict") ||
             string_equals(&tok.chars, "__restrict")) {
    tok.kind = TK_Restrict;
  } else if (string_equals(&tok.chars, "enum")) {
    tok.kind = TK_Enum;
  } else if (string_equals(&tok.chars, "union")) {
    tok.kind = TK_Union;
  } else if (string_equals(&tok.chars, "__attribute__")) {
    tok.kind = TK_Attribute;
  } else if (string_equals(&tok.chars, "__extension__")) {
    tok.kind = TK_Extension;
  } else if (string_equals(&tok.chars, "__asm__") ||
             string_equals(&tok.chars, "asm")) {
    tok.kind = TK_Asm;
  } else if (string_equals(&tok.chars, "__inline") ||
             string_equals(&tok.chars, "inline")) {
    tok.kind = TK_Inline;
  } else if (string_equals(&tok.chars, "pragma")) {
    tok.kind = TK_Pragma;
  } else if (string_equals(&tok.chars, "__builtin_va_list")) {
    tok.kind = TK_BuiltinVAList;
  } else if (string_equals(&tok.chars, "typedef")) {
    tok.kind = TK_Typedef;
  } else if (string_equals(&tok.chars, "struct")) {
    tok.kind = TK_Struct;
  } else if (string_equals(&tok.chars, "return")) {
    tok.kind = TK_Return;
  } else if (string_equals(&tok.chars, "static_assert")) {
    tok.kind = TK_StaticAssert;
  } else if (string_equals(&tok.chars, "sizeof")) {
    tok.kind = TK_SizeOf;
  } else if (string_equals(&tok.chars, "alignof")) {
    tok.kind = TK_AlignOf;
  } else if (string_equals(&tok.chars, "if")) {
    tok.kind = TK_If;
  } else if (string_equals(&tok.chars, "else")) {
    tok.kind = TK_Else;
  } else if (string_equals(&tok.chars, "while")) {
    tok.kind = TK_While;
  } else if (string_equals(&tok.chars, "for")) {
    tok.kind = TK_For;
  } else if (string_equals(&tok.chars, "switch")) {
    tok.kind = TK_Switch;
  } else if (string_equals(&tok.chars, "break")) {
    tok.kind = TK_Break;
  } else if (string_equals(&tok.chars, "continue")) {
    tok.kind = TK_Continue;
  } else if (string_equals(&tok.chars, "case")) {
    tok.kind = TK_Case;
  } else if (string_equals(&tok.chars, "default")) {
    tok.kind = TK_Default;
  } else if (string_equals(&tok.chars, "extern")) {
    tok.kind = TK_Extern;
  } else if (string_equals(&tok.chars, "static")) {
    tok.kind = TK_Static;
  } else if (string_equals(&tok.chars, "auto")) {
    tok.kind = TK_Auto;
  } else if (string_equals(&tok.chars, "register")) {
    tok.kind = TK_Register;
  } else if (string_equals(&tok.chars, "thread_local")) {
    tok.kind = TK_ThreadLocal;
  } else if (string_equals(&tok.chars, "__PRETTY_FUNCTION__")) {
    tok.kind = TK_PrettyFunction;
  } else if (string_equals(&tok.chars, "true")) {
    tok.kind = TK_True;
  } else if (string_equals(&tok.chars, "false")) {
    tok.kind = TK_False;
  } else {
    tok.kind = TK_Identifier;
  }

  return tok;
}
