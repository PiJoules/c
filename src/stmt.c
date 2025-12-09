#include "stmt.h"

#include <stdlib.h>

#include "expr.h"
#include "type.h"
#include "vector.h"

void statement_construct(Statement* stmt, const StatementVtable* vtable) {
  stmt->vtable = vtable;
}

void statement_destroy(Statement* stmt) { stmt->vtable->dtor(stmt); }

static void declaration_destroy(Statement*);

static const StatementVtable DeclarationVtable = {
    .kind = SK_Declaration,
    .dtor = declaration_destroy,
};

void declaration_construct(Declaration* decl, const char* name, Type* type,
                           Expr* init) {
  statement_construct(&decl->base, &DeclarationVtable);
  decl->name = strdup(name);
  decl->type = type;
  decl->initializer = init;
}

void declaration_destroy(Statement* stmt) {
  Declaration* decl = (Declaration*)stmt;
  free(decl->name);
  type_destroy(decl->type);
  free(decl->type);
  if (decl->initializer) {
    expr_destroy(decl->initializer);
    free(decl->initializer);
  }
}

static void continue_stmt_destroy(Statement*) {}

static const StatementVtable ContinueStmtVtable = {
    .kind = SK_ContinueStmt,
    .dtor = continue_stmt_destroy,
};

void continue_stmt_construct(ContinueStmt* stmt) {
  statement_construct(&stmt->base, &ContinueStmtVtable);
}

static void break_stmt_destroy(Statement*) {}

static const StatementVtable BreakStmtVtable = {
    .kind = SK_BreakStmt,
    .dtor = break_stmt_destroy,
};

void break_stmt_construct(BreakStmt* stmt) {
  statement_construct(&stmt->base, &BreakStmtVtable);
}

static void if_stmt_destroy(Statement*);

static const StatementVtable IfStmtVtable = {
    .kind = SK_IfStmt,
    .dtor = if_stmt_destroy,
};

void if_stmt_construct(IfStmt* stmt, Expr* cond, Statement* body) {
  statement_construct(&stmt->base, &IfStmtVtable);
  stmt->cond = cond;
  stmt->body = body;
  stmt->else_stmt = NULL;
}

void if_stmt_destroy(Statement* stmt) {
  IfStmt* ifstmt = (IfStmt*)stmt;
  if (ifstmt->cond) {
    expr_destroy(ifstmt->cond);
    free(ifstmt->cond);
  }

  if (ifstmt->body) {
    statement_destroy(ifstmt->body);
    free(ifstmt->body);
  }

  if (ifstmt->else_stmt) {
    statement_destroy(ifstmt->else_stmt);
    free(ifstmt->else_stmt);
  }
}

void switch_case_destroy(SwitchCase* switch_case) {
  expr_destroy(switch_case->cond);
  free(switch_case->cond);

  for (size_t i = 0; i < switch_case->stmts.size; ++i) {
    Statement** stmt = vector_at(&switch_case->stmts, i);
    statement_destroy(*stmt);
    free(*stmt);
  }

  vector_destroy(&switch_case->stmts);
}

static void switch_stmt_destroy(Statement*);

static const StatementVtable SwitchStmtVtable = {
    .kind = SK_SwitchStmt,
    .dtor = switch_stmt_destroy,
};

void switch_stmt_construct(SwitchStmt* stmt, Expr* cond, vector cases,
                           vector* default_stmts) {
  statement_construct(&stmt->base, &SwitchStmtVtable);
  stmt->cond = cond;
  stmt->cases = cases;
  stmt->default_stmts = default_stmts;
}

void switch_stmt_destroy(Statement* stmt) {
  SwitchStmt* switch_stmt = (SwitchStmt*)stmt;

  expr_destroy(switch_stmt->cond);
  free(switch_stmt->cond);

  for (size_t i = 0; i < switch_stmt->cases.size; ++i) {
    SwitchCase* switch_case = vector_at(&switch_stmt->cases, i);
    switch_case_destroy(switch_case);
  }
  vector_destroy(&switch_stmt->cases);

  if (switch_stmt->default_stmts) {
    for (size_t i = 0; i < switch_stmt->default_stmts->size; ++i) {
      Statement** stmt = vector_at(switch_stmt->default_stmts, i);
      statement_destroy(*stmt);
      free(*stmt);
    }
    vector_destroy(switch_stmt->default_stmts);
    free(switch_stmt->default_stmts);
  }
}

static void while_stmt_destroy(Statement*);

static const StatementVtable WhileStmtVtable = {
    .kind = SK_WhileStmt,
    .dtor = while_stmt_destroy,
};

void while_stmt_construct(WhileStmt* stmt, Expr* cond, Statement* body) {
  statement_construct(&stmt->base, &WhileStmtVtable);
  stmt->cond = cond;
  assert(cond);
  stmt->body = body;
}

void while_stmt_destroy(Statement* stmt) {
  WhileStmt* while_stmt = (WhileStmt*)stmt;
  expr_destroy(while_stmt->cond);
  free(while_stmt->cond);

  if (while_stmt->body) {
    statement_destroy(while_stmt->body);
    free(while_stmt->body);
  }
}

static void for_stmt_destroy(Statement*);

static const StatementVtable ForStmtVtable = {
    .kind = SK_ForStmt,
    .dtor = for_stmt_destroy,
};

void for_stmt_construct(ForStmt* stmt, Statement* init, Expr* cond, Expr* iter,
                        Statement* body) {
  statement_construct(&stmt->base, &ForStmtVtable);
  stmt->init = init;
  stmt->cond = cond;
  stmt->iter = iter;
  stmt->body = body;
}

void for_stmt_destroy(Statement* stmt) {
  ForStmt* for_stmt = (ForStmt*)stmt;
  if (for_stmt->init) {
    statement_destroy(for_stmt->init);
    free(for_stmt->init);
  }
  if (for_stmt->cond) {
    expr_destroy(for_stmt->cond);
    free(for_stmt->cond);
  }
  if (for_stmt->iter) {
    expr_destroy(for_stmt->iter);
    free(for_stmt->iter);
  }
  if (for_stmt->body) {
    statement_destroy(for_stmt->body);
    free(for_stmt->body);
  }
}

static void compound_stmt_destroy(Statement*);

static const StatementVtable CompoundStmtVtable = {
    .kind = SK_CompoundStmt,
    .dtor = compound_stmt_destroy,
};

void compound_stmt_construct(CompoundStmt* stmt, vector body) {
  statement_construct(&stmt->base, &CompoundStmtVtable);
  stmt->body = body;
}

void compound_stmt_destroy(Statement* stmt) {
  CompoundStmt* cmpd = (CompoundStmt*)stmt;
  for (size_t i = 0; i < cmpd->body.size; ++i) {
    Statement** stmt = vector_at(&cmpd->body, i);
    statement_destroy(*stmt);
    free(*stmt);
  }
  vector_destroy(&cmpd->body);
}

static void return_stmt_destroy(Statement*);

static const StatementVtable ReturnStmtVtable = {
    .kind = SK_ReturnStmt,
    .dtor = return_stmt_destroy,
};

void return_stmt_construct(ReturnStmt* stmt, Expr* expr) {
  statement_construct(&stmt->base, &ReturnStmtVtable);
  stmt->expr = expr;
}

void return_stmt_destroy(Statement* stmt) {
  ReturnStmt* ret_stmt = (ReturnStmt*)stmt;
  if (ret_stmt->expr) {
    expr_destroy(ret_stmt->expr);
    free(ret_stmt->expr);
  }
}

static void expr_stmt_destroy(Statement*);

static const StatementVtable ExprStmtVtable = {
    .kind = SK_ExprStmt,
    .dtor = expr_stmt_destroy,
};

void expr_stmt_construct(ExprStmt* stmt, Expr* expr) {
  statement_construct(&stmt->base, &ExprStmtVtable);
  stmt->expr = expr;
}

void expr_stmt_destroy(Statement* stmt) {
  ExprStmt* expr_stmt = (ExprStmt*)stmt;
  expr_destroy(expr_stmt->expr);
  free(expr_stmt->expr);
}

static void stmt_expr_destroy(Expr* expr);

static const ExprVtable StmtExprVtable = {
    .kind = EK_StmtExpr,
    .dtor = stmt_expr_destroy,
};

void stmt_expr_construct(StmtExpr* se, CompoundStmt* stmt) {
  expr_construct(&se->expr, &StmtExprVtable);
  se->stmt = stmt;
}

void stmt_expr_destroy(Expr* expr) {
  StmtExpr* se = (StmtExpr*)expr;
  statement_destroy(&se->stmt->base);
  free(se->stmt);
}
