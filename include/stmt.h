#ifndef STMT_H_
#define STMT_H_

#include "expr.h"
#include "source-location.h"
#include "type.h"
#include "vector.h"

typedef enum {
  SK_ExprStmt,
  SK_IfStmt,
  SK_CompoundStmt,
  SK_ReturnStmt,
  SK_Declaration,
  SK_ForStmt,
  SK_WhileStmt,
  SK_SwitchStmt,
  SK_BreakStmt,
  SK_ContinueStmt,
} StatementKind;

struct Statement;

typedef struct {
  StatementKind kind;
  void (*dtor)(struct Statement*);
} StatementVtable;

struct Statement {
  const StatementVtable* vtable;
  SourceLocation loc;
};
typedef struct Statement Statement;

void statement_construct(Statement* stmt, const StatementVtable* vtable,
                         const SourceLocation* loc);
void statement_destroy(Statement* stmt);

typedef struct {
  Statement base;
  char* name;
  Type* type;
  Expr* initializer;  // Optional.
} Declaration;

void declaration_construct(Declaration* decl, const char* name, Type* type,
                           Expr* init, const SourceLocation* loc);

typedef struct {
  Statement base;
} ContinueStmt;

void continue_stmt_construct(ContinueStmt* stmt, const SourceLocation* loc);

typedef struct {
  Statement base;
} BreakStmt;

void break_stmt_construct(BreakStmt* stmt, const SourceLocation* loc);

struct IfStmt {
  Statement base;
  Expr* cond;  // Required only for the first If. All chained Ifs through the
               // else_stmt can have this optionally.
  Statement* body;       // Optional.
  Statement* else_stmt;  // Optional.
};
typedef struct IfStmt IfStmt;

void if_stmt_construct(IfStmt* stmt, Expr* cond, Statement* body,
                       const SourceLocation* loc);

typedef struct {
  Expr* cond;

  // Vector of Statement pointers.
  vector stmts;
} SwitchCase;

void switch_case_destroy(SwitchCase* switch_case);

typedef struct {
  Statement base;
  Expr* cond;

  // Vector of SwitchCases.
  vector cases;

  // Vector of Statement pointers.
  //
  // Optional. If not provided, then this switch has no default branch.
  vector* default_stmts;
} SwitchStmt;

void switch_stmt_construct(SwitchStmt* stmt, Expr* cond, vector cases,
                           vector* default_stmts, const SourceLocation* loc);

typedef struct {
  Statement base;
  Expr* cond;
  Statement* body;  // Optional.
} WhileStmt;

void while_stmt_construct(WhileStmt* stmt, Expr* cond, Statement* body,
                          const SourceLocation* loc);

typedef struct {
  Statement base;
  Statement* init;  // Optional. This can be either an ExprStmt or
                    // Declaration.
  Expr* cond;       // Optional.
  Expr* iter;       // Optional.
  Statement* body;  // Optional.
} ForStmt;

void for_stmt_construct(ForStmt* stmt, Statement* init, Expr* cond, Expr* iter,
                        Statement* body, const SourceLocation* loc);

struct CompoundStmt {
  Statement base;
  vector body;  // vector of Statement pointers.
};
typedef struct CompoundStmt CompoundStmt;

void compound_stmt_construct(CompoundStmt* stmt, vector body,
                             const SourceLocation* loc);

typedef struct {
  Statement base;
  Expr* expr;  // Optional. NULL means returning void.
} ReturnStmt;

void return_stmt_construct(ReturnStmt* stmt, Expr* expr,
                           const SourceLocation* loc);

typedef struct {
  Statement base;
  Expr* expr;
} ExprStmt;

void expr_stmt_construct(ExprStmt* stmt, Expr* expr, const SourceLocation* loc);

#endif  // STMT_H_
