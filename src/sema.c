#include "sema.h"

#include <stdint.h>

#include "common.h"
#include "tree-map.h"
#include "type.h"
#include "vector.h"

void sema_construct(Sema* sema) {
  string_tree_map_construct(&sema->typedef_types);
  string_tree_map_construct(&sema->struct_types);
  string_tree_map_construct(&sema->union_types);
  string_tree_map_construct(&sema->enum_types);
  string_tree_map_construct(&sema->globals);
  string_tree_map_construct(&sema->enum_values);
  string_tree_map_construct(&sema->enum_names);

  builtin_type_construct(&sema->bt_Char, BTK_Char);
  builtin_type_construct(&sema->bt_SignedChar, BTK_SignedChar);
  builtin_type_construct(&sema->bt_UnsignedChar, BTK_UnsignedChar);
  builtin_type_construct(&sema->bt_Short, BTK_Short);
  builtin_type_construct(&sema->bt_UnsignedShort, BTK_UnsignedShort);
  builtin_type_construct(&sema->bt_Int, BTK_Int);
  builtin_type_construct(&sema->bt_UnsignedInt, BTK_UnsignedInt);
  builtin_type_construct(&sema->bt_Long, BTK_Long);
  builtin_type_construct(&sema->bt_UnsignedLong, BTK_UnsignedLong);
  builtin_type_construct(&sema->bt_LongLong, BTK_LongLong);
  builtin_type_construct(&sema->bt_UnsignedLongLong, BTK_UnsignedLongLong);
  builtin_type_construct(&sema->bt_Float, BTK_Float);
  builtin_type_construct(&sema->bt_Double, BTK_Double);
  builtin_type_construct(&sema->bt_Void, BTK_Void);
  builtin_type_construct(&sema->bt_Bool, BTK_Bool);

  {
    BuiltinType* chars = create_builtin_type(BTK_Char);
    type_set_const(&chars->type);
    pointer_type_construct(&sema->str_ty, &chars->type);
  }

  {
    BuiltinType* ret_ty = malloc(sizeof(BuiltinType));
    builtin_type_construct(ret_ty, BTK_Void);

    vector args;
    vector_construct(&args, sizeof(FunctionArg), alignof(FunctionArg));

    FunctionType* builtin_trap_type = malloc(sizeof(FunctionType));
    function_type_construct(builtin_trap_type, &ret_ty->type, args);

    global_variable_construct(&sema->builtin_trap, "__builtin_trap",
                              &builtin_trap_type->type);

    sema_handle_global_variable(sema, &sema->builtin_trap);
  }

  vector_construct(&sema->address_of_storage, sizeof(NonOwningPointerType*),
                   alignof(NonOwningPointerType*));
}

void sema_destroy(Sema* sema) {
  tree_map_destroy(&sema->typedef_types);
  tree_map_destroy(&sema->struct_types);
  tree_map_destroy(&sema->union_types);
  tree_map_destroy(&sema->enum_types);
  tree_map_destroy(&sema->globals);
  tree_map_destroy(&sema->enum_values);
  tree_map_destroy(&sema->enum_names);

  // NOTE: We don't need to destroy the builtin types since they don't have
  // any members that need cleanup.

  type_destroy(&sema->str_ty.type);

  top_level_node_destroy(&sema->builtin_trap.node);

  NonOwningPointerType** start = vector_begin(&sema->address_of_storage);
  NonOwningPointerType** end = vector_end(&sema->address_of_storage);
  for (NonOwningPointerType** it = start; it != end; ++it) free(*it);
  vector_destroy(&sema->address_of_storage);
}

const BuiltinType* sema_get_integral_type_for_enum(const Sema* sema,
                                                   const EnumType*) {
  // FIXME: This is ok for small types but will fail for sufficiently large
  // enough enums.
  return &sema->bt_Int;
}

uint64_t result_to_u64(const ConstExprResult* res) {
  switch (res->result_kind) {
    case RK_Boolean:
      return (uint64_t)res->result.b;
    case RK_Int:
      assert(res->result.i >= 0);
      return (uint64_t)res->result.i;
    case RK_UnsignedLongLong:
      return res->result.ull;
  }
}

// FIXME: This should also have a typedef context.
const Type* sema_resolve_named_type_from_name(const Sema* sema,
                                              const char* name) {
  void* found;
  ASSERT_MSG(tree_map_get(&sema->typedef_types, name, &found),
             "Unknown type '%s'", name);
  Type* res = found;
  assert(res->vtable->kind != TK_NamedType &&
         "All typedef types should be flattened to an unnamed type when being "
         "added.");
  return res;
}

const Type* sema_resolve_named_type(const Sema* sema, const NamedType* nt) {
  return sema_resolve_named_type_from_name(sema, nt->name);
}

const Type* sema_resolve_maybe_named_type(const Sema* sema, const Type* type) {
  if (type->vtable->kind == TK_NamedType)
    return sema_resolve_named_type(sema, (const NamedType*)type);
  return type;
}

const StructType* sema_resolve_struct_type(const Sema* sema,
                                           const StructType* st) {
  if (st->members)
    return st;

  void* found;
  ASSERT_MSG(tree_map_get(&sema->struct_types, st->name, &found),
             "No struct named '%s'", st->name);

  return found;
}

const UnionType* sema_resolve_union_type(const Sema* sema,
                                         const UnionType* ut) {
  if (ut->members)
    return ut;

  void* found;
  ASSERT_MSG(tree_map_get(&sema->union_types, ut->name, &found),
             "No union named '%s'", ut->name);

  return found;
}

const Member* sema_get_struct_member(const Sema* sema, const StructType* st,
                                     const char* name, size_t* offset) {
  return struct_get_member(sema_resolve_struct_type(sema, st), name, offset);
}

const Type* sema_resolve_maybe_struct_type(const Sema* sema, const Type* type) {
  if (type->vtable->kind == TK_StructType)
    return &sema_resolve_struct_type(sema, (const StructType*)type)->type;
  return type;
}

bool sema_is_enum_type(const Sema* sema, const Type* type,
                       const TreeMap* local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  return type->vtable->kind == TK_EnumType;
}

bool sema_is_pointer_type(const Sema* sema, const Type* type,
                          const TreeMap* local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  return is_pointer_type(type);
}

bool sema_is_array_type(const Sema* sema, const Type* type,
                        const TreeMap* local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  return is_array_type(type);
}

const ArrayType* sema_get_array_type(const Sema* sema, const Type* type,
                                     const TreeMap* local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  if (is_array_type(type))
    return (const ArrayType*)type;
  return NULL;
}

const Type* sema_get_pointee(const Sema* sema, const Type* type,
                             const TreeMap* local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  return get_pointee(type);
}

bool sema_is_struct_type(const Sema* sema, const Type* type,
                         const TreeMap* local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  return type->vtable->kind == TK_StructType;
}

const StructType* sema_get_struct_type(const Sema* sema, const Type* type,
                                       const TreeMap* local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  if (type->vtable->kind == TK_StructType)
    return (const StructType*)type;
  return NULL;
}

// TODO: Pass fewer args around.
bool sema_is_pointer_to(const Sema* sema, const Type* type, TypeKind kind,
                        const TreeMap* local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  if (!is_pointer_type(type))
    return false;

  const Type* pointee = get_pointee(type);
  pointee = sema_resolve_maybe_named_type(sema, pointee);
  return pointee->vtable->kind == TK_StructType;
}

static bool sema_types_are_compatible_impl(Sema* sema, const Type* lhs,
                                           const Type* rhs, bool ignore_quals);

bool sema_struct_or_union_components_are_compatible(
    Sema* sema, const char* lhs_name, const vector* lhs_members,
    const char* rhs_name, const vector* rhs_members, bool ignore_quals) {
  // If one is declared with a tag, the other must also be declared with the
  // same tag.
  if ((lhs_name && !rhs_name) || (!lhs_name && rhs_name))
    return false;

  // This should be enough since we wouldn't be able to define different
  // structs/unions with the same name.
  if (lhs_name && rhs_name && strcmp(lhs_name, rhs_name) != 0)
    return false;

  if (lhs_members && rhs_members) {
    if (lhs_members->size != rhs_members->size)
      return false;

    for (size_t i = 0; i < lhs_members->size; ++i) {
      Member* lhs_member = vector_at(lhs_members, i);
      Member* rhs_member = vector_at(rhs_members, i);
      if ((lhs_member->name && !rhs_member->name) ||
          (!lhs_member->name && rhs_member->name))
        return false;

      if (lhs_member->name && rhs_member->name &&
          strcmp(lhs_member->name, rhs_member->name) != 0)
        return false;

      if ((lhs_member->bitfield && !rhs_member->bitfield) ||
          (!lhs_member->bitfield && rhs_member->bitfield))
        return false;

      if (lhs_member->bitfield && rhs_member->bitfield) {
        ConstExprResult lhs_bitfield =
            sema_eval_expr(sema, lhs_member->bitfield);
        ConstExprResult rhs_bitfield =
            sema_eval_expr(sema, rhs_member->bitfield);
        if (compare_constexpr_result_types(&lhs_bitfield, &rhs_bitfield))
          return false;
      }

      if (!sema_types_are_compatible_impl(sema, lhs_member->type,
                                          rhs_member->type, ignore_quals))
        return false;
    }
  }

  return true;
}

bool sema_types_are_compatible_impl(Sema* sema, const Type* lhs,
                                    const Type* rhs, bool ignore_quals) {
  if (lhs->vtable->kind == TK_NamedType) {
    const Type* resolved_lhs = sema_resolve_named_type(sema, (NamedType*)lhs);
    return sema_types_are_compatible_impl(sema, resolved_lhs, rhs,
                                          ignore_quals);
  }

  if (rhs->vtable->kind == TK_NamedType) {
    const Type* resolved_rhs = sema_resolve_named_type(sema, (NamedType*)rhs);
    return sema_types_are_compatible_impl(sema, lhs, resolved_rhs,
                                          ignore_quals);
  }

  if (lhs->vtable->kind != rhs->vtable->kind)
    return false;

  if (!ignore_quals && lhs->qualifiers != rhs->qualifiers)
    return false;

  switch (lhs->vtable->kind) {
    case TK_BuiltinType: {
      if (((BuiltinType*)lhs)->kind != ((BuiltinType*)rhs)->kind)
        return false;
      break;
    }
    case TK_PointerType: {
      const Type* lhs_pointee = ((PointerType*)lhs)->pointee;
      const Type* rhs_pointee = ((PointerType*)rhs)->pointee;
      if (!sema_types_are_compatible_impl(sema, lhs_pointee, rhs_pointee,
                                          ignore_quals))
        return false;
      break;
    }
    case TK_ArrayType: {
      ArrayType* lhs_arr = (ArrayType*)lhs;
      ArrayType* rhs_arr = (ArrayType*)rhs;
      if (!sema_types_are_compatible_impl(sema, lhs_arr->elem_type,
                                          rhs_arr->elem_type, ignore_quals))
        return false;

      if (lhs_arr->size && rhs_arr->size) {
        ConstExprResult lhs_res = sema_eval_expr(sema, lhs_arr->size);
        ConstExprResult rhs_res = sema_eval_expr(sema, rhs_arr->size);
        if (compare_constexpr_result_types(&lhs_res, &rhs_res) != 0)
          return false;
      }

      // Arrays of unknown bounds are compatible with any array of compatible
      // element type.
      break;
    }
    case TK_StructType: {
      StructType* lhs_struct = (StructType*)lhs;
      StructType* rhs_struct = (StructType*)rhs;

      if (!sema_struct_or_union_components_are_compatible(
              sema, lhs_struct->name, lhs_struct->members, rhs_struct->name,
              rhs_struct->members, ignore_quals))
        return false;

      break;
    }
    case TK_UnionType: {
      UnionType* lhs_union = (UnionType*)lhs;
      UnionType* rhs_union = (UnionType*)rhs;

      if (!sema_struct_or_union_components_are_compatible(
              sema, lhs_union->name, lhs_union->members, rhs_union->name,
              rhs_union->members, ignore_quals))
        return false;

      break;
    }
    case TK_EnumType: {
      // If a fixed underlying type is not specified, the enum is compatible
      // with one of: char, a signed int type, or an unsigned int type. This is
      // implementation defined.
      break;
    }
    case TK_FunctionType: {
      FunctionType* lhs_func = (FunctionType*)lhs;
      FunctionType* rhs_func = (FunctionType*)rhs;
      if (!sema_types_are_compatible_impl(sema, lhs_func->return_type,
                                          rhs_func->return_type, ignore_quals))
        return false;

      if (lhs_func->has_var_args != rhs_func->has_var_args)
        return false;

      if (lhs_func->pos_args.size != rhs_func->pos_args.size)
        return false;

      for (size_t i = 0; i < lhs_func->pos_args.size; ++i) {
        // FIXME: This should also account for array-to-pointer and
        // function-to-pointer type adjustments.
        Type* lhs_arg = ((FunctionArg*)vector_at(&lhs_func->pos_args, i))->type;
        Type* rhs_arg = ((FunctionArg*)vector_at(&rhs_func->pos_args, i))->type;
        if (!sema_types_are_compatible_impl(sema, lhs_arg, rhs_arg,
                                            ignore_quals))
          return false;
      }
      break;
    }
    case TK_NamedType:
    case TK_ReplacementSentinelType:
      UNREACHABLE_MSG("This type should not be handled here: %d",
                      lhs->vtable->kind);
  }

  return true;
}

bool sema_types_are_compatible(Sema* sema, const Type* lhs, const Type* rhs) {
  return sema_types_are_compatible_impl(sema, lhs, rhs, /*ignore_quals=*/false);
}

bool sema_types_are_compatible_ignore_quals(Sema* sema, const Type* lhs,
                                            const Type* rhs) {
  return sema_types_are_compatible_impl(sema, lhs, rhs, /*ignore_quals=*/true);
}

static void verify_function_type(Sema* sema, const FunctionType* func_ty) {
  size_t num_args = func_ty->pos_args.size;
  for (size_t i = 0; i < num_args; ++i) {
    FunctionArg* arg = vector_at(&func_ty->pos_args, i);
    if (is_void_type(arg->type)) {
      assert(num_args == 1);
      assert(!func_ty->has_var_args);
      assert(arg->name == NULL);
    }
  }
}

void sema_handle_function_definition(Sema* sema, FunctionDefinition* f) {
  assert(f->type->vtable->kind == TK_FunctionType);
  verify_function_type(sema, (const FunctionType*)f->type);

  assert(!tree_map_get(&sema->enum_values, f->name, NULL));

  void* val;
  if (!tree_map_get(&sema->globals, f->name, &val)) {
    // This func was not declared prior. Unconditionally save this one.
    tree_map_set(&sema->globals, f->name, f);
    return;
  }

  // This global was declared prior.
  TopLevelNode* found = val;
  ASSERT_MSG(found->vtable->kind != TLNK_FunctionDefinition,
             "Redefinition of function '%s'", f->name);

  // This must be a global variable.
  assert(found->vtable->kind == TLNK_GlobalVariable);
  GlobalVariable* gv = val;
  ASSERT_MSG(!gv->initializer, "Redefinition of function '%s'", f->name);

  ASSERT_MSG(sema_types_are_compatible(sema, gv->type, f->type),
             "redefinition of '%s' with a different type", gv->name);
}

void sema_handle_global_variable(Sema* sema, GlobalVariable* gv) {
  assert(!tree_map_get(&sema->enum_values, gv->name, NULL));

  void* val;
  if (!tree_map_get(&sema->globals, gv->name, &val)) {
    // This global was not declared prior. Unconditionally save this one.
    tree_map_set(&sema->globals, gv->name, gv);
    return;
  }

  // This global was declared prior. Check that we have the same compatible
  // types.
  TopLevelNode* found_val = val;
  if (found_val->vtable->kind == TLNK_GlobalVariable) {
    GlobalVariable* found = val;
    ASSERT_MSG(sema_types_are_compatible(sema, found->type, gv->type),
               "redefinition of '%s' with a different type", gv->name);

    // Ensure we don't have multiple definitions.
    ASSERT_MSG(!(found->initializer && gv->initializer),
               "redefinition of '%s' with a different value", gv->name);

    if (!found->initializer && gv->initializer) {
      // Save the one with the definition.
      tree_map_set(&sema->globals, gv->name, gv);
    }

    return;
  }

  // This must be a function.
  assert(found_val->vtable->kind == TLNK_FunctionDefinition);
  FunctionDefinition* found = val;
  ASSERT_MSG(sema_types_are_compatible(sema, found->type, gv->type),
             "redefinition of '%s' with a different type", gv->name);
  assert(found->type->vtable->kind == TK_FunctionType);
  verify_function_type(sema, (const FunctionType*)found->type);

  // The new global variable must be a re-declaration since we can only
  // have one function definition.
  ASSERT_MSG(!gv->initializer, "Redefinition of function '%s'", gv->name);
}

static void sema_handle_struct_declaration_impl(Sema* sema,
                                                StructType* struct_ty) {
  // This is an anonymous struct.
  if (!struct_ty->name)
    return;

  // We received a struct declaration but not definition.
  if (!struct_ty->members)
    return;

  void* found;
  if (tree_map_get(&sema->struct_types, struct_ty->name, &found)) {
    StructType* found_struct = found;
    ASSERT_MSG(!found_struct->members, "Duplicate struct definition '%s'",
               struct_ty->name);
  }

  tree_map_set(&sema->struct_types, struct_ty->name, struct_ty);
}

void sema_handle_struct_declaration(Sema* sema, StructDeclaration* decl) {
  sema_handle_struct_declaration_impl(sema, decl->type);
}

static void sema_handle_union_declaration_impl(Sema* sema,
                                               UnionType* union_ty) {
  // This is an anonymous union.
  if (!union_ty->name)
    return;

  // We received a uniondeclaration but not definition.
  if (!union_ty->members)
    return;

  void* found;
  if (tree_map_get(&sema->union_types, union_ty->name, &found)) {
    UnionType* found_union = found;
    assert(found_union->members);
  }

  tree_map_set(&sema->union_types, union_ty->name, union_ty);
}

void sema_handle_union_declaration(Sema* sema, UnionDeclaration* decl) {
  sema_handle_union_declaration_impl(sema, decl->type);
}

static void sema_handle_enum_declaration_impl(Sema* sema, EnumType* enum_ty) {
  if (!enum_ty->members)
    return;

  if (enum_ty->name) {
    void* found;
    if (tree_map_get(&sema->enum_types, enum_ty->name, &found)) {
      EnumType* found_enum = found;
      ASSERT_MSG(!found_enum->members, "Duplicate enum definition '%s'",
                 enum_ty->name);
    }

    tree_map_set(&sema->enum_types, enum_ty->name, enum_ty);
  }

  // Register the individual members.
  int val = 0;
  for (size_t i = 0; i < enum_ty->members->size; ++i) {
    EnumMember* member = vector_at(enum_ty->members, i);
    assert(!tree_map_get(&sema->enum_values, member->name, NULL));

    if (member->value) {
      ConstExprResult res = sema_eval_expr(sema, member->value);
      switch (res.result_kind) {
        case RK_Boolean:
          UNREACHABLE_MSG("TODO: Can a bool be assigned to an enum member?");
        case RK_Int:
          val = res.result.i;
          break;
        case RK_UnsignedLongLong:
          val = (int)res.result.ull;
          break;
      }
    }

    tree_map_set(&sema->enum_values, member->name, (void*)(intptr_t)val);
    tree_map_set(&sema->enum_names, member->name, enum_ty);

    val++;
  }
}

void sema_handle_enum_declaration(Sema* sema, EnumDeclaration* decl) {
  sema_handle_enum_declaration_impl(sema, decl->type);
}

void sema_add_typedef_type(Sema* sema, const char* name, Type* type) {
  ASSERT_MSG(!tree_map_get(&sema->typedef_types, name, NULL),
             "typedef for '%s' already exists", name);

  if (type->vtable->kind == TK_StructType) {
    StructType* struct_ty = (StructType*)type;
    sema_handle_struct_declaration_impl(sema, struct_ty);
  } else if (type->vtable->kind == TK_EnumType) {
    EnumType* enum_ty = (EnumType*)type;
    sema_handle_enum_declaration_impl(sema, enum_ty);
  }

  switch (type->vtable->kind) {
    case TK_StructType:
    case TK_UnionType:
    case TK_FunctionType:
    case TK_BuiltinType:
    case TK_EnumType:  // TODO: Handle enum values.
    case TK_PointerType:
    case TK_ArrayType:
      tree_map_set(&sema->typedef_types, name, type);
      break;
    case TK_NamedType: {
      const NamedType* nt = (const NamedType*)type;
      void* found;
      ASSERT_MSG(tree_map_get(&sema->typedef_types, nt->name, &found),
                 "Unknown type '%s'", nt->name);

      // NOTE: This effectively flattens the type tree and we lose information
      // regarding one typedef aliasing to another typedef.
      assert(((Type*)found)->vtable->kind != TK_NamedType &&
             "All typedef types should be flattened to an unnamed type when "
             "being added.");
      tree_map_set(&sema->typedef_types, name, found);
      break;
    }
    case TK_ReplacementSentinelType:
      UNREACHABLE_MSG("Replacement sentinel type should not be used");
      break;
  }
}

const Type* sema_get_type_of_unop_expr(Sema* sema, const UnOp* expr,
                                       const TreeMap* local_ctx) {
  switch (expr->op) {
    case UOK_Not:
      return &sema->bt_Bool.type;
    case UOK_BitNot:
    case UOK_PostInc:
    case UOK_PreInc:
    case UOK_PostDec:
    case UOK_PreDec:
    case UOK_Plus:
    case UOK_Negate:
      return sema_get_type_of_expr_in_ctx(sema, expr->subexpr, local_ctx);
    case UOK_AddrOf: {
      const Type* sub_type =
          sema_get_type_of_expr_in_ctx(sema, expr->subexpr, local_ctx);
      NonOwningPointerType** ptr =
          vector_append_storage(&sema->address_of_storage);
      *ptr = malloc(sizeof(NonOwningPointerType));
      non_owning_pointer_type_construct(*ptr, sub_type);
      return (Type*)(*ptr);
    }
    case UOK_Deref: {
      const Type* sub_type =
          sema_get_type_of_expr_in_ctx(sema, expr->subexpr, local_ctx);
      if (sema_is_pointer_type(sema, sub_type, local_ctx))
        return sema_get_pointee(sema, sub_type, local_ctx);

      if (sema_is_array_type(sema, sub_type, local_ctx))
        return sema_get_array_type(sema, sub_type, local_ctx)->elem_type;

      UNREACHABLE_MSG(
          "Attempting to dereference type that is not a pointer or array");
    }
    default:
      UNREACHABLE_MSG("TODO: Implement get type for unop kind %d", expr->op);
  }
}

bool sema_is_unsigned_integral_type(const Sema* sema, const Type* type) {
  type = sema_resolve_maybe_named_type(sema, type);
  return is_unsigned_integral_type(type);
}

const Type* sema_get_corresponding_unsigned_type(const Sema* sema,
                                                 const BuiltinType* bt) {
  if (is_unsigned_integral_type(&bt->type))
    return &bt->type;

  switch (bt->kind) {
    case BTK_Char:
    case BTK_SignedChar:
      return &sema->bt_UnsignedChar.type;
    case BTK_Short:
      return &sema->bt_UnsignedShort.type;
    case BTK_Int:
      return &sema->bt_UnsignedInt.type;
    case BTK_Long:
      return &sema->bt_UnsignedLong.type;
    case BTK_LongLong:
      return &sema->bt_UnsignedLongLong.type;
    default:
      UNREACHABLE_MSG("Non-signed integral type: %d", bt->kind);
  }
}

// Find the common type for usual arithmetic conversions.
const Type* sema_get_common_arithmetic_type(Sema* sema, const Type* lhs_ty,
                                            const Type* rhs_ty,
                                            const TreeMap* local_ctx) {
  if (lhs_ty->vtable->kind == TK_NamedType)
    lhs_ty = sema_resolve_named_type(sema, (const NamedType*)lhs_ty);
  if (rhs_ty->vtable->kind == TK_NamedType)
    rhs_ty = sema_resolve_named_type(sema, (const NamedType*)rhs_ty);

  // If the types are the same, that type is the common type.
  if (sema_types_are_compatible_ignore_quals(sema, lhs_ty, rhs_ty))
    return lhs_ty;

  // TODO: Handle non-integral types also.
  assert(is_integral_type(lhs_ty) && is_integral_type(rhs_ty));

  const BuiltinType* lhs_bt = (const BuiltinType*)lhs_ty;
  const BuiltinType* rhs_bt = (const BuiltinType*)rhs_ty;
  unsigned lhs_rank = get_integral_rank(lhs_bt);
  unsigned rhs_rank = get_integral_rank(rhs_bt);

  // If the types have the same signedness (both signed or both unsigned),
  // the operand whose type has the lesser conversion rank is implicitly
  // converted to the other type.
  if (is_unsigned_integral_type(lhs_ty) == is_unsigned_integral_type(rhs_ty)) {
    return lhs_rank > rhs_rank ? lhs_ty : rhs_ty;
  }

  const Type* unsigned_ty;
  const Type* signed_ty;
  unsigned unsigned_rank;
  unsigned signed_rank;
  if (is_unsigned_integral_type(lhs_ty)) {
    unsigned_ty = lhs_ty;
    signed_ty = rhs_ty;
    unsigned_rank = lhs_rank;
    signed_rank = rhs_rank;
  } else {
    unsigned_ty = rhs_ty;
    signed_ty = lhs_ty;
    unsigned_rank = rhs_rank;
    signed_rank = lhs_rank;
  }

  // If the unsigned type has conversion rank greater than or equal to the
  // rank of the signed type, then the operand with the signed type is
  // implicitly converted to the unsigned type.
  if (unsigned_rank >= signed_rank)
    return unsigned_ty;

  // If the signed type can represent all values of the unsigned type, then
  // the operand with the unsigned type is implicitly converted to the
  // signed type.
  size_t unsigned_size = sema_eval_sizeof_type(sema, unsigned_ty);
  size_t signed_size = sema_eval_sizeof_type(sema, signed_ty);
  if (signed_size > unsigned_size)
    return signed_ty;

  // Else, both operands undergo implicit conversion to the unsigned type
  // counterpart of the signed operand's type.
  return sema_get_corresponding_unsigned_type(sema,
                                              (const BuiltinType*)signed_ty);
}

const Type* sema_get_common_arithmetic_type_of_exprs(Sema* sema,
                                                     const Expr* lhs,
                                                     const Expr* rhs,
                                                     const TreeMap* local_ctx) {
  const Type* lhs_ty = sema_get_type_of_expr_in_ctx(sema, lhs, local_ctx);
  const Type* rhs_ty = sema_get_type_of_expr_in_ctx(sema, rhs, local_ctx);
  return sema_get_common_arithmetic_type(sema, lhs_ty, rhs_ty, local_ctx);
}

const Type* sema_get_result_type_of_binop_expr(Sema* sema, const BinOp* expr,
                                               const TreeMap* local_ctx) {
  const Type* lhs_ty = sema_get_type_of_expr_in_ctx(sema, expr->lhs, local_ctx);
  const Type* rhs_ty = sema_get_type_of_expr_in_ctx(sema, expr->rhs, local_ctx);

  if (lhs_ty->vtable->kind == TK_NamedType)
    lhs_ty = sema_resolve_named_type(sema, (const NamedType*)lhs_ty);
  if (rhs_ty->vtable->kind == TK_NamedType)
    rhs_ty = sema_resolve_named_type(sema, (const NamedType*)rhs_ty);

  switch (expr->op) {
    case BOK_Eq:
    case BOK_Ne:
    case BOK_Lt:
    case BOK_Gt:
    case BOK_Le:
    case BOK_Ge:
    case BOK_LogicalOr:
    case BOK_LogicalAnd:
      return &sema->bt_Bool.type;
    case BOK_Add:
    case BOK_Sub:
    case BOK_Mul:
      if (sema_is_pointer_type(sema, lhs_ty, local_ctx))
        return lhs_ty;
      if (sema_is_pointer_type(sema, rhs_ty, local_ctx))
        return rhs_ty;
      [[fallthrough]];
    case BOK_Div:
    case BOK_Mod:
    case BOK_Xor:
    case BOK_BitwiseOr:
    case BOK_BitwiseAnd:
      return sema_get_common_arithmetic_type(sema, lhs_ty, rhs_ty, local_ctx);
    case BOK_Assign:
    case BOK_MulAssign:
    case BOK_DivAssign:
    case BOK_ModAssign:
    case BOK_AddAssign:
    case BOK_SubAssign:
    case BOK_LShiftAssign:
    case BOK_RShiftAssign:
    case BOK_AndAssign:
    case BOK_OrAssign:
    case BOK_XorAssign:
    case BOK_LShift:  // FIXME: The type for these 2 should be the lhs after
                      // promotion.
    case BOK_RShift:
      return lhs_ty;
    case BOK_Comma:
      return rhs_ty;
    default:
      UNREACHABLE_MSG("TODO: Implement get type for binop kind %d on %zu:%zu",
                      expr->op, expr->expr.line, expr->expr.col);
  }
}

const StructType* sema_get_struct_type_from_member_access(
    Sema* sema, const MemberAccess* access, const TreeMap* local_ctx) {
  const Type* base_ty =
      sema_get_type_of_expr_in_ctx(sema, access->base, local_ctx);
  base_ty = sema_resolve_maybe_named_type(sema, base_ty);

  if (access->is_arrow) {
    assert(sema_is_pointer_to(sema, base_ty, TK_StructType, local_ctx));
    base_ty = sema_get_pointee(sema, base_ty, local_ctx);
    base_ty = sema_resolve_maybe_named_type(sema, base_ty);
  }
  assert(sema_is_struct_type(sema, base_ty, local_ctx));

  return sema_resolve_struct_type(sema, (const StructType*)base_ty);
}

bool sema_is_function_or_function_ptr(Sema* sema, const Type* ty,
                                      const TreeMap* local_ctx) {
  if (ty->vtable->kind == TK_FunctionType)
    return true;
  return sema_is_pointer_type(sema, ty, local_ctx) &&
         sema_get_pointee(sema, ty, local_ctx)->vtable->kind == TK_FunctionType;
}

const FunctionType* sema_get_function(Sema* sema, const Type* ty,
                                      const TreeMap* local_ctx) {
  if (ty->vtable->kind == TK_FunctionType)
    return (const FunctionType*)ty;

  ty = sema_get_pointee(sema, ty, local_ctx);
  assert(ty->vtable->kind == TK_FunctionType);
  return (const FunctionType*)ty;
}

// Infer the type of this expression. The caller of this is not in charge of
// destroying the type.
const Type* sema_get_type_of_expr_in_ctx(Sema* sema, const Expr* expr,
                                         const TreeMap* local_ctx) {
  switch (expr->vtable->kind) {
    case EK_String:
    case EK_PrettyFunction:
      return &sema->str_ty.type;
    case EK_SizeOf:
    case EK_AlignOf:
      return sema_resolve_named_type_from_name(sema, "size_t");
    case EK_Int:
      return &((const Int*)expr)->type.type;
    case EK_Bool:
      return &sema->bt_Bool.type;
    case EK_Char:
      return &sema->bt_Char.type;
    case EK_DeclRef: {
      const DeclRef* decl = (const DeclRef*)expr;
      void* val;

      // Not a global. Search the local scope.
      if (tree_map_get(local_ctx, decl->name, &val))
        return (const Type*)val;

      // Maybe an enum?
      if (tree_map_get(&sema->enum_names, decl->name, &val))
        return (const Type*)val;

      if (tree_map_get(&sema->globals, decl->name, &val)) {
        if (((TopLevelNode*)val)->vtable->kind == TLNK_GlobalVariable) {
          GlobalVariable* gv = val;
          return gv->type;
        } else {
          assert(((TopLevelNode*)val)->vtable->kind == TLNK_FunctionDefinition);
          FunctionDefinition* f = val;
          return f->type;
        }
      }

      UNREACHABLE_MSG("Unknown symbol '%s'", decl->name);
    }
    case EK_FunctionParam:
      return ((const FunctionParam*)expr)->type;
    case EK_UnOp:
      return sema_get_type_of_unop_expr(sema, (const UnOp*)expr, local_ctx);
    case EK_BinOp:
      return sema_get_result_type_of_binop_expr(sema, (const BinOp*)expr,
                                                local_ctx);
    case EK_Call: {
      const Call* call = (const Call*)expr;
      const Type* ty =
          sema_get_type_of_expr_in_ctx(sema, call->base, local_ctx);
      assert(sema_is_function_or_function_ptr(sema, ty, local_ctx));
      return sema_get_function(sema, ty, local_ctx)->return_type;
    }
    case EK_Cast: {
      const Cast* cast = (const Cast*)expr;
      return cast->to;
    }
    case EK_MemberAccess: {
      const MemberAccess* access = (const MemberAccess*)expr;
      const StructType* base_ty =
          sema_get_struct_type_from_member_access(sema, access, local_ctx);

      const Member* member = sema_get_struct_member(
          sema, base_ty, access->member, /*offset=*/NULL);
      return member->type;
    }
    case EK_Conditional: {
      const Conditional* conditional = (const Conditional*)expr;
      return sema_get_common_arithmetic_type_of_exprs(
          sema, conditional->true_expr, conditional->false_expr, local_ctx);
    }
    case EK_Index: {
      const Index* index = (const Index*)expr;
      const Type* base_ty =
          sema_get_type_of_expr_in_ctx(sema, index->base, local_ctx);
      base_ty = sema_resolve_maybe_named_type(sema, base_ty);
      if (base_ty->vtable->kind == TK_PointerType)
        return sema_get_pointee(sema, base_ty, local_ctx);

      if (base_ty->vtable->kind == TK_ArrayType)
        return sema_get_array_type(sema, base_ty, local_ctx)->elem_type;

      UNREACHABLE_MSG(
          "Attempting to index a type that isn't a pointer or array: %d",
          base_ty->vtable->kind);
    }
    case EK_StmtExpr: {
      const StmtExpr* stmt_expr = (const StmtExpr*)expr;

      // If you use some other kind of statement last within the braces, the
      // construct has type void, and thus effectively no value.
      if (!stmt_expr->stmt)
        return &sema->bt_Void.type;

      Statement* last = *(Statement**)vector_back(&stmt_expr->stmt->body);
      if (last->vtable->kind != SK_ExprStmt)
        return &sema->bt_Void.type;

      Expr* expr = ((ExprStmt*)last)->expr;
      return sema_get_type_of_expr_in_ctx(sema, expr, local_ctx);
    }
    case EK_InitializerList:
      UNREACHABLE_MSG(
          "Initializer lists do not have types. They are a syntactic construct "
          "used for initializing aggregate types. Callers of this function "
          "should not call this function with an InitializerList. They should "
          "have custom logic for handling them.");
    default:
      UNREACHABLE_MSG(
          "TODO: Implement sema_get_type_of_expr_in_ctx for this expression %d",
          expr->vtable->kind);
  }
}

// Infer the type of this expression in the global context. The caller of this
// is not in charge of destroying the type.
const Type* sema_get_type_of_expr(Sema* sema, const Expr* expr) {
  TreeMap local_ctx;
  string_tree_map_construct(&local_ctx);
  const Type* res = sema_get_type_of_expr_in_ctx(sema, expr, &local_ctx);
  tree_map_destroy(&local_ctx);
  return res;
}

size_t builtin_type_get_size(const BuiltinType* bt) {
  // TODO: ATM these are the HOST values, not the target values.
  switch (bt->kind) {
    case BTK_Char:
      return sizeof(char);
    case BTK_SignedChar:
      return sizeof(signed char);
    case BTK_UnsignedChar:
      return sizeof(unsigned char);
    case BTK_Short:
      return sizeof(short);
    case BTK_UnsignedShort:
      return sizeof(unsigned short);
    case BTK_Int:
      return sizeof(int);
    case BTK_UnsignedInt:
      return sizeof(unsigned int);
    case BTK_Long:
      return sizeof(long);
    case BTK_UnsignedLong:
      return sizeof(unsigned long);
    case BTK_LongLong:
      return sizeof(long long);
    case BTK_UnsignedLongLong:
      return sizeof(unsigned long long);
    case BTK_Float:
      return sizeof(float);
    case BTK_Double:
      return sizeof(double);
    case BTK_LongDouble:
      return sizeof(long double);
    case BTK_Float128:
      return sizeof(__float128);
    case BTK_Bool:
      return sizeof(bool);
    case BTK_ComplexFloat:
      return sizeof(_Complex float);
    case BTK_ComplexDouble:
      return sizeof(_Complex double);
    case BTK_ComplexLongDouble:
      return sizeof(_Complex long double);
    case BTK_BuiltinVAList:
      return sizeof(__builtin_va_list);
    case BTK_Void:
      UNREACHABLE_MSG("Attempting to get sizeof void");
  }
}

static size_t builtin_type_get_align(const BuiltinType* bt) {
  // TODO: ATM these are the HOST values, not the target values.
  switch (bt->kind) {
    case BTK_Char:
      return alignof(char);
    case BTK_SignedChar:
      return alignof(signed char);
    case BTK_UnsignedChar:
      return alignof(unsigned char);
    case BTK_Short:
      return alignof(short);
    case BTK_UnsignedShort:
      return alignof(unsigned short);
    case BTK_Int:
      return alignof(int);
    case BTK_UnsignedInt:
      return alignof(unsigned int);
    case BTK_Long:
      return alignof(long);
    case BTK_UnsignedLong:
      return alignof(unsigned long);
    case BTK_LongLong:
      return alignof(long long);
    case BTK_UnsignedLongLong:
      return alignof(unsigned long long);
    case BTK_Float:
      return alignof(float);
    case BTK_Double:
      return alignof(double);
    case BTK_LongDouble:
      return alignof(long double);
    case BTK_Float128:
      return alignof(__float128);
    case BTK_Bool:
      return alignof(bool);
    case BTK_ComplexFloat:
      return alignof(_Complex float);
    case BTK_ComplexDouble:
      return alignof(_Complex double);
    case BTK_ComplexLongDouble:
      return alignof(_Complex long double);
    case BTK_BuiltinVAList:
      return alignof(__builtin_va_list);
    case BTK_Void:
      UNREACHABLE_MSG("Attempting to get alignof void");
  }
}

size_t sema_eval_alignof_members(Sema* sema, const vector* members) {
  assert(members);
  size_t max_align = 0;
  for (size_t i = 0; i < members->size; ++i) {
    const Member* member = vector_at(members, i);
    size_t member_align = sema_eval_alignof_type(sema, member->type);
    if (member_align > max_align)
      max_align = member_align;
  }
  return max_align;
}

size_t sema_eval_alignof_type(Sema* sema, const Type* type) {
  switch (type->vtable->kind) {
    case TK_BuiltinType:
      return builtin_type_get_align((const BuiltinType*)type);
    case TK_PointerType:
      // TODO: These are host values. Also is the alignment of all pointer
      // types guaranteed to be the same?
      return alignof(void*);
    case TK_NamedType: {
      const NamedType* nt = (const NamedType*)type;
      void* found;
      ASSERT_MSG(tree_map_get(&sema->typedef_types, nt->name, &found),
                 "Unknown type '%s'", nt->name);
      return sema_eval_alignof_type(sema, (const Type*)found);
    }
    case TK_EnumType:
      return builtin_type_get_align(
          sema_get_integral_type_for_enum(sema, (const EnumType*)type));
    case TK_ArrayType:
      return sema_eval_alignof_array(sema, (const ArrayType*)type);
    case TK_FunctionType:
      UNREACHABLE_MSG("Cannot take alignof function type!");
    case TK_StructType: {
      const StructType* struct_ty =
          sema_resolve_struct_type(sema, (const StructType*)type);
      return sema_eval_alignof_members(sema, struct_ty->members);
    }
    case TK_UnionType: {
      const UnionType* union_ty =
          sema_resolve_union_type(sema, (const UnionType*)type);
      return sema_eval_alignof_members(sema, union_ty->members);
    }
    case TK_ReplacementSentinelType:
      UNREACHABLE_MSG("Replacement sentinel type should not be used");
  }
}

size_t sema_eval_sizeof_struct_type(Sema* sema, const StructType* type) {
  type = sema_resolve_struct_type(sema, type);
  ASSERT_MSG(type->members, "Taking sizeof incomplete struct type '%s'",
             type->name);

  size_t size = 0;
  size_t max_align = 0;
  for (size_t i = 0; i < type->members->size; ++i) {
    // TODO: Account for bitfields.
    const Member* member = vector_at(type->members, i);
    const Type* member_ty = member->type;
    size_t align = sema_eval_alignof_type(sema, member_ty);
    if (align > max_align)
      max_align = align;
    if (size % align != 0)
      size = align_up(size, align);
    size += sema_eval_sizeof_type(sema, member_ty);
  }
  size = align_up(size, max_align);
  return size;
}

const Member* sema_get_largest_union_member(Sema* sema, const UnionType* type) {
  type = sema_resolve_union_type(sema, type);
  assert(type->members && type->members->size);

  size_t size = 0;
  const Member* largest;
  for (size_t i = 0; i < type->members->size; ++i) {
    // TODO: Account for bitfields.
    const Member* member = vector_at(type->members, i);
    const Type* member_ty = member->type;
    size_t member_size = sema_eval_sizeof_type(sema, member_ty);
    if (member_size > size) {
      size = member_size;
      largest = member;
    }
  }
  assert(size);
  return largest;
}

size_t sema_eval_sizeof_union_type(Sema* sema, const UnionType* type) {
  return sema_eval_sizeof_type(sema,
                               sema_get_largest_union_member(sema, type)->type);
}

size_t sema_eval_sizeof_array(Sema* sema, const ArrayType* arr) {
  size_t elem_size = sema_eval_sizeof_type(sema, arr->elem_type);
  assert(arr->size);
  ConstExprResult num_elems = sema_eval_expr(sema, arr->size);
  switch (num_elems.result_kind) {
    case RK_UnsignedLongLong:
      return elem_size * num_elems.result.ull;
    case RK_Int:
      assert(num_elems.result.i >= 0);
      return elem_size * (size_t)num_elems.result.i;
    default:
      UNREACHABLE_MSG("Bool number of array elements?");
  }
}

size_t sema_eval_sizeof_type(Sema* sema, const Type* type) {
  switch (type->vtable->kind) {
    case TK_BuiltinType:
      return builtin_type_get_size((const BuiltinType*)type);
    case TK_PointerType:
      // FIXME: The size of a pointer to different types is NOT guaranteed to
      // be the same size for each individual pointee type. All pointers only
      // happen to be the same size for the host platform I'm building this on.
      return sizeof(void*);
    case TK_NamedType: {
      const NamedType* nt = (const NamedType*)type;
      void* found;
      ASSERT_MSG(tree_map_get(&sema->typedef_types, nt->name, &found),
                 "Unknown type '%s'\n", nt->name);
      return sema_eval_sizeof_type(sema, (const Type*)found);
    }
    case TK_EnumType:
      return builtin_type_get_size(
          sema_get_integral_type_for_enum(sema, (const EnumType*)type));
    case TK_ArrayType:
      return sema_eval_sizeof_array(sema, (const ArrayType*)type);
    case TK_FunctionType:
      UNREACHABLE_MSG("Cannot take sizeof function type!");
    case TK_StructType:
      return sema_eval_sizeof_struct_type(sema, (const StructType*)type);
    case TK_UnionType:
      return sema_eval_sizeof_union_type(sema, (const UnionType*)type);
    case TK_ReplacementSentinelType:
      UNREACHABLE_MSG("Replacement sentinel type should not be used");
  }
}

int compare_constexpr_result_types(const ConstExprResult* lhs,
                                   const ConstExprResult* rhs) {
  // TODO: Would be neat to see if the compiler optimizes this into a normal cmp
  // instruction.
  switch (lhs->result_kind) {
    case RK_Boolean:
      switch (rhs->result_kind) {
        case RK_Boolean:
          return (int)(!(lhs->result.b == rhs->result.b));
        case RK_Int:
          return (int)((int)lhs->result.b == rhs->result.i);
        case RK_UnsignedLongLong:
          return (int)((unsigned long long)lhs->result.b == rhs->result.ull);
      }
    case RK_Int: {
      switch (rhs->result_kind) {
        case RK_Boolean:
          if (lhs->result.i < rhs->result.b)
            return -1;
          if (lhs->result.i > rhs->result.b)
            return 1;
          return 0;
        case RK_Int:
          if (lhs->result.i < rhs->result.i)
            return -1;
          if (lhs->result.i > rhs->result.i)
            return 1;
          return 0;
        case RK_UnsignedLongLong:
          if (lhs->result.i < rhs->result.ull)
            return -1;
          if (lhs->result.i > rhs->result.ull)
            return 1;
          return 0;
      }
    }
    case RK_UnsignedLongLong: {
      switch (rhs->result_kind) {
        case RK_Boolean:
          if (lhs->result.ull < rhs->result.b)
            return -1;
          if (lhs->result.ull > rhs->result.b)
            return 1;
          return 0;
        case RK_Int:
          if (lhs->result.ull < rhs->result.i)
            return -1;
          if (lhs->result.ull > rhs->result.i)
            return 1;
          return 0;
        case RK_UnsignedLongLong:
          if (lhs->result.ull < rhs->result.ull)
            return -1;
          if (lhs->result.ull > rhs->result.ull)
            return 1;
          return 0;
      }
    }
  }
}

size_t sema_eval_alignof_array(Sema* sema, const ArrayType* arr) {
  return sema_eval_alignof_type(sema, arr->elem_type);
}

ConstExprResult sema_eval_binop(Sema* sema, const BinOp* expr) {
  ConstExprResult lhs = sema_eval_expr(sema, expr->lhs);
  ConstExprResult rhs = sema_eval_expr(sema, expr->rhs);

  switch (expr->op) {
    case BOK_Eq: {
      ConstExprResult res;
      res.result_kind = RK_Boolean;
      res.result.b = compare_constexpr_result_types(&lhs, &rhs) == 0;
      return res;
    }
    case BOK_Add: {
      assert(lhs.result_kind == rhs.result_kind);
      ConstExprResult res;
      res.result_kind = lhs.result_kind;
      switch (lhs.result_kind) {
        case RK_Boolean:
          UNREACHABLE_MSG("TODO: Handle adding bools");
        case RK_Int:
          res.result.i = lhs.result.i + rhs.result.i;
          break;
        case RK_UnsignedLongLong:
          res.result.ull = lhs.result.ull + rhs.result.ull;
          break;
      }
      return res;
    }
    case BOK_Div: {
      assert(lhs.result_kind == rhs.result_kind);
      ConstExprResult res;
      res.result_kind = lhs.result_kind;
      switch (lhs.result_kind) {
        case RK_Boolean:
          UNREACHABLE_MSG("Dividing bools");
        case RK_Int:
          assert(rhs.result.i);
          res.result.i = lhs.result.i / rhs.result.i;
          break;
        case RK_UnsignedLongLong:
          assert(rhs.result.ull);
          res.result.ull = lhs.result.ull / rhs.result.ull;
          break;
      }
      return res;
    }
    case BOK_LShift:
      switch (lhs.result_kind) {
        case RK_Boolean:
          UNREACHABLE_MSG("Cannot shift boolean");
        case RK_Int:
          assert(sizeof(int) * 8 > result_to_u64(&rhs));
          lhs.result.i <<= result_to_u64(&rhs);
          return lhs;
        case RK_UnsignedLongLong:
          assert(sizeof(unsigned long long) * 8 > result_to_u64(&rhs));
          lhs.result.ull <<= result_to_u64(&rhs);
          return lhs;
      }
      break;
    case BOK_RShift:
      switch (lhs.result_kind) {
        case RK_Boolean:
          UNREACHABLE_MSG("Cannot shift boolean");
        case RK_Int:
          assert(sizeof(int) * 8 > result_to_u64(&rhs));
          lhs.result.i >>= result_to_u64(&rhs);
          return lhs;
        case RK_UnsignedLongLong:
          assert(sizeof(unsigned long long) * 8 > result_to_u64(&rhs));
          lhs.result.ull >>= result_to_u64(&rhs);
          return lhs;
      }
      break;
    case BOK_BitwiseOr:
      assert(lhs.result_kind == rhs.result_kind);
      switch (lhs.result_kind) {
        case RK_Boolean:
          lhs.result.b |= rhs.result.b;
          return lhs;
        case RK_Int:
          lhs.result.i |= rhs.result.i;
          return lhs;
        case RK_UnsignedLongLong:
          lhs.result.ull |= rhs.result.ull;
          return lhs;
      }
      break;
    case BOK_Lt: {
      assert(lhs.result_kind == rhs.result_kind);
      switch (lhs.result_kind) {
        case RK_Boolean:
          UNREACHABLE_MSG("TODO: Check if booleans can be compared %zu:%zu",
                          expr->expr.line, expr->expr.col);
        case RK_Int:
          lhs.result.b = lhs.result.i < rhs.result.i;
          lhs.result_kind = RK_Boolean;
          return lhs;
        case RK_UnsignedLongLong:
          lhs.result.b = lhs.result.ull < rhs.result.ull;
          lhs.result_kind = RK_Boolean;
          return lhs;
      }
    }
    default:
      UNREACHABLE_MSG(
          "TODO: Implement constant evaluation for the remaining binops %d",
          expr->op);
  }
}

ConstExprResult sema_eval_alignof(Sema* sema, const AlignOf* ao) {
  const Type* type;
  if (ao->is_expr) {
    type = sema_get_type_of_expr(sema, (const Expr*)ao->expr_or_type);
  } else {
    type = (const Type*)ao->expr_or_type;
  }

  size_t size = sema_eval_alignof_type(sema, type);
  ConstExprResult res;
  res.result_kind = RK_UnsignedLongLong;
  res.result.ull = size;
  return res;
}

ConstExprResult sema_eval_sizeof(Sema* sema, const SizeOf* so) {
  const Type* type;
  if (so->is_expr) {
    type = sema_get_type_of_expr(sema, (const Expr*)so->expr_or_type);
  } else {
    type = (const Type*)so->expr_or_type;
  }

  size_t size = sema_eval_sizeof_type(sema, type);
  ConstExprResult res;
  res.result_kind = RK_UnsignedLongLong;
  res.result.ull = size;
  return res;
}

ConstExprResult sema_eval_expr(Sema* sema, const Expr* expr) {
  switch (expr->vtable->kind) {
    case EK_BinOp:
      return sema_eval_binop(sema, (const BinOp*)expr);
    case EK_SizeOf:
      return sema_eval_sizeof(sema, (const SizeOf*)expr);
    case EK_Int: {
      ConstExprResult res;
      res.result_kind = RK_Int;
      res.result.i = (int)((const Int*)expr)->val;
      return res;
    }
    case EK_UnOp: {
      const UnOp* unop = (const UnOp*)expr;
      ConstExprResult sub_res = sema_eval_expr(sema, unop->subexpr);
      switch (unop->op) {
        case UOK_Negate:
          switch (sub_res.result_kind) {
            case RK_Boolean:
              UNREACHABLE_MSG("Cannot negate boolean");
            case RK_Int:
              sub_res.result.i = -sub_res.result.i;
              return sub_res;
            case RK_UnsignedLongLong:
              sub_res.result.ull = -sub_res.result.ull;
              return sub_res;
          }
          break;
        default:
          UNREACHABLE_MSG("TODO: Handle constexpr eval for other unop kinds %d",
                          unop->op);
      }
    }
    case EK_DeclRef: {
      const DeclRef* decl = (const DeclRef*)expr;
      void* val;
      if (tree_map_get(&sema->enum_values, decl->name, &val)) {
        ConstExprResult res;
        res.result_kind = RK_Int;
        res.result.i = (int)(intptr_t)val;
        return res;
      }

      if (tree_map_get(&sema->globals, decl->name, &val)) {
        assert(((TopLevelNode*)val)->vtable->kind == TLNK_GlobalVariable);
        const GlobalVariable* gv = val;
        assert(gv->initializer);
        return sema_eval_expr(sema, gv->initializer);
      }

      UNREACHABLE_MSG("Unknown global '%s'", decl->name);
    }
    case EK_Conditional: {
      const Conditional* conditional = (const Conditional*)expr;
      ConstExprResult cond_res = sema_eval_expr(sema, conditional->cond);
      assert(cond_res.result_kind == RK_Boolean);
      return sema_eval_expr(sema, cond_res.result.b ? conditional->true_expr
                                                    : conditional->false_expr);
    }
    default:
      UNREACHABLE_MSG(
          "TODO: Implement constant evaluation for the remaining "
          "expressions (%d)",
          expr->vtable->kind);
  }
}

void sema_verify_static_assert_condition(Sema* sema, const Expr* cond) {
  ConstExprResult res = sema_eval_expr(sema, cond);
  switch (res.result_kind) {
    case RK_Boolean:
      if (res.result.b)
        return;
      break;
    case RK_Int:
      if (res.result.i)
        return;
      break;
    case RK_UnsignedLongLong:
      if (res.result.ull)
        return;
      break;
  }
  UNREACHABLE_MSG("static_assert failed");
}
