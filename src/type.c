#include "type.h"

#include <stdio.h>

#include "common.h"
#include "expr.h"

void type_construct(Type* type, const TypeVtable* vtable) {
  type->vtable = vtable;
  type->qualifiers = 0;
}

void type_destroy(Type* type) { type->vtable->dtor(type); }

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
