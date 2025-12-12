#include "top-level-node.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "expr.h"
#include "type.h"
#include "vector.h"

void top_level_node_construct(TopLevelNode* node,
                              const TopLevelNodeVtable* vtable,
                              const SourceLocation* loc) {
  node->vtable = vtable;
  node->loc = *loc;
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

void typedef_construct(Typedef* td, const SourceLocation* loc) {
  top_level_node_construct(&td->node, &TypedefVtable, loc);
  td->type = NULL;
  td->name = NULL;
}

static void static_assert_destroy(TopLevelNode*);

static const TopLevelNodeVtable StaticAssertVtable = {
    .kind = TLNK_StaticAssert,
    .dtor = static_assert_destroy,
};

void static_assert_construct(StaticAssert* sa, Expr* expr,
                             const SourceLocation* loc) {
  top_level_node_construct(&sa->node, &StaticAssertVtable, loc);
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

void global_variable_construct(GlobalVariable* gv, const char* name, Type* type,
                               const SourceLocation* loc) {
  top_level_node_construct(&gv->node, &GlobalVariableVtable, loc);
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
                                            StructType* type,
                                            const SourceLocation* loc) {
  top_level_node_construct(&decl->node, &StructDeclarationVtable, loc);
  decl->type = type;
}

void struct_declaration_construct(StructDeclaration* decl, char* name,
                                  vector* members, const SourceLocation* loc) {
  top_level_node_construct(&decl->node, &StructDeclarationVtable, loc);
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

void enum_declaration_construct_from_type(EnumDeclaration* decl, EnumType* type,
                                          const SourceLocation* loc) {
  top_level_node_construct(&decl->node, &EnumDeclarationVtable, loc);
  decl->type = type;
}

void enum_declaration_construct(EnumDeclaration* decl, char* name,
                                vector* members, const SourceLocation* loc) {
  top_level_node_construct(&decl->node, &EnumDeclarationVtable, loc);
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
                                           UnionType* type,
                                           const SourceLocation* loc) {
  top_level_node_construct(&decl->node, &UnionDeclarationVtable, loc);
  decl->type = type;
}

void union_declaration_construct(UnionDeclaration* decl, char* name,
                                 vector* members, const SourceLocation* loc) {
  top_level_node_construct(&decl->node, &UnionDeclarationVtable, loc);
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
                                   Type* type, CompoundStmt* body,
                                   const SourceLocation* loc) {
  top_level_node_construct(&f->node, &FunctionDefinitionVtable, loc);
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
