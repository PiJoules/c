#ifndef EXPR_H_
#define EXPR_H_

#include "type.h"

typedef enum {
  EK_SizeOf,
  EK_AlignOf,
  EK_UnOp,
  EK_BinOp,
  EK_Conditional,  // x ? y : z
  EK_DeclRef,
  EK_PrettyFunction,
  EK_Int,
  EK_Bool,  // true/false
  EK_String,
  EK_Char,
  EK_InitializerList,
  EK_Index,         // [ ]
  EK_MemberAccess,  // . or ->
  EK_Call,
  EK_Cast,
  EK_FunctionParam,
  EK_StmtExpr,
} ExprKind;

struct Expr;

typedef struct {
  ExprKind kind;
  void (*dtor)(struct Expr*);
} ExprVtable;

struct Expr {
  const ExprVtable* vtable;
  size_t line;
  size_t col;
};
typedef struct Expr Expr;

void expr_construct(Expr* expr, const ExprVtable* vtable);
void expr_destroy(struct Expr*);

typedef struct {
  Expr expr;
  Expr* base;
  Type* to;
} Cast;

void cast_construct(Cast* cast, Expr* base, Type* to);

#endif  // EXPR_H_
