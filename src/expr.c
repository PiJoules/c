#include "expr.h"

#include <assert.h>
#include <stdlib.h>

#include "source-location.h"
#include "stmt.h"

void expr_construct(Expr* expr, const ExprVtable* vtable,
                    const SourceLocation* loc) {
  expr->vtable = vtable;
  expr->loc = *loc;
}

void expr_destroy(Expr* expr) { expr->vtable->dtor(expr); }

static void cast_destroy(Expr* expr);

static const ExprVtable CastVtable = {
    .kind = EK_Cast,
    .dtor = cast_destroy,
};

void cast_construct(Cast* cast, Expr* base, Type* to,
                    const SourceLocation* loc) {
  expr_construct(&cast->expr, &CastVtable, loc);
  cast->base = base;
  cast->to = to;
}

void cast_destroy(Expr* expr) {
  Cast* cast = (Cast*)expr;
  expr_destroy(cast->base);
  free(cast->base);
  type_destroy(cast->to);
  free(cast->to);
}

static void sizeof_destroy(Expr* expr);

static const ExprVtable SizeOfVtable = {
    .kind = EK_SizeOf,
    .dtor = sizeof_destroy,
};

void sizeof_construct(SizeOf* so, void* expr_or_type, bool is_expr,
                      const SourceLocation* loc) {
  expr_construct(&so->expr, &SizeOfVtable, loc);
  so->expr_or_type = expr_or_type;
  so->is_expr = is_expr;
}

void sizeof_destroy(Expr* expr) {
  SizeOf* so = (SizeOf*)expr;
  if (so->is_expr) {
    expr_destroy(so->expr_or_type);
  } else {
    type_destroy(so->expr_or_type);
  }
  free(so->expr_or_type);
}

static void alignof_destroy(Expr* expr);

static const ExprVtable AlignOfVtable = {
    .kind = EK_AlignOf,
    .dtor = alignof_destroy,
};

void alignof_construct(AlignOf* ao, void* expr_or_type, bool is_expr,
                       const SourceLocation* loc) {
  expr_construct(&ao->expr, &AlignOfVtable, loc);
  ao->expr_or_type = expr_or_type;
  ao->is_expr = is_expr;
}

void alignof_destroy(Expr* expr) {
  AlignOf* ao = (AlignOf*)expr;
  if (ao->is_expr) {
    expr_destroy(ao->expr_or_type);
  } else {
    type_destroy(ao->expr_or_type);
  }
  free(ao->expr_or_type);
}

static void unop_destroy(Expr* expr);

static const ExprVtable UnOpVtable = {
    .kind = EK_UnOp,
    .dtor = unop_destroy,
};

void unop_construct(UnOp* unop, Expr* subexpr, UnOpKind op,
                    const SourceLocation* loc) {
  expr_construct(&unop->expr, &UnOpVtable, loc);
  unop->subexpr = subexpr;
  unop->op = op;
}

void unop_destroy(Expr* expr) {
  UnOp* unop = (UnOp*)expr;
  expr_destroy(unop->subexpr);
  free(unop->subexpr);
}

static void binop_destroy(Expr* expr);

static const ExprVtable BinOpVtable = {
    .kind = EK_BinOp,
    .dtor = binop_destroy,
};

void binop_construct(BinOp* binop, Expr* lhs, Expr* rhs, BinOpKind op,
                     const SourceLocation* loc) {
  expr_construct(&binop->expr, &BinOpVtable, loc);
  binop->lhs = lhs;
  binop->rhs = rhs;
  binop->op = op;
}

void binop_destroy(Expr* expr) {
  BinOp* binop = (BinOp*)expr;
  expr_destroy(binop->lhs);
  free(binop->lhs);
  expr_destroy(binop->rhs);
  free(binop->rhs);
}

static void conditional_destroy(Expr* expr);

static const ExprVtable ConditionalVtable = {
    .kind = EK_Conditional,
    .dtor = conditional_destroy,
};

void conditional_construct(Conditional* c, Expr* cond, Expr* true_expr,
                           Expr* false_expr, const SourceLocation* loc) {
  expr_construct(&c->expr, &ConditionalVtable, loc);
  c->cond = cond;
  c->true_expr = true_expr;
  c->false_expr = false_expr;
}

void conditional_destroy(Expr* expr) {
  Conditional* c = (Conditional*)expr;
  expr_destroy(c->cond);
  free(c->cond);
  expr_destroy(c->true_expr);
  free(c->true_expr);
  expr_destroy(c->false_expr);
  free(c->false_expr);
}

static void declref_destroy(Expr* expr);

static const ExprVtable DeclRefVtable = {
    .kind = EK_DeclRef,
    .dtor = declref_destroy,
};

void declref_construct(DeclRef* ref, const char* name,
                       const SourceLocation* loc) {
  expr_construct(&ref->expr, &DeclRefVtable, loc);
  size_t len = strlen(name);
  ref->name = malloc(sizeof(char) * (len + 1));
  memcpy(ref->name, name, len);
  ref->name[len] = 0;
}

void declref_destroy(Expr* expr) {
  DeclRef* ref = (DeclRef*)expr;
  free(ref->name);
}

static void bool_destroy(Expr*) {}

static const ExprVtable BoolVtable = {
    .kind = EK_Bool,
    .dtor = bool_destroy,
};

void bool_construct(Bool* b, bool val, const SourceLocation* loc) {
  expr_construct(&b->expr, &BoolVtable, loc);
  b->val = val;
}

static void char_destroy(Expr*) {}

static const ExprVtable CharVtable = {
    .kind = EK_Char,
    .dtor = char_destroy,
};

void char_construct(Char* c, char val, const SourceLocation* loc) {
  expr_construct(&c->expr, &CharVtable, loc);
  c->val = val;
}

static void pretty_function_destroy(Expr* expr) {}

static const ExprVtable PrettyFunctionVtable = {
    .kind = EK_PrettyFunction,
    .dtor = pretty_function_destroy,
};

void pretty_function_construct(PrettyFunction* pf, const SourceLocation* loc) {
  expr_construct(&pf->expr, &PrettyFunctionVtable, loc);
}

static void int_destroy(Expr* expr);

static const ExprVtable IntVtable = {
    .kind = EK_Int,
    .dtor = int_destroy,
};

void int_construct(Int* i, uint64_t val, BuiltinTypeKind kind,
                   const SourceLocation* loc) {
  expr_construct(&i->expr, &IntVtable, loc);
  i->val = val;
  builtin_type_construct(&i->type, kind);
}

void int_destroy(Expr* expr) {
  Int* i = (Int*)expr;
  type_destroy(&i->type.type);
}

static void string_literal_destroy(Expr* expr);

static const ExprVtable StringLiteralVtable = {
    .kind = EK_String,
    .dtor = string_literal_destroy,
};

void string_literal_construct(StringLiteral* s, const char* val, size_t len,
                              const SourceLocation* loc) {
  expr_construct(&s->expr, &StringLiteralVtable, loc);
  s->val = malloc(sizeof(char) * (len + 1));
  memcpy(s->val, val, len);
  s->val[len] = 0;
}

void string_literal_destroy(Expr* expr) {
  StringLiteral* s = (StringLiteral*)expr;
  free(s->val);
}

static void initializer_list_destroy(Expr* expr);

static const ExprVtable InitializerListVtable = {
    .kind = EK_InitializerList,
    .dtor = initializer_list_destroy,
};

void initializer_list_construct(InitializerList* init, vector elems,
                                const SourceLocation* loc) {
  expr_construct(&init->expr, &InitializerListVtable, loc);
  init->elems = elems;
}

void initializer_list_destroy(Expr* expr) {
  InitializerList* init = (InitializerList*)expr;
  for (size_t i = 0; i < init->elems.size; ++i) {
    InitializerListElem* elem = vector_at(&init->elems, i);
    if (elem->name)
      free(elem->name);
    expr_destroy(elem->expr);
    free(elem->expr);
  }
  vector_destroy(&init->elems);
}

static void index_destroy(Expr* expr);

static const ExprVtable IndexVtable = {
    .kind = EK_Index,
    .dtor = index_destroy,
};

void index_construct(Index* index_expr, Expr* base, Expr* idx,
                     const SourceLocation* loc) {
  expr_construct(&index_expr->expr, &IndexVtable, loc);
  index_expr->base = base;
  index_expr->idx = idx;
}

void index_destroy(Expr* expr) {
  Index* index_expr = (Index*)expr;
  expr_destroy(index_expr->base);
  free(index_expr->base);
  expr_destroy(index_expr->idx);
  free(index_expr->idx);
}

static void call_destroy(Expr* expr);

static const ExprVtable CallVtable = {
    .kind = EK_Call,
    .dtor = call_destroy,
};

void call_construct(Call* call, Expr* base, vector v,
                    const SourceLocation* loc) {
  expr_construct(&call->expr, &CallVtable, loc);
  call->base = base;
  call->args = v;
}

void call_destroy(Expr* expr) {
  Call* call = (Call*)expr;
  expr_destroy(call->base);
  free(call->base);

  for (size_t i = 0; i < call->args.size; ++i) {
    Expr** expr = vector_at(&call->args, i);
    expr_destroy(*expr);
    free(*expr);
  }
  vector_destroy(&call->args);
}

static void member_access_destroy(Expr* expr);

static const ExprVtable MemberAccessVtable = {
    .kind = EK_MemberAccess,
    .dtor = member_access_destroy,
};

void member_access_construct(MemberAccess* member_access, Expr* base,
                             const char* member, bool is_arrow,
                             const SourceLocation* loc) {
  expr_construct(&member_access->expr, &MemberAccessVtable, loc);
  member_access->base = base;
  member_access->member = strdup(member);
  member_access->is_arrow = is_arrow;
}

void member_access_destroy(Expr* expr) {
  MemberAccess* member_access = (MemberAccess*)expr;
  expr_destroy(member_access->base);
  free(member_access->base);
  free(member_access->member);
}

static void function_param_destroy(Expr*) {}

static const ExprVtable FunctionParamVtable = {
    .kind = EK_FunctionParam,
    .dtor = function_param_destroy,
};

void function_param_construct(FunctionParam* param, const char* name,
                              const Type* type, const SourceLocation* loc) {
  expr_construct(&param->expr, &FunctionParamVtable, loc);
  param->name = name;
  param->type = type;
}

static void stmt_expr_destroy(Expr* expr);

static const ExprVtable StmtExprVtable = {
    .kind = EK_StmtExpr,
    .dtor = stmt_expr_destroy,
};

void stmt_expr_construct(StmtExpr* se, CompoundStmt* stmt,
                         const SourceLocation* loc) {
  expr_construct(&se->expr, &StmtExprVtable, loc);
  se->stmt = stmt;
}

void stmt_expr_destroy(Expr* expr) {
  StmtExpr* se = (StmtExpr*)expr;
  statement_destroy(&se->stmt->base);
  free(se->stmt);
}
