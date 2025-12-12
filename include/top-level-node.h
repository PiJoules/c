#ifndef TOP_LEVEL_NODE_H_
#define TOP_LEVEL_NODE_H_

#include "expr.h"
#include "source-location.h"
#include "stmt.h"
#include "type.h"
#include "vector.h"

typedef enum {
  TLNK_Typedef,
  TLNK_StaticAssert,
  TLNK_GlobalVariable,  // Also accounts for function decls, but not function
                        // defs
  TLNK_FunctionDefinition,
  TLNK_StructDeclaration,
  TLNK_EnumDeclaration,
  TLNK_UnionDeclaration,
} TopLevelNodeKind;

struct TopLevelNode;

// When creating "Node subclasses" via the parse_* methods, we can safely cast
// pointers to these subobjects as Node pointers. That is, we can do something
// like:
//
//   struct ChildNode {
//     Node node;
//     ...  // Other fields
//   };
//
//   void child_dtor(Node *node) {
//     Child *child = (Child *)node;
//     ...  // Other stuff during destruction
//   }
//
//   Node *parse_child_node() {
//     ChildNode *child = malloc(sizeof(ChildNode));
//     child->node.dtor = child_dtor;
//     ...  // Initalize some members od child->node and child
//     return &child->node;
//   }
//
//   Node *node = parse_child_node();
//   node_destroy(node);  // Invoke child_dtor via node_destroy
//
// Here the Child* in child_dtor can be safely accessed via the Node* from
// &child->node in parse_child_node because a pointer to a structure also
// points to ints initial member and vice versa. See
// https://stackoverflow.com/a/19011044/2775471. This does not break type
// aliasing rules.
//

struct TopLevelNode;

typedef struct {
  TopLevelNodeKind kind;
  void (*dtor)(struct TopLevelNode*);
} TopLevelNodeVtable;

struct TopLevelNode {
  const TopLevelNodeVtable* vtable;
  SourceLocation loc;
};
typedef struct TopLevelNode TopLevelNode;

void top_level_node_construct(TopLevelNode* node,
                              const TopLevelNodeVtable* vtable,
                              const SourceLocation* loc);
void top_level_node_destroy(TopLevelNode* node);

typedef struct {
  TopLevelNode node;
  Type* type;
  char* name;
} Typedef;

void typedef_construct(Typedef* td, const SourceLocation* loc);

typedef struct {
  TopLevelNode node;
  Expr* expr;
} StaticAssert;

void static_assert_construct(StaticAssert* sa, Expr* expr,
                             const SourceLocation* loc);

typedef struct {
  TopLevelNode node;
  char* name;
  Type* type;
  Expr* initializer;  // Optional.

  bool is_extern;  // false implies `static`.
  bool is_thread_local;
} GlobalVariable;

void global_variable_construct(GlobalVariable* gv, const char* name, Type* type,
                               const SourceLocation* loc);

typedef struct {
  TopLevelNode node;
  StructType* type;
} StructDeclaration;

void struct_declaration_construct_from_type(StructDeclaration* decl,
                                            StructType* type,
                                            const SourceLocation* loc);
void struct_declaration_construct(StructDeclaration* decl, char* name,
                                  vector* members, const SourceLocation* loc);

typedef struct {
  TopLevelNode node;
  EnumType* type;
} EnumDeclaration;

void enum_declaration_construct_from_type(EnumDeclaration* decl, EnumType* type,
                                          const SourceLocation* loc);
void enum_declaration_construct(EnumDeclaration* decl, char* name,
                                vector* members, const SourceLocation* loc);

typedef struct {
  TopLevelNode node;
  UnionType* type;
} UnionDeclaration;

void union_declaration_construct_from_type(UnionDeclaration* decl,
                                           UnionType* type,
                                           const SourceLocation* loc);
void union_declaration_construct(UnionDeclaration* decl, char* name,
                                 vector* members, const SourceLocation* loc);

typedef struct {
  TopLevelNode node;
  char* name;
  Type* type;
  CompoundStmt* body;
  bool is_extern;  // false implies this is `static`.
} FunctionDefinition;

void function_definition_construct(FunctionDefinition* f, const char* name,
                                   Type* type, CompoundStmt* body,
                                   const SourceLocation* loc);

#endif  // TOP_LEVEL_NODE_H_
