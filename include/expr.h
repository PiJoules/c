#ifndef EXPR_H_
#define EXPR_H_

#include "source-location.h"
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
  SourceLocation loc;
};
typedef struct Expr Expr;

void expr_construct(Expr* expr, const ExprVtable* vtable,
                    const SourceLocation* loc);
void expr_destroy(struct Expr*);

typedef struct {
  Expr expr;
  Expr* base;
  Type* to;
} Cast;

void cast_construct(Cast* cast, Expr* base, Type* to,
                    const SourceLocation* loc);

typedef struct {
  Expr expr;
  void* expr_or_type;
  bool is_expr;
} SizeOf;

void sizeof_construct(SizeOf* so, void* expr_or_type, bool is_expr,
                      const SourceLocation* loc);

typedef struct {
  Expr expr;
  void* expr_or_type;
  bool is_expr;
} AlignOf;

void alignof_construct(AlignOf* ao, void* expr_or_type, bool is_expr,
                       const SourceLocation* loc);

typedef enum {
  UOK_PreInc,
  UOK_PostInc,
  UOK_PreDec,
  UOK_PostDec,
  UOK_AddrOf,
  UOK_Deref,
  UOK_Plus,
  UOK_Negate,
  UOK_BitNot,
  UOK_Not,
} UnOpKind;

typedef struct {
  Expr expr;
  Expr* subexpr;
  UnOpKind op;
} UnOp;

void unop_construct(UnOp* unop, Expr* subexpr, UnOpKind op,
                    const SourceLocation* loc);

typedef enum {
  BOK_Comma,  // (x, y) returns y
  BOK_Xor,
  BOK_LogicalOr,
  BOK_LogicalAnd,
  BOK_BitwiseOr,
  BOK_BitwiseAnd,
  BOK_Eq,
  BOK_Ne,
  BOK_Lt,
  BOK_Gt,
  BOK_Le,
  BOK_Ge,
  BOK_LShift,
  BOK_RShift,
  BOK_Add,
  BOK_Sub,
  BOK_Mul,
  BOK_Div,
  BOK_Mod,

  BOK_AssignFirst,
  BOK_Assign = BOK_AssignFirst,
  BOK_MulAssign,
  BOK_DivAssign,
  BOK_ModAssign,
  BOK_AddAssign,
  BOK_SubAssign,
  BOK_LShiftAssign,
  BOK_RShiftAssign,
  BOK_AndAssign,
  BOK_OrAssign,
  BOK_XorAssign,
  BOK_AssignLast = BOK_XorAssign,
} BinOpKind;

static inline bool is_logical_binop(BinOpKind kind) {
  return kind == BOK_LogicalOr || kind == BOK_LogicalAnd;
}

static inline bool is_assign_binop(BinOpKind kind) {
  return BOK_AssignFirst <= kind && kind <= BOK_AssignLast;
}

typedef struct {
  Expr expr;
  Expr* lhs;
  Expr* rhs;
  BinOpKind op;
} BinOp;

void binop_construct(BinOp* binop, Expr* lhs, Expr* rhs, BinOpKind op,
                     const SourceLocation* loc);

typedef struct {
  Expr expr;
  Expr* cond;
  Expr* true_expr;
  Expr* false_expr;
} Conditional;

void conditional_construct(Conditional* c, Expr* cond, Expr* true_expr,
                           Expr* false_expr, const SourceLocation* loc);

typedef struct {
  Expr expr;
  char* name;
} DeclRef;

void declref_construct(DeclRef* ref, const char* name,
                       const SourceLocation* loc);

typedef struct {
  Expr expr;
  bool val;
} Bool;

void bool_construct(Bool* b, bool val, const SourceLocation* loc);

typedef struct {
  Expr expr;
  char val;
} Char;

void char_construct(Char* c, char val, const SourceLocation* loc);

typedef struct {
  Expr expr;
} PrettyFunction;

void pretty_function_construct(PrettyFunction* pf, const SourceLocation* loc);

typedef struct {
  Expr expr;
  uint64_t val;
  BuiltinType type;
} Int;

void int_construct(Int* i, uint64_t val, BuiltinTypeKind kind,
                   const SourceLocation* loc);

typedef struct {
  Expr expr;
  char* val;
} StringLiteral;

void string_literal_construct(StringLiteral* s, const char* val, size_t len,
                              const SourceLocation* loc);

typedef struct {
  char* name;  // Optional.
  Expr* expr;
} InitializerListElem;

typedef struct {
  Expr expr;

  // This is a vector of InitializerListElems.
  vector elems;
} InitializerList;

void initializer_list_construct(InitializerList* init, vector elems,
                                const SourceLocation* loc);

typedef struct {
  Expr expr;
  Expr* base;
  Expr* idx;
} Index;

void index_construct(Index* index_expr, Expr* base, Expr* idx,
                     const SourceLocation* loc);

typedef struct {
  Expr expr;
  Expr* base;
  vector args;  // Vector of Expr*
} Call;

void call_construct(Call* call, Expr* base, vector v,
                    const SourceLocation* loc);

typedef struct {
  Expr expr;
  Expr* base;
  char* member;
  bool is_arrow;
} MemberAccess;

void member_access_construct(MemberAccess* member_access, Expr* base,
                             const char* member, bool is_arrow,
                             const SourceLocation* loc);

typedef struct {
  Expr expr;
  // The caller should not destroy or free these.
  const char* name;
  const Type* type;
} FunctionParam;

void function_param_construct(FunctionParam* param, const char* name,
                              const Type* type, const SourceLocation* loc);

struct CompoundStmt;

typedef struct {
  Expr expr;
  struct CompoundStmt* stmt;  // Optional. NULL indicates an empty statement
                              // expression, which evaluates to `void`.
} StmtExpr;

void stmt_expr_construct(StmtExpr* se, struct CompoundStmt* stmt,
                         const SourceLocation* loc);

#endif  // EXPR_H_
