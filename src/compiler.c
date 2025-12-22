#include <assert.h>
#include <ctype.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/DebugInfo.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argparse.h"
#include "ast-dump.h"
#include "common.h"
#include "cstring.h"
#include "expr.h"
#include "ifstream.h"
#include "istream.h"
#include "lexer.h"
#include "parser.h"
#include "path.h"
#include "preprocessor.h"
#include "sema.h"
#include "sstream.h"
#include "stmt.h"
#include "tree-map.h"
#include "type.h"
#include "vector.h"

// LLVM does not actually have a way of providing the CHAR_BIT for the arch.
// Clang unconditionally sets it to 8.
//
// See:
// https://github.com/llvm/llvm-project/blob/8fb748b4a7c45b56c2e4f37c327fd88958ab3758/clang/lib/Frontend/InitPreprocessor.cpp#L1103
// https://github.com/llvm/llvm-project/blob/8fb748b4a7c45b56c2e4f37c327fd88958ab3758/clang/include/clang/Basic/TargetInfo.h#L502
//
// This is probably fine since AFAICT, upstream clang doesn't have any backends
// where a byte is not 8 bits. If there was, that would be pretty interesting.
static const int kCharBit = 8;

///
/// Start Compiler Implementation
///

typedef struct {
  // The compiler does not own the Sema or the module. It only modifies them.
  LLVMModuleRef mod;
  Sema* sema;
  LLVMDIBuilderRef dibuilder;
  LLVMMetadataRef dicu;
  LLVMMetadataRef difile;
} Compiler;

void compiler_construct(Compiler* compiler, LLVMModuleRef mod, Sema* sema,
                        LLVMDIBuilderRef dibuilder) {
  compiler->mod = mod;
  compiler->sema = sema;
  compiler->dibuilder = dibuilder;
  size_t len;
  const char* name = LLVMGetSourceFileName(mod, &len);
  compiler->difile = LLVMDIBuilderCreateFile(dibuilder, name, len, "", 0);
  compiler->dicu = LLVMDIBuilderCreateCompileUnit(
      dibuilder, LLVMDWARFSourceLanguageC, compiler->difile,
      /*Producer=*/"", /*ProducerLen=*/0, /*IsOptimized=*/0,
      /*Flags=*/"", /*FlagsLen=*/0, /*RuntimeVer=*/0, /*SplitName=*/"",
      /*SplitNameLen=*/0, LLVMDWARFEmissionFull, /*DWOId=*/0,
      /*SplitDebugInlining=*/0, /*DebugInfoForProfiling=*/0, /*SysRoot=*/"",
      /*SysRootLen=*/0, /*SDK=*/"", /*SDKLen=*/0);
}

void compiler_destroy(Compiler* compiler) {}

LLVMTypeRef get_llvm_type(Compiler* compiler, const Type* type,
                          const TreeMap* local_ctx);

LLVMTypeRef get_llvm_struct_type(Compiler* compiler, const StructType* st,
                                 const TreeMap* local_ctx) {
  st = sema_resolve_struct_type(compiler->sema, st);

  void* llvm_struct;

  // First see if we already made one.
  if (st->name) {
    if ((llvm_struct = LLVMGetTypeByName(compiler->mod, st->name)))
      return llvm_struct;
  }

  assert(st->members);

  // Doesn't exits. Create the struct body.
  vector elems;
  vector_construct(&elems, sizeof(LLVMTypeRef), alignof(LLVMTypeRef));
  for (size_t i = 0; i < st->members->size; ++i) {
    Member* member = vector_at(st->members, i);
    LLVMTypeRef* storage = vector_append_storage(&elems);
    *storage = get_llvm_type(compiler, member->type, local_ctx);
  }

  if (st->name) {
    llvm_struct =
        LLVMStructCreateNamed(LLVMGetModuleContext(compiler->mod), st->name);
    LLVMStructSetBody(llvm_struct, elems.data, (unsigned)elems.size,
                      st->packed);
  } else {
    llvm_struct = LLVMStructType(elems.data, (unsigned)elems.size, st->packed);
  }

  vector_destroy(&elems);

  return llvm_struct;
}

LLVMTypeRef get_llvm_union_type(Compiler* compiler, const UnionType* ut,
                                const TreeMap* local_ctx) {
  ut = sema_resolve_union_type(compiler->sema, ut);

  void* llvm_union;

  // First see if we already made one.
  if (ut->name) {
    if ((llvm_union = LLVMGetTypeByName(compiler->mod, ut->name)))
      return llvm_union;
  }

  assert(ut->members);

  // Doesn't exits. Create the struct body which only consists of the largest
  // union member.
  LLVMTypeRef member = get_llvm_type(
      compiler,
      sema_get_largest_union_member(compiler->sema, ut, local_ctx)->type,
      local_ctx);

  if (ut->name) {
    llvm_union =
        LLVMStructCreateNamed(LLVMGetModuleContext(compiler->mod), ut->name);
    LLVMStructSetBody(llvm_union, &member, /*ElementCount=*/1, ut->packed);
  } else {
    llvm_union = LLVMStructType(&member, /*ElementCount=*/1, ut->packed);
  }

  return llvm_union;
}

LLVMTypeRef get_opaque_ptr(Compiler* compiler) {
  return LLVMPointerTypeInContext(LLVMGetModuleContext(compiler->mod),
                                  /*AddressSpace=*/0);
}

LLVMTypeRef get_llvm_array_type(Compiler* compiler, const ArrayType* at,
                                const TreeMap* local_ctx) {
  if (!at->size)
    return get_opaque_ptr(compiler);

  ConstExprResult size =
      sema_eval_expr_in_ctx(compiler->sema, at->size, local_ctx);
  LLVMTypeRef elem = get_llvm_type(compiler, at->elem_type, local_ctx);
  switch (size.result_kind) {
    case RK_UnsignedLongLong:
      return LLVMArrayType(elem, (unsigned)size.result.ull);
    case RK_Int:
      return LLVMArrayType(elem, (unsigned)size.result.i);
    case RK_Boolean:
      return LLVMArrayType(elem, (unsigned)size.result.b);
  }
}

LLVMTypeRef get_llvm_function_type(Compiler* compiler, const FunctionType* ft,
                                   const TreeMap* local_ctx) {
  LLVMTypeRef ret = get_llvm_type(compiler, ft->return_type, local_ctx);

  vector params;
  vector_construct(&params, sizeof(LLVMTypeRef), alignof(LLVMTypeRef));
  for (size_t i = 0; i < ft->pos_args.size; ++i) {
    Type* arg_ty = ((FunctionArg*)vector_at(&ft->pos_args, i))->type;
    if (is_void_type(arg_ty))
      continue;
    LLVMTypeRef* storage = vector_append_storage(&params);
    *storage = get_llvm_type(compiler, arg_ty, local_ctx);
  }

  LLVMTypeRef res = LLVMFunctionType(ret, params.data, (unsigned)params.size,
                                     ft->has_var_args);

  vector_destroy(&params);

  return res;
}

LLVMTypeRef get_llvm_builtin_type(Compiler* compiler, const BuiltinType* bt);

static LLVMTypeRef get_builtin_va_list(Compiler* compiler) {
  // TODO: This is a target-dependent type. It should have different
  // implementations per target.
  //
  // https://refspecs.linuxbase.org/elf/x86_64-abi-0.21.pdf
  //
  // On x86-64, this is
  //
  //   typedef struct {
  //     unsigned int gp_offset;
  //     unsigned int fp_offset;
  //     void *overflow_arg_area;
  //     void *reg_save_area;
  //   } va_list[1];
  //
  LLVMTypeRef ui =
      get_llvm_builtin_type(compiler, &compiler->sema->bt_UnsignedInt);
  LLVMTypeRef vp = get_opaque_ptr(compiler);
  LLVMTypeRef elems[] = {ui, ui, vp, vp};
  return LLVMStructType(elems, /*ElementCount=*/4, /*Packed=*/0);
}

LLVMTypeRef get_llvm_builtin_type(Compiler* compiler, const BuiltinType* bt) {
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);
  switch (bt->kind) {
    case BTK_Char:
    case BTK_SignedChar:
    case BTK_UnsignedChar:
    case BTK_Short:
    case BTK_UnsignedShort:
    case BTK_Int:
    case BTK_UnsignedInt:
    case BTK_Long:
    case BTK_UnsignedLong:
    case BTK_LongLong:
    case BTK_UnsignedLongLong: {
      size_t size = builtin_type_get_size(bt);
      assert(size);
      return LLVMIntType((unsigned)(size * kCharBit));
    }
    case BTK_Float:
      return LLVMFloatTypeInContext(ctx);
    case BTK_Double:
      return LLVMDoubleTypeInContext(ctx);
    case BTK_LongDouble:
    case BTK_Float128:
      return LLVMFP128TypeInContext(ctx);
    case BTK_Void:
      return LLVMVoidType();
    case BTK_Bool:
      return LLVMIntType(kCharBit);
    case BTK_ComplexFloat: {
      LLVMTypeRef elems[] = {LLVMFloatTypeInContext(ctx),
                             LLVMFloatTypeInContext(ctx)};
      return LLVMStructType(elems, /*ElementCount=*/2, /*Packed=*/0);
    }
    case BTK_ComplexDouble: {
      LLVMTypeRef elems[] = {LLVMDoubleTypeInContext(ctx),
                             LLVMDoubleTypeInContext(ctx)};
      return LLVMStructType(elems, /*ElementCount=*/2, /*Packed=*/0);
    }
    case BTK_ComplexLongDouble: {
      LLVMTypeRef elems[] = {LLVMFP128TypeInContext(ctx),
                             LLVMFP128TypeInContext(ctx)};
      return LLVMStructType(elems, /*ElementCount=*/2, /*Packed=*/0);
    }
    case BTK_BuiltinVAList:
      return get_builtin_va_list(compiler);
  }
}

LLVMTypeRef get_llvm_type(Compiler* compiler, const Type* type,
                          const TreeMap* local_ctx) {
  switch (type->vtable->kind) {
    case TK_BuiltinType:
      return get_llvm_builtin_type(compiler, (const BuiltinType*)type);
    case TK_EnumType: {
      const BuiltinType* repr = sema_get_integral_type_for_enum(
          compiler->sema, (const EnumType*)type);
      return get_llvm_type(compiler, &repr->type, local_ctx);
    }
    case TK_NamedType:
      return get_llvm_type(
          compiler,
          sema_resolve_named_type(compiler->sema, (const NamedType*)type),
          local_ctx);
    case TK_StructType:
      return get_llvm_struct_type(compiler, (const StructType*)type, local_ctx);
    case TK_UnionType:
      return get_llvm_union_type(compiler, (const UnionType*)type, local_ctx);
    case TK_PointerType:
      return get_opaque_ptr(compiler);
    case TK_ArrayType:
      return get_llvm_array_type(compiler, (const ArrayType*)type, local_ctx);
    case TK_FunctionType:
      return get_llvm_function_type(compiler, (const FunctionType*)type,
                                    local_ctx);
    case TK_ReplacementSentinelType:
      UNREACHABLE_MSG("This type should not be handled here.");
  }
}

LLVMValueRef compile_expr(Compiler* compiler, LLVMBuilderRef builder,
                          const Expr* expr, TreeMap* local_ctx,
                          TreeMap* local_allocas, LLVMBasicBlockRef break_bb,
                          LLVMBasicBlockRef cont_bb);

LLVMTypeRef get_llvm_type_of_expr_global_ctx(Compiler* compiler,
                                             const Expr* expr);

LLVMValueRef maybe_compile_constant_implicit_cast(Compiler* compiler,
                                                  const Expr* expr,
                                                  const Type* to_ty,
                                                  const TreeMap* local_ctx);

LLVMValueRef get_named_global(Compiler* compiler, const char* name) {
  // If not in the local scope, check the global scope.
  //
  // TODO: How come there's no wrapper for Module::getNamedValue?
  // Without it, I need to explicitly call the individual getters for
  // functions, global variables, adn aliases.
  LLVMValueRef val = LLVMGetNamedGlobal(compiler->mod, name);

  if (!val)
    val = LLVMGetNamedFunction(compiler->mod, name);

  if (!val) {
    // Maybe an enum?
    void* res;
    if (tree_map_get(&compiler->sema->enum_values, name, &res)) {
      EnumType* enum_ty = NULL;
      tree_map_get(&compiler->sema->enum_names, name, (void*)&enum_ty);
      assert(enum_ty);

      TreeMap dummy_ctx;
      string_tree_map_construct(&dummy_ctx);
      LLVMTypeRef llvm_ty = get_llvm_type(compiler, &enum_ty->type, &dummy_ctx);
      tree_map_destroy(&dummy_ctx);

      return LLVMConstInt(llvm_ty, (unsigned long long)res,
                          /*IsSigned=*/1);
    }
  }

  return val;
}

LLVMTypeRef get_llvm_type_of_expr(Compiler* compiler, const Expr* expr,
                                  const TreeMap* local_ctx);

LLVMValueRef compile_constant_expr(Compiler* compiler, const Expr* expr,
                                   const Type* to_ty,
                                   const TreeMap* local_ctx) {
  switch (expr->vtable->kind) {
    case EK_DeclRef: {
      const DeclRef* decl = (const DeclRef*)expr;
      LLVMValueRef glob = get_named_global(compiler, decl->name);
      assert(glob);
      return glob;
    }
    case EK_SizeOf: {
      ConstExprResult res = sema_eval_sizeof_in_ctx(
          compiler->sema, (const SizeOf*)expr, local_ctx);
      assert(res.result_kind == RK_UnsignedLongLong);
      LLVMTypeRef llvm_type = get_llvm_type_of_expr_global_ctx(compiler, expr);
      return LLVMConstInt(llvm_type, res.result.ull, /*IsSigned=*/0);
    }
    case EK_String: {
      const StringLiteral* s = (const StringLiteral*)expr;
      LLVMValueRef seq = LLVMConstString(s->val, (unsigned)strlen(s->val),
                                         /*DontNullTerminate=*/false);
      if (is_array_type(to_ty))
        return seq;

      // Need to construct a string pointer manually.
      LLVMValueRef glob = LLVMAddGlobal(compiler->mod, LLVMTypeOf(seq), "");
      LLVMSetInitializer(glob, seq);
      return glob;
    }
    case EK_Char: {
      const Type* type =
          sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);
      assert(type->vtable->kind == TK_BuiltinType);
      bool is_signed = !is_unsigned_integral_type(type);
      LLVMTypeRef llvm_type = get_llvm_type_of_expr_global_ctx(compiler, expr);
      return LLVMConstInt(llvm_type, (unsigned)((const Char*)expr)->val,
                          is_signed);
    }
    case EK_Int: {
      const Type* type =
          sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);
      assert(type->vtable->kind == TK_BuiltinType);
      bool is_signed = !is_unsigned_integral_type(type);
      LLVMTypeRef llvm_type = get_llvm_type_of_expr_global_ctx(compiler, expr);
      return LLVMConstInt(llvm_type, ((const Int*)expr)->val, is_signed);
    }
    case EK_BinOp: {
      LLVMTypeRef llvm_type = get_llvm_type_of_expr(compiler, expr, local_ctx);
      ConstExprResult res =
          sema_eval_expr_in_ctx(compiler->sema, expr, local_ctx);
      switch (res.result_kind) {
        case RK_Boolean:
          return LLVMConstInt(llvm_type, res.result.b, /*IsSigned=*/0);
        case RK_Int:
          return LLVMConstInt(llvm_type, (unsigned long long)res.result.i,
                              /*IsSigned=*/1);
        case RK_UnsignedLongLong:
          return LLVMConstInt(llvm_type, res.result.ull, /*IsSigned=*/0);
      }
    }
    case EK_Cast: {
      const Cast* cast = (const Cast*)expr;
      const Type* from_ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, cast->base, local_ctx);
      const Type* to_ty = cast->to;

      LLVMValueRef from =
          compile_constant_expr(compiler, cast->base, to_ty, local_ctx);

      if (sema_types_are_compatible(compiler->sema, from_ty, to_ty, local_ctx))
        return from;

      if (is_integral_type(from_ty) && is_pointer_type(to_ty))
        return LLVMConstIntToPtr(from,
                                 get_llvm_type(compiler, to_ty, local_ctx));

      UNREACHABLE_MSG("TODO: Unhandled constant cast conversion");
    }
    case EK_InitializerList: {
      const InitializerList* init = (const InitializerList*)expr;

      LLVMValueRef res;
      bool is_array_ty = sema_is_array_type(compiler->sema, to_ty, local_ctx);

      LLVMValueRef* constants = malloc(sizeof(LLVMValueRef) * init->elems.size);
      for (size_t i = 0; i < init->elems.size; ++i) {
        const InitializerListElem* elem = vector_at(&init->elems, i);
        if (elem->expr->vtable->kind == EK_InitializerList) {
          if (is_array_ty) {
            const ArrayType* arr_ty =
                sema_get_array_type(compiler->sema, to_ty, local_ctx);
            constants[i] = maybe_compile_constant_implicit_cast(
                compiler, elem->expr, arr_ty->elem_type, local_ctx);
          } else {
            const StructType* struct_ty =
                sema_get_struct_type(compiler->sema, to_ty, local_ctx);
            const Member* member_ty = struct_get_nth_member(struct_ty, i);
            constants[i] = maybe_compile_constant_implicit_cast(
                compiler, elem->expr, member_ty->type, local_ctx);
          }
        } else {
          const Type* elem_ty = sema_get_type_of_expr_in_ctx(
              compiler->sema, elem->expr, local_ctx);
          constants[i] = maybe_compile_constant_implicit_cast(
              compiler, elem->expr, elem_ty, local_ctx);
        }
      }

      if (is_array_ty) {
        const ArrayType* arr_ty =
            sema_get_array_type(compiler->sema, to_ty, local_ctx);
        res = LLVMConstArray(
            get_llvm_type(compiler, arr_ty->elem_type, local_ctx), constants,
            (unsigned)init->elems.size);
      } else {
        assert(sema_is_struct_type(compiler->sema, to_ty, local_ctx));
        res = LLVMConstStruct(constants, (unsigned)init->elems.size,
                              /*Packed=*/0);
      }
      free(constants);

      return res;
    }
    case EK_UnOp: {
      const UnOp* unop = (const UnOp*)expr;
      switch (unop->op) {
        case UOK_AddrOf:
          return compile_constant_expr(compiler, unop->subexpr, to_ty,
                                       local_ctx);
        default:
          UNREACHABLE_MSG(
              "TODO: Implement IR constant expr evaluation for unop expr op %d",
              unop->op);
      }
    }
    default:
      UNREACHABLE_MSG(
          "TODO: Implement IR constant expr evaluation for expr kind %d",
          expr->vtable->kind);
  }
}

LLVMValueRef maybe_compile_constant_implicit_cast(Compiler* compiler,
                                                  const Expr* expr,
                                                  const Type* to_ty,
                                                  const TreeMap* local_ctx) {
  LLVMValueRef from = compile_constant_expr(compiler, expr, to_ty, local_ctx);

  // We cannot disambiguate if an initializer list is for an array or struct.
  // They should not be treated like normal expressions, so just use the
  // destination type in this case.
  const Type* from_ty =
      expr->vtable->kind == EK_InitializerList
          ? to_ty
          : sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);

  from_ty = sema_resolve_maybe_named_type(compiler->sema, from_ty);
  to_ty = sema_resolve_maybe_named_type(compiler->sema, to_ty);

  if (sema_types_are_compatible_ignore_quals(compiler->sema, from_ty, to_ty,
                                             local_ctx))
    return from;

  if (from_ty->vtable->kind == TK_EnumType) {
    from_ty = &sema_get_integral_type_for_enum(compiler->sema,
                                               (const EnumType*)from_ty)
                   ->type;
  }
  if (to_ty->vtable->kind == TK_EnumType) {
    to_ty =
        &sema_get_integral_type_for_enum(compiler->sema, (const EnumType*)to_ty)
             ->type;
  }

  if (is_integral_type(from_ty) && is_integral_type(to_ty)) {
    unsigned long long from_val = LLVMConstIntGetZExtValue(from);
    LLVMTypeRef llvm_to_ty = get_llvm_type(compiler, to_ty, local_ctx);
    bool is_signed = !is_unsigned_integral_type(to_ty);
    return LLVMConstInt(llvm_to_ty, from_val, is_signed);
  }

  UNREACHABLE_MSG(
      "TODO: Unhandled implicit constant cast conversion:\n"
      "lhs: %d %d (%s) %p\n"
      "rhs: %d %d (%s) %p\n",
      from_ty->vtable->kind, from_ty->qualifiers,
      (from_ty->vtable->kind == TK_StructType
           ? ((const StructType*)from_ty)->name
           : ""),
      from_ty, to_ty->vtable->kind, to_ty->qualifiers,
      (to_ty->vtable->kind == TK_StructType ? ((const StructType*)to_ty)->name
                                            : ""),
      to_ty);
}

static bool should_use_icmp(LLVMTypeRef type) {
  switch (LLVMGetTypeKind(type)) {
    case LLVMIntegerTypeKind:
    case LLVMPointerTypeKind:
      return true;
    default:
      return false;
  }
}

// NOTE: This always returns an i1.
LLVMValueRef compile_to_bool(Compiler* compiler, LLVMBuilderRef builder,
                             const Expr* expr, TreeMap* local_ctx,
                             TreeMap* local_allocas, LLVMBasicBlockRef break_bb,
                             LLVMBasicBlockRef cont_bb) {
  const Type* type =
      sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);

  if (type->vtable->kind == TK_NamedType)
    type = sema_resolve_named_type(compiler->sema, (const NamedType*)type);

  switch (type->vtable->kind) {
    case TK_BuiltinType:
    case TK_PointerType:
    case TK_EnumType:
    case TK_ArrayType: {
      // Easiest (but not always correct) way is to compare against zero.
      LLVMValueRef val = compile_expr(compiler, builder, expr, local_ctx,
                                      local_allocas, break_bb, cont_bb);
      LLVMTypeRef type = LLVMTypeOf(val);
      LLVMValueRef zero = LLVMConstNull(type);
      if (should_use_icmp(type)) {
        return LLVMBuildICmp(builder, LLVMIntNE, val, zero, "to_bool");
      } else {
        return LLVMBuildFCmp(builder, LLVMRealUNE, val, zero, "to_bool");
      }
    }
    case TK_StructType:
    case TK_UnionType:
    case TK_NamedType:
    case TK_FunctionType:
    case TK_ReplacementSentinelType:
      UNREACHABLE_MSG("Cannot convert this type to bool %d",
                      type->vtable->kind);
  }
}

LLVMValueRef compile_lvalue_ptr(Compiler* compiler, LLVMBuilderRef builder,
                                const Expr* expr, TreeMap* local_ctx,
                                TreeMap* local_allocas,
                                LLVMBasicBlockRef break_bb,
                                LLVMBasicBlockRef cont_bb);

LLVMValueRef compile_unop_lvalue_ptr(Compiler* compiler, LLVMBuilderRef builder,
                                     const UnOp* expr, TreeMap* local_ctx,
                                     TreeMap* local_allocas,
                                     LLVMBasicBlockRef break_bb,
                                     LLVMBasicBlockRef cont_bb) {
  switch (expr->op) {
    case UOK_Deref: {
      return compile_expr(compiler, builder, expr->subexpr, local_ctx,
                          local_allocas, break_bb, cont_bb);
    }
    default:
      UNREACHABLE_MSG("TODO: Implement compile_unop_lvalue_ptr for this op %d",
                      expr->op);
  }
}

unsigned get_llvm_ptr_size_in_bytes(const Compiler* compiler) {
  LLVMTargetDataRef data_layout = LLVMGetModuleDataLayout(compiler->mod);
  unsigned size = LLVMPointerSize(data_layout);
  assert(size);
  return size;
}

unsigned get_llvm_ptr_size_in_bits(const Compiler* compiler) {
  return get_llvm_ptr_size_in_bytes(compiler) * kCharBit;
}

LLVMTypeRef get_llvm_ptr_as_int(const Compiler* compiler) {
  return LLVMIntType(get_llvm_ptr_size_in_bits(compiler));
}

// This is a wrapper for whenever we would load/store an LLVMValueRef but we
// need to take into account alignment of the original type.
static LLVMValueRef get_aligned_load(Compiler* compiler, LLVMBuilderRef builder,
                                     const Type* type, LLVMValueRef ptr,
                                     const char* name,
                                     const TreeMap* local_ctx) {
  LLVMTypeRef llvm_type = get_llvm_type(compiler, type, local_ctx);
  LLVMValueRef load = LLVMBuildLoad2(builder, llvm_type, ptr, name);

  if (type->align) {
    size_t alignment = sema_eval_alignof_type(compiler->sema, type, local_ctx);
    LLVMSetAlignment(load, (unsigned)alignment);
  }

  return load;
}

static void get_aligned_store(Compiler* compiler, LLVMBuilderRef builder,
                              const Type* type, LLVMValueRef val,
                              LLVMValueRef ptr, const TreeMap* local_ctx) {
  LLVMValueRef store = LLVMBuildStore(builder, val, ptr);

  if (type->align) {
    size_t alignment = sema_eval_alignof_type(compiler->sema, type, local_ctx);
    LLVMSetAlignment(store, (unsigned)alignment);
  }
}

static LLVMValueRef get_aligned_alloca(Compiler* compiler,
                                       LLVMBuilderRef builder, const Type* type,
                                       const char* name,
                                       const TreeMap* local_ctx) {
  LLVMTypeRef llvm_type = get_llvm_type(compiler, type, local_ctx);
  LLVMValueRef alloca = LLVMBuildAlloca(builder, llvm_type, name);

  if (type->align) {
    size_t alignment = sema_eval_alignof_type(compiler->sema, type, local_ctx);
    LLVMSetAlignment(alloca, (unsigned)alignment);
  }

  return alloca;
}

LLVMValueRef compile_unop(Compiler* compiler, LLVMBuilderRef builder,
                          const UnOp* expr, TreeMap* local_ctx,
                          TreeMap* local_allocas, LLVMBasicBlockRef break_bb,
                          LLVMBasicBlockRef cont_bb) {
  switch (expr->op) {
    case UOK_Not: {
      // Result is always an i1.
      LLVMValueRef to_bool =
          compile_to_bool(compiler, builder, expr->subexpr, local_ctx,
                          local_allocas, break_bb, cont_bb);
      LLVMValueRef zero = LLVMConstNull(LLVMInt1Type());
      LLVMValueRef res =
          LLVMBuildICmp(builder, LLVMIntEQ, to_bool, zero, "not");
      return LLVMBuildZExt(builder, res, LLVMInt8Type(), "");
    }
    case UOK_BitNot: {
      LLVMValueRef val =
          compile_expr(compiler, builder, expr->subexpr, local_ctx,
                       local_allocas, break_bb, cont_bb);
      LLVMValueRef ones = LLVMConstAllOnes(LLVMTypeOf(val));
      return LLVMBuildXor(builder, val, ones, "");
    }
    case UOK_PreDec:
    case UOK_PostDec:
    case UOK_PreInc:
    case UOK_PostInc: {
      bool is_pre = expr->op == UOK_PreInc || expr->op == UOK_PreDec;
      bool is_inc = expr->op == UOK_PreInc || expr->op == UOK_PostInc;
      LLVMValueRef ptr =
          compile_lvalue_ptr(compiler, builder, expr->subexpr, local_ctx,
                             local_allocas, break_bb, cont_bb);
      const Type* type = sema_get_type_of_expr_in_ctx(compiler->sema,
                                                      expr->subexpr, local_ctx);
      LLVMTypeRef llvm_type = get_llvm_type(compiler, type, local_ctx);
      LLVMValueRef val =
          get_aligned_load(compiler, builder, type, ptr, "", local_ctx);

      LLVMValueRef one;
      if (LLVMGetTypeKind(llvm_type) == LLVMPointerTypeKind) {
        const Type* pointee = sema_get_pointee(compiler->sema, type, local_ctx);
        size_t pointee_size =
            sema_eval_sizeof_type(compiler->sema, pointee, local_ctx);

        LLVMTypeRef int_ty = get_llvm_ptr_as_int(compiler);
        one = LLVMConstInt(int_ty, pointee_size,
                           /*signed=*/0);
        val = LLVMBuildPtrToInt(builder, val, int_ty, "");
      } else {
        one = LLVMConstInt(llvm_type, 1, /*signed=*/0);
      }

      LLVMValueRef postop = is_inc ? LLVMBuildAdd(builder, val, one, "")
                                   : LLVMBuildSub(builder, val, one, "");

      if (LLVMGetTypeKind(llvm_type) == LLVMPointerTypeKind)
        postop = LLVMBuildIntToPtr(builder, postop, llvm_type, "");

      get_aligned_store(compiler, builder, type, postop, ptr, local_ctx);
      return is_pre ? postop : val;
    }
    case UOK_Negate: {
      LLVMValueRef val =
          compile_expr(compiler, builder, expr->subexpr, local_ctx,
                       local_allocas, break_bb, cont_bb);
      LLVMValueRef zero = LLVMConstNull(LLVMTypeOf(val));
      return LLVMBuildSub(builder, zero, val, "");
    }
    case UOK_AddrOf:
      return compile_lvalue_ptr(compiler, builder, expr->subexpr, local_ctx,
                                local_allocas, break_bb, cont_bb);
    case UOK_Deref: {
      LLVMValueRef ptr =
          compile_expr(compiler, builder, expr->subexpr, local_ctx,
                       local_allocas, break_bb, cont_bb);
      return get_aligned_load(
          compiler, builder,
          sema_get_type_of_expr_in_ctx(compiler->sema, &expr->expr, local_ctx),
          ptr, "", local_ctx);
    }
    default:
      UNREACHABLE_MSG("TODO: Implement compile_unop for this op %d", expr->op);
  }
}

LLVMValueRef compile_implicit_cast(Compiler* compiler, LLVMBuilderRef builder,
                                   const Expr* from, const Type* to,
                                   TreeMap* local_ctx, TreeMap* local_allocas,
                                   LLVMBasicBlockRef break_bb,
                                   LLVMBasicBlockRef cont_bb) {
  to = sema_resolve_maybe_named_type(compiler->sema, to);
  if (is_builtin_type(to, BTK_Bool)) {
    LLVMValueRef res = compile_to_bool(compiler, builder, from, local_ctx,
                                       local_allocas, break_bb, cont_bb);
    return LLVMBuildZExt(builder, res, LLVMInt8Type(), "");
  }

  const Type* from_ty =
      sema_get_type_of_expr_in_ctx(compiler->sema, from, local_ctx);

  LLVMValueRef llvm_from;
  if (sema_is_array_type(compiler->sema, from_ty, local_ctx)) {
    // For an array type specifically, do not use compile_expr since it will
    // load the array into an llvm array but we want to keep it as a pointer.
    //
    // TODO: Maybe come up with a better way for this rather than having this
    // one check here.
    llvm_from = compile_lvalue_ptr(compiler, builder, from, local_ctx,
                                   local_allocas, break_bb, cont_bb);
  } else {
    llvm_from = compile_expr(compiler, builder, from, local_ctx, local_allocas,
                             break_bb, cont_bb);
  }

  if (sema_types_are_compatible(compiler->sema, from_ty, to, local_ctx))
    return llvm_from;

  LLVMTypeRef llvm_to_ty = get_llvm_type(compiler, to, local_ctx);
  LLVMTypeRef llvm_from_ty = LLVMTypeOf(llvm_from);

  if (LLVMGetTypeKind(llvm_from_ty) == LLVMPointerTypeKind) {
    if (LLVMGetTypeKind(llvm_to_ty) == LLVMPointerTypeKind)
      return llvm_from;

    if (LLVMGetTypeKind(llvm_to_ty) == LLVMIntegerTypeKind) {
      // TODO: Check the size.
      // unsigned int_width = LLVMGetIntTypeWidth(llvm_from_ty);
      return LLVMBuildPtrToInt(builder, llvm_from, llvm_to_ty, "");
    }
  }

  if (LLVMGetTypeKind(llvm_from_ty) == LLVMIntegerTypeKind) {
    if (LLVMGetTypeKind(llvm_to_ty) == LLVMIntegerTypeKind) {
      unsigned from_size = LLVMGetIntTypeWidth(llvm_from_ty);
      unsigned to_size = LLVMGetIntTypeWidth(llvm_to_ty);
      if (from_size == to_size)
        return llvm_from;
      else if (from_size > to_size)
        return LLVMBuildTrunc(builder, llvm_from, llvm_to_ty, "");
      else if (is_unsigned_integral_type(from_ty))
        return LLVMBuildZExt(builder, llvm_from, llvm_to_ty, "");
      else
        return LLVMBuildSExt(builder, llvm_from, llvm_to_ty, "");
    }

    if (LLVMGetTypeKind(llvm_to_ty) == LLVMPointerTypeKind) {
      unsigned from_size = LLVMGetIntTypeWidth(llvm_from_ty);
      assert(from_size);

      unsigned ptr_size = get_llvm_ptr_size_in_bits(compiler);

      assert(from_size <= ptr_size);

      if (from_size < ptr_size)
        llvm_from =
            LLVMBuildZExt(builder, llvm_from, LLVMIntType(ptr_size), "");

      return LLVMBuildIntToPtr(builder, llvm_from, llvm_to_ty, "");
    }
  }

  // If type-name is void, then expression is evaluated for its side-effects and
  // its returned value is discarded, same as when expression is used on its
  // own, as an expression statement.
  if (is_builtin_type(to, BTK_Void))
    return compile_expr(compiler, builder, from, local_ctx, local_allocas,
                        break_bb, cont_bb);

  printf("TODO: Handle this implicit cast for expression at %s:%zu:%zu\n",
         source_location_filename(&from->loc), source_location_line(&from->loc),
         source_location_col(&from->loc));
  LLVMDumpValue(llvm_from);
  printf("\n");
  LLVMDumpType(llvm_to_ty);
  printf("\n");
  printf("function:\n");
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMDumpValue(fn);
  __builtin_trap();
}

// This creates an alloca in this function but ensures it's always at the start
// of the function. Having an alloca in the middle of the function can cause
// the stack pointer to keep decrementing if it's in a loop.
LLVMValueRef build_alloca_at_func_start(Compiler* compiler,
                                        LLVMBuilderRef builder,
                                        const char* name, const Type* type,
                                        const TreeMap* local_ctx) {
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMBasicBlockRef current_bb = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry_bb = LLVMGetEntryBasicBlock(fn);

  // Place builder at the start of the entry BB. If it's empty, place the
  // builder at the end of the bb.
  LLVMValueRef inst = LLVMGetFirstInstruction(entry_bb);
  if (inst)
    LLVMPositionBuilder(builder, entry_bb, inst);
  else
    LLVMPositionBuilderAtEnd(builder, entry_bb);

  LLVMValueRef alloca =
      get_aligned_alloca(compiler, builder, type, name, local_ctx);

  LLVMPositionBuilderAtEnd(builder, current_bb);

  return alloca;
}

LLVMValueRef compile_conditional(Compiler* compiler, LLVMBuilderRef builder,
                                 const Conditional* expr, TreeMap* local_ctx,
                                 TreeMap* local_allocas,
                                 LLVMBasicBlockRef break_bb,
                                 LLVMBasicBlockRef cont_bb) {
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);

  LLVMValueRef cond = compile_to_bool(compiler, builder, expr->cond, local_ctx,
                                      local_allocas, break_bb, cont_bb);

  assert(LLVMGetTypeKind(LLVMTypeOf(cond)) == LLVMIntegerTypeKind);
  assert(LLVMGetIntTypeWidth(LLVMTypeOf(cond)) == 1);

  // Create blocks for the then and else cases. Insert the 'if' block at the
  // end of the function.
  LLVMBasicBlockRef ifbb = LLVMAppendBasicBlockInContext(ctx, fn, "if");
  LLVMBasicBlockRef elsebb = LLVMCreateBasicBlockInContext(ctx, "else");
  LLVMBasicBlockRef mergebb = LLVMCreateBasicBlockInContext(ctx, "merge");

  LLVMBuildCondBr(builder, cond, ifbb, elsebb);

  // Emit if BB.
  LLVMPositionBuilderAtEnd(builder, ifbb);
  LLVMValueRef true_expr =
      compile_expr(compiler, builder, expr->true_expr, local_ctx, local_allocas,
                   break_bb, cont_bb);
  LLVMBuildBr(builder, mergebb);
  ifbb = LLVMGetInsertBlock(builder);  // Codegen of 'if' can change the current
                                       // block, update ifbb for the PHI.

  // Emit potential else BB.
  LLVMAppendExistingBasicBlock(fn, elsebb);
  LLVMPositionBuilderAtEnd(builder, elsebb);
  LLVMValueRef false_expr =
      compile_expr(compiler, builder, expr->false_expr, local_ctx,
                   local_allocas, break_bb, cont_bb);
  LLVMBuildBr(builder, mergebb);
  elsebb =
      LLVMGetInsertBlock(builder);  // Codegen of 'else' can change the current
                                    // block, update ifbb for the PHI.

  // Emit merge BB.
  LLVMAppendExistingBasicBlock(fn, mergebb);
  LLVMPositionBuilderAtEnd(builder, mergebb);

  const Type* common_ty = sema_get_common_arithmetic_type_of_exprs(
      compiler->sema, expr->true_expr, expr->false_expr, local_ctx);
  LLVMValueRef phi =
      LLVMBuildPhi(builder, get_llvm_type(compiler, common_ty, local_ctx), "");
  LLVMValueRef incoming_vals[] = {true_expr, false_expr};
  LLVMBasicBlockRef incoming_blocks[] = {ifbb, elsebb};
  LLVMAddIncoming(phi, incoming_vals, incoming_blocks, 2);
  return phi;
}

LLVMValueRef compile_logical_binop(Compiler* compiler, LLVMBuilderRef builder,
                                   const Expr* lhs, const Expr* rhs,
                                   BinOpKind op, TreeMap* local_ctx,
                                   TreeMap* local_allocas,
                                   LLVMBasicBlockRef break_bb,
                                   LLVMBasicBlockRef cont_bb) {
  // Account for short-circuit evaluation.
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);

  LLVMValueRef lhs_val = compile_to_bool(compiler, builder, lhs, local_ctx,
                                         local_allocas, break_bb, cont_bb);
  LLVMBasicBlockRef current_bb = LLVMGetInsertBlock(builder);

  LLVMBasicBlockRef eval_rhs_bb =
      LLVMAppendBasicBlockInContext(ctx, fn, "sc_rhs");
  LLVMBasicBlockRef res_bb = LLVMCreateBasicBlockInContext(ctx, "sc_res");

  switch (op) {
    case BOK_LogicalAnd:
      // Jump to the rhs if lsh is true. Otherwise go to the result bb.
      LLVMBuildCondBr(builder, lhs_val, eval_rhs_bb, res_bb);
      break;
    case BOK_LogicalOr:
      // Jump to the res BB if true. Otherwise check rhs.
      LLVMBuildCondBr(builder, lhs_val, res_bb, eval_rhs_bb);
      break;
    default:
      UNREACHABLE_MSG("Unhandled local operator %d", op);
  }

  // BB for evaluating RHS.
  LLVMPositionBuilderAtEnd(builder, eval_rhs_bb);
  LLVMValueRef rhs_val = compile_to_bool(compiler, builder, rhs, local_ctx,
                                         local_allocas, break_bb, cont_bb);
  rhs_val = LLVMBuildZExt(builder, rhs_val, LLVMInt8Type(), "");
  LLVMBuildBr(builder, res_bb);
  eval_rhs_bb = LLVMGetInsertBlock(builder);

  // Result BB.
  LLVMAppendExistingBasicBlock(fn, res_bb);
  LLVMPositionBuilderAtEnd(builder, res_bb);

  LLVMValueRef phi = LLVMBuildPhi(builder, LLVMInt8Type(), "");

  LLVMValueRef default_val =
      LLVMConstInt(LLVMInt8Type(), op == BOK_LogicalOr, /*IsSigned=*/0);

  LLVMValueRef incoming_vals[] = {
      default_val,
      rhs_val,
  };
  LLVMBasicBlockRef incoming_blocks[] = {current_bb, eval_rhs_bb};
  LLVMAddIncoming(phi, incoming_vals, incoming_blocks, 2);

  return phi;
}

LLVMValueRef compile_binop(Compiler* compiler, LLVMBuilderRef builder,
                           const BinOp* expr, TreeMap* local_ctx,
                           TreeMap* local_allocas, LLVMBasicBlockRef break_bb,
                           LLVMBasicBlockRef cont_bb) {
  const Type* lhs_ty =
      sema_get_type_of_expr_in_ctx(compiler->sema, expr->lhs, local_ctx);
  const Type* rhs_ty =
      sema_get_type_of_expr_in_ctx(compiler->sema, expr->rhs, local_ctx);
  const Expr* maybe_ptr = NULL;
  const Expr* offset;
  const Type* maybe_ptr_ty;
  if (sema_is_pointer_type(compiler->sema, lhs_ty, local_ctx)) {
    maybe_ptr = expr->lhs;
    maybe_ptr_ty = lhs_ty;
    offset = expr->rhs;
  } else if (sema_is_pointer_type(compiler->sema, rhs_ty, local_ctx)) {
    maybe_ptr = expr->rhs;
    maybe_ptr_ty = rhs_ty;
    offset = expr->lhs;
  }

  bool can_be_used_for_ptr_arithmetic =
      expr->op == BOK_Add || expr->op == BOK_Sub || expr->op == BOK_AddAssign ||
      expr->op == BOK_SubAssign;

  if (maybe_ptr && can_be_used_for_ptr_arithmetic) {
    const Type* pointee_ty =
        sema_get_pointee(compiler->sema, maybe_ptr_ty, local_ctx);
    LLVMTypeRef llvm_base_ty = get_llvm_type(compiler, pointee_ty, local_ctx);

    LLVMValueRef llvm_offset = compile_expr(
        compiler, builder, offset, local_ctx, local_allocas, break_bb, cont_bb);

    LLVMValueRef llvm_base;
    LLVMValueRef ptr;
    bool assign_op = expr->op == BOK_AddAssign || expr->op == BOK_SubAssign;
    if (!assign_op) {
      llvm_base = compile_expr(compiler, builder, maybe_ptr, local_ctx,
                               local_allocas, break_bb, cont_bb);
    } else {
      assert(maybe_ptr == expr->lhs);
      assert(offset == expr->rhs);
      ptr = compile_lvalue_ptr(compiler, builder, maybe_ptr, local_ctx,
                               local_allocas, break_bb, cont_bb);
      llvm_base = LLVMBuildLoad2(builder, get_opaque_ptr(compiler), ptr, "");
    }

    LLVMValueRef offsets[] = {llvm_offset};
    LLVMValueRef gep =
        LLVMBuildGEP2(builder, llvm_base_ty, llvm_base, offsets, 1, "");

    if (assign_op)
      LLVMBuildStore(builder, gep, ptr);

    return gep;
  }

  // FIXME: This currently evaluates all operands before doing the actual binop.
  // This breaks the rules for short-circuit evaluation. For example, this will
  // not be evaluated correctly:
  //
  //   bool b = ptr && *ptr;
  //
  // Even if `ptr` is NULL, the dereference will still occur. We need to keep
  // track of sequence points.
  //
  LLVMValueRef lhs;
  LLVMValueRef rhs;
  const Type* common_ty;
  if (is_logical_binop(expr->op)) {
    return compile_logical_binop(compiler, builder, expr->lhs, expr->rhs,
                                 expr->op, local_ctx, local_allocas, break_bb,
                                 cont_bb);
  } else if (is_assign_binop(expr->op)) {
    rhs = compile_implicit_cast(compiler, builder, expr->rhs, lhs_ty, local_ctx,
                                local_allocas, break_bb, cont_bb);
    lhs = compile_lvalue_ptr(compiler, builder, expr->lhs, local_ctx,
                             local_allocas, break_bb, cont_bb);
    common_ty = lhs_ty;
  } else if (expr->op == BOK_Eq || expr->op == BOK_Ne) {
    if (sema_is_pointer_type(compiler->sema, lhs_ty, local_ctx) &&
        sema_is_pointer_type(compiler->sema, rhs_ty, local_ctx)) {
      // TODO: When comparing pointers, only pointers of to the same type can be
      // compared. The only exception to this is void pointers. We should check
      // that both operands are the same first before comparing. Here we just
      // assume they are the same.
      common_ty = lhs_ty;
    } else {
      common_ty = sema_get_common_arithmetic_type(compiler->sema, lhs_ty,
                                                  rhs_ty, local_ctx);
    }
    lhs = compile_implicit_cast(compiler, builder, expr->lhs, common_ty,
                                local_ctx, local_allocas, break_bb, cont_bb);
    rhs = compile_implicit_cast(compiler, builder, expr->rhs, common_ty,
                                local_ctx, local_allocas, break_bb, cont_bb);
  } else {
    common_ty = sema_get_common_arithmetic_type(compiler->sema, lhs_ty, rhs_ty,
                                                local_ctx);
    lhs = compile_implicit_cast(compiler, builder, expr->lhs, common_ty,
                                local_ctx, local_allocas, break_bb, cont_bb);
    rhs = compile_implicit_cast(compiler, builder, expr->rhs, common_ty,
                                local_ctx, local_allocas, break_bb, cont_bb);
  }

  LLVMValueRef res;

  bool common_is_unsigned =
      sema_is_unsigned_integral_type(compiler->sema, common_ty);

  switch (expr->op) {
    case BOK_Comma:
      res = rhs;
      break;
    case BOK_Lt: {
      LLVMIntPredicate op = common_is_unsigned ? LLVMIntULT : LLVMIntSLT;
      res = LLVMBuildICmp(builder, op, lhs, rhs, "");
      break;
    }
    case BOK_Gt: {
      LLVMIntPredicate op = common_is_unsigned ? LLVMIntUGT : LLVMIntSGT;
      res = LLVMBuildICmp(builder, op, lhs, rhs, "");
      break;
    }
    case BOK_Le: {
      LLVMIntPredicate op = common_is_unsigned ? LLVMIntULE : LLVMIntSLE;
      res = LLVMBuildICmp(builder, op, lhs, rhs, "");
      break;
    }
    case BOK_Ge: {
      LLVMIntPredicate op = common_is_unsigned ? LLVMIntUGE : LLVMIntSGE;
      res = LLVMBuildICmp(builder, op, lhs, rhs, "");
      break;
    }
    case BOK_Ne:
      res = LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "");
      break;
    case BOK_Eq:
      res = LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "");
      break;
    case BOK_Mod:
      if (common_is_unsigned)
        res = LLVMBuildURem(builder, lhs, rhs, "");
      else
        res = LLVMBuildSRem(builder, lhs, rhs, "");
      break;
    case BOK_Add:
      res = LLVMBuildAdd(builder, lhs, rhs, "");
      break;
    case BOK_Sub:
      res = LLVMBuildSub(builder, lhs, rhs, "");
      break;
    case BOK_Mul:
      res = LLVMBuildMul(builder, lhs, rhs, "");
      break;
    case BOK_Div:
      if (common_is_unsigned)
        res = LLVMBuildUDiv(builder, lhs, rhs, "");
      else
        res = LLVMBuildSDiv(builder, lhs, rhs, "");
      break;
    case BOK_BitwiseAnd:
      res = LLVMBuildAnd(builder, lhs, rhs, "");
      break;
    case BOK_BitwiseOr:
      res = LLVMBuildOr(builder, lhs, rhs, "");
      break;
    case BOK_LogicalAnd:
      res = LLVMBuildAnd(builder, lhs, rhs, "");
      break;
    case BOK_LogicalOr:
      res = LLVMBuildOr(builder, lhs, rhs, "");
      break;
    case BOK_Assign:
      get_aligned_store(compiler, builder, common_ty, rhs, lhs, local_ctx);
      res = rhs;
      break;
    case BOK_AddAssign: {
      LLVMValueRef lhs_val =
          get_aligned_load(compiler, builder, common_ty, lhs, "", local_ctx);
      res = LLVMBuildAdd(builder, lhs_val, rhs, "");
      get_aligned_store(compiler, builder, common_ty, res, lhs, local_ctx);
      break;
    }
    case BOK_OrAssign: {
      LLVMValueRef lhs_val =
          get_aligned_load(compiler, builder, common_ty, lhs, "", local_ctx);
      res = LLVMBuildOr(builder, lhs_val, rhs, "");
      get_aligned_store(compiler, builder, common_ty, res, lhs, local_ctx);
      break;
    }
    case BOK_LShift:
    case BOK_RShift:
    case BOK_LShiftAssign:
    case BOK_RShiftAssign: {
      bool is_assign =
          expr->op == BOK_LShiftAssign || expr->op == BOK_RShiftAssign;
      bool is_shl = expr->op == BOK_LShiftAssign || expr->op == BOK_LShift;
      LLVMValueRef lhs_val =
          is_assign
              ? get_aligned_load(compiler, builder, lhs_ty, lhs, "", local_ctx)
              : lhs;
      if (is_shl) {
        res = LLVMBuildShl(builder, lhs_val, rhs, "");
      } else {
        res = LLVMBuildAShr(builder, lhs_val, rhs, "");
      }
      if (is_assign)
        get_aligned_store(compiler, builder, lhs_ty, res, lhs, local_ctx);
      break;
    }
    default:
      UNREACHABLE_MSG("TODO: Implement compile_binop for this op %d", expr->op);
  }

  const Type* res_ty =
      sema_get_type_of_expr_in_ctx(compiler->sema, &expr->expr, local_ctx);
  if (is_bool_type(res_ty)) {
    // Cast up any i1s to i8s since bools are always kCharBits.
    res = LLVMBuildZExt(builder, res, LLVMIntType(kCharBit), "");
  }

  return res;
}

LLVMTypeRef get_llvm_type_of_expr(Compiler* compiler, const Expr* expr,
                                  const TreeMap* local_ctx) {
  const Type* ty =
      sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);
  return get_llvm_type(compiler, ty, local_ctx);
}

LLVMTypeRef get_llvm_type_of_expr_global_ctx(Compiler* compiler,
                                             const Expr* expr) {
  TreeMap local_ctx;
  string_tree_map_construct(&local_ctx);
  const Type* type =
      sema_get_type_of_expr_in_ctx(compiler->sema, expr, &local_ctx);
  LLVMTypeRef llvm_type = get_llvm_type(compiler, type, &local_ctx);
  tree_map_destroy(&local_ctx);
  return llvm_type;
}

// Returns a vector of LLVMValueRefs.
// `args` is a vector of Expr* (the same as `Call::args`).
// `func_args` is a vector of FunctionArg (same as `FunctionType::pos_args`).
vector compile_call_args(Compiler* compiler, LLVMBuilderRef builder,
                         const vector* args, const vector* func_args,
                         TreeMap* local_ctx, TreeMap* local_allocas,
                         LLVMBasicBlockRef break_bb,
                         LLVMBasicBlockRef cont_bb) {
  vector llvm_args;
  vector_construct(&llvm_args, sizeof(LLVMValueRef), alignof(LLVMValueRef));
  for (size_t i = 0; i < args->size; ++i) {
    LLVMValueRef* storage = vector_append_storage(&llvm_args);
    const Expr* arg = *(const Expr**)vector_at(args, i);

    if (i < func_args->size) {
      const FunctionArg* func_arg_ty = vector_at(func_args, i);
      *storage =
          compile_implicit_cast(compiler, builder, arg, func_arg_ty->type,
                                local_ctx, local_allocas, break_bb, cont_bb);
    } else {
      // This should be a varargs function.
      const Type* arg_ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, arg, local_ctx);
      *storage =
          compile_implicit_cast(compiler, builder, arg, arg_ty, local_ctx,
                                local_allocas, break_bb, cont_bb);
    }
  }
  return llvm_args;
}

LLVMValueRef call_llvm_debugtrap(Compiler* compiler, LLVMBuilderRef builder) {
  // FIXME: We should be able to do `sizeof(name)`.
  const char* name = "llvm.debugtrap";
  unsigned intrinsic_id = LLVMLookupIntrinsicID(name, 14);
  LLVMValueRef intrinsic = LLVMGetIntrinsicDeclaration(
      compiler->mod, intrinsic_id, /*ParamTypes=*/NULL,
      /*ParamCount=*/0);
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);
  LLVMTypeRef intrinsic_ty = LLVMIntrinsicGetType(
      ctx, intrinsic_id, /*ParamTypes=*/NULL, /*ParamCount=*/0);
  return LLVMBuildCall2(builder, intrinsic_ty, intrinsic,
                        /*Args=*/NULL, /*NumArgs=*/0, "");
}

void compile_compound_statement(Compiler* compiler, LLVMBuilderRef builder,
                                const CompoundStmt* compound,
                                TreeMap* local_ctx, TreeMap* local_allocas,
                                LLVMBasicBlockRef break_bb,
                                LLVMBasicBlockRef cont_bb,
                                LLVMValueRef* last_expr);

// Compile an expression and return it.
//
// The only time this function returns NULL is for a StmtExpr which can evaluate
// to void. All other cases should return a valid LLVMValueRef.
//
// BasicBlocks are propagated soley for proper handling of statement expressions
// which still need to respect semantics for `break` and `continue`.
LLVMValueRef compile_expr(Compiler* compiler, LLVMBuilderRef builder,
                          const Expr* expr, TreeMap* local_ctx,
                          TreeMap* local_allocas, LLVMBasicBlockRef break_bb,
                          LLVMBasicBlockRef cont_bb) {
  const Type* type =
      sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);
  LLVMTypeRef llvm_type = get_llvm_type(compiler, type, local_ctx);
  switch (expr->vtable->kind) {
    case EK_String: {
      const StringLiteral* s = (const StringLiteral*)expr;
      return LLVMBuildGlobalString(builder, s->val, "");
    }
    case EK_PrettyFunction: {
      LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
      size_t size;
      return LLVMBuildGlobalString(builder, LLVMGetValueName2(fn, &size), "");
    }
    case EK_Int: {
      const Int* i = (const Int*)expr;
      bool has_sign = !is_unsigned_integral_type(&i->type.type);
      assert(LLVMGetTypeKind(llvm_type) == LLVMIntegerTypeKind &&
             LLVMGetIntTypeWidth(llvm_type) > 0);
      return LLVMConstInt(llvm_type, i->val, has_sign);
    }
    case EK_Bool: {
      const Bool* b = (const Bool*)expr;
      return LLVMConstInt(llvm_type, b->val, /*UsSugbed=*/false);
    }
    case EK_Char: {
      const Char* c = (const Char*)expr;
      assert(c->val >= 0);
      return LLVMConstInt(llvm_type, (unsigned long long)c->val,
                          /*UsSugbed=*/false);
    }
    case EK_SizeOf: {
      ConstExprResult res = sema_eval_sizeof_in_ctx(
          compiler->sema, (const SizeOf*)expr, local_ctx);
      assert(res.result_kind == RK_UnsignedLongLong);
      return LLVMConstInt(llvm_type, res.result.ull, /*IsSigned=*/0);
    }
    case EK_AlignOf: {
      ConstExprResult res = sema_eval_alignof_in_ctx(
          compiler->sema, (const AlignOf*)expr, local_ctx);
      assert(res.result_kind == RK_UnsignedLongLong);
      return LLVMConstInt(llvm_type, res.result.ull, /*IsSigned=*/0);
    }
    case EK_UnOp:
      return compile_unop(compiler, builder, (const UnOp*)expr, local_ctx,
                          local_allocas, break_bb, cont_bb);
    case EK_BinOp:
      return compile_binop(compiler, builder, (const BinOp*)expr, local_ctx,
                           local_allocas, break_bb, cont_bb);
    case EK_DeclRef: {
      const DeclRef* decl = (const DeclRef*)expr;

      LLVMValueRef val = NULL;
      bool is_func = LLVMGetTypeKind(llvm_type) == LLVMFunctionTypeKind;
      if (is_func && !tree_map_get(local_allocas, decl->name, (void*)&val)) {
        LLVMValueRef val = LLVMGetNamedFunction(compiler->mod, decl->name);
        assert(val);
        return val;
      }

      const Type* decl_ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);
      if (decl_ty->vtable->kind == TK_EnumType) {
        void* enum_val;
        if (tree_map_get(&compiler->sema->enum_values, decl->name, &enum_val)) {
          LLVMTypeRef enum_ty =
              get_llvm_type_of_expr(compiler, expr, local_ctx);
          return LLVMConstInt(enum_ty, (uintptr_t)enum_val, /*signed=*/1);
        }

        UNREACHABLE_MSG("Couldn't find value for enum member '%s'", decl->name);
      }

      val = compile_lvalue_ptr(compiler, builder, expr, local_ctx,
                               local_allocas, break_bb, cont_bb);

      if (sema_is_array_type(compiler->sema, decl_ty, local_ctx))
        return val;

      return get_aligned_load(compiler, builder, type, val, "", local_ctx);
    }
    case EK_Call: {
      const Call* call = (const Call*)expr;
      if (call->base->vtable->kind == EK_DeclRef) {
        const DeclRef* decl = (const DeclRef*)call->base;
        if (strcmp(decl->name, "__builtin_trap") == 0)
          return call_llvm_debugtrap(compiler, builder);
      }

      const Type* ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, call->base, local_ctx);
      const FunctionType* func_ty =
          sema_get_function(compiler->sema, ty, local_ctx);
      LLVMTypeRef llvm_func_ty =
          get_llvm_type(compiler, &func_ty->type, local_ctx);
      LLVMValueRef llvm_func =
          compile_expr(compiler, builder, call->base, local_ctx, local_allocas,
                       break_bb, cont_bb);

      vector llvm_args =
          compile_call_args(compiler, builder, &call->args, &func_ty->pos_args,
                            local_ctx, local_allocas, break_bb, cont_bb);

      LLVMValueRef res =
          LLVMBuildCall2(builder, llvm_func_ty, llvm_func, llvm_args.data,
                         (unsigned)llvm_args.size, "");

      LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);
      LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
      LLVMMetadataRef local_scope = LLVMGetSubprogram(fn);
      assert(local_scope);

      // TODO: Fill these in with correct values.
      LLVMMetadataRef debug_loc = LLVMDIBuilderCreateDebugLocation(
          ctx, (unsigned)source_location_line(&expr->loc),
          (unsigned)source_location_col(&expr->loc), local_scope,
          /*InlinedAt=*/NULL);
      LLVMInstructionSetDebugLoc(res, debug_loc);

      vector_destroy(&llvm_args);

      return res;
    }
    case EK_MemberAccess: {
      const MemberAccess* access = (const MemberAccess*)expr;
      const StructType* base_ty = sema_get_struct_type_from_member_access(
          compiler->sema, access, local_ctx);
      size_t offset;
      const Member* member = sema_get_struct_member(compiler->sema, base_ty,
                                                    access->member, &offset);

      LLVMValueRef ptr;
      if (access->is_arrow) {
        ptr = compile_expr(compiler, builder, access->base, local_ctx,
                           local_allocas, break_bb, cont_bb);
        LLVMTypeRef llvm_base_ty =
            get_llvm_type(compiler, &base_ty->type, local_ctx);
        LLVMValueRef llvm_offset =
            LLVMConstInt(LLVMInt32Type(), offset, /*signed=*/0);
        LLVMValueRef offsets[] = {LLVMConstNull(LLVMInt32Type()), llvm_offset};
        ptr = LLVMBuildGEP2(builder, llvm_base_ty, ptr, offsets, 2, "");
      } else {
        ptr = compile_lvalue_ptr(compiler, builder, expr, local_ctx,
                                 local_allocas, break_bb, cont_bb);
      }
      return get_aligned_load(compiler, builder, member->type, ptr, "",
                              local_ctx);
    }
    case EK_Conditional: {
      const Conditional* conditional = (const Conditional*)expr;
      return compile_conditional(compiler, builder, conditional, local_ctx,
                                 local_allocas, break_bb, cont_bb);
    }
    case EK_Cast: {
      const Cast* cast = (const Cast*)expr;
      return compile_implicit_cast(compiler, builder, cast->base, cast->to,
                                   local_ctx, local_allocas, break_bb, cont_bb);
    }
    case EK_Index: {
      LLVMValueRef ptr = compile_lvalue_ptr(compiler, builder, expr, local_ctx,
                                            local_allocas, break_bb, cont_bb);
      const Type* ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);
      return get_aligned_load(compiler, builder, ty, ptr, "", local_ctx);
    }
    case EK_StmtExpr: {
      const StmtExpr* stmt_expr = (const StmtExpr*)expr;
      if (!stmt_expr->stmt)
        return NULL;

      assert(stmt_expr->stmt->body.size > 0);
      LLVMValueRef expr = NULL;
      compile_compound_statement(compiler, builder, stmt_expr->stmt, local_ctx,
                                 local_allocas, break_bb, cont_bb, &expr);
      return expr;
    }
    default:
      UNREACHABLE_MSG("TODO: Implement compile_expr for this expr %d",
                      expr->vtable->kind);
  }
}

void compile_statement(Compiler* compiler, LLVMBuilderRef builder,
                       const Statement* stmt, TreeMap* local_ctx,
                       TreeMap* local_allocas, LLVMBasicBlockRef break_bb,
                       LLVMBasicBlockRef cont_bb, LLVMValueRef* last_expr);

void compile_if_statement(Compiler* compiler, LLVMBuilderRef builder,
                          const IfStmt* stmt, TreeMap* local_ctx,
                          TreeMap* local_allocas, LLVMBasicBlockRef break_bb,
                          LLVMBasicBlockRef cont_bb) {
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);
  LLVMValueRef cond = compile_to_bool(compiler, builder, stmt->cond, local_ctx,
                                      local_allocas, break_bb, cont_bb);

  assert(LLVMGetTypeKind(LLVMTypeOf(cond)) == LLVMIntegerTypeKind);
  assert(LLVMGetIntTypeWidth(LLVMTypeOf(cond)) == 1);

  // Create blocks for the then and else cases. Insert the 'if' block at the
  // end of the function.
  LLVMBasicBlockRef ifbb = LLVMAppendBasicBlockInContext(ctx, fn, "if");
  LLVMBasicBlockRef elsebb = LLVMCreateBasicBlockInContext(ctx, "else");
  LLVMBasicBlockRef mergebb = LLVMCreateBasicBlockInContext(ctx, "merge");

  LLVMBuildCondBr(builder, cond, ifbb, elsebb);

  // Emit if BB.
  LLVMPositionBuilderAtEnd(builder, ifbb);

  if (stmt->body) {
    TreeMap local_ctx_cpy;
    TreeMap local_allocas_cpy;
    string_tree_map_construct(&local_ctx_cpy);
    string_tree_map_construct(&local_allocas_cpy);
    tree_map_clone(&local_ctx_cpy, local_ctx);
    tree_map_clone(&local_allocas_cpy, local_allocas);

    compile_statement(compiler, builder, stmt->body, &local_ctx_cpy,
                      &local_allocas_cpy, break_bb, cont_bb,
                      /*last_expr=*/NULL);

    tree_map_destroy(&local_ctx_cpy);
    tree_map_destroy(&local_allocas_cpy);
  }

  LLVMBasicBlockRef last_bb = LLVMGetInsertBlock(builder);
  LLVMValueRef last = LLVMGetLastInstruction(last_bb);
  bool all_branches_terminate = true;
  if (!last || !LLVMIsATerminatorInst(last)) {
    LLVMBuildBr(builder, mergebb);
    all_branches_terminate = false;
  }

  // Emit potential else BB.
  LLVMAppendExistingBasicBlock(fn, elsebb);
  LLVMPositionBuilderAtEnd(builder, elsebb);
  if (stmt->else_stmt) {
    {
      TreeMap local_ctx_cpy;
      TreeMap local_allocas_cpy;
      string_tree_map_construct(&local_ctx_cpy);
      string_tree_map_construct(&local_allocas_cpy);
      tree_map_clone(&local_ctx_cpy, local_ctx);
      tree_map_clone(&local_allocas_cpy, local_allocas);

      compile_statement(compiler, builder, stmt->else_stmt, local_ctx,
                        local_allocas, break_bb, cont_bb, /*last_expr=*/NULL);

      tree_map_destroy(&local_ctx_cpy);
      tree_map_destroy(&local_allocas_cpy);
    }

    LLVMBasicBlockRef last_bb = LLVMGetInsertBlock(builder);
    LLVMValueRef last = LLVMGetLastInstruction(last_bb);
    if (!last || !LLVMIsATerminatorInst(last)) {
      LLVMBuildBr(builder, mergebb);
      all_branches_terminate = false;
    }
  } else {
    LLVMBuildBr(builder, mergebb);
    all_branches_terminate = false;
  }

  if (!all_branches_terminate) {
    // Emit merge BB.
    LLVMAppendExistingBasicBlock(fn, mergebb);
    LLVMPositionBuilderAtEnd(builder, mergebb);
  } else {
    // For some reason, there's no function for just deleting a BB. The delete
    // function both removes a block from a function and deletes it.
    LLVMAppendExistingBasicBlock(fn, mergebb);
    LLVMDeleteBasicBlock(mergebb);
  }
}

bool last_instruction_is_terminator(LLVMBuilderRef builder) {
  LLVMBasicBlockRef last_bb = LLVMGetInsertBlock(builder);
  LLVMValueRef last = LLVMGetLastInstruction(last_bb);
  return last && LLVMIsATerminatorInst(last);
}

void compile_for_statement(Compiler* compiler, LLVMBuilderRef builder,
                           const ForStmt* stmt, TreeMap* local_ctx,
                           TreeMap* local_allocas, LLVMBasicBlockRef break_bb,
                           LLVMBasicBlockRef cont_bb) {
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);

  // Scope local to the loop.
  TreeMap local_ctx_cpy;
  TreeMap local_allocas_cpy;
  string_tree_map_construct(&local_ctx_cpy);
  string_tree_map_construct(&local_allocas_cpy);
  tree_map_clone(&local_ctx_cpy, local_ctx);
  tree_map_clone(&local_allocas_cpy, local_allocas);

  // Emit the initializer if any.
  if (stmt->init) {
    compile_statement(compiler, builder, stmt->init, &local_ctx_cpy,
                      &local_allocas_cpy, break_bb, cont_bb,
                      /*last_expr=*/NULL);
  }

  LLVMBasicBlockRef for_start =
      LLVMAppendBasicBlockInContext(ctx, fn, "for_start");
  LLVMBuildBr(builder, for_start);
  LLVMPositionBuilderAtEnd(builder, for_start);

  LLVMBasicBlockRef for_end = LLVMCreateBasicBlockInContext(ctx, "for_end");
  LLVMBasicBlockRef for_iter = LLVMCreateBasicBlockInContext(ctx, "for_iter");

  // Check the condition if any.
  if (stmt->cond) {
    LLVMValueRef cond =
        compile_to_bool(compiler, builder, stmt->cond, &local_ctx_cpy,
                        &local_allocas_cpy, break_bb, cont_bb);
    LLVMBasicBlockRef for_body = LLVMCreateBasicBlockInContext(ctx, "for_body");
    LLVMBuildCondBr(builder, cond, for_body, for_end);

    LLVMAppendExistingBasicBlock(fn, for_body);
    LLVMPositionBuilderAtEnd(builder, for_body);
  }

  // Now emit the body.
  if (stmt->body) {
    compile_statement(compiler, builder, stmt->body, &local_ctx_cpy,
                      &local_allocas_cpy, for_end, for_iter,
                      /*last_expr=*/NULL);
  }

  // Branch to the iterator.
  if (!last_instruction_is_terminator(builder))
    LLVMBuildBr(builder, for_iter);

  LLVMAppendExistingBasicBlock(fn, for_iter);
  LLVMPositionBuilderAtEnd(builder, for_iter);

  // Then do the iter.
  if (stmt->iter) {
    compile_expr(compiler, builder, stmt->iter, &local_ctx_cpy,
                 &local_allocas_cpy, break_bb, cont_bb);
  }

  // And branch back to the start.
  if (!last_instruction_is_terminator(builder))
    LLVMBuildBr(builder, for_start);

  // End of for loop.
  LLVMAppendExistingBasicBlock(fn, for_end);
  LLVMPositionBuilderAtEnd(builder, for_end);

  tree_map_destroy(&local_ctx_cpy);
  tree_map_destroy(&local_allocas_cpy);
}

void compile_while_statement(Compiler* compiler, LLVMBuilderRef builder,
                             const WhileStmt* stmt, TreeMap* local_ctx,
                             TreeMap* local_allocas, LLVMBasicBlockRef break_bb,
                             LLVMBasicBlockRef cont_bb) {
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);

  // Scope local to the loop.
  TreeMap local_ctx_cpy;
  TreeMap local_allocas_cpy;
  string_tree_map_construct(&local_ctx_cpy);
  string_tree_map_construct(&local_allocas_cpy);
  tree_map_clone(&local_ctx_cpy, local_ctx);
  tree_map_clone(&local_allocas_cpy, local_allocas);

  LLVMBasicBlockRef while_start =
      LLVMAppendBasicBlockInContext(ctx, fn, "while_start");
  LLVMBuildBr(builder, while_start);
  LLVMPositionBuilderAtEnd(builder, while_start);

  LLVMBasicBlockRef while_end = LLVMCreateBasicBlockInContext(ctx, "while_end");

  // Check the condition.
  LLVMValueRef cond =
      compile_to_bool(compiler, builder, stmt->cond, &local_ctx_cpy,
                      &local_allocas_cpy, break_bb, cont_bb);
  LLVMBasicBlockRef while_body =
      LLVMCreateBasicBlockInContext(ctx, "while_body");
  LLVMBuildCondBr(builder, cond, while_body, while_end);

  LLVMAppendExistingBasicBlock(fn, while_body);
  LLVMPositionBuilderAtEnd(builder, while_body);

  // Now emit the body.
  if (stmt->body) {
    compile_statement(compiler, builder, stmt->body, &local_ctx_cpy,
                      &local_allocas_cpy, while_end, while_start,
                      /*last_expr=*/NULL);
  }

  // And branch back to the start.
  if (!last_instruction_is_terminator(builder))
    LLVMBuildBr(builder, while_start);

  // End of while loop.
  LLVMAppendExistingBasicBlock(fn, while_end);
  LLVMPositionBuilderAtEnd(builder, while_end);

  tree_map_destroy(&local_ctx_cpy);
  tree_map_destroy(&local_allocas_cpy);
}

// TODO: This should just be a switch instruction!!!
void compile_switch_statement(Compiler* compiler, LLVMBuilderRef builder,
                              const SwitchStmt* stmt, TreeMap* local_ctx,
                              TreeMap* local_allocas,
                              LLVMBasicBlockRef break_bb,
                              LLVMBasicBlockRef cont_bb) {
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);

  LLVMValueRef check = compile_expr(compiler, builder, stmt->cond, local_ctx,
                                    local_allocas, break_bb, cont_bb);

  LLVMBasicBlockRef end_bb = LLVMCreateBasicBlockInContext(ctx, "switch_end");

  LLVMValueRef should_fallthrough = LLVMConstNull(LLVMInt1Type());

  for (size_t i = 0; i < stmt->cases.size; ++i) {
    const SwitchCase* switch_case = vector_at(&stmt->cases, i);

    LLVMBasicBlockRef case_bb = LLVMAppendBasicBlockInContext(ctx, fn, "case");
    LLVMBasicBlockRef next_bb = LLVMCreateBasicBlockInContext(ctx, "next");

    const Type* common_ty = sema_get_common_arithmetic_type_of_exprs(
        compiler->sema, stmt->cond, switch_case->cond, local_ctx);

    LLVMValueRef case_val =
        compile_implicit_cast(compiler, builder, switch_case->cond, common_ty,
                              local_ctx, local_allocas, break_bb, cont_bb);
    LLVMValueRef cond = LLVMBuildICmp(builder, LLVMIntEQ, check, case_val, "");
    should_fallthrough = LLVMBuildOr(builder, cond, should_fallthrough, "");
    LLVMBuildCondBr(builder, should_fallthrough, case_bb, next_bb);

    LLVMPositionBuilderAtEnd(builder, case_bb);

    bool last_stmt_terminates = false;
    for (size_t j = 0; j < switch_case->stmts.size; ++j) {
      Statement** case_stmt = vector_at(&switch_case->stmts, j);
      compile_statement(compiler, builder, *case_stmt, local_ctx, local_allocas,
                        end_bb, cont_bb, /*last_expr=*/NULL);

      if ((last_stmt_terminates = last_instruction_is_terminator(builder)))
        break;
    }

    if (!last_stmt_terminates)
      LLVMBuildBr(builder, next_bb);

    LLVMAppendExistingBasicBlock(fn, next_bb);
    LLVMPositionBuilderAtEnd(builder, next_bb);
  }

  // FIXME: This will not work if the default is *not* the last block.
  if (stmt->default_stmts) {
    for (size_t i = 0; i < stmt->default_stmts->size; ++i) {
      Statement** default_stmt = vector_at(stmt->default_stmts, i);
      compile_statement(compiler, builder, *default_stmt, local_ctx,
                        local_allocas, end_bb, cont_bb, /*last_expr=*/NULL);

      if (last_instruction_is_terminator(builder))
        break;
    }
  }

  if (!last_instruction_is_terminator(builder))
    LLVMBuildBr(builder, end_bb);

  LLVMAppendExistingBasicBlock(fn, end_bb);
  LLVMPositionBuilderAtEnd(builder, end_bb);
}

// Compile a compound statement. If the body is not empty and the last statement
// is an expression statement and `last_expr` is provided, set `last_expr` to
// the resulting LLVMValueRef that expression compiles to.
void compile_compound_statement(Compiler* compiler, LLVMBuilderRef builder,
                                const CompoundStmt* compound,
                                TreeMap* local_ctx, TreeMap* local_allocas,
                                LLVMBasicBlockRef break_bb,
                                LLVMBasicBlockRef cont_bb,
                                LLVMValueRef* last_expr) {
  TreeMap local_ctx_cpy;
  TreeMap local_allocas_cpy;
  string_tree_map_construct(&local_ctx_cpy);
  string_tree_map_construct(&local_allocas_cpy);
  tree_map_clone(&local_ctx_cpy, local_ctx);
  tree_map_clone(&local_allocas_cpy, local_allocas);

  for (size_t i = 0; i < compound->body.size; ++i) {
    Statement* stmt = *(Statement**)vector_at(&compound->body, i);
    compile_statement(compiler, builder, stmt, &local_ctx_cpy,
                      &local_allocas_cpy, break_bb, cont_bb, last_expr);
    if (last_instruction_is_terminator(builder))
      break;
  }

  tree_map_destroy(&local_ctx_cpy);
  tree_map_destroy(&local_allocas_cpy);
  return;
}

// Compile a compound statement. If the statement is an expression statement and
// `last_expr` is provided, set `last_expr` to the resulting LLVMValueRef that
// expression compiles to.
void compile_statement(Compiler* compiler, LLVMBuilderRef builder,
                       const Statement* stmt, TreeMap* local_ctx,
                       TreeMap* local_allocas, LLVMBasicBlockRef break_bb,
                       LLVMBasicBlockRef cont_bb, LLVMValueRef* last_expr) {
  switch (stmt->vtable->kind) {
    case SK_ExprStmt: {
      LLVMValueRef expr =
          compile_expr(compiler, builder, ((const ExprStmt*)stmt)->expr,
                       local_ctx, local_allocas, break_bb, cont_bb);
      if (last_expr)
        *last_expr = expr;
      return;
    }
    case SK_IfStmt:
      return compile_if_statement(compiler, builder, (const IfStmt*)stmt,
                                  local_ctx, local_allocas, break_bb, cont_bb);
    case SK_WhileStmt:
      return compile_while_statement(compiler, builder, (const WhileStmt*)stmt,
                                     local_ctx, local_allocas, break_bb,
                                     cont_bb);
    case SK_ForStmt:
      return compile_for_statement(compiler, builder, (const ForStmt*)stmt,
                                   local_ctx, local_allocas, break_bb, cont_bb);
    case SK_ReturnStmt: {
      const ReturnStmt* ret = (const ReturnStmt*)stmt;
      if (!ret->expr) {
        LLVMBuildRetVoid(builder);
        return;
      }
      LLVMValueRef val = compile_expr(compiler, builder, ret->expr, local_ctx,
                                      local_allocas, break_bb, cont_bb);

      // TODO: There should be an ImplicitCast AST node that we can parser over
      // rather than doing this here.

      const Type* expr_ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, ret->expr, local_ctx);
      if (is_void_type(expr_ty))
        LLVMBuildRetVoid(builder);
      else
        LLVMBuildRet(builder, val);
      return;
    }
    case SK_ContinueStmt: {
      LLVMBuildBr(builder, cont_bb);
      return;
    }
    case SK_BreakStmt: {
      LLVMBuildBr(builder, break_bb);
      return;
    }
    case SK_CompoundStmt:
      return compile_compound_statement(
          compiler, builder, (const CompoundStmt*)stmt, local_ctx,
          local_allocas, break_bb, cont_bb, /*last_expr=*/NULL);
    case SK_Declaration: {
      const Declaration* decl = (const Declaration*)stmt;

      LLVMTypeRef llvm_ty = NULL;
      if (sema_is_array_type(compiler->sema, decl->type, local_ctx)) {
        const ArrayType* arr_ty =
            sema_get_array_type(compiler->sema, decl->type, local_ctx);
        if (!arr_ty->size) {
          // Get the type from the initializer.
          assert(decl->initializer);

          if (decl->initializer->vtable->kind == EK_InitializerList) {
            // Since this is an array, we can just get the first element.
            const InitializerList* init =
                (const InitializerList*)decl->initializer;

            assert(init->elems.size > 0);
            InitializerListElem* first_elem = vector_at(&init->elems, 0);
            LLVMTypeRef elem_ty =
                get_llvm_type_of_expr(compiler, first_elem->expr, local_ctx);
            llvm_ty = LLVMArrayType(elem_ty, (unsigned)init->elems.size);
          } else {
            llvm_ty =
                get_llvm_type_of_expr(compiler, decl->initializer, local_ctx);
          }
        }
      }

      if (!llvm_ty)
        llvm_ty = get_llvm_type(compiler, decl->type, local_ctx);

      LLVMValueRef alloca = build_alloca_at_func_start(
          compiler, builder, decl->name, decl->type, local_ctx);

      if (decl->initializer) {
        if (decl->initializer->vtable->kind == EK_InitializerList) {
          const InitializerList* init =
              (const InitializerList*)decl->initializer;

          size_t agg_size;
          bool is_array =
              sema_is_array_type(compiler->sema, decl->type, local_ctx);
          if (is_array) {
            const ArrayType* arr_ty =
                sema_get_array_type(compiler->sema, decl->type, local_ctx);
            if (arr_ty->size) {
              agg_size =
                  sema_eval_sizeof_type(compiler->sema, decl->type, local_ctx);
            } else {
              assert(init->elems.size > 0);
              agg_size = sema_eval_sizeof_type(compiler->sema,
                                               arr_ty->elem_type, local_ctx) *
                         init->elems.size;
            }
          } else {
            agg_size =
                sema_eval_sizeof_type(compiler->sema, decl->type, local_ctx);
          }
          LLVMBuildMemSet(
              builder, alloca, LLVMConstNull(LLVMInt8Type()),
              LLVMConstInt(LLVMInt32Type(), agg_size, /*IsSigned=*/0),
              /*Align=*/0);

          for (size_t i = 0; i < init->elems.size; ++i) {
            const InitializerListElem* elem = vector_at(&init->elems, i);
            LLVMValueRef val =
                compile_expr(compiler, builder, elem->expr, local_ctx,
                             local_allocas, break_bb, cont_bb);

            // TODO: Handle designated initializer names.

            LLVMValueRef gep;
            if (is_array) {
              LLVMValueRef offsets[] = {
                  LLVMConstNull(LLVMInt32Type()),
                  LLVMConstInt(LLVMInt32Type(), i, /*IsSigned=*/0),
              };
              gep = LLVMBuildGEP2(builder, llvm_ty, alloca, offsets, 2, "");
            } else {
              LLVMValueRef offsets[] = {
                  LLVMConstNull(LLVMInt32Type()),
                  LLVMConstInt(LLVMInt32Type(), i, /*IsSigned=*/0)};
              gep = LLVMBuildGEP2(builder, llvm_ty, alloca, offsets, 2, "");
            }
            LLVMBuildStore(builder, val, gep);
          }
        } else {
          LLVMValueRef init = compile_implicit_cast(
              compiler, builder, decl->initializer, decl->type, local_ctx,
              local_allocas, break_bb, cont_bb);
          get_aligned_store(compiler, builder, decl->type, init, alloca,
                            local_ctx);
        }
      }

      // Note this may override allocas and types declared in a higher scope,
      // but this should be ok since we should've cloned any local contexts
      // before entering a lower scope.
      tree_map_set(local_allocas, decl->name, alloca);
      tree_map_set(local_ctx, decl->name, decl->type);

      return;
    }
    case SK_SwitchStmt: {
      const SwitchStmt* switch_stmt = (const SwitchStmt*)stmt;
      compile_switch_statement(compiler, builder, switch_stmt, local_ctx,
                               local_allocas, break_bb, cont_bb);
      return;
    }
    default:
      UNREACHABLE_MSG("TODO: Implement compile_statement for this stmt %d",
                      stmt->vtable->kind);
  }
}

LLVMMetadataRef get_di_function_type(Compiler* compiler,
                                     const FunctionType* func_ty) {
  vector args;
  vector_construct(&args, sizeof(LLVMMetadataRef), alignof(LLVMMetadataRef));

  for (size_t i = 0; i < func_ty->pos_args.size; ++i) {
    LLVMMetadataRef* arg_md = vector_append_storage(&args);
    // TODO: Use the correct type.
    *arg_md = LLVMDIBuilderCreateUnspecifiedType(
        compiler->dibuilder, /*Name=*/"TODO", /*NameLen=*/4);
  }

  LLVMMetadataRef md = LLVMDIBuilderCreateSubroutineType(
      compiler->dibuilder, compiler->difile, args.data,
      (unsigned)func_ty->pos_args.size, /*Flags=*/0);
  vector_destroy(&args);
  return md;
}

void compile_function_definition(Compiler* compiler,
                                 const FunctionDefinition* f) {
  FunctionType* func_ty = (FunctionType*)(f->type);

  TreeMap local_ctx;
  string_tree_map_construct(&local_ctx);

  // TODO: Have a global version of `get_llvm_type`. The local_ctx is really
  // unused here but it's needed as an argument.
  LLVMTypeRef llvm_func_ty = get_llvm_type(compiler, f->type, &local_ctx);

  LLVMValueRef func = get_named_global(compiler, f->name);
  if (!func)
    func = LLVMAddFunction(compiler->mod, f->name, llvm_func_ty);
  assert(LLVMGetTypeKind(LLVMTypeOf(func)) == LLVMPointerTypeKind);

  if (!f->is_extern)
    LLVMSetLinkage(func, LLVMInternalLinkage);

  // TODO: Fill out the line number and other relevant fields.
  LLVMMetadataRef subprogram = LLVMDIBuilderCreateFunction(
      compiler->dibuilder, compiler->difile, f->name, strlen(f->name), f->name,
      strlen(f->name), compiler->difile, /*LineNo=*/1,
      get_di_function_type(compiler, func_ty), /*IsLocalToUnit=*/0,
      /*IsDefinition=*/1, /*ScopeLine=*/0,
      /*Flags=*/0, /*IsOptimized=*/0);
  LLVMSetSubprogram(func, subprogram);
  LLVMDIBuilderFinalizeSubprogram(compiler->dibuilder, subprogram);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);
  // TODO: Fill these in with correct values.
  LLVMMetadataRef debug_loc = LLVMDIBuilderCreateDebugLocation(
      ctx, /*Line=*/1, /*Column=*/0, subprogram, /*InlinedAt=*/NULL);
  LLVMSetCurrentDebugLocation2(builder, debug_loc);

  TreeMap local_allocas;
  string_tree_map_construct(&local_allocas);

  for (size_t i = 0; i < func_ty->pos_args.size; ++i) {
    FunctionArg* arg = vector_at(&func_ty->pos_args, i);
    if (!arg->name)
      continue;

    // Copy the parameter locally.
    LLVMValueRef llvm_arg = LLVMGetParam(func, (unsigned)i);
    LLVMValueRef alloca = build_alloca_at_func_start(
        compiler, builder, arg->name, arg->type, &local_ctx);
    get_aligned_store(compiler, builder, arg->type, llvm_arg, alloca,
                      &local_ctx);

    tree_map_set(&local_ctx, arg->name, arg->type);
    tree_map_set(&local_allocas, arg->name, alloca);
  }

  compile_statement(compiler, builder, &f->body->base, &local_ctx,
                    &local_allocas, /*break_bb=*/NULL, /*cont_bb=*/NULL,
                    /*last_expr=*/NULL);

  if (!last_instruction_is_terminator(builder)) {
    if (is_void_type(func_ty->return_type)) {
      LLVMBuildRetVoid(builder);
    } else {
      call_llvm_debugtrap(compiler, builder);
      LLVMBuildUnreachable(builder);
    }
  }

  tree_map_destroy(&local_ctx);
  tree_map_destroy(&local_allocas);

  LLVMDisposeBuilder(builder);

  if (LLVMVerifyFunction(func, LLVMPrintMessageAction)) {
    printf("Verify function '%s' failed\n", f->name);
    LLVMDumpValue(func);
    __builtin_trap();
  }
}

LLVMValueRef compile_lvalue_ptr(Compiler* compiler, LLVMBuilderRef builder,
                                const Expr* expr, TreeMap* local_ctx,
                                TreeMap* local_allocas,
                                LLVMBasicBlockRef break_bb,
                                LLVMBasicBlockRef cont_bb) {
  switch (expr->vtable->kind) {
    case EK_DeclRef: {
      const DeclRef* decl = (const DeclRef*)expr;

      LLVMValueRef val = NULL;
      tree_map_get(local_allocas, decl->name, (void*)&val);

      if (!val)
        val = get_named_global(compiler, decl->name);

      if (!val) {
        printf("Couldn't find alloca or global for '%s'\n", decl->name);
        LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
        LLVMDumpValue(fn);
        __builtin_trap();
      }

      assert(LLVMGetTypeKind(LLVMTypeOf(val)) == LLVMPointerTypeKind);
      return val;
    }
    case EK_MemberAccess: {
      const MemberAccess* access = (const MemberAccess*)expr;
      const StructType* base_ty = sema_get_struct_type_from_member_access(
          compiler->sema, access, local_ctx);
      size_t offset;
      sema_get_struct_member(compiler->sema, base_ty, access->member, &offset);

      LLVMValueRef base_llvm;
      if (access->is_arrow) {
        base_llvm = compile_expr(compiler, builder, access->base, local_ctx,
                                 local_allocas, break_bb, cont_bb);
      } else {
        base_llvm =
            compile_lvalue_ptr(compiler, builder, access->base, local_ctx,
                               local_allocas, break_bb, cont_bb);
      }

      LLVMTypeRef llvm_base_ty =
          get_llvm_type(compiler, &base_ty->type, local_ctx);
      LLVMValueRef llvm_offset =
          LLVMConstInt(LLVMInt32Type(), offset, /*signed=*/0);
      LLVMValueRef offsets[] = {LLVMConstNull(LLVMInt32Type()), llvm_offset};
      LLVMValueRef gep =
          LLVMBuildGEP2(builder, llvm_base_ty, base_llvm, offsets, 2, "");
      return gep;
    }
    case EK_UnOp:
      return compile_unop_lvalue_ptr(compiler, builder, (const UnOp*)expr,
                                     local_ctx, local_allocas, break_bb,
                                     cont_bb);
    case EK_Cast: {
      const Cast* cast = (const Cast*)expr;
      const Type* src_ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);
      const Type* dst_ty = cast->to;
      LLVMTypeRef llvm_src_ty = get_llvm_type(compiler, src_ty, local_ctx);
      LLVMTypeRef llvm_dst_ty = get_llvm_type(compiler, dst_ty, local_ctx);
      if (LLVMGetTypeKind(llvm_src_ty) == LLVMGetTypeKind(llvm_dst_ty))
        return compile_lvalue_ptr(compiler, builder, cast->base, local_ctx,
                                  local_allocas, break_bb, cont_bb);

      UNREACHABLE_MSG("Unhandled lvalue ptr cast from %d to %d",
                      LLVMGetTypeKind(llvm_src_ty),
                      LLVMGetTypeKind(llvm_dst_ty));
    }
    case EK_Index: {
      const Index* index = (const Index*)expr;
      const Expr* base = index->base;
      const Type* base_ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, base, local_ctx);

      LLVMValueRef idx = compile_expr(compiler, builder, index->idx, local_ctx,
                                      local_allocas, break_bb, cont_bb);

      LLVMValueRef base_ptr = compile_expr(compiler, builder, base, local_ctx,
                                           local_allocas, break_bb, cont_bb);

      assert(LLVMGetTypeKind(LLVMTypeOf(base_ptr)) == LLVMPointerTypeKind);

      if (sema_is_pointer_type(compiler->sema, base_ty, local_ctx)) {
        LLVMValueRef offsets[] = {idx};

        const Type* pointee_ty =
            sema_get_pointee(compiler->sema, base_ty, local_ctx);
        LLVMTypeRef llvm_pointee_ty =
            get_llvm_type(compiler, pointee_ty, local_ctx);
        return LLVMBuildGEP2(builder, llvm_pointee_ty, base_ptr, offsets, 1,
                             "idx_ptr");
      } else if (sema_is_array_type(compiler->sema, base_ty, local_ctx)) {
        // TODO: This could probably be consolidated with the branch above.

        LLVMValueRef offsets[] = {idx};

        const Type* elem_ty =
            sema_get_array_type(compiler->sema, base_ty, local_ctx)->elem_type;

        LLVMTypeRef llvm_elem_ty = get_llvm_type(compiler, elem_ty, local_ctx);
        return LLVMBuildGEP2(builder, llvm_elem_ty, base_ptr, offsets, 1,
                             "idx_arr");
      }

      UNREACHABLE_MSG("Indexing non-ptr or arr type");
    }
    default:
      UNREACHABLE_MSG("TODO: Handle lvalue for EK %d", expr->vtable->kind);
  }
}

void compile_global_variable(Compiler* compiler, const GlobalVariable* gv) {
  TreeMap dummy_ctx;
  string_tree_map_construct(&dummy_ctx);
  LLVMTypeRef ty = get_llvm_type(compiler, gv->type, &dummy_ctx);

  if (gv->type->vtable->kind == TK_FunctionType) {
    if (!get_named_global(compiler, gv->name)) {
      LLVMAddFunction(compiler->mod, gv->name, ty);
      assert(!gv->initializer &&
             "If this had an initializer, it would be a function definition.");
    }
    tree_map_destroy(&dummy_ctx);
    return;
  }

  LLVMValueRef glob = LLVMAddGlobal(compiler->mod, ty, gv->name);
  if (gv->initializer) {
    // TODO: This would be handled much easier if we had a node for implicit
    // casts.
    LLVMValueRef val = maybe_compile_constant_implicit_cast(
        compiler, gv->initializer, gv->type, &dummy_ctx);
    LLVMSetInitializer(glob, val);

    if (!gv->is_extern)
      LLVMSetLinkage(glob, LLVMInternalLinkage);
  }

  tree_map_destroy(&dummy_ctx);
}

///
/// End Compiler Implementation
///

const struct Argument kArguments[] = {
    {0, "input_file", "Input file", PM_RequiredPositional},
    {'I', "include", "Include directory", PM_Multiple},
    {'v', "verbose", "Enable verbose output", PM_StoreTrue},
    // TODO: All this compiler does is just compile, so this is unused. It's
    // only here to consume the `-c` in the command line.
    {'c', "compile", "Only compile the input into an object file",
     PM_StoreTrue},
    {'o', "output", "Output file", PM_Optional},
    {0, "emit-llvm", "Emit LLVM IR to output instead of object code",
     PM_StoreTrue},
    {0, "ast-dump", "Dump the AST", PM_StoreTrue},
    {'E', "only-preprocess",
     "Stop after the preprocessing stage and only output the processed file",
     PM_StoreTrue},
};
const size_t kNumArguments = sizeof(kArguments) / sizeof(struct Argument);

static void destroy_ast_nodes(vector* ast_nodes) {
  for (size_t i = 0; i < ast_nodes->size; ++i) {
    TopLevelNode* node = *(TopLevelNode**)vector_at(ast_nodes, i);
    top_level_node_destroy(node);
    free(node);
  }
  vector_destroy(ast_nodes);
}

static void destroy_include_dirs(vector* include_dir_paths) {
  for (Path* it = vector_begin(include_dir_paths);
       it != vector_end(include_dir_paths); ++it) {
    path_destroy(it);
  }
  vector_destroy(include_dir_paths);
}

int main(int argc, char** argv) {
  RunStringTests();
  RunVectorTests();
  RunTreeMapTests();
  RunParserTests();

  TreeMap parsed_args;
  string_tree_map_construct(&parsed_args);
  parse_args(kNumArguments, kArguments, &parsed_args, argc, argv);

  struct ParsedArgument* include_dirs = NULL;
  tree_map_get(&parsed_args, "include", &include_dirs);
  assert(include_dirs && include_dirs->kind == PAK_StringVector);

  vector include_dir_paths;  // vector of Paths
  vector_construct(&include_dir_paths, sizeof(Path), alignof(Path));

  for (const char** it = vector_begin(include_dirs->value);
       it != vector_end(include_dirs->value); ++it) {
    Path* path = vector_append_storage(&include_dir_paths);
    path_construct_from_c_str(path, *it);
  }

  struct ParsedArgument* verbose;
  if (tree_map_get(&parsed_args, "verbose", &verbose) &&
      verbose->stored_value) {
    size_t num_includes = ((vector*)include_dirs->value)->size;
    printf("Included directories (%zu):\n", num_includes);
    for (size_t i = 0; i < num_includes; ++i) {
      printf("  %s\n",
             *(const char**)vector_at((vector*)include_dirs->value, i));
    }
  }

  struct ParsedArgument* input_arg;
  ASSERT_MSG(tree_map_get(&parsed_args, "input_file", &input_arg),
             "No input file provided");
  const char* input_filename = input_arg->value;

  // Parsing + compiling
  vector ast_nodes;
  vector_construct(&ast_nodes, sizeof(TopLevelNode*), alignof(TopLevelNode*));
  {
    FileInputStream file_input;
    file_input_stream_construct(&file_input, input_filename);
    PreprocessorInputStream pp;
    preprocessor_input_stream_construct(&pp, &file_input, &include_dir_paths);

    struct ParsedArgument* only_preprocess;
    if (tree_map_get(&parsed_args, "only-preprocess", &only_preprocess) &&
        only_preprocess->stored_value) {
      while (!input_stream_eof(&pp.base)) {
        putchar((char)input_stream_read(&pp.base));
      }

      destroy_parsed_args(&parsed_args);
      input_stream_destroy(&pp.base);
      input_stream_destroy(&file_input.base);
      destroy_include_dirs(&include_dir_paths);
      return 0;
    }

    Parser parser;
    parser_construct(&parser, &pp.base, input_filename);

    // Construct the AST.
    while (true) {
      const Token* token = parser_peek_token(&parser);
      if (token->kind == TK_Eof)
        break;

      TRACE("Parsing top level node at %s:%zu:%zu (%s)",
            source_location_filename(&token->loc),
            source_location_line(&token->loc), source_location_col(&token->loc),
            token->chars.data);

      // Note that the parse_* functions destroy the tokens.
      TopLevelNode* top_level_decl = parse_top_level_decl(&parser);

      TopLevelNode** storage = vector_append_storage(&ast_nodes);
      *storage = top_level_decl;
    }

    struct ParsedArgument* do_dump_ast;
    if (tree_map_get(&parsed_args, "ast-dump", &do_dump_ast) &&
        do_dump_ast->stored_value) {
      dump_ast(&ast_nodes);

      destroy_parsed_args(&parsed_args);
      destroy_ast_nodes(&ast_nodes);
      parser_destroy(&parser);
      input_stream_destroy(&pp.base);

      destroy_include_dirs(&include_dir_paths);
      return 0;
    }

    parser_destroy(&parser);
    input_stream_destroy(&pp.base);
    input_stream_destroy(&file_input.base);
  }

  Sema sema;
  sema_construct(&sema);

  // Perform semantic analysis on the AST.
  for (const TopLevelNode** it = vector_begin(&ast_nodes);
       it != vector_end(&ast_nodes); ++it) {
    const TopLevelNode* top_level_decl = *it;

    switch (top_level_decl->vtable->kind) {
      case TLNK_Typedef: {
        Typedef* td = (Typedef*)top_level_decl;
        sema_add_typedef_type(&sema, td->name, td->type);
        break;
      }
      case TLNK_StaticAssert: {
        StaticAssert* sa = (StaticAssert*)top_level_decl;

        TreeMap dummy_ctx;
        string_tree_map_construct(&dummy_ctx);
        sema_verify_static_assert_condition(&sema, sa->expr, &dummy_ctx);
        tree_map_destroy(&dummy_ctx);

        break;
      }
      case TLNK_GlobalVariable: {
        GlobalVariable* gv = (GlobalVariable*)top_level_decl;
        sema_handle_global_variable(&sema, gv);
        break;
      }
      case TLNK_FunctionDefinition: {
        FunctionDefinition* f = (FunctionDefinition*)top_level_decl;
        sema_handle_function_definition(&sema, f);
        break;
      }
      case TLNK_StructDeclaration: {
        StructDeclaration* decl = (StructDeclaration*)top_level_decl;
        sema_handle_struct_declaration(&sema, decl);
        break;
      }
      case TLNK_EnumDeclaration: {
        EnumDeclaration* decl = (EnumDeclaration*)top_level_decl;
        sema_handle_enum_declaration(&sema, decl);
        break;
      }
      case TLNK_UnionDeclaration: {
        UnionDeclaration* decl = (UnionDeclaration*)top_level_decl;
        sema_handle_union_declaration(&sema, decl);
        break;
      }
    }
  }

  // LLVM Initialization
  LLVMModuleRef mod = LLVMModuleCreateWithName(input_filename);
  LLVMDIBuilderRef dibuilder = LLVMCreateDIBuilder(mod);

  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86Target();
  LLVMInitializeX86TargetMC();
  LLVMInitializeX86AsmParser();
  LLVMInitializeX86AsmPrinter();

  char* triple = LLVMGetDefaultTargetTriple();

  char* error;
  LLVMTargetRef target;
  if (LLVMGetTargetFromTriple(triple, &target, &error)) {
    printf("llvm error: %s\n", error);
    LLVMDisposeMessage(error);
    LLVMDisposeMessage(triple);
    return -1;
  }

  char* cpu = LLVMGetHostCPUName();
  char* features = LLVMGetHostCPUFeatures();

  LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
      target, triple, cpu, features, LLVMCodeGenLevelNone, LLVMRelocPIC,
      LLVMCodeModelDefault);

  LLVMDisposeMessage(triple);
  LLVMDisposeMessage(cpu);
  LLVMDisposeMessage(features);

  LLVMTargetDataRef data_layout = LLVMCreateTargetDataLayout(target_machine);
  LLVMSetModuleDataLayout(mod, data_layout);

  Compiler compiler;
  compiler_construct(&compiler, mod, &sema, dibuilder);

  const char* output = "out.obj";
  struct ParsedArgument* output_arg;
  if (tree_map_get(&parsed_args, "output", &output_arg))
    output = output_arg->value;

  // Compile the AST.
  for (const TopLevelNode** it = vector_begin(&ast_nodes);
       it != vector_end(&ast_nodes); ++it) {
    const TopLevelNode* top_level_decl = *it;

    switch (top_level_decl->vtable->kind) {
      case TLNK_Typedef:
      case TLNK_StaticAssert:
      case TLNK_StructDeclaration:
      case TLNK_EnumDeclaration:
      case TLNK_UnionDeclaration:
        break;
      case TLNK_GlobalVariable: {
        GlobalVariable* gv = (GlobalVariable*)top_level_decl;
        compile_global_variable(&compiler, gv);
        break;
      }
      case TLNK_FunctionDefinition: {
        FunctionDefinition* f = (FunctionDefinition*)top_level_decl;
        compile_function_definition(&compiler, f);
        break;
      }
    }

    if (LLVMVerifyModule(mod, LLVMPrintMessageAction, NULL)) {
      printf("Verify module failed\n");
      LLVMDumpModule(mod);
      __builtin_trap();
    }
  }

  LLVMDIBuilderFinalize(dibuilder);

  struct ParsedArgument* emit_llvm;
  int ret_code = 0;

  if (tree_map_get(&parsed_args, "emit-llvm", &emit_llvm) &&
      emit_llvm->stored_value) {
    if (LLVMPrintModuleToFile(mod, output, &error)) {
      printf("llvm error: %s\n", error);
      LLVMDisposeMessage(error);
      ret_code = -1;
    }
  } else if (LLVMTargetMachineEmitToFile(target_machine, mod, output,
                                         LLVMObjectFile, &error)) {
    ret_code = -1;
  }

  destroy_ast_nodes(&ast_nodes);

  compiler_destroy(&compiler);
  sema_destroy(&sema);
  destroy_parsed_args(&parsed_args);

  destroy_include_dirs(&include_dir_paths);

  LLVMDisposeModule(mod);
  LLVMDisposeTargetData(data_layout);
  LLVMDisposeTargetMachine(target_machine);
  LLVMDisposeDIBuilder(dibuilder);

  if (ret_code != 0) {
    printf("llvm error: %s\n", error);
    LLVMDisposeMessage(error);
  }

  return ret_code;
}
