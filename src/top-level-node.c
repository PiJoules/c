#include "top-level-node.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "expr.h"
#include "type.h"
#include "vector.h"

void top_level_node_construct(TopLevelNode* node,
                              const TopLevelNodeVtable* vtable) {
  node->vtable = vtable;
}

void top_level_node_destroy(TopLevelNode* node) { node->vtable->dtor(node); }

static void typedef_destroy(TopLevelNode*);

static const TopLevelNodeVtable TypedefVtable = {
    .kind = TLNK_Typedef,
    .dtor = typedef_destroy,
};

void typedef_destroy(TopLevelNode* node) {
  Typedef* td = (Typedef*)node;

  if (td->type) {
    type_destroy(td->type);
    free(td->type);
  }

  if (td->name)
    free(td->name);
}

void typedef_construct(Typedef* td) {
  top_level_node_construct(&td->node, &TypedefVtable);
  td->type = NULL;
  td->name = NULL;
}

static void static_assert_destroy(TopLevelNode*);

static const TopLevelNodeVtable StaticAssertVtable = {
    .kind = TLNK_StaticAssert,
    .dtor = static_assert_destroy,
};

void static_assert_construct(StaticAssert* sa, Expr* expr) {
  top_level_node_construct(&sa->node, &StaticAssertVtable);
  sa->expr = expr;
}

void static_assert_destroy(TopLevelNode* node) {
  StaticAssert* sa = (StaticAssert*)node;
  sa->expr->vtable->dtor(sa->expr);
  free(sa->expr);
}

static void global_variable_destroy(TopLevelNode*);

static const TopLevelNodeVtable GlobalVariableVtable = {
    .kind = TLNK_GlobalVariable,
    .dtor = global_variable_destroy,
};

void global_variable_construct(GlobalVariable* gv, const char* name,
                               Type* type) {
  top_level_node_construct(&gv->node, &GlobalVariableVtable);
  gv->name = strdup(name);
  gv->type = type;
  gv->initializer = NULL;
  gv->is_extern = true;
  gv->is_thread_local = false;
}

void global_variable_destroy(TopLevelNode* node) {
  GlobalVariable* gv = (GlobalVariable*)node;
  free(gv->name);
  type_destroy(gv->type);
  free(gv->type);
  if (gv->initializer) {
    expr_destroy(gv->initializer);
    free(gv->initializer);
  }
}

static void struct_declaration_destroy(TopLevelNode*);

static const TopLevelNodeVtable StructDeclarationVtable = {
    .kind = TLNK_StructDeclaration,
    .dtor = struct_declaration_destroy,
};

void struct_declaration_construct_from_type(StructDeclaration* decl,
                                            StructType* type) {
  top_level_node_construct(&decl->node, &StructDeclarationVtable);
  decl->type = type;
}

void struct_declaration_construct(StructDeclaration* decl, char* name,
                                  vector* members) {
  top_level_node_construct(&decl->node, &StructDeclarationVtable);
  decl->type = malloc(sizeof(StructType));
  struct_type_construct(decl->type, name, members);
}

void struct_declaration_destroy(TopLevelNode* node) {
  StructDeclaration* decl = (StructDeclaration*)node;
  type_destroy(&decl->type->type);
  free(decl->type);
}

static void enum_declaration_destroy(TopLevelNode*);

static const TopLevelNodeVtable EnumDeclarationVtable = {
    .kind = TLNK_EnumDeclaration,
    .dtor = enum_declaration_destroy,
};

void enum_declaration_construct_from_type(EnumDeclaration* decl,
                                          EnumType* type) {
  top_level_node_construct(&decl->node, &EnumDeclarationVtable);
  decl->type = type;
}

void enum_declaration_construct(EnumDeclaration* decl, char* name,
                                vector* members) {
  top_level_node_construct(&decl->node, &EnumDeclarationVtable);
  decl->type = malloc(sizeof(EnumType));
  enum_type_construct(decl->type, name, members);
}

void enum_declaration_destroy(TopLevelNode* node) {
  EnumDeclaration* decl = (EnumDeclaration*)node;
  type_destroy(&decl->type->type);
  free(decl->type);
}

static void union_declaration_destroy(TopLevelNode*);

static const TopLevelNodeVtable UnionDeclarationVtable = {
    .kind = TLNK_UnionDeclaration,
    .dtor = union_declaration_destroy,
};

void union_declaration_construct_from_type(UnionDeclaration* decl,
                                           UnionType* type) {
  top_level_node_construct(&decl->node, &UnionDeclarationVtable);
  decl->type = type;
}

void union_declaration_construct(UnionDeclaration* decl, char* name,
                                 vector* members) {
  top_level_node_construct(&decl->node, &UnionDeclarationVtable);
  decl->type = malloc(sizeof(UnionType));
  union_type_construct(decl->type, name, members);
}

void union_declaration_destroy(TopLevelNode* node) {
  UnionDeclaration* decl = (UnionDeclaration*)node;
  type_destroy(&decl->type->type);
  free(decl->type);
}

static void function_definition_destroy(TopLevelNode*);

static const TopLevelNodeVtable FunctionDefinitionVtable = {
    .kind = TLNK_FunctionDefinition,
    .dtor = function_definition_destroy,
};

void function_definition_construct(FunctionDefinition* f, const char* name,
                                   Type* type, CompoundStmt* body) {
  top_level_node_construct(&f->node, &FunctionDefinitionVtable);
  f->name = strdup(name);
  assert(type->vtable->kind == TK_FunctionType);
  f->type = type;
  f->body = body;
  f->is_extern = true;
}

void function_definition_destroy(TopLevelNode* node) {
  FunctionDefinition* f = (FunctionDefinition*)node;
  free(f->name);
  type_destroy(f->type);
  free(f->type);

  statement_destroy(&f->body->base);
  free(f->body);
}

static void sizeof_destroy(Expr* expr);

static const ExprVtable SizeOfVtable = {
    .kind = EK_SizeOf,
    .dtor = sizeof_destroy,
};

void sizeof_construct(SizeOf* so, void* expr_or_type, bool is_expr) {
  expr_construct(&so->expr, &SizeOfVtable);
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

void alignof_construct(AlignOf* ao, void* expr_or_type, bool is_expr) {
  expr_construct(&ao->expr, &AlignOfVtable);
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

void unop_construct(UnOp* unop, Expr* subexpr, UnOpKind op) {
  expr_construct(&unop->expr, &UnOpVtable);
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

void binop_construct(BinOp* binop, Expr* lhs, Expr* rhs, BinOpKind op) {
  expr_construct(&binop->expr, &BinOpVtable);
  binop->lhs = lhs;
  binop->rhs = rhs;
  binop->op = op;

  assert(lhs->line != 0 && lhs->col != 0);
  assert(rhs->line != 0 && rhs->col != 0);
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
                           Expr* false_expr) {
  expr_construct(&c->expr, &ConditionalVtable);
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

void declref_construct(DeclRef* ref, const char* name) {
  expr_construct(&ref->expr, &DeclRefVtable);
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

void bool_construct(Bool* b, bool val) {
  expr_construct(&b->expr, &BoolVtable);
  b->val = val;
}

static void char_destroy(Expr*) {}

static const ExprVtable CharVtable = {
    .kind = EK_Char,
    .dtor = char_destroy,
};

void char_construct(Char* c, char val) {
  expr_construct(&c->expr, &CharVtable);
  c->val = val;
}

static void pretty_function_destroy(Expr* expr) {}

static const ExprVtable PrettyFunctionVtable = {
    .kind = EK_PrettyFunction,
    .dtor = pretty_function_destroy,
};

void pretty_function_construct(PrettyFunction* pf) {
  expr_construct(&pf->expr, &PrettyFunctionVtable);
}

static void int_destroy(Expr* expr);

static const ExprVtable IntVtable = {
    .kind = EK_Int,
    .dtor = int_destroy,
};

void int_construct(Int* i, uint64_t val, BuiltinTypeKind kind) {
  expr_construct(&i->expr, &IntVtable);
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

void string_literal_construct(StringLiteral* s, const char* val, size_t len) {
  expr_construct(&s->expr, &StringLiteralVtable);
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

void initializer_list_construct(InitializerList* init, vector elems) {
  expr_construct(&init->expr, &InitializerListVtable);
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

void index_construct(Index* index_expr, Expr* base, Expr* idx) {
  expr_construct(&index_expr->expr, &IndexVtable);
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

void call_construct(Call* call, Expr* base, vector v) {
  expr_construct(&call->expr, &CallVtable);
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
                             const char* member, bool is_arrow) {
  expr_construct(&member_access->expr, &MemberAccessVtable);
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
                              const Type* type) {
  expr_construct(&param->expr, &FunctionParamVtable);
  param->name = name;
  param->type = type;
}
