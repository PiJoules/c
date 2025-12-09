#include "expr.h"

#include <stdlib.h>

void expr_construct(Expr* expr, const ExprVtable* vtable) {
  expr->vtable = vtable;
  expr->line = expr->col = 0;
}

void expr_destroy(Expr* expr) { expr->vtable->dtor(expr); }

static void cast_destroy(Expr* expr);

static const ExprVtable CastVtable = {
    .kind = EK_Cast,
    .dtor = cast_destroy,
};

void cast_construct(Cast* cast, Expr* base, Type* to) {
  expr_construct(&cast->expr, &CastVtable);
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
