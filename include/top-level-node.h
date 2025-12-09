#ifndef TOP_LEVEL_NODE_H_
#define TOP_LEVEL_NODE_H_

#include "expr.h"
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
};
typedef struct TopLevelNode TopLevelNode;

void top_level_node_construct(TopLevelNode* node,
                              const TopLevelNodeVtable* vtable);
void top_level_node_destroy(TopLevelNode* node);

typedef struct {
  TopLevelNode node;
  Type* type;
  char* name;
} Typedef;

void typedef_construct(Typedef* td);

typedef struct {
  TopLevelNode node;
  Expr* expr;
} StaticAssert;

void static_assert_construct(StaticAssert* sa, Expr* expr);

typedef struct {
  TopLevelNode node;
  char* name;
  Type* type;
  Expr* initializer;  // Optional.

  bool is_extern;  // false implies `static`.
  bool is_thread_local;
} GlobalVariable;

void global_variable_construct(GlobalVariable* gv, const char* name,
                               Type* type);

typedef struct {
  TopLevelNode node;
  StructType* type;
} StructDeclaration;

void struct_declaration_construct_from_type(StructDeclaration* decl,
                                            StructType* type);
void struct_declaration_construct(StructDeclaration* decl, char* name,
                                  vector* members);

typedef struct {
  TopLevelNode node;
  EnumType* type;
} EnumDeclaration;

void enum_declaration_construct_from_type(EnumDeclaration* decl,
                                          EnumType* type);
void enum_declaration_construct(EnumDeclaration* decl, char* name,
                                vector* members);

typedef struct {
  TopLevelNode node;
  UnionType* type;
} UnionDeclaration;

void union_declaration_construct_from_type(UnionDeclaration* decl,
                                           UnionType* type);
void union_declaration_construct(UnionDeclaration* decl, char* name,
                                 vector* members);

typedef struct {
  TopLevelNode node;
  char* name;
  Type* type;
  CompoundStmt* body;
  bool is_extern;  // false implies this is `static`.
} FunctionDefinition;

void function_definition_construct(FunctionDefinition* f, const char* name,
                                   Type* type, CompoundStmt* body);

typedef struct {
  Expr expr;
  void* expr_or_type;
  bool is_expr;
} SizeOf;

void sizeof_construct(SizeOf* so, void* expr_or_type, bool is_expr);

typedef struct {
  Expr expr;
  void* expr_or_type;
  bool is_expr;
} AlignOf;

void alignof_construct(AlignOf* ao, void* expr_or_type, bool is_expr);

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

void unop_construct(UnOp* unop, Expr* subexpr, UnOpKind op);

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

void binop_construct(BinOp* binop, Expr* lhs, Expr* rhs, BinOpKind op);

typedef struct {
  Expr expr;
  Expr* cond;
  Expr* true_expr;
  Expr* false_expr;
} Conditional;

void conditional_construct(Conditional* c, Expr* cond, Expr* true_expr,
                           Expr* false_expr);

typedef struct {
  Expr expr;
  char* name;
} DeclRef;

void declref_construct(DeclRef* ref, const char* name);

typedef struct {
  Expr expr;
  bool val;
} Bool;

void bool_construct(Bool* b, bool val);

typedef struct {
  Expr expr;
  char val;
} Char;

void char_construct(Char* c, char val);

typedef struct {
  Expr expr;
} PrettyFunction;

void pretty_function_construct(PrettyFunction* pf);

typedef struct {
  Expr expr;
  uint64_t val;
  BuiltinType type;
} Int;

void int_construct(Int* i, uint64_t val, BuiltinTypeKind kind);

typedef struct {
  Expr expr;
  char* val;
} StringLiteral;

void string_literal_construct(StringLiteral* s, const char* val, size_t len);

typedef struct {
  char* name;  // Optional.
  Expr* expr;
} InitializerListElem;

typedef struct {
  Expr expr;

  // This is a vector of InitializerListElems.
  vector elems;
} InitializerList;

void initializer_list_construct(InitializerList* init, vector elems);

typedef struct {
  Expr expr;
  Expr* base;
  Expr* idx;
} Index;

void index_construct(Index* index_expr, Expr* base, Expr* idx);

typedef struct {
  Expr expr;
  Expr* base;
  vector args;  // Vector of Expr*
} Call;

void call_construct(Call* call, Expr* base, vector v);

typedef struct {
  Expr expr;
  Expr* base;
  char* member;
  bool is_arrow;
} MemberAccess;

void member_access_construct(MemberAccess* member_access, Expr* base,
                             const char* member, bool is_arrow);

typedef struct {
  Expr expr;
  // The caller should not destroy or free these.
  const char* name;
  const Type* type;
} FunctionParam;

void function_param_construct(FunctionParam* param, const char* name,
                              const Type* type);

#endif  // TOP_LEVEL_NODE_H_
