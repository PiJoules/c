#ifndef TYPE_H_
#define TYPE_H_

#include <stdint.h>
#include <string.h>

#include "vector.h"

typedef enum {
  TK_BuiltinType,
  TK_NamedType,  // Named from a typedef
  TK_StructType,
  TK_EnumType,
  TK_UnionType,
  TK_PointerType,
  TK_ArrayType,
  TK_FunctionType,

  // This is a special type used for lazy replacements of types in the AST.
  // This does not reflect an C types.
  TK_ReplacementSentinelType,
} TypeKind;

struct Type;

typedef void (*TypeDtor)(struct Type*);
typedef size_t (*TypeGetSize)(const struct Type*);
typedef void (*TypeDump)(const struct Type*);

typedef struct {
  TypeKind kind;
  TypeDtor dtor;
  TypeDump dump;
} TypeVtable;

static const int kConstShift = 0;
static const int kVolatileShift = 1;
static const int kRestrictShift = 2;
static const int kConstMask = 1 << kConstShift;
static const int kVolatileMask = 1 << kVolatileShift;
static const int kRestrictMask = 1 << kRestrictShift;

typedef uint8_t Qualifiers;

struct Type {
  const TypeVtable* vtable;

  // bit 0: const
  // bit 1: volatile
  // bit 2: restrict
  Qualifiers qualifiers;
};
typedef struct Type Type;

void type_construct(Type* type, const TypeVtable* vtable);
void type_destroy(Type* type);
void type_dump(const Type* type);

static inline void type_set_const(Type* type) {
  type->qualifiers |= kConstMask;
}
static inline void type_set_volatile(Type* type) {
  type->qualifiers |= kVolatileMask;
}
static inline void type_set_restrict(Type* type) {
  type->qualifiers |= kRestrictMask;
}
static inline bool type_is_const(Type* type) {
  return (bool)(type->qualifiers & kConstMask);
}
static inline bool type_is_volatile(Type* type) {
  return (bool)(type->qualifiers & kVolatileMask);
}
static inline bool type_is_restrict(Type* type) {
  return (bool)(type->qualifiers & kRestrictMask);
}

typedef struct {
  Type type;
} ReplacementSentinelType;

ReplacementSentinelType GetSentinel();

typedef enum {
  BTK_Char,
  BTK_SignedChar,
  BTK_UnsignedChar,
  BTK_Short,
  BTK_UnsignedShort,
  BTK_Int,
  BTK_UnsignedInt,
  BTK_Long,
  BTK_UnsignedLong,
  BTK_LongLong,
  BTK_UnsignedLongLong,
  BTK_Float,
  BTK_Double,
  BTK_LongDouble,
  BTK_Float128,
  BTK_ComplexFloat,
  BTK_ComplexDouble,
  BTK_ComplexLongDouble,
  BTK_Void,
  BTK_Bool,
  BTK_BuiltinVAList,
} BuiltinTypeKind;

typedef struct {
  Type type;
  BuiltinTypeKind kind;
} BuiltinType;

void builtin_type_construct(BuiltinType* bt, BuiltinTypeKind kind);
BuiltinType* create_builtin_type(BuiltinTypeKind kind);

bool is_builtin_type(const Type* type, BuiltinTypeKind kind);
bool is_pointer_type(const Type* type);
bool is_pointer_to(const Type* type, TypeKind pointee);
bool is_array_type(const Type* type);
bool is_integral_type(const Type* type);
bool is_unsigned_integral_type(const Type* type);
static inline bool is_void_type(const Type* type) {
  return is_builtin_type(type, BTK_Void);
}
static inline bool is_bool_type(const Type* type) {
  return is_builtin_type(type, BTK_Bool);
}

unsigned get_integral_rank(const BuiltinType* bt);

typedef struct {
  Type* type;
  char* name;             // Optional. Allocated by the creator of this Member.
  struct Expr* bitfield;  // Optional.
} Member;

void struct_member_destroy(Member* member);

typedef struct {
  Type type;
  char* name;  // Optional.

  // This is a vector of Members.
  // NULL indicates this is a struct declaration but not definition.
  vector* members;

  bool packed;
} StructType;

void struct_type_construct(StructType* st, char* name, vector* members);
const Member* struct_get_member(const StructType* st, const char* name,
                                size_t* offset);
const Member* struct_get_nth_member(const StructType* st, size_t n);

typedef struct {
  Type type;
  char* name;  // Optional.

  // This is a vector of Members.
  // NULL indicates this is a struct declaration but not definition.
  vector* members;

  bool packed;
} UnionType;

void union_type_construct(UnionType* ut, char* name, vector* members);
const Member* union_get_member(const UnionType* ut, const char* name,
                               size_t* offset);

typedef struct {
  char* name;          // Allocated by the creator of this EnumMember.
  struct Expr* value;  // Optional
} EnumMember;

typedef struct {
  Type type;
  char* name;  // Optional.

  // This is a vector of EnumMembers.
  // NULL indicates this is an enum declaration but not definition.
  vector* members;
} EnumType;

void enum_type_construct(EnumType* et, char* name, vector* members);

typedef struct {
  char* name;  // Optional.
  Type* type;
} FunctionArg;

typedef struct {
  Type type;
  Type* return_type;
  vector pos_args;  // vector of FunctionArgs.
  bool has_var_args;
} FunctionType;

void function_type_construct(FunctionType* f, Type* return_type,
                             vector pos_args);
Type* get_arg_type(const FunctionType* func, size_t i);

typedef struct {
  Type type;
  Type* elem_type;
  struct Expr* size;  // NULL indicates no specified size.
} ArrayType;

void array_type_construct(ArrayType* arr, Type* elem_type, struct Expr* size);
ArrayType* create_array_of(Type* elem, struct Expr* size);

typedef struct {
  Type type;
  Type* pointee;
} PointerType;

void pointer_type_construct(PointerType* ptr, Type* pointee);
PointerType* create_pointer_to(Type* type);
const Type* get_pointee(const Type* type);

// This is just like a PointerType with the only difference being it doesn't
// "own" the underlying pointee type. That is, it will not destroy and free
// the underlying type on this type's destruction. This should only be used by
// Sema which needs to lazily create pointer types via AddressOf.
//
// This is meant to act as a POD, so users creating this object do not need to
// explicitly destroy it.
typedef struct {
  Type type;
  const Type* pointee;
} NonOwningPointerType;

void non_owning_pointer_type_construct(NonOwningPointerType* ptr,
                                       const Type* pointee);

typedef struct {
  Type type;
  char* name;
} NamedType;

void named_type_construct(NamedType* nt, const char* name);
NamedType* create_named_type(const char* name);

#endif  // TYPE_H_
