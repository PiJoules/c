#include "type.h"

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "expr.h"

void type_construct(Type* type, const TypeVtable* vtable) {
  type->vtable = vtable;
  type->qualifiers = 0;
  type->align = NULL;
}

void type_destroy(Type* type) {
  type->vtable->dtor(type);
  if (type->align) {
    expr_destroy(type->align);
    free(type->align);
  }
}

void type_dump(const Type* type) {
  ASSERT_MSG(type->vtable->dump, "TODO: Implement dump for type %d",
             type->vtable->kind);
  type->vtable->dump(type);
}

static void replacement_sentinel_type_destroy(Type*) {}

static const TypeVtable ReplacementSentinelTypeVtable = {
    .kind = TK_ReplacementSentinelType,
    .dtor = &replacement_sentinel_type_destroy,
    .dump = NULL,
};

ReplacementSentinelType GetSentinel() {
  ReplacementSentinelType sentinel;
  type_construct(&sentinel.type, &ReplacementSentinelTypeVtable);
  return sentinel;
}

static void builtin_type_destroy(Type*) {}
static void builtin_type_dump(const Type* type);

static const TypeVtable BuiltinTypeVtable = {
    .kind = TK_BuiltinType,
    .dtor = builtin_type_destroy,
    .dump = builtin_type_dump,
};

void builtin_type_construct(BuiltinType* bt, BuiltinTypeKind kind) {
  type_construct(&bt->type, &BuiltinTypeVtable);
  bt->kind = kind;
}

BuiltinType* create_builtin_type(BuiltinTypeKind kind) {
  BuiltinType* bt = malloc(sizeof(BuiltinType));
  builtin_type_construct(bt, kind);
  return bt;
}

static void builtin_type_dump(const Type* type) {
  const BuiltinType* bt = (const BuiltinType*)type;
  printf("BuiltinType kind=%d\n", bt->kind);
}

bool is_builtin_type(const Type* type, BuiltinTypeKind kind) {
  if (type->vtable->kind != TK_BuiltinType)
    return false;
  return ((const BuiltinType*)type)->kind == kind;
}

bool is_pointer_type(const Type* type) {
  return type->vtable->kind == TK_PointerType;
}

bool is_array_type(const Type* type) {
  return type->vtable->kind == TK_ArrayType;
}

bool is_integral_type(const Type* type) {
  if (type->vtable->kind != TK_BuiltinType)
    return false;

  switch (((const BuiltinType*)type)->kind) {
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
    case BTK_UnsignedLongLong:
    case BTK_Bool:
      return true;
    default:
      return false;
  }
}

unsigned get_integral_rank(const BuiltinType* bt) {
  assert(is_integral_type(&bt->type));

  switch (bt->kind) {
    case BTK_Bool:
      return 0;
    case BTK_Char:
    case BTK_SignedChar:
    case BTK_UnsignedChar:
      return 1;
    case BTK_Short:
    case BTK_UnsignedShort:
      return 2;
    case BTK_Int:
    case BTK_UnsignedInt:
      return 3;
    case BTK_Long:
    case BTK_UnsignedLong:
      return 4;
    case BTK_LongLong:
    case BTK_UnsignedLongLong:
      return 5;
    default:
      UNREACHABLE_MSG("Unhandled builtin type kind %d", bt->kind);
  }
}

bool is_unsigned_integral_type(const Type* type) {
  if (type->vtable->kind != TK_BuiltinType)
    return false;

  switch (((const BuiltinType*)type)->kind) {
    case BTK_UnsignedChar:
    case BTK_UnsignedShort:
    case BTK_UnsignedInt:
    case BTK_UnsignedLong:
    case BTK_UnsignedLongLong:
      return true;
    default:
      return false;
  }
}

static void struct_type_destroy(Type*);
static void struct_type_dump(const Type*);

static const TypeVtable StructTypeVtable = {
    .kind = TK_StructType,
    .dtor = struct_type_destroy,
    .dump = struct_type_dump,
};

void struct_member_destroy(Member* member) {
  if (member->name)
    free(member->name);

  type_destroy(member->type);
  free(member->type);

  if (member->bitfield) {
    expr_destroy(member->bitfield);
    free(member->bitfield);
  }
}

void struct_type_construct(StructType* st, char* name, vector* members) {
  type_construct(&st->type, &StructTypeVtable);
  st->name = name;
  st->members = members;

  // Empty structs are not allowed in C.
  if (members)
    assert(members->size > 0);

  st->packed = false;
}

void struct_type_destroy(Type* type) {
  StructType* st = (StructType*)type;

  if (st->name)
    free(st->name);

  if (st->members) {
    for (size_t i = 0; i < st->members->size; ++i) {
      Member* sm = vector_at(st->members, i);
      struct_member_destroy(sm);
    }
    vector_destroy(st->members);
    free(st->members);
  }
}

void struct_type_dump(const Type* type) {
  const StructType* st = (const StructType*)type;
  printf("<StructType name=%s packed=%d>\n", st->name, st->packed);
  if (st->members) {
    for (size_t i = 0; i < st->members->size; ++i) {
      Member* sm = vector_at(st->members, i);
      printf("Member name=%s type=\n", sm->name);
      type_dump(sm->type);
    }
  }
  printf("</StructType>\n");
}

const Member* struct_get_member(const StructType* st, const char* name,
                                size_t* offset) {
  ASSERT_MSG(st->members, "No members in struct '%s'", st->name);

  for (size_t i = 0; i < st->members->size; ++i) {
    const Member* member = vector_at(st->members, i);
    if (strcmp(member->name, name) == 0) {
      if (offset)
        *offset = i;
      return member;
    }
  }

  UNREACHABLE_MSG("No member named '%s'", name);
}

const Member* struct_get_nth_member(const StructType* st, size_t n) {
  assert(st->members);
  assert(st->members->size > n);
  return vector_at(st->members, n);
}

static void union_type_destroy(Type*);
static void union_type_dump(const Type*);

static const TypeVtable UnionTypeVtable = {
    .kind = TK_UnionType,
    .dtor = union_type_destroy,
    .dump = union_type_dump,
};

void union_type_construct(UnionType* ut, char* name, vector* members) {
  type_construct(&ut->type, &UnionTypeVtable);
  ut->name = name;
  ut->members = members;

  // Empty unions are not allowed in C, though most compilers offer this as an
  // extension.
  if (members)
    assert(members->size > 0);

  ut->packed = false;
}

// TODO: Much of these methods can be shared between unions and structs.
void union_type_destroy(Type* type) {
  UnionType* ut = (UnionType*)type;

  if (ut->name)
    free(ut->name);

  if (ut->members) {
    for (size_t i = 0; i < ut->members->size; ++i) {
      Member* sm = vector_at(ut->members, i);
      struct_member_destroy(sm);
    }
    vector_destroy(ut->members);
    free(ut->members);
  }
}

void union_type_dump(const Type* type) {
  const UnionType* ut = (const UnionType*)type;
  printf("<UnionType name=%s packed=%d>\n", ut->name, ut->packed);
  if (ut->members) {
    for (size_t i = 0; i < ut->members->size; ++i) {
      Member* sm = vector_at(ut->members, i);
      printf("Member name=%s type=\n", sm->name);
      type_dump(sm->type);
    }
  }
  printf("</UnionType>\n");
}

const Member* union_get_member(const UnionType* ut, const char* name,
                               size_t* offset) {
  assert(ut->members);

  for (size_t i = 0; i < ut->members->size; ++i) {
    const Member* member = vector_at(ut->members, i);
    if (strcmp(member->name, name) == 0) {
      if (offset)
        *offset = i;
      return member;
    }
  }

  UNREACHABLE_MSG("No member named '%s'", name);
}

static void enum_type_destroy(Type*);
static void enum_type_dump(const Type*);

static const TypeVtable EnumTypeVtable = {
    .kind = TK_EnumType,
    .dtor = enum_type_destroy,
    .dump = enum_type_dump,
};

void enum_type_construct(EnumType* et, char* name, vector* members) {
  type_construct(&et->type, &EnumTypeVtable);
  et->name = name;
  et->members = members;
}

void enum_type_destroy(Type* type) {
  EnumType* et = (EnumType*)type;

  if (et->name)
    free(et->name);

  if (et->members) {
    for (size_t i = 0; i < et->members->size; ++i) {
      EnumMember* em = vector_at(et->members, i);

      if (em->name)
        free(em->name);

      if (em->value) {
        expr_destroy(em->value);
        free(em->value);
      }
    }
    vector_destroy(et->members);
    free(et->members);
  }
}

void enum_type_dump(const Type* type) {
  const EnumType* et = (const EnumType*)type;
  printf("<EnumType name=%s>\n", et->name);
}

static void function_type_destroy(Type*);
static void function_type_dump(const Type*);

static const TypeVtable FunctionTypeVtable = {
    .kind = TK_FunctionType,
    .dtor = function_type_destroy,
    .dump = function_type_dump,
};

void function_type_construct(FunctionType* f, Type* return_type,
                             vector pos_args) {
  type_construct(&f->type, &FunctionTypeVtable);
  f->return_type = return_type;
  f->pos_args = pos_args;
  f->has_var_args = false;
}

void function_type_destroy(Type* type) {
  FunctionType* f = (FunctionType*)type;
  type_destroy(f->return_type);
  free(f->return_type);

  for (size_t i = 0; i < f->pos_args.size; ++i) {
    FunctionArg* a = vector_at(&f->pos_args, i);
    if (a->name)
      free(a->name);
    type_destroy(a->type);
    free(a->type);
  }
  vector_destroy(&f->pos_args);
}

void function_type_dump(const Type* type) {
  const FunctionType* ft = (const FunctionType*)type;
  printf("<FunctionType has_var_args=%d>\n", ft->has_var_args);
  printf("return_type ");
  type_dump(ft->return_type);
  for (size_t i = 0; i < ft->pos_args.size; ++i) {
    FunctionArg* a = vector_at(&ft->pos_args, i);
    printf("pos_arg i=%zu name=%s ", i, a->name);
    type_dump(a->type);
  }
  printf("</FunctionType>\n");
}

Type* get_arg_type(const FunctionType* func, size_t i) {
  FunctionArg* a = vector_at(&func->pos_args, i);
  return a->type;
}

static void array_type_destroy(Type*);

static const TypeVtable ArrayTypeVtable = {
    .kind = TK_ArrayType,
    .dtor = array_type_destroy,
    .dump = NULL,
};

void array_type_construct(ArrayType* arr, Type* elem_type, struct Expr* size) {
  type_construct(&arr->type, &ArrayTypeVtable);
  arr->elem_type = elem_type;
  arr->size = size;
}

ArrayType* create_array_of(Type* elem, struct Expr* size) {
  ArrayType* arr = malloc(sizeof(ArrayType));
  array_type_construct(arr, elem, size);
  return arr;
}

void array_type_destroy(Type* type) {
  ArrayType* arr = (ArrayType*)type;
  type_destroy(arr->elem_type);
  free(arr->elem_type);

  if (arr->size) {
    expr_destroy(arr->size);
    free(arr->size);
  }
}

static void pointer_type_destroy(Type*);
static void pointer_type_dump(const Type*);

static const TypeVtable PointerTypeVtable = {
    .kind = TK_PointerType,
    .dtor = pointer_type_destroy,
    .dump = pointer_type_dump,
};

void pointer_type_construct(PointerType* ptr, Type* pointee) {
  type_construct(&ptr->type, &PointerTypeVtable);
  ptr->pointee = pointee;
}

void pointer_type_destroy(Type* type) {
  PointerType* ptr = (PointerType*)type;
  type_destroy(ptr->pointee);
  free(ptr->pointee);
}

void pointer_type_dump(const Type* type) {
  const PointerType* pt = (const PointerType*)type;
  printf("<PointerType>\n");
  type_dump(pt->pointee);
  printf("</PointerType>\n");
}

PointerType* create_pointer_to(Type* type) {
  PointerType* ptr = malloc(sizeof(PointerType));
  pointer_type_construct(ptr, type);
  return ptr;
}

bool is_pointer_to(const Type* type, TypeKind pointee) {
  if (!is_pointer_type(type))
    return false;
  return ((const PointerType*)type)->pointee->vtable->kind == pointee;
}

const Type* get_pointee(const Type* type) {
  return ((const PointerType*)type)->pointee;
}

static void non_owning_pointer_type_destroy(Type*) {}
static void non_owning_pointer_type_dump(const Type*);

static const TypeVtable NonOwningPointerTypeVtable = {
    .kind = TK_PointerType,
    .dtor = non_owning_pointer_type_destroy,
    .dump = non_owning_pointer_type_dump,
};

void non_owning_pointer_type_construct(NonOwningPointerType* ptr,
                                       const Type* pointee) {
  type_construct(&ptr->type, &NonOwningPointerTypeVtable);
  ptr->pointee = pointee;
}

void non_owning_pointer_type_dump(const Type* type) {
  const NonOwningPointerType* pt = (const NonOwningPointerType*)type;
  printf("<NonOwningPointerType>\n");
  type_dump(pt->pointee);
  printf("</NonOwningPointerType>\n");
}

static void named_type_destroy(Type*);
static void named_type_dump(const Type*);

static const TypeVtable NamedTypeVtable = {
    .kind = TK_NamedType,
    .dtor = named_type_destroy,
    .dump = named_type_dump,
};

void named_type_construct(NamedType* nt, const char* name) {
  type_construct(&nt->type, &NamedTypeVtable);
  nt->name = strdup(name);
}

void named_type_destroy(Type* type) { free(((NamedType*)type)->name); }

void named_type_dump(const Type* type) {
  const NamedType* nt = (const NamedType*)type;
  printf("NamedType name=%s\n", nt->name);
}

NamedType* create_named_type(const char* name) {
  NamedType* nt = malloc(sizeof(NamedType));
  named_type_construct(nt, name);
  return nt;
}
