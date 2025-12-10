#ifndef SEMA_H_
#define SEMA_H_

#include <stdint.h>

#include "expr.h"
#include "top-level-node.h"
#include "tree-map.h"
#include "type.h"
#include "vector.h"

///
/// This performs semantic analysis and makes any needed adjustments to the AST
/// before passing it to the compiler.
///

typedef struct {
  TreeMap typedef_types;
  TreeMap struct_types;
  TreeMap union_types;
  TreeMap enum_types;

  // This is a map of all globals seen in this TU. If any of them are
  // definitions, those globals will be stored here over the ones without
  // definitions.
  //
  // The values are either GlobalVariable or FunctionDefinition pointers.
  TreeMap globals;

  // This is a map of all enum names values declared in this TU to their values.
  //
  // The values are ints.
  TreeMap enum_values;

  // This is a map of all enum names to pointers to their corresponding
  // EnumTypes.
  //
  // The keys here must be kept in sync with the kets of enum_values.
  TreeMap enum_names;

  BuiltinType bt_Char;
  BuiltinType bt_SignedChar;
  BuiltinType bt_UnsignedChar;
  BuiltinType bt_Short;
  BuiltinType bt_UnsignedShort;
  BuiltinType bt_Int;
  BuiltinType bt_UnsignedInt;
  BuiltinType bt_Long;
  BuiltinType bt_UnsignedLong;
  BuiltinType bt_LongLong;
  BuiltinType bt_UnsignedLongLong;
  BuiltinType bt_Float;
  BuiltinType bt_Double;
  BuiltinType bt_LongDouble;
  BuiltinType bt_Void;
  BuiltinType bt_Bool;

  PointerType str_ty;

  GlobalVariable builtin_trap;

  // This is a vector of all types that Sema needs to create on the fly via
  // the address of operator. This is a vector of pointers to
  // NonOwningPointerTypes.
  vector address_of_storage;
} Sema;

void sema_construct(Sema* sema);
void sema_destroy(Sema* sema);

void sema_handle_global_variable(Sema* sema, GlobalVariable* gv);
void sema_handle_function_definition(Sema* sema, FunctionDefinition* f);
void sema_handle_struct_declaration(Sema* sema, StructDeclaration* decl);
void sema_handle_union_declaration(Sema* sema, UnionDeclaration* decl);
void sema_handle_enum_declaration(Sema* sema, EnumDeclaration* decl);

void sema_add_typedef_type(Sema* sema, const char* name, Type* type);

const BuiltinType* sema_get_integral_type_for_enum(const Sema* sema,
                                                   const EnumType*);
const ArrayType* sema_get_array_type(const Sema* sema, const Type* type,
                                     const TreeMap* local_ctx);
const Type* sema_get_pointee(const Sema* sema, const Type* type,
                             const TreeMap* local_ctx);
const StructType* sema_get_struct_type(const Sema* sema, const Type* type,
                                       const TreeMap* local_ctx);
const Type* sema_get_type_of_expr_in_ctx(Sema* sema, const Expr* expr,
                                         const TreeMap* local_ctx);
const Type* sema_get_type_of_unop_expr(Sema* sema, const UnOp* expr,
                                       const TreeMap* local_ctx);

const Type* sema_resolve_named_type_from_name(const Sema* sema,
                                              const char* name);
const Type* sema_resolve_named_type(const Sema* sema, const NamedType* nt);
const Type* sema_resolve_maybe_named_type(const Sema* sema, const Type* type);
const Type* sema_resolve_maybe_struct_type(const Sema* sema, const Type* type);
const StructType* sema_resolve_struct_type(const Sema* sema,
                                           const StructType* st);
const UnionType* sema_resolve_union_type(const Sema* sema, const UnionType* ut);

const FunctionType* sema_get_function(Sema* sema, const Type* ty,
                                      const TreeMap* local_ctx);
const Member* sema_get_struct_member(const Sema* sema, const StructType* st,
                                     const char* name, size_t* offset);
const Type* sema_get_corresponding_unsigned_type(const Sema* sema,
                                                 const BuiltinType* bt);

// Find the common type for usual arithmetic conversions.
const Type* sema_get_common_arithmetic_type(Sema* sema, const Type* lhs_ty,
                                            const Type* rhs_ty,
                                            const TreeMap* local_ctx);

const Type* sema_get_common_arithmetic_type_of_exprs(Sema* sema,
                                                     const Expr* lhs,
                                                     const Expr* rhs,
                                                     const TreeMap* local_ctx);
const Type* sema_get_result_type_of_binop_expr(Sema* sema, const BinOp* expr,
                                               const TreeMap* local_ctx);
const StructType* sema_get_struct_type_from_member_access(
    Sema* sema, const MemberAccess* access, const TreeMap* local_ctx);
const Member* sema_get_largest_union_member(Sema* sema, const UnionType* type);

// Infer the type of this expression. The caller of this is not in charge of
// destroying the type.
const Type* sema_get_type_of_expr_in_ctx(Sema* sema, const Expr* expr,
                                         const TreeMap* local_ctx);

// Infer the type of this expression in the global context. The caller of this
// is not in charge of destroying the type.
const Type* sema_get_type_of_expr(Sema* sema, const Expr* expr);

bool sema_is_enum_type(const Sema* sema, const Type* type,
                       const TreeMap* local_ctx);
bool sema_is_pointer_type(const Sema* sema, const Type* type,
                          const TreeMap* local_ctx);
bool sema_is_array_type(const Sema* sema, const Type* type,
                        const TreeMap* local_ctx);
bool sema_is_struct_type(const Sema* sema, const Type* type,
                         const TreeMap* local_ctx);
bool sema_is_pointer_to(const Sema* sema, const Type* type, TypeKind kind,
                        const TreeMap* local_ctx);
bool sema_is_unsigned_integral_type(const Sema* sema, const Type* type);
bool sema_is_function_or_function_ptr(Sema* sema, const Type* ty,
                                      const TreeMap* local_ctx);

size_t sema_eval_sizeof_type(Sema* sema, const Type* type);
size_t sema_eval_alignof_type(Sema* sema, const Type* type);
size_t sema_eval_sizeof_array(Sema*, const ArrayType*);
size_t sema_eval_alignof_array(Sema*, const ArrayType*);
size_t sema_eval_alignof_members(Sema* sema, const vector* members);
size_t sema_eval_sizeof_struct_type(Sema* sema, const StructType* type);
size_t sema_eval_sizeof_union_type(Sema* sema, const UnionType* type);

bool sema_struct_or_union_components_are_compatible(
    Sema* sema, const char* lhs_name, const vector* lhs_members,
    const char* rhs_name, const vector* rhs_members, bool ignore_quals);
bool sema_types_are_compatible(Sema* sema, const Type* lhs, const Type* rhs);
bool sema_types_are_compatible_ignore_quals(Sema* sema, const Type* lhs,
                                            const Type* rhs);

void sema_verify_static_assert_condition(Sema* sema, const Expr* cond);

typedef enum {
  // TODO: Add the other expression result kinds.
  RK_Boolean,
  RK_Int,
  RK_UnsignedLongLong,
} ConstExprResultKind;

typedef struct {
  ConstExprResultKind result_kind;

  // TODO: This should be a union.
  struct {
    bool b;
    int i;
    unsigned long long ull;
  } result;
} ConstExprResult;

ConstExprResult sema_eval_expr(Sema*, const Expr*);
ConstExprResult sema_eval_binop(Sema* sema, const BinOp* expr);
ConstExprResult sema_eval_alignof(Sema* sema, const AlignOf* ao);
ConstExprResult sema_eval_sizeof(Sema* sema, const SizeOf* so);

int compare_constexpr_result_types(const ConstExprResult*,
                                   const ConstExprResult*);
uint64_t result_to_u64(const ConstExprResult* res);

// TODO: ATM these are the HOST values, not the target values.
size_t builtin_type_get_size(const BuiltinType* bt);

#endif  // SEMA_H_
