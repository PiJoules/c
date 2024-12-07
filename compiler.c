typedef unsigned long long uint64_t;
static_assert(sizeof(uint64_t) == 8);

// This likely isn't the best size but it's suitable for 64-bit host.
typedef unsigned long long size_t;
static_assert(sizeof(size_t) == 8);

typedef unsigned long long uintptr_t;
static_assert(sizeof(uintptr_t) == sizeof(void *));

typedef unsigned char uint8_t;
static_assert(sizeof(uint8_t) == 1);

// IO functions
int printf(const char *, ...);

// File functions
// Normally each of these would operate on FILE pointers, but for ABI purposes
// we can use void* here.
void *fopen(const char *, const char *);
int fclose(void *);
int fgetc(void *);
int feof(void *);

// Alloc functions
void *malloc(size_t);
void *realloc(void *, size_t);
void free(void *);

// String functions
size_t strlen(const char *);
int strncmp(const char *, const char *, size_t);
void *memcpy(void *, const void *, size_t);
int strcmp(const char *, const char *);
char *strdup(const char *);

// ctype.h functions
int isspace(int);
int isalnum(int);
int isdigit(int);

unsigned long long strtoull(const char *restrict, char **restrict, int);

void assert(bool b) {
  if (!b)
    __builtin_trap();
}

// FIXME: This shold be a macro.
void *const NULL = (void *)(0);

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

// LLVM API
typedef struct LLVMOpaqueType *LLVMTypeRef;
typedef int LLVMBool;
typedef struct LLVMOpaqueContext *LLVMContextRef;
typedef struct LLVMOpaqueModule *LLVMModuleRef;
typedef struct LLVMTarget *LLVMTargetRef;
typedef struct LLVMOpaqueTargetMachine *LLVMTargetMachineRef;
typedef struct LLVMOpaqueTargetData *LLVMTargetDataRef;
typedef struct LLVMOpaqueValue *LLVMValueRef;
typedef struct LLVMOpaqueBasicBlock *LLVMBasicBlockRef;
typedef struct LLVMOpaqueBuilder *LLVMBuilderRef;

LLVMModuleRef LLVMModuleCreateWithName(const char *ModuleID);
const char *LLVMGetDataLayoutStr(LLVMModuleRef M);

typedef enum {
  LLVMAbortProcessAction, /* verifier will print to stderr and abort() */
  LLVMPrintMessageAction, /* verifier will print to stderr and return 1 */
  LLVMReturnStatusAction  /* verifier will just return 1 */
} LLVMVerifierFailureAction;
LLVMBool LLVMVerifyModule(LLVMModuleRef M, LLVMVerifierFailureAction Action,
                          char **OutMessage);
LLVMBool LLVMVerifyFunction(LLVMValueRef Fn, LLVMVerifierFailureAction Action);
void LLVMDumpValue(LLVMValueRef Val);
void LLVMDumpType(LLVMTypeRef Val);
void LLVMDumpModule(LLVMModuleRef M);

LLVMValueRef LLVMAddGlobal(LLVMModuleRef M, LLVMTypeRef Ty, const char *Name);
void LLVMSetInitializer(LLVMValueRef GlobalVar, LLVMValueRef ConstantVal);
LLVMValueRef LLVMAddFunction(LLVMModuleRef M, const char *Name,
                             LLVMTypeRef FuncTy);
LLVMValueRef LLVMGetNamedFunction(LLVMModuleRef M, const char *Name);
LLVMValueRef LLVMGetNamedGlobal(LLVMModuleRef M, const char *Name);
char *LLVMPrintModuleToString(LLVMModuleRef M);
char *LLVMPrintValueToString(LLVMValueRef Val);
void LLVMDisposeMessage(char *Message);
LLVMTypeRef LLVMVoidType(void);
LLVMTypeRef LLVMInt1Type();
LLVMTypeRef LLVMInt8Type();
LLVMTypeRef LLVMInt16Type();
LLVMTypeRef LLVMInt32Type();
LLVMTypeRef LLVMIntType(unsigned NumBits);
LLVMTypeRef LLVMFloatTypeInContext(LLVMContextRef C);
LLVMTypeRef LLVMDoubleTypeInContext(LLVMContextRef C);
LLVMTypeRef LLVMFP128TypeInContext(LLVMContextRef C);
void *LLVMStructCreateNamed(void *C, const char *Name);
LLVMTypeRef LLVMStructType(LLVMTypeRef *ElementTypes, unsigned ElementCount,
                           LLVMBool Packed);
void *LLVMGetTypeByName(void *M, const char *Name);
void LLVMStructSetBody(LLVMTypeRef StructTy, LLVMTypeRef *ElementTypes,
                       unsigned ElementCount, LLVMBool Packed);
LLVMTypeRef LLVMPointerTypeInContext(LLVMContextRef C, unsigned AddressSpace);
LLVMTypeRef LLVMArrayType(LLVMTypeRef ElementType, unsigned ElementCount);
LLVMTypeRef LLVMFunctionType(LLVMTypeRef ReturnType, LLVMTypeRef *ParamTypes,
                             unsigned ParamCount, LLVMBool IsVarArg);
LLVMTypeRef LLVMPointerType(LLVMTypeRef ElementType, unsigned AddressSpace);
char *LLVMPrintTypeToString(LLVMTypeRef Val);

LLVMBasicBlockRef LLVMAppendBasicBlock(LLVMValueRef Fn, const char *Name);
LLVMBuilderRef LLVMCreateBuilder(void);
void LLVMPositionBuilderAtEnd(LLVMBuilderRef Builder, LLVMBasicBlockRef Block);
LLVMValueRef LLVMConstInt(LLVMTypeRef IntTy, unsigned long long N,
                          LLVMBool SignExtend);
unsigned long long LLVMConstIntGetZExtValue(LLVMValueRef ConstantVal);
LLVMBasicBlockRef LLVMAppendBasicBlockInContext(LLVMContextRef C,
                                                LLVMValueRef Fn,
                                                const char *Name);
LLVMBasicBlockRef LLVMCreateBasicBlockInContext(LLVMContextRef C,
                                                const char *Name);
LLVMContextRef LLVMGetBuilderContext(LLVMBuilderRef Builder);
LLVMBasicBlockRef LLVMGetInsertBlock(LLVMBuilderRef Builder);
LLVMValueRef LLVMGetBasicBlockParent(LLVMBasicBlockRef BB);
LLVMValueRef LLVMBuildCondBr(LLVMBuilderRef, LLVMValueRef If,
                             LLVMBasicBlockRef Then, LLVMBasicBlockRef Else);
void LLVMPositionBuilderAtEnd(LLVMBuilderRef Builder, LLVMBasicBlockRef Block);
LLVMValueRef LLVMBuildBr(LLVMBuilderRef, LLVMBasicBlockRef Dest);
LLVMContextRef LLVMGetValueContext(LLVMValueRef Val);
LLVMContextRef LLVMGetModuleContext(LLVMModuleRef M);
void LLVMAppendExistingBasicBlock(LLVMValueRef Fn, LLVMBasicBlockRef BB);
LLVMTypeRef LLVMTypeOf(LLVMValueRef Val);
LLVMValueRef LLVMConstNull(LLVMTypeRef Ty);
LLVMValueRef LLVMConstAllOnes(LLVMTypeRef Ty);
LLVMValueRef LLVMGetParam(LLVMValueRef Fn, unsigned Index);
void LLVMSetValueName2(LLVMValueRef Val, const char *Name, size_t NameLen);
LLVMValueRef LLVMBuildAlloca(LLVMBuilderRef, LLVMTypeRef Ty, const char *Name);
LLVMValueRef LLVMBuildStore(LLVMBuilderRef, LLVMValueRef Val, LLVMValueRef Ptr);
LLVMValueRef LLVMBuildLoad2(LLVMBuilderRef, LLVMTypeRef Ty,
                            LLVMValueRef PointerVal, const char *Name);
LLVMValueRef LLVMBuildCall2(LLVMBuilderRef, LLVMTypeRef, LLVMValueRef Fn,
                            LLVMValueRef *Args, unsigned NumArgs,
                            const char *Name);
unsigned LLVMLookupIntrinsicID(const char *Name, size_t NameLen);
LLVMValueRef LLVMGetIntrinsicDeclaration(LLVMModuleRef Mod, unsigned ID,
                                         LLVMTypeRef *ParamTypes,
                                         size_t ParamCount);
LLVMTypeRef LLVMIntrinsicGetType(LLVMContextRef Ctx, unsigned ID,
                                 LLVMTypeRef *ParamTypes, size_t ParamCount);
LLVMValueRef LLVMBuildRetVoid(LLVMBuilderRef);
LLVMValueRef LLVMBuildRet(LLVMBuilderRef, LLVMValueRef V);
LLVMValueRef LLVMConstIntToPtr(LLVMValueRef ConstantVal, LLVMTypeRef ToType);
LLVMValueRef LLVMConstPtrToInt(LLVMValueRef ConstantVal, LLVMTypeRef ToType);
LLVMValueRef LLVMBuildAdd(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS,
                          const char *Name);
LLVMValueRef LLVMBuildSub(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS,
                          const char *Name);
LLVMValueRef LLVMBuildAnd(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS,
                          const char *Name);
LLVMValueRef LLVMBuildOr(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS,
                         const char *Name);
LLVMValueRef LLVMBuildTrunc(LLVMBuilderRef, LLVMValueRef Val,
                            LLVMTypeRef DestTy, const char *Name);
LLVMValueRef LLVMBuildZExt(LLVMBuilderRef, LLVMValueRef Val, LLVMTypeRef DestTy,
                           const char *Name);
LLVMValueRef LLVMBuildSExt(LLVMBuilderRef, LLVMValueRef Val, LLVMTypeRef DestTy,
                           const char *Name);
LLVMValueRef LLVMBuildGlobalStringPtr(LLVMBuilderRef B, const char *Str,
                                      const char *Name);
LLVMValueRef LLVMBuildXor(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS,
                          const char *Name);
LLVMValueRef LLVMBuildGEP2(LLVMBuilderRef B, LLVMTypeRef Ty,
                           LLVMValueRef Pointer, LLVMValueRef *Indices,
                           unsigned NumIndices, const char *Name);
LLVMValueRef 	LLVMBuildPhi (LLVMBuilderRef, LLVMTypeRef Ty, const char *Name);

typedef enum {
  LLVMIntEQ = 32, /**< equal */
  LLVMIntNE,      /**< not equal */
  LLVMIntUGT,     /**< unsigned greater than */
  LLVMIntUGE,     /**< unsigned greater or equal */
  LLVMIntULT,     /**< unsigned less than */
  LLVMIntULE,     /**< unsigned less or equal */
  LLVMIntSGT,     /**< signed greater than */
  LLVMIntSGE,     /**< signed greater or equal */
  LLVMIntSLT,     /**< signed less than */
  LLVMIntSLE      /**< signed less or equal */
} LLVMIntPredicate;

typedef enum {
  LLVMRealPredicateFalse, /**< Always false (always folded) */
  LLVMRealOEQ,            /**< True if ordered and equal */
  LLVMRealOGT,            /**< True if ordered and greater than */
  LLVMRealOGE,            /**< True if ordered and greater than or equal */
  LLVMRealOLT,            /**< True if ordered and less than */
  LLVMRealOLE,            /**< True if ordered and less than or equal */
  LLVMRealONE,            /**< True if ordered and operands are unequal */
  LLVMRealORD,            /**< True if ordered (no nans) */
  LLVMRealUNO,            /**< True if unordered: isnan(X) | isnan(Y) */
  LLVMRealUEQ,            /**< True if unordered or equal */
  LLVMRealUGT,            /**< True if unordered or greater than */
  LLVMRealUGE,            /**< True if unordered, greater than, or equal */
  LLVMRealULT,            /**< True if unordered or less than */
  LLVMRealULE,            /**< True if unordered, less than, or equal */
  LLVMRealUNE,            /**< True if unordered or not equal */
  LLVMRealPredicateTrue   /**< Always true (always folded) */
} LLVMRealPredicate;
LLVMValueRef LLVMBuildICmp(LLVMBuilderRef, LLVMIntPredicate Op,
                           LLVMValueRef LHS, LLVMValueRef RHS,
                           const char *Name);
LLVMValueRef LLVMBuildFCmp(LLVMBuilderRef, LLVMRealPredicate Op,
                           LLVMValueRef LHS, LLVMValueRef RHS,
                           const char *Name);

typedef enum {
  LLVMVoidTypeKind = 0,     /**< type with no size */
  LLVMHalfTypeKind = 1,     /**< 16 bit floating point type */
  LLVMFloatTypeKind = 2,    /**< 32 bit floating point type */
  LLVMDoubleTypeKind = 3,   /**< 64 bit floating point type */
  LLVMX86_FP80TypeKind = 4, /**< 80 bit floating point type (X87) */
  LLVMFP128TypeKind = 5, /**< 128 bit floating point type (112-bit mantissa)*/
  LLVMPPC_FP128TypeKind = 6, /**< 128 bit floating point type (two 64-bits) */
  LLVMLabelTypeKind = 7,     /**< Labels */
  LLVMIntegerTypeKind = 8,   /**< Arbitrary bit width integers */
  LLVMFunctionTypeKind = 9,  /**< Functions */
  LLVMStructTypeKind = 10,   /**< Structures */
  LLVMArrayTypeKind = 11,    /**< Arrays */
  LLVMPointerTypeKind = 12,  /**< Pointers */
  LLVMVectorTypeKind = 13,   /**< Fixed width SIMD vector type */
  LLVMMetadataTypeKind = 14, /**< Metadata */
                             /* 15 previously used by LLVMX86_MMXTypeKind */
  LLVMTokenTypeKind = 16,    /**< Tokens */
  LLVMScalableVectorTypeKind = 17, /**< Scalable SIMD vector type */
  LLVMBFloatTypeKind = 18,         /**< 16 bit brain floating point type */
  LLVMX86_AMXTypeKind = 19,        /**< X86 AMX */
  LLVMTargetExtTypeKind = 20,      /**< Target extension type */
} LLVMTypeKind;
LLVMTypeKind LLVMGetTypeKind(LLVMTypeRef Ty);
unsigned LLVMGetIntTypeWidth(LLVMTypeRef IntegerTy);

void LLVMInitializeX86TargetInfo();
void LLVMInitializeX86Target();
void LLVMInitializeX86TargetMC();
void LLVMInitializeX86AsmParser();
void LLVMInitializeX86AsmPrinter();

typedef enum {
  LLVMCodeGenLevelNone,
  LLVMCodeGenLevelLess,
  LLVMCodeGenLevelDefault,
  LLVMCodeGenLevelAggressive
} LLVMCodeGenOptLevel;

typedef enum {
  LLVMRelocDefault,
  LLVMRelocStatic,
  LLVMRelocPIC,
  LLVMRelocDynamicNoPic,
  LLVMRelocROPI,
  LLVMRelocRWPI,
  LLVMRelocROPI_RWPI
} LLVMRelocMode;

typedef enum {
  LLVMCodeModelDefault,
  LLVMCodeModelJITDefault,
  LLVMCodeModelTiny,
  LLVMCodeModelSmall,
  LLVMCodeModelKernel,
  LLVMCodeModelMedium,
  LLVMCodeModelLarge
} LLVMCodeModel;

typedef enum { LLVMAssemblyFile, LLVMObjectFile } LLVMCodeGenFileType;

char *LLVMGetDefaultTargetTriple(void);
LLVMBool LLVMGetTargetFromTriple(const char *Triple, LLVMTargetRef *T,
                                 char **ErrorMessage);
char *LLVMGetHostCPUName(void);
char *LLVMGetHostCPUFeatures(void);
LLVMTargetMachineRef LLVMCreateTargetMachine(
    LLVMTargetRef T, const char *Triple, const char *CPU, const char *Features,
    LLVMCodeGenOptLevel Level, LLVMRelocMode Reloc, LLVMCodeModel CodeModel);
LLVMTargetDataRef LLVMCreateTargetDataLayout(LLVMTargetMachineRef T);
void LLVMSetModuleDataLayout(LLVMModuleRef M, LLVMTargetDataRef DL);

///
/// Start Vector Implementation
///

static bool is_power_of_2(size_t x) { return (x & (x - 1)) == 0 && x != 0; }

static size_t align_up(size_t x, size_t alignment) {
  assert(is_power_of_2(alignment) && "Invalid alignment");
  return (x + (alignment - 1)) & ~(alignment - 1);
}

typedef struct {
  size_t data_size;
  size_t data_alignment;

  void *data;
  size_t size;
  size_t capacity;  // In bytes
} vector;

const size_t kDefaultVectorCapacity = 16;

void vector_construct(vector *v, size_t data_size, size_t data_alignment) {
  assert(is_power_of_2(data_alignment) && "Invalid alignment");

  v->data_size = data_size;
  v->data_alignment = data_alignment;
  v->size = 0;

  v->capacity =
      kDefaultVectorCapacity < data_size ? data_size : kDefaultVectorCapacity;
  v->capacity = align_up(v->capacity, data_alignment);

  v->data = malloc(v->capacity);
  assert(v->data);
  assert((uintptr_t)v->data % data_alignment == 0);
}

// Each user of a vector should destroy the object in it before calling
// this.
void vector_destroy(vector *v) { free(v->data); }

void vector_reserve(vector *v, size_t new_capacity) {
  if (new_capacity <= v->capacity)
    return;

  // Double the capacity.
  v->capacity = align_up(new_capacity * 2, v->data_alignment);

  v->data = realloc(v->data, v->capacity);
  assert(v->data);
  assert((uintptr_t)v->data % v->data_alignment == 0);
}

// Allocate more storage to the vector and return a pointer to the new
// storage so it can be initialized by the user.
void *vector_append_storage(vector *v) {
  size_t allocation_size = align_up(v->data_size, v->data_alignment);
  vector_reserve(v, (v->size + 1) * allocation_size);
  return (char *)v->data + (allocation_size * (v->size++));
}

// Return a pointer to the allocation of this element.
void *vector_at(const vector *v, size_t i) {
  size_t allocation_size = align_up(v->data_size, v->data_alignment);
  void *ptr = (char *)v->data + allocation_size * i;
  assert((uintptr_t)ptr % v->data_alignment == 0);
  return ptr;
}

///
/// End Vector Implementation
///

///
/// Start Vector Tests
///

static void TestVectorConstruction() {
  vector v;
  vector_construct(&v, /*data_size=*/4, /*data_alignment=*/4);

  assert(v.data != NULL);
  assert(v.size == 0);
  assert(v.capacity > 0);
  assert(v.capacity % 4 == 0);
  assert((uintptr_t)v.data % 4 == 0);

  vector_destroy(&v);
}

static void TestVectorCapacityReservation() {
  vector v;
  vector_construct(&v, 4, 4);
  size_t old_cap = v.capacity;
  vector_reserve(&v, old_cap * 2);

  assert(v.capacity >= old_cap);
  assert(v.size == 0);

  vector_destroy(&v);
}

static void TestVectorAppend() {
  vector v;
  vector_construct(&v, sizeof(int), alignof(int));

  int *storage = vector_append_storage(&v);
  *storage = 10;
  int *storage2 = vector_append_storage(&v);
  *storage2 = 100;

  assert(*(int *)vector_at(&v, 0) == 10);
  assert((uintptr_t)vector_at(&v, 0) % alignof(int) == 0);

  assert(*(int *)vector_at(&v, 1) == 100);
  assert((uintptr_t)vector_at(&v, 1) % alignof(int) == 0);

  assert(v.size == 2);

  vector_destroy(&v);
}

static void TestVectorResizing() {
  vector v;
  vector_construct(&v, sizeof(int), alignof(int));

  // Keep increasing the vector size until we exceed the capacity.
  // We'll keep adding until the size is twice the original. The
  // vector should automatically resize itself.
  size_t init_cap = v.capacity;
  for (size_t i = 0; i < init_cap * 2; ++i) {
    vector_append_storage(&v);
  }

  assert(v.size == init_cap * 2);
  assert(v.capacity >= init_cap * 2);

  vector_destroy(&v);
}

static void RunVectorTests() {
  TestVectorConstruction();
  TestVectorCapacityReservation();
  TestVectorAppend();
  TestVectorResizing();
}

///
/// End Vector Tests
///

///
/// Start Tree Map Implementation
///

typedef int (*TreeMapCmp)(const void *, const void *);
typedef const void *(*KeyCtor)(const void *);
typedef void (*KeyDtor)(const void *);

static void key_dtor_default(const void *) {}

static const void *key_ctor_default(const void *key) { return key; }

// The tree map does not own any of its values or keys. It's only in charge
// of destroying nodes it creates.
struct TreeMapNode {
  const void *key;
  void *value;
  struct TreeMapNode *left, *right;
  TreeMapCmp cmp;
  KeyCtor key_ctor;
  KeyDtor key_dtor;
};

typedef struct TreeMapNode TreeMap;

void tree_map_construct(TreeMap *map, TreeMapCmp cmp, KeyCtor ctor,
                        KeyDtor dtor) {
  map->left = NULL;
  map->right = NULL;
  map->key = NULL;
  map->value = NULL;
  map->cmp = cmp;
  map->key_ctor = ctor ? ctor : key_ctor_default;
  map->key_dtor = dtor ? dtor : key_dtor_default;
}

void tree_map_destroy(TreeMap *map) {
  if (map->left) {
    tree_map_destroy(map->left);
    free(map->left);
  }

  if (map->right) {
    tree_map_destroy(map->right);
    free(map->right);
  }

  map->key_dtor(map->key);
}

void tree_map_set(TreeMap *map, const void *key, void *val) {
  if (map->key == NULL) {
    map->key = map->key_ctor(key);
    map->value = val;
    return;
  }

  int cmp = map->cmp(map->key, key);
  if (cmp < 0) {
    if (map->left == NULL) {
      map->left = malloc(sizeof(TreeMap));
      assert(map->left);
      tree_map_construct(map->left, map->cmp, map->key_ctor, map->key_dtor);
    }
    tree_map_set(map->left, key, val);
  } else if (cmp > 0) {
    if (map->right == NULL) {
      map->right = malloc(sizeof(TreeMap));
      assert(map->right);
      tree_map_construct(map->right, map->cmp, map->key_ctor, map->key_dtor);
    }
    tree_map_set(map->right, key, val);
  } else {
    map->value = val;
  }
}

// Get a value from the map corresponding to a key. If the value was found,
// return true and set the value in `val`. Otherwise, return false.
bool tree_map_get(const TreeMap *map, const void *key, void **val) {
  if (map->key == NULL)
    return false;

  int cmp = map->cmp(map->key, key);
  if (cmp < 0) {
    if (map->left == NULL)
      return false;
    return tree_map_get(map->left, key, val);
  } else if (cmp > 0) {
    if (map->right == NULL)
      return false;
    return tree_map_get(map->right, key, val);
  } else {
    if (val)
      *val = map->value;
    return true;
  }
}

static int string_tree_map_cmp(const void *lhs, const void *rhs) {
  return strcmp(lhs, rhs);
}

static void string_tree_map_key_dtor(const void *key) {
  if (key)
    free((void *)key);
}

static const void *string_tree_map_key_ctor(const void *key) {
  size_t len = strlen((const char *)key);
  char *cpy = malloc(sizeof(char) * (len + 1));
  memcpy(cpy, key, len);
  cpy[len] = 0;
  return cpy;
}

void string_tree_map_construct(TreeMap *map) {
  tree_map_construct(map, string_tree_map_cmp, string_tree_map_key_ctor,
                     string_tree_map_key_dtor);
}

///
/// End Tree Map Implementation
///

///
/// Start Tree Map Tests
///

static void TestTreeMapConstruction() {
  TreeMap m;
  tree_map_construct(&m, NULL, NULL, NULL);

  assert(m.left == NULL);
  assert(m.right == NULL);
  assert(m.value == NULL);
  assert(m.key == NULL);

  tree_map_destroy(&m);
}

static void TestTreeMapInsertion() {
  TreeMap m;
  string_tree_map_construct(&m);

  void *res = NULL;
  assert(!tree_map_get(&m, "key", &res));

  const char *val = "val";
  tree_map_set(&m, "key", (char *)val);

  assert(tree_map_get(&m, "key", &res));
  assert(res == val);

  const char *val2 = "val2";
  tree_map_set(&m, "key2", (char *)val2);

  assert(tree_map_get(&m, "key2", &res));
  assert(res == val2);
  assert(tree_map_get(&m, "key", &res));
  assert(res == val);

  tree_map_destroy(&m);
}

static void TestTreeMapOverrideKeyValue() {
  TreeMap m;
  string_tree_map_construct(&m);

  const char *val = "val";
  tree_map_set(&m, "key", (char *)val);

  void *res = NULL;
  assert(tree_map_get(&m, "key", &res));
  assert(res == val);

  const char *newval = "newval";
  tree_map_set(&m, "key", (char *)newval);

  assert(tree_map_get(&m, "key", &res));
  assert(res == newval);

  tree_map_destroy(&m);
}

static void RunTreeMapTests() {
  TestTreeMapConstruction();
  TestTreeMapInsertion();
  TestTreeMapOverrideKeyValue();
}

///
/// End Tree Map Tests
///

///
/// Start String Implementation
///

typedef struct {
  char *data;

  // capacity is always greater than size to account for null terminator.
  size_t size;
  size_t capacity;
} string;

const size_t kDefaultStringCapacity = 16;

void string_construct(string *s) {
  s->data = malloc(kDefaultStringCapacity);
  assert(s->data);
  s->data[0] = 0;  // NULL terminator

  s->size = 0;
  s->capacity = kDefaultStringCapacity;
}

void string_destroy(string *s) { free(s->data); }

void string_clear(string *s) {
  s->size = 0;
  s->data[0] = 0;
}

void string_reserve(string *s, size_t new_capacity) {
  if (new_capacity <= s->capacity)
    return;

  // Double the capacity.
  s->capacity = new_capacity * 2;
  s->data = realloc(s->data, s->capacity);
  assert(s->data);
}

void string_append(string *s, const char *suffix) {
  size_t len = strlen(suffix);
  string_reserve(s, s->size + len + 1);  // +1 for null terminator
  memcpy(s->data + s->size, suffix, len);
  s->size += len;
  s->data[s->size] = 0;
}

void string_append_char(string *s, char c) {
  string_reserve(s, s->size + 1 /*char size*/ + 1 /*Null terminator*/);
  s->data[s->size] = c;
  s->size++;
  s->data[s->size] = 0;
}

bool string_equals(const string *s, const char *s2) {
  return strcmp(s->data, s2) == 0;
}

///
/// End String Implementation
///

///
/// Start String tests
///

static void TestStringConstruction() {
  string s;
  string_construct(&s);

  assert(s.data != NULL);
  assert(s.size == 0);
  assert(s.capacity > 0);
  assert(string_equals(&s, ""));

  string_destroy(&s);
}

static void TestStringCapacityReservation() {
  string s;
  string_construct(&s);
  size_t old_cap = s.capacity;
  string_reserve(&s, old_cap * 2);

  assert(s.capacity >= old_cap);
  assert(string_equals(&s, ""));
  assert(s.size == 0);
  assert(s.data[0] == 0 && "An empty string should be null-terminated.");

  string_destroy(&s);
}

static void TestStringAppend() {
  string s;
  string_construct(&s);
  string_append(&s, "abc");

  assert(s.size == 3);
  assert(string_equals(&s, "abc"));
  assert(s.data[s.size] == 0);

  string_append(&s, "xyz");

  assert(s.size == 6);
  assert(string_equals(&s, "abcxyz"));
  assert(s.data[s.size] == 0);

  string_destroy(&s);
}

static void TestStringAppendChar() {
  string s;
  string_construct(&s);
  string_append_char(&s, 'a');
  string_append_char(&s, 'b');
  string_append_char(&s, 'c');

  assert(s.size == 3);
  assert(string_equals(&s, "abc"));
  assert(s.data[s.size] == 0);

  string_append_char(&s, 'x');
  string_append_char(&s, 'y');
  string_append_char(&s, 'z');

  assert(s.size == 6);
  assert(string_equals(&s, "abcxyz"));
  assert(s.data[s.size] == 0);

  string_destroy(&s);
}

static void TestStringResizing() {
  string s;
  string_construct(&s);

  // Keep increasing the string size until we exceed the capacity.
  // We'll keep adding until the size is twice the original. The
  // string should automatically resize itself.
  size_t init_cap = s.capacity;
  for (size_t i = 0; i < init_cap * 2; ++i) {
    string_append_char(&s, 'a');
  }

  assert(s.size == init_cap * 2);

  for (size_t i = 0; i < init_cap * 2; ++i) {
    assert(s.data[i] == 'a');
  }

  assert(s.capacity >= init_cap * 2);
  assert(s.data[s.size] == 0);

  string_destroy(&s);
}

static void TestStringClear() {
  string s;
  string_construct(&s);

  string_append(&s, "abc");
  assert(s.size == 3);
  assert(string_equals(&s, "abc"));

  string_clear(&s);
  assert(s.size == 0);
  assert(string_equals(&s, ""));
}

static void RunStringTests() {
  TestStringConstruction();
  TestStringCapacityReservation();
  TestStringAppend();
  TestStringAppendChar();
  TestStringResizing();
  TestStringClear();
}

///
/// End String tests
///

///
/// Start Input Stream Implementation
///

struct InputStream;

typedef struct {
  void (*dtor)(struct InputStream *);
  int (*read)(struct InputStream *);
  int (*eof)(struct InputStream *);
} InputStreamVtable;

struct InputStream {
  const InputStreamVtable *vtable;
};
typedef struct InputStream InputStream;

void input_stream_construct(InputStream *is, const InputStreamVtable *vtable) {
  is->vtable = vtable;
}

void input_stream_destroy(InputStream *is) { is->vtable->dtor(is); }
int input_stream_read(InputStream *is) { return is->vtable->read(is); }
int input_stream_eof(InputStream *is) { return is->vtable->eof(is); }

void file_input_stream_destroy(InputStream *);
int file_input_stream_read(InputStream *);
int file_input_stream_eof(InputStream *);

const InputStreamVtable FileInputStreamVtable = {
    .dtor = file_input_stream_destroy,
    .read = file_input_stream_read,
    .eof = file_input_stream_eof,
};

typedef struct {
  InputStream base;
  void *input_file;
} FileInputStream;

void file_input_stream_construct(FileInputStream *fis, const char *input) {
  input_stream_construct(&fis->base, &FileInputStreamVtable);
  fis->input_file = fopen(input, "r");
}

void file_input_stream_destroy(InputStream *is) {
  FileInputStream *fis = (FileInputStream *)is;
  fclose(fis->input_file);
}

int file_input_stream_read(InputStream *is) {
  FileInputStream *fis = (FileInputStream *)is;
  return fgetc(fis->input_file);
}

int file_input_stream_eof(InputStream *is) {
  FileInputStream *fis = (FileInputStream *)is;
  return feof(fis->input_file);
}

void string_input_stream_destroy(InputStream *);
int string_input_stream_read(InputStream *);
int string_input_stream_eof(InputStream *);

const InputStreamVtable StringInputStreamVtable = {
    .dtor = string_input_stream_destroy,
    .read = string_input_stream_read,
    .eof = string_input_stream_eof,
};

typedef struct {
  InputStream base;
  char *string;

  size_t pos_;
  size_t len_;
} StringInputStream;

void string_input_stream_construct(StringInputStream *sis, const char *string) {
  input_stream_construct(&sis->base, &StringInputStreamVtable);
  sis->string = strdup(string);
  sis->pos_ = 0;
  sis->len_ = strlen(string);
}

void string_input_stream_destroy(InputStream *is) {
  StringInputStream *sis = (StringInputStream *)is;
  free(sis->string);
}

int string_input_stream_read(InputStream *is) {
  StringInputStream *sis = (StringInputStream *)is;
  if (sis->pos_ >= sis->len_)
    return -1;
  return sis->string[sis->pos_++];
}

int string_input_stream_eof(InputStream *is) {
  StringInputStream *sis = (StringInputStream *)is;
  return sis->pos_ >= sis->len_;
}

///
/// End Input Stream Implementation
///

///
/// Start Lexer Implementation
///

typedef enum {
  TK_BuiltinTypesFirst,
  TK_Char = TK_BuiltinTypesFirst,
  TK_Short,
  TK_Int,
  TK_Signed,
  TK_Unsigned,
  TK_Long,
  TK_Float,
  TK_Double,
  TK_LongDouble,
  TK_Void,
  TK_Bool,
  TK_BuiltinTypesLast = TK_Bool,

  // Qualifiers
  TK_QualifiersFirst,
  TK_Const = TK_QualifiersFirst,
  TK_Volatile,
  TK_Restrict,
  TK_QualifiersLast = TK_Restrict,

  // Storage specifiers
  TK_StorageClassSpecifiersFirst,
  TK_Extern = TK_StorageClassSpecifiersFirst,
  TK_Static,
  TK_Auto,
  TK_Register,
  TK_ThreadLocal,
  TK_StorageClassSpecifiersLast = TK_ThreadLocal,

  // Operations
  TK_LogicalAnd,  // &&
  TK_LogicalOr,   // ||
  TK_Arrow,       // ->, Dereference
  TK_Star,        // *, Pointer, dereference, multiplication
  TK_Ampersand,   // &, Address-of, bitwise-and
  TK_Not,         // !
  TK_Eq,          // ==
  TK_Ne,          // !=
  TK_Lt,          // <
  TK_Gt,          // >
  TK_Le,          // <=
  TK_Ge,          // >=
  TK_Div,         // /
  TK_Mod,         // %
  TK_Add,         // +
  TK_Sub,         // -
  TK_LShift,      // <<
  TK_RShift,      // >>
  TK_Or,          // |
  TK_Xor,         // ^
  TK_BitNot,      // ~

  TK_Inc,  // ++
  TK_Dec,  // --

  // Assignment ops
  TK_Assign,        // =
  TK_MulAssign,     // *-
  TK_DivAssign,     // /=
  TK_ModAssign,     // %=
  TK_AddAssign,     // +=
  TK_SubAssign,     // -=
  TK_LShiftAssign,  // <<=
  TK_RShiftAssign,  // >>+
  TK_AndAssign,     // &=
  TK_OrAssign,      // |=
  TK_XorAssign,     // ^=

  // Other Keywords
  TK_Enum,
  TK_Union,
  TK_Typedef,
  TK_Struct,
  TK_Return,
  TK_StaticAssert,
  TK_SizeOf,
  TK_If,
  TK_Else,
  TK_While,

  TK_Identifier,

  // Literals
  TK_IntLiteral,
  TK_StringLiteral,

  // Single-char tokens
  TK_LPar,          // (
  TK_RPar,          // )
  TK_LCurlyBrace,   // {
  TK_RCurlyBrace,   // }
  TK_LSquareBrace,  // [
  TK_RSquareBrace,  // ]
  TK_Semicolon,     // ;
  TK_Colon,         // :
  TK_Comma,         // ,
  TK_Question,      // ?

  TK_Dot,       // .
  TK_Ellipsis,  // ...

  // This can be returned at the end if there's whitespace at the EOF.
  TK_Eof = -1,
} TokenKind;

static bool is_builtin_type_token(TokenKind kind) {
  return TK_BuiltinTypesFirst <= kind && kind <= TK_BuiltinTypesLast;
}

static bool is_qualifier_token(TokenKind kind) {
  return TK_QualifiersFirst <= kind && kind <= TK_QualifiersLast;
}

static bool is_storage_class_specifier_token(TokenKind kind) {
  return TK_StorageClassSpecifiersFirst <= kind &&
         kind <= TK_StorageClassSpecifiersLast;
}

typedef struct {
  string chars;
  TokenKind kind;
  size_t line, col;
} Token;

typedef struct {
  InputStream *input;

  // 0 indicates no lookahead. Non-zero means we have a lookahead.
  int lookahead;

  // Lexer users should never access these directly. Instead refer to the line
  // and col returned in the Token.
  size_t line_, col_;
} Lexer;

static inline bool is_eof(int c) { return c < 0; }

void lexer_construct(Lexer *lexer, InputStream *input) {
  lexer->input = input;
  lexer->lookahead = 0;
  lexer->line_ = 1;
  // Start at zero since this is incremented for each char read.
  lexer->col_ = 0;
}

void lexer_destroy(Lexer *lexer) {}

bool lexer_has_lookahead(const Lexer *lexer) { return lexer->lookahead != 0; }

// Return true if the lexer can read a more token. This means there's at least
// a lookahead or something we can read from the file stream.
bool lexer_can_get_char(Lexer *lexer) {
  if (lexer_has_lookahead(lexer))
    return true;

  if (input_stream_eof(lexer->input))
    return false;

  // It's possible that we have no lookahead but have recently popped the very
  // last char in a stream with fgetc. In that case, the very next call to
  // fgetc will return EOF and calls to feof will return true. To account for
  // this, let's read a character then set it as the lookahead. If the fgetc
  // call hit EOF, then feof will always return true from now on. If not, then
  // we can set this valid char as the lookahead.
  int peek = input_stream_read(lexer->input);
  if (is_eof(peek))
    return false;

  if (peek == '\n') {
    lexer->line_++;
    lexer->col_ = 0;
  } else {
    lexer->col_++;
  }

  lexer->lookahead = peek;
  return true;
}

// Peek a character from the lexer. If the lexer reached EOF then the
// returned value is an EOF value but it's not stored as a lookahead.
int lexer_peek_char(Lexer *lexer) {
  if (!lexer_has_lookahead(lexer)) {
    if (input_stream_eof(lexer->input))
      return -1;

    int res = input_stream_read(lexer->input);
    if (is_eof(res))
      return -1;

    if (res == '\n') {
      lexer->line_++;
      lexer->col_ = 0;
    } else {
      lexer->col_++;
    }

    lexer->lookahead = res;
    assert(lexer->lookahead != 0 && "Invalid lookahead");
    assert(!is_eof(lexer->lookahead));
  }

  return lexer->lookahead;
}

int lexer_get_char(Lexer *lexer) {
  if (lexer_has_lookahead(lexer)) {
    int c = lexer->lookahead;
    lexer->lookahead = 0;
    return c;
  }

  int res = input_stream_read(lexer->input);
  if (res == '\n') {
    lexer->line_++;
    lexer->col_ = 0;
  } else if (!is_eof(res)) {
    lexer->col_++;
  }
  return res;
}

void token_construct(Token *tok) { string_construct(&tok->chars); }

void token_destroy(Token *tok) { string_destroy(&tok->chars); }

static bool is_kw_char(int c) { return c == '_' || isalnum(c); }

// Skip whitespace and ensure the cursor is on a character that isn't ws,
// unless it's EOF.
void skip_ws(Lexer *lexer) {
  int c;
  while (1) {
    c = lexer_peek_char(lexer);
    if (is_eof(c))
      return;

    if (!isspace(c))
      return;

    // Continue and keep popping from the stream until we hit EOF or a
    // non-space character.
    lexer_get_char(lexer);
  }
}

// Peek a char. If it matches the expected char, consume it and return true.
// Otherwise, don't consume it and return false.
bool lexer_peek_then_consume_char(Lexer *lexer, char c) {
  if (lexer_peek_char(lexer) != c)
    return false;
  lexer_get_char(lexer);
  return true;
}

// Callers of this function should destroy the token.
Token lex(Lexer *lexer) {
  Token tok;
  token_construct(&tok);
  tok.kind = TK_Eof;  // Default value

  int c;
  while (1) {
    skip_ws(lexer);

    if (is_eof(lexer_peek_char(lexer))) {
      tok.line = lexer->line_;
      tok.col = lexer->col_;
      return tok;
    }

    c = lexer_get_char(lexer);
    assert(!is_eof(c) && !isspace(c));

    string_append_char(&tok.chars, (char)c);

    // First handle potential comments before anything else.
    if (c == '/') {
      tok.line = lexer->line_;
      tok.col = lexer->col_;

      if (lexer_peek_char(lexer) == '/') {
        // This is a comment. Consume all characters until the newline.
        while (lexer_get_char(lexer) != '\n')
          ;
        string_clear(&tok.chars);
        continue;
      }

      if (lexer_peek_char(lexer) == '*') {
        // This is a comment. Consume all characters until the final `*/`.
        while (1) {
          if (lexer_get_char(lexer) == '*') {
            if (lexer_peek_then_consume_char(lexer, '/'))
              break;
          }
        }
        string_clear(&tok.chars);
        continue;
      }

      // Normal division.
      tok.kind = TK_Div;
      return tok;
    }

    break;
  }

  tok.line = lexer->line_;
  tok.col = lexer->col_;

  // Handle elipsis.
  if (c == '.') {
    if (lexer_peek_then_consume_char(lexer, '.')) {
      if (!lexer_peek_then_consume_char(lexer, '.')) {
        printf("%llu:%llu: Expected 3 '.' for elipses but found 2.", tok.line,
               tok.col);
        __builtin_trap();
      }
      string_append(&tok.chars, "..");
      tok.kind = TK_Ellipsis;
    } else {
      tok.kind = TK_Dot;
    }
    return tok;
  }

  // Non-single char tokens starting with special characters.
  if (c == '+') {
    if (lexer_peek_then_consume_char(lexer, '+')) {
      tok.kind = TK_Inc;
      string_append_char(&tok.chars, '+');
    } else if (lexer_peek_then_consume_char(lexer, '=')) {
      tok.kind = TK_AddAssign;
      string_append_char(&tok.chars, '=');
    } else {
      tok.kind = TK_Add;
    }
    return tok;
  }

  if (c == '-') {
    if (lexer_peek_then_consume_char(lexer, '-')) {
      tok.kind = TK_Dec;
      string_append_char(&tok.chars, '-');
    } else if (lexer_peek_then_consume_char(lexer, '=')) {
      tok.kind = TK_SubAssign;
      string_append_char(&tok.chars, '=');
    } else if (lexer_peek_then_consume_char(lexer, '>')) {
      tok.kind = TK_Arrow;
      string_append_char(&tok.chars, '>');
    } else {
      tok.kind = TK_Sub;
    }
    return tok;
  }

  if (c == '!') {
    if (lexer_peek_then_consume_char(lexer, '=')) {
      tok.kind = TK_Ne;
      string_append_char(&tok.chars, '=');
    } else {
      tok.kind = TK_Not;
    }
    return tok;
  }

  if (c == '=') {
    if (lexer_peek_then_consume_char(lexer, '=')) {
      string_append_char(&tok.chars, '=');
      tok.kind = TK_Eq;
    } else {
      tok.kind = TK_Assign;
    }
    return tok;
  }

  if (c == '&') {
    // Disambiguate between bitwise-and/address-of vs logical-and.
    if (lexer_peek_then_consume_char(lexer, '&')) {
      string_append_char(&tok.chars, '&');
      tok.kind = TK_LogicalAnd;
    } else {
      tok.kind = TK_Ampersand;
    }
    return tok;
  }

  if (c == '%') {
    if (lexer_peek_then_consume_char(lexer, '=')) {
      string_append_char(&tok.chars, '=');
      tok.kind = TK_ModAssign;
    } else {
      tok.kind = TK_Mod;
    }
    return tok;
  }

  if (c == '<') {
    if (lexer_peek_then_consume_char(lexer, '=')) {
      string_append_char(&tok.chars, '=');
      tok.kind = TK_Le;
    } else {
      tok.kind = TK_Lt;
    }
    return tok;
  }

  if (c == '>') {
    if (lexer_peek_then_consume_char(lexer, '=')) {
      string_append_char(&tok.chars, '=');
      tok.kind = TK_Ge;
    } else {
      tok.kind = TK_Gt;
    }
    return tok;
  }

  if (c == '"') {
    // TODO: Handle escape chars.
    while (!lexer_peek_then_consume_char(lexer, '"')) {
      int c = lexer_get_char(lexer);
      if (is_eof(c)) {
        printf("Got EOF before finishing string parsing\n");
        __builtin_trap();
      }
      string_append_char(&tok.chars, (char)c);
    }

    string_append_char(&tok.chars, '"');
    tok.kind = TK_StringLiteral;
    return tok;
  }

  // Handle special single character tokens.
  switch (c) {
    case '(':
      tok.kind = TK_LPar;
      return tok;
    case ')':
      tok.kind = TK_RPar;
      return tok;
    case '{':
      tok.kind = TK_LCurlyBrace;
      return tok;
    case '}':
      tok.kind = TK_RCurlyBrace;
      return tok;
    // TODO: This will not work with C23 attributes which follow [[...]] syntax
    case '[':
      tok.kind = TK_LSquareBrace;
      return tok;
    case ']':
      tok.kind = TK_RSquareBrace;
      return tok;
    case '*':
      tok.kind = TK_Star;
      return tok;
    case ';':
      tok.kind = TK_Semicolon;
      return tok;
    case ':':
      tok.kind = TK_Colon;
      return tok;
    case ',':
      tok.kind = TK_Comma;
      return tok;
    case '?':
      tok.kind = TK_Question;
      return tok;
    case '~':
      tok.kind = TK_BitNot;
      return tok;
  }

  // Integers
  if (isdigit(c)) {
    tok.kind = TK_IntLiteral;

    while (isdigit(lexer_peek_char(lexer)))
      string_append_char(&tok.chars, (char)lexer_get_char(lexer));

    return tok;
  }

  // Keywords
  while (is_kw_char(lexer_peek_char(lexer))) {
    string_append_char(&tok.chars, (char)lexer_get_char(lexer));
  }

  if (string_equals(&tok.chars, "char")) {
    tok.kind = TK_Char;
  } else if (string_equals(&tok.chars, "bool")) {
    tok.kind = TK_Bool;
  } else if (string_equals(&tok.chars, "int")) {
    tok.kind = TK_Int;
  } else if (string_equals(&tok.chars, "unsigned")) {
    tok.kind = TK_Unsigned;
  } else if (string_equals(&tok.chars, "long")) {
    tok.kind = TK_Long;
  } else if (string_equals(&tok.chars, "void")) {
    tok.kind = TK_Void;
  } else if (string_equals(&tok.chars, "const")) {
    tok.kind = TK_Const;
  } else if (string_equals(&tok.chars, "volatile")) {
    tok.kind = TK_Volatile;
  } else if (string_equals(&tok.chars, "restrict")) {
    tok.kind = TK_Restrict;
  } else if (string_equals(&tok.chars, "enum")) {
    tok.kind = TK_Enum;
  } else if (string_equals(&tok.chars, "union")) {
    tok.kind = TK_Union;
  } else if (string_equals(&tok.chars, "typedef")) {
    tok.kind = TK_Typedef;
  } else if (string_equals(&tok.chars, "struct")) {
    tok.kind = TK_Struct;
  } else if (string_equals(&tok.chars, "return")) {
    tok.kind = TK_Return;
  } else if (string_equals(&tok.chars, "static_assert")) {
    tok.kind = TK_StaticAssert;
  } else if (string_equals(&tok.chars, "sizeof")) {
    tok.kind = TK_SizeOf;
  } else if (string_equals(&tok.chars, "if")) {
    tok.kind = TK_If;
  } else if (string_equals(&tok.chars, "else")) {
    tok.kind = TK_Else;
  } else if (string_equals(&tok.chars, "while")) {
    tok.kind = TK_While;
  } else if (string_equals(&tok.chars, "extern")) {
    tok.kind = TK_Extern;
  } else if (string_equals(&tok.chars, "static")) {
    tok.kind = TK_Static;
  } else if (string_equals(&tok.chars, "auto")) {
    tok.kind = TK_Auto;
  } else if (string_equals(&tok.chars, "register")) {
    tok.kind = TK_Register;
  } else if (string_equals(&tok.chars, "thread_local")) {
    tok.kind = TK_ThreadLocal;
  } else {
    tok.kind = TK_Identifier;
  }

  return tok;
}

///
/// End Lexer Implementation
///

///
/// Start Type Implementation
///

typedef enum {
  TK_BuiltinType,
  TK_NamedType,  // Named from a typedef
  TK_StructType,
  TK_EnumType,
  TK_PointerType,
  TK_ArrayType,
  TK_FunctionType,

  // This is a special type used for lazy replacements of types in the AST.
  // This does not reflect an C types.
  TK_ReplacementSentinelType,
} TypeKind;

struct Type;

typedef void (*TypeDtor)(struct Type *);
typedef size_t (*TypeGetSize)(const struct Type *);

typedef struct {
  TypeKind kind;
  TypeDtor dtor;
} TypeVtable;

static const int kConstShift = 0;
static const int kVolatileShift = 1;
static const int kRestrictShift = 2;
static const int kConstMask = 1 << kConstShift;
static const int kVolatileMask = 1 << kVolatileShift;
static const int kRestrictMask = 1 << kRestrictShift;

typedef uint8_t Qualifiers;

struct Type {
  const TypeVtable *vtable;

  // bit 0: const
  // bit 1: volatile
  // bit 2: restrict
  Qualifiers qualifiers;
};
typedef struct Type Type;

void type_construct(Type *type, const TypeVtable *vtable) {
  type->vtable = vtable;
  type->qualifiers = 0;
}

void type_destroy(Type *type) { type->vtable->dtor(type); }

void type_set_const(Type *type) { type->qualifiers |= kConstMask; }
void type_set_volatile(Type *type) { type->qualifiers |= kVolatileMask; }
void type_set_restrict(Type *type) { type->qualifiers |= kRestrictMask; }

bool type_is_const(Type *type) { return type->qualifiers & kConstMask; }
bool type_is_volatile(Type *type) { return type->qualifiers & kVolatileMask; }
bool type_is_restrict(Type *type) { return type->qualifiers & kRestrictMask; }

typedef struct {
  Type type;
} ReplacementSentinelType;

void replacement_sentinel_type_destroy(Type *) {}

const TypeVtable ReplacementSentinelTypeVtable = {
    .kind = TK_ReplacementSentinelType,
    .dtor = replacement_sentinel_type_destroy,
};

ReplacementSentinelType GetSentinel() {
  ReplacementSentinelType sentinel;
  type_construct(&sentinel.type, &ReplacementSentinelTypeVtable);
  return sentinel;
}

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
  BTK_Void,
  BTK_Bool,
} BuiltinTypeKind;

void builtin_type_destroy(Type *) {}

const TypeVtable BuiltinTypeVtable = {
    .kind = TK_BuiltinType,
    .dtor = builtin_type_destroy,
};

typedef struct {
  Type type;
  BuiltinTypeKind kind;
} BuiltinType;

void builtin_type_construct(BuiltinType *bt, BuiltinTypeKind kind) {
  type_construct(&bt->type, &BuiltinTypeVtable);
  bt->kind = kind;
}

BuiltinType *create_builtin_type(BuiltinTypeKind kind) {
  BuiltinType *bt = malloc(sizeof(BuiltinType));
  builtin_type_construct(bt, kind);
  return bt;
}

bool is_builtin_type(const Type *type, BuiltinTypeKind kind) {
  if (type->vtable->kind != TK_BuiltinType)
    return false;
  return ((const BuiltinType *)type)->kind == kind;
}

bool is_pointer_type(const Type *type) {
  return type->vtable->kind == TK_PointerType;
}

bool is_integral_type(const Type *type) {
  if (type->vtable->kind != TK_BuiltinType)
    return false;

  switch (((const BuiltinType *)type)->kind) {
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

unsigned get_integral_rank(const BuiltinType *bt) {
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
      __builtin_trap();
  }
}

bool is_unsigned_integral_type(const Type *type) {
  if (type->vtable->kind != TK_BuiltinType)
    return false;

  switch (((const BuiltinType *)type)->kind) {
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

bool is_void_type(const Type *type) { return is_builtin_type(type, BTK_Void); }
bool is_bool_type(const Type *type) { return is_builtin_type(type, BTK_Bool); }

void struct_type_destroy(Type *);

const TypeVtable StructTypeVtable = {
    .kind = TK_StructType,
    .dtor = struct_type_destroy,
};

typedef struct {
  Type *type;
  char *name;    // Optional. Allocated by the creator of this StructMember.
  int bitfield;  // -1 indicates no bitfield. 0 or greater represents the
                 // bitfield size.
} StructMember;

typedef struct {
  Type type;
  char *name;  // Optional.

  // This is a vector of StructMembers.
  // NULL indicates this is a struct declaration but not definition.
  vector *members;

  bool packed;
} StructType;

void struct_type_construct(StructType *st, char *name, vector *members) {
  type_construct(&st->type, &StructTypeVtable);
  st->name = name;
  st->members = members;
  st->packed = false;
}

void struct_type_destroy(Type *type) {
  StructType *st = (StructType *)type;

  if (st->name)
    free(st->name);

  if (st->members) {
    for (size_t i = 0; i < st->members->size; ++i) {
      StructMember *sm = vector_at(st->members, 0);
      if (sm->name)
        free(sm->name);
      free(sm->type);
    }
    vector_destroy(st->members);
    free(st->members);
  }
}

const StructMember *struct_get_member(const StructType *st, const char *name,
                                      size_t *offset) {
  if (!st->members)
    return NULL;

  for (size_t i = 0; i < st->members->size; ++i) {
    const StructMember *member = vector_at(st->members, i);
    if (strcmp(member->name, name) == 0) {
      if (offset)
        *offset = i;
      return member;
    }
  }

  return NULL;
}

void enum_type_destroy(Type *);

const TypeVtable EnumTypeVtable = {
    .kind = TK_EnumType,
    .dtor = enum_type_destroy,
};

struct Expr;

void expr_destroy(struct Expr *);

typedef struct {
  char *name;          // Allocated by the creator of this EnumMember.
  struct Expr *value;  // Optional
} EnumMember;

typedef struct {
  Type type;
  char *name;  // Optional.

  // This is a vector of EnumMembers.
  // NULL indicates this is an enum declaration but not definition.
  vector *members;
} EnumType;

void enum_type_construct(EnumType *et, char *name, vector *members) {
  type_construct(&et->type, &EnumTypeVtable);
  et->name = name;
  et->members = members;
}

void enum_type_destroy(Type *type) {
  EnumType *et = (EnumType *)type;

  if (et->name)
    free(et->name);

  if (et->members) {
    for (size_t i = 0; i < et->members->size; ++i) {
      EnumMember *em = vector_at(et->members, 0);

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

void function_type_destroy(Type *);

const TypeVtable FunctionTypeVtable = {
    .kind = TK_FunctionType,
    .dtor = function_type_destroy,
};

typedef struct {
  char *name;  // Optional.
  Type *type;
} FunctionArg;

typedef struct {
  Type type;
  Type *return_type;
  vector pos_args;  // vector of FunctionArgs.
  bool has_var_args;
} FunctionType;

void function_type_construct(FunctionType *f, Type *return_type,
                             vector pos_args) {
  type_construct(&f->type, &FunctionTypeVtable);
  f->return_type = return_type;
  f->pos_args = pos_args;
  f->has_var_args = false;
}

void function_type_destroy(Type *type) {
  FunctionType *f = (FunctionType *)type;
  type_destroy(f->return_type);
  free(f->return_type);

  for (size_t i = 0; i < f->pos_args.size; ++i) {
    FunctionArg *a = vector_at(&f->pos_args, i);
    if (a->name)
      free(a->name);
    type_destroy(a->type);
    free(a->type);
  }
  vector_destroy(&f->pos_args);
}

Type *get_arg_type(const FunctionType *func, size_t i) {
  FunctionArg *a = vector_at(&func->pos_args, i);
  return a->type;
}

void array_type_destroy(Type *);

const TypeVtable ArrayTypeVtable = {
    .kind = TK_ArrayType,
    .dtor = array_type_destroy,
};

typedef struct {
  Type type;
  Type *elem_type;
  struct Expr *size;  // NULL indicates no size.
} ArrayType;

void array_type_construct(ArrayType *arr, Type *elem_type, struct Expr *size) {
  type_construct(&arr->type, &ArrayTypeVtable);
  arr->elem_type = elem_type;
  arr->size = size;
}

ArrayType *create_array_of(Type *elem, struct Expr *size) {
  ArrayType *arr = malloc(sizeof(ArrayType));
  array_type_construct(arr, elem, size);
  return arr;
}

void array_type_destroy(Type *type) {
  ArrayType *arr = (ArrayType *)type;
  type_destroy(arr->elem_type);
  free(arr->elem_type);

  if (arr->size) {
    expr_destroy(arr->size);
    free(arr->size);
  }
}

void pointer_type_destroy(Type *);

const TypeVtable PointerTypeVtable = {
    .kind = TK_PointerType,
    .dtor = pointer_type_destroy,
};

typedef struct {
  Type type;
  Type *pointee;
} PointerType;

void pointer_type_construct(PointerType *ptr, Type *pointee) {
  type_construct(&ptr->type, &PointerTypeVtable);
  ptr->pointee = pointee;
}

void pointer_type_destroy(Type *type) {
  PointerType *ptr = (PointerType *)type;
  type_destroy(ptr->pointee);
  free(ptr->pointee);
}

PointerType *create_pointer_to(Type *type) {
  PointerType *ptr = malloc(sizeof(PointerType));
  pointer_type_construct(ptr, type);
  return ptr;
}

bool is_pointer_to(const Type *type, TypeKind pointee) {
  if (!is_pointer_type(type))
    return false;
  return ((const PointerType *)type)->pointee->vtable->kind == pointee;
}

const Type *get_pointee(const Type *type) {
  return ((const PointerType *)type)->pointee;
}

void named_type_destroy(Type *);

const TypeVtable NamedTypeVtable = {
    .kind = TK_NamedType,
    .dtor = named_type_destroy,
};

typedef struct {
  Type type;
  char *name;
} NamedType;

void named_type_construct(NamedType *nt, const char *name) {
  type_construct(&nt->type, &NamedTypeVtable);

  size_t len = strlen(name);
  nt->name = malloc(sizeof(char) * (len + 1));
  memcpy(nt->name, name, len);
  nt->name[len] = 0;
}

void named_type_destroy(Type *type) { free(((NamedType *)type)->name); }

NamedType *create_named_type(const char *name) {
  NamedType *nt = malloc(sizeof(NamedType));
  named_type_construct(nt, name);
  return nt;
}

///
/// End Type Implementation
///

///
/// Start Parser Implementation
///

typedef struct {
  Lexer lexer;
  Token lookahead;
  bool has_lookahead;

  // Needed for distinguishing between "(" <expr> ")" and "(" <type> ")".
  // See https://en.wikipedia.org/wiki/Lexer_hack
  // This is really only used as a set rather than a map.
  TreeMap typedef_types;
} Parser;

void parser_construct(Parser *parser, InputStream *input) {
  lexer_construct(&parser->lexer, input);
  parser->has_lookahead = false;
  string_tree_map_construct(&parser->typedef_types);
}

void parser_destroy(Parser *parser) {
  lexer_destroy(&parser->lexer);
  if (parser->has_lookahead)
    token_destroy(&parser->lookahead);
  tree_map_destroy(&parser->typedef_types);
}

void parser_define_named_type(Parser *parser, const char *name) {
  tree_map_set(&parser->typedef_types, name, /*val=*/NULL);
}

bool parser_has_named_type(const Parser *parser, const char *name) {
  return tree_map_get(&parser->typedef_types, name, /*val=*/NULL);
}

static void expect_token(const Token *tok, TokenKind expected) {
  if (tok->kind != expected) {
    printf("%llu:%llu: Expected token %d but found %d: '%s'\n", tok->line,
           tok->col, expected, tok->kind, tok->chars.data);
    __builtin_trap();
  }
}

// Callers of this should destroy the token.
Token parser_pop_token(Parser *parser) {
  if (parser->has_lookahead) {
    parser->has_lookahead = false;
    return parser->lookahead;
  }
  return lex(&parser->lexer);
}

// Callers of this should not destroy the token. Instead callers of
// parser_pop_token should destroy it.
const Token *parser_peek_token(Parser *parser) {
  if (!parser->has_lookahead) {
    parser->lookahead = lex(&parser->lexer);
    parser->has_lookahead = true;
  }
  return &parser->lookahead;
}

bool next_token_is(Parser *parser, TokenKind kind) {
  return parser_peek_token(parser)->kind == kind;
}

static void expect_next_token(Parser *parser, TokenKind expected) {
  expect_token(parser_peek_token(parser), expected);
}

void parser_skip_next_token(Parser *parser) {
  Token token = parser_pop_token(parser);
  token_destroy(&token);
}

void parser_consume_token(Parser *parser, TokenKind kind) {
  Token next = parser_pop_token(parser);
  if (next.kind != kind) {
    printf("%llu:%llu: Expected token kind %d but found %d: '%s'\n", next.line,
           next.col, kind, next.kind, next.chars.data);
    __builtin_trap();
  }
  token_destroy(&next);
}

// Similar to `parser_consume_token`, but it will only consume the token if
// it's the one we expect. Otherwise, no token is consumed.
void parser_consume_token_if_matches(Parser *parser, TokenKind kind) {
  if (next_token_is(parser, kind))
    parser_consume_token(parser, kind);
}

typedef enum {
  EK_SizeOf,
  EK_UnOp,
  EK_BinOp,
  EK_Conditional,  // x ? y : z
  EK_DeclRef,
  EK_Int,
  EK_String,
  EK_Index,         // [ ]
  EK_MemberAccess,  // . or ->
  EK_Call,
  EK_Cast,
  EK_FunctionParam,
} ExprKind;

typedef void (*ExprDtor)(struct Expr *);

typedef struct {
  ExprKind kind;
  ExprDtor dtor;
} ExprVtable;

struct Expr {
  const ExprVtable *vtable;
  size_t line, col;
};
typedef struct Expr Expr;

void expr_construct(Expr *expr, const ExprVtable *vtable) {
  expr->vtable = vtable;
  expr->line = expr->col = 0;
}

void expr_destroy(Expr *expr) { expr->vtable->dtor(expr); }

typedef enum {
  NK_Typedef,
  NK_StaticAssert,
  NK_GlobalVariable,  // Also accounts for function decls, but not function defs
  NK_FunctionDefinition,
} NodeKind;

struct Node;

typedef void (*NodeDtor)(struct Node *);

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

typedef struct {
  NodeKind kind;
  NodeDtor dtor;
} NodeVtable;

struct Node {
  const NodeVtable *vtable;
};
typedef struct Node Node;

void node_construct(Node *node, const NodeVtable *vtable) {
  node->vtable = vtable;
}

void node_destroy(Node *node) { node->vtable->dtor(node); }

void typedef_destroy(Node *);

const NodeVtable TypedefVtable = {
    .kind = NK_Typedef,
    .dtor = typedef_destroy,
};

typedef struct {
  Node node;
  Type *type;
  char *name;
} Typedef;

void typedef_destroy(Node *node) {
  Typedef *td = (Typedef *)node;

  if (td->type)
    type_destroy(td->type);

  if (td->name)
    free(td->name);
}

void typedef_construct(Typedef *td) {
  node_construct(&td->node, &TypedefVtable);
  td->type = NULL;
  td->name = NULL;
}

Expr *parse_expr(Parser *);
Expr *parse_cast_expr(Parser *);
Expr *parse_conditional_expr(Parser *);
Expr *parse_assignment_expr(Parser *);
Type *parse_type(Parser *parser);
bool is_next_token_type_token(Parser *);

void static_assert_destroy(Node *);

const NodeVtable StaticAssertVtable = {
    .kind = NK_StaticAssert,
    .dtor = static_assert_destroy,
};

typedef struct {
  Node node;
  Expr *expr;
} StaticAssert;

void static_assert_construct(StaticAssert *sa, Expr *expr) {
  node_construct(&sa->node, &StaticAssertVtable);
  sa->expr = expr;
}

void static_assert_destroy(Node *node) {
  StaticAssert *sa = (StaticAssert *)node;
  sa->expr->vtable->dtor(sa->expr);
  free(sa->expr);
}

void global_variable_destroy(Node *);

const NodeVtable GlobalVariableVtable = {
    .kind = NK_GlobalVariable,
    .dtor = global_variable_destroy,
};

typedef struct {
  Node node;
  char *name;
  Type *type;
  Expr *initializer;

  bool is_extern;  // false implies `static`.
  bool is_thread_local;
} GlobalVariable;

void global_variable_construct(GlobalVariable *gv, const char *name,
                               Type *type) {
  node_construct(&gv->node, &GlobalVariableVtable);
  gv->name = strdup(name);
  gv->type = type;
  gv->initializer = NULL;
  gv->is_extern = true;
  gv->is_thread_local = false;
}

void global_variable_destroy(Node *node) {
  GlobalVariable *gv = (GlobalVariable *)node;
  free(gv->name);
  type_destroy(gv->type);
  free(gv->type);
  if (gv->initializer) {
    expr_destroy(gv->initializer);
    free(gv->initializer);
  }
}

typedef enum {
  SK_ExprStmt,
  SK_IfStmt,
  SK_CompoundStmt,
  SK_ReturnStmt,
} StatementKind;

struct Statement;

typedef void (*StatementDtor)(struct Statement *);

typedef struct {
  StatementKind kind;
  StatementDtor dtor;
} StatementVtable;

struct Statement {
  const StatementVtable *vtable;
};
typedef struct Statement Statement;

void statement_construct(Statement *stmt, const StatementVtable *vtable) {
  stmt->vtable = vtable;
}

void statement_destroy(Statement *stmt) { stmt->vtable->dtor(stmt); }

void if_stmt_destroy(Statement *);

const StatementVtable IfStmtVtable = {
    .kind = SK_IfStmt,
    .dtor = if_stmt_destroy,
};

struct IfStmt {
  Statement base;
  Expr *cond;  // Required only for the first If. All chained Ifs through the
               // else_stmt can have this optionally.
  Statement *body;
  struct IfStmt *else_stmt;  // Optional.
};
typedef struct IfStmt IfStmt;

void if_stmt_construct(IfStmt *stmt, Expr *cond, Statement *body) {
  statement_construct(&stmt->base, &IfStmtVtable);
  stmt->cond = cond;
  stmt->body = body;
  stmt->else_stmt = NULL;
}

void if_stmt_destroy(Statement *stmt) {
  IfStmt *ifstmt = (IfStmt *)stmt;
  if (ifstmt->cond) {
    expr_destroy(ifstmt->cond);
    free(ifstmt->cond);
  }

  statement_destroy(ifstmt->body);
  free(ifstmt->body);

  if (ifstmt->else_stmt) {
    statement_destroy(&ifstmt->else_stmt->base);
    free(ifstmt->else_stmt);
  }
}

void compound_stmt_destroy(Statement *);

const StatementVtable CompoundStmtVtable = {
    .kind = SK_CompoundStmt,
    .dtor = compound_stmt_destroy,
};

typedef struct {
  Statement base;
  vector body;  // vector of Statement pointers.
} CompoundStmt;

void compound_stmt_construct(CompoundStmt *stmt, vector body) {
  statement_construct(&stmt->base, &CompoundStmtVtable);
  stmt->body = body;
}

void compound_stmt_destroy(Statement *stmt) {
  CompoundStmt *cmpd = (CompoundStmt *)stmt;
  for (size_t i = 0; i < cmpd->body.size; ++i) {
    Statement **stmt = vector_at(&cmpd->body, i);
    statement_destroy(*stmt);
    free(*stmt);
  }
  vector_destroy(&cmpd->body);
}

void return_stmt_destroy(Statement *);

const StatementVtable ReturnStmtVtable = {
    .kind = SK_ReturnStmt,
    .dtor = return_stmt_destroy,
};

typedef struct {
  Statement base;
  Expr *expr;  // Optional. NULL means returning void.
} ReturnStmt;

void return_stmt_construct(ReturnStmt *stmt, Expr *expr) {
  statement_construct(&stmt->base, &ReturnStmtVtable);
  stmt->expr = expr;
}

void return_stmt_destroy(Statement *stmt) {
  ReturnStmt *ret_stmt = (ReturnStmt *)stmt;
  if (ret_stmt->expr) {
    expr_destroy(ret_stmt->expr);
    free(ret_stmt->expr);
  }
}

void expr_stmt_destroy(Statement *);

const StatementVtable ExprStmtVtable = {
    .kind = SK_ExprStmt,
    .dtor = expr_stmt_destroy,
};

typedef struct {
  Statement base;
  Expr *expr;
} ExprStmt;

void expr_stmt_construct(ExprStmt *stmt, Expr *expr) {
  statement_construct(&stmt->base, &ExprStmtVtable);
  stmt->expr = expr;
}

void expr_stmt_destroy(Statement *stmt) {
  ExprStmt *expr_stmt = (ExprStmt *)stmt;
  expr_destroy(expr_stmt->expr);
  free(expr_stmt->expr);
}

void function_definition_destroy(Node *);

const NodeVtable FunctionDefinitionVtable = {
    .kind = NK_FunctionDefinition,
    .dtor = function_definition_destroy,
};

typedef struct {
  Node node;
  char *name;
  Type *type;
  CompoundStmt *body;
  bool is_extern;  // false implies this is `static`.
} FunctionDefinition;

void function_definition_construct(FunctionDefinition *f, const char *name,
                                   Type *type, CompoundStmt *body) {
  node_construct(&f->node, &FunctionDefinitionVtable);
  f->name = strdup(name);
  assert(type->vtable->kind == TK_FunctionType);
  f->type = type;
  f->body = body;
  f->is_extern = true;
}

void function_definition_destroy(Node *node) {
  FunctionDefinition *f = (FunctionDefinition *)node;
  free(f->name);
  type_destroy(f->type);
  free(f->type);

  statement_destroy(&f->body->base);
  free(f->body);
}

Qualifiers parse_maybe_qualifiers(Parser *parser, bool *found) {
  Qualifiers quals = 0;
  if (found)
    *found = true;
  while (1) {
    switch (parser_peek_token(parser)->kind) {
      case TK_Const:
        quals |= kConstMask;
        parser_skip_next_token(parser);
        continue;
      case TK_Volatile:
        quals |= kVolatileMask;
        parser_skip_next_token(parser);
        continue;
      case TK_Restrict:
        quals |= kRestrictMask;
        parser_skip_next_token(parser);
        continue;
      default:
        if (found)
          *found = false;
    }
    break;
  }
  return quals;
}

Type *maybe_parse_pointers_and_qualifiers(Parser *parser, Type *base,
                                          Type ***type_usage_addr) {
  while (next_token_is(parser, TK_Star)) {
    parser_consume_token(parser, TK_Star);

    PointerType *ptr = create_pointer_to(base);
    if (type_usage_addr && *type_usage_addr == NULL) {
      // Check NULL to capture the very first usage.
      Type **pointee = &ptr->pointee;
      *type_usage_addr = pointee;
    }
    base = &ptr->type;

    // May be more '*'s or qualifiers.
    base->qualifiers |= parse_maybe_qualifiers(parser, /*found=*/NULL);
  }
  return base;
}

bool is_token_type_token(const Parser *parser, const Token *tok);

typedef struct {
  unsigned int extern_ : 1;
  unsigned int static_ : 1;
  unsigned int auto_ : 1;
  unsigned int register_ : 1;
  unsigned int thread_local_ : 1;
} FoundStorageClasses;

Type *parse_type_for_declaration(Parser *parser, char **name,
                                 FoundStorageClasses *);

StructType *parse_struct_type(Parser *parser) {
  parser_consume_token(parser, TK_Struct);

  const Token *peek = parser_peek_token(parser);
  char *name;
  if (peek->kind == TK_Identifier) {
    name = strdup(peek->chars.data);
    parser_consume_token(parser, TK_Identifier);
  } else {
    name = NULL;
  }

  StructType *struct_ty = malloc(sizeof(StructType));

  if (!next_token_is(parser, TK_LCurlyBrace)) {
    struct_type_construct(struct_ty, name, /*members=*/NULL);
    return struct_ty;
  }

  parser_consume_token(parser, TK_LCurlyBrace);

  vector *members = malloc(sizeof(vector));
  vector_construct(members, sizeof(StructMember), alignof(StructMember));
  while (!next_token_is(parser, TK_RCurlyBrace)) {
    char *member_name = NULL;
    Type *member_ty =
        parse_type_for_declaration(parser, &member_name, /*storage=*/NULL);
    assert(member_name);

    StructMember *member = vector_append_storage(members);
    member->type = member_ty;
    member->name = member_name;
    // TODO: Handle bitfields.
    member->bitfield = -1;

    parser_consume_token(parser, TK_Semicolon);
  }

  parser_consume_token(parser, TK_RCurlyBrace);

  struct_type_construct(struct_ty, name, members);
  return struct_ty;
}

EnumType *parse_enum_type(Parser *parser) {
  parser_consume_token(parser, TK_Enum);

  const Token *peek = parser_peek_token(parser);
  char *name;
  if (peek->kind == TK_Identifier) {
    name = strdup(peek->chars.data);
    parser_consume_token(parser, TK_Identifier);
  } else {
    name = NULL;
  }

  EnumType *enum_ty = malloc(sizeof(EnumType));

  if (!next_token_is(parser, TK_LCurlyBrace)) {
    enum_type_construct(enum_ty, name, /*members=*/NULL);
    return enum_ty;
  }

  parser_consume_token(parser, TK_LCurlyBrace);

  vector *members = malloc(sizeof(vector));
  vector_construct(members, sizeof(EnumMember), alignof(EnumMember));
  while (!next_token_is(parser, TK_RCurlyBrace)) {
    Token next = parser_pop_token(parser);
    assert(next.kind == TK_Identifier);
    char *member_name = strdup(next.chars.data);
    token_destroy(&next);

    Expr *member_val;
    if (next_token_is(parser, TK_Assign)) {
      parser_consume_token(parser, TK_Assign);
      // The RHS can only be a `constant_expression` which can only be a
      // `conditional_expression. This prevents enums which assign values from
      // looking like comma-separated expressions.
      member_val = parse_conditional_expr(parser);
    } else {
      member_val = NULL;
    }

    EnumMember *member = vector_append_storage(members);
    member->name = member_name;
    member->value = member_val;

    parser_consume_token_if_matches(parser, TK_Comma);
  }

  parser_consume_token(parser, TK_RCurlyBrace);

  enum_type_construct(enum_ty, name, members);
  return enum_ty;
}

Type *parse_specifiers_and_qualifiers_and_storage(
    Parser *parser, FoundStorageClasses *storage) {
  Qualifiers quals = 0;

  struct TypeSpecifier {
    unsigned int char_ : 1;
    unsigned int short_ : 1;
    unsigned int int_ : 1;
    unsigned int signed_ : 1;
    unsigned int unsigned_ : 1;
    unsigned int long_ : 2;  // long can be specified up to 2 times
    unsigned int float_ : 1;
    unsigned int double_ : 1;
    unsigned int void_ : 1;
    unsigned int bool_ : 1;
    unsigned int named_ : 1;
    unsigned int struct_ : 1;
    unsigned int enum_ : 1;
  };

  struct TypeSpecifier spec = {};

  StructType *struct_ty;
  EnumType *enum_ty;
  bool should_stop = false;
  char *name;
  while (!should_stop) {
    bool consume_next_token = true;
    const Token *peek = parser_peek_token(parser);
    switch (peek->kind) {
      case TK_Struct:
        assert(!spec.struct_);
        assert(!spec.named_);
        assert(!spec.enum_);
        spec.struct_ = 1;
        consume_next_token = false;
        struct_ty = parse_struct_type(parser);
        break;
      case TK_Enum:
        assert(!spec.struct_);
        assert(!spec.named_);
        assert(!spec.enum_);
        spec.enum_ = 1;
        consume_next_token = false;
        enum_ty = parse_enum_type(parser);
        break;
      case TK_Char:
        spec.char_ = 1;
        break;
      case TK_Short:
        spec.short_ = 1;
        break;
      case TK_Int:
        spec.int_ = 1;
        break;
      case TK_Unsigned:
        spec.unsigned_ = 1;
        break;
      case TK_Long:
        spec.long_++;
        assert(spec.long_ < 3);
        break;
      case TK_Float:
        spec.float_ = 1;
        break;
      case TK_Double:
        spec.double_ = 1;
        break;
      case TK_Void:
        spec.void_ = 1;
        break;
      case TK_Bool:
        spec.bool_ = 1;
        break;
      case TK_Const:
        quals |= kConstMask;
        break;
      case TK_Volatile:
        quals |= kVolatileMask;
        break;
      case TK_Restrict:
        quals |= kRestrictMask;
        break;
      case TK_Extern:
        if (storage)
          storage->extern_ = 1;
        break;
      case TK_Static:
        if (storage)
          storage->static_ = 1;
        break;
      case TK_Auto:
        if (storage)
          storage->auto_ = 1;
        break;
      case TK_Register:
        if (storage)
          storage->register_ = 1;
        break;
      case TK_ThreadLocal:
        if (storage)
          storage->thread_local_ = 1;
        break;
      case TK_Identifier:
        if (is_token_type_token(parser, peek)) {
          assert(!spec.struct_);
          assert(!spec.named_);
          assert(!spec.enum_);
          spec.named_ = 1;
          name = strdup(peek->chars.data);
          break;
        }
        [[fallthrough]];
      default:
        consume_next_token = false;
        should_stop = true;
        break;
    }

    if (consume_next_token)
      parser_skip_next_token(parser);
  }

  if (spec.named_) {
    NamedType *nt = create_named_type(name);
    nt->type.qualifiers = quals;
    free(name);
    return &nt->type;
  }

  if (spec.struct_) {
    struct_ty->type.qualifiers = quals;
    return &struct_ty->type;
  }

  if (spec.enum_) {
    enum_ty->type.qualifiers = quals;
    return &enum_ty->type;
  }

  BuiltinTypeKind kind;
  if (spec.char_) {
    if (spec.signed_) {
      kind = BTK_SignedChar;
    } else if (spec.unsigned_) {
      kind = BTK_UnsignedChar;
    } else {
      kind = BTK_Char;
    }
  } else if (spec.short_) {
    if (spec.unsigned_) {
      kind = BTK_UnsignedShort;
    } else {
      kind = BTK_Short;
    }
  } else if (spec.long_) {
    assert(spec.long_ < 3);
    if (spec.long_ == 2) {
      if (spec.unsigned_) {
        kind = BTK_UnsignedLongLong;
      } else {
        kind = BTK_LongLong;
      }
    } else {
      if (spec.double_) {
        kind = BTK_LongDouble;
      } else if (spec.unsigned_) {
        kind = BTK_UnsignedLong;
      } else {
        kind = BTK_Long;
      }
    }
  } else if (spec.int_) {
    if (spec.unsigned_) {
      kind = BTK_UnsignedInt;
    } else {
      kind = BTK_Int;
    }
  } else if (spec.unsigned_) {
    kind = BTK_UnsignedInt;
  } else if (spec.float_) {
    kind = BTK_Float;
  } else if (spec.bool_) {
    kind = BTK_Bool;
  } else if (spec.void_) {
    kind = BTK_Void;
  } else {
    const Token *tok = parser_peek_token(parser);
    printf(
        "%llu:%llu: parse_specifiers_and_qualifiers: Unhandled token (%d): "
        "'%s'\n",
        tok->line, tok->col, tok->kind, tok->chars.data);
    __builtin_trap();
  }

  BuiltinType *bt = malloc(sizeof(BuiltinType));
  builtin_type_construct(bt, kind);
  bt->type.qualifiers = quals;
  return &bt->type;
}

//
// <pointer> = "*" <typequalifier>*
//           | "*" <pointer>
//           | "*" <typequalifier>* <pointer>
//
Type *parse_pointers_and_qualifiers(Parser *parser, Type *base) {
  expect_next_token(parser, TK_Star);

  PointerType *ptr = malloc(sizeof(PointerType));
  pointer_type_construct(ptr, base);
  base = &ptr->type;

  parser_consume_token(parser, TK_Star);

  // May be more '*'s or qualifiers.
  base->qualifiers |= parse_maybe_qualifiers(parser, /*found=*/NULL);

  if (next_token_is(parser, TK_Star))
    return parse_pointers_and_qualifiers(parser, base);

  return base;
}

Type *parse_declarator_maybe_type_suffix(Parser *parser, Type *outer_ty,
                                         Type ***type_usage_addr);

Type *parse_array_suffix(Parser *parser, Type *outer_ty,
                         Type ***type_usage_addr) {
  parser_consume_token(parser, TK_LSquareBrace);

  Expr *expr;
  if (!next_token_is(parser, TK_RSquareBrace)) {
    expr = parse_expr(parser);
  } else {
    expr = NULL;
  }

  parser_consume_token(parser, TK_RSquareBrace);

  // <blank> is an array (of size ...) of <remaining>
  Type *remaining =
      parse_declarator_maybe_type_suffix(parser, outer_ty, type_usage_addr);

  ArrayType *arr = create_array_of(remaining, expr);
  if (remaining == outer_ty && type_usage_addr && *type_usage_addr == NULL) {
    Type **elem = &arr->elem_type;
    *type_usage_addr = elem;
  }
  return &arr->type;
}

Type *parse_function_suffix(Parser *parser, Type *outer_ty,
                            Type ***type_usage_addr) {
  parser_consume_token(parser, TK_LPar);

  vector param_tys;
  vector_construct(&param_tys, sizeof(FunctionArg), alignof(FunctionArg));

  bool has_var_args = false;
  while (!next_token_is(parser, TK_RPar)) {
    // Handle varags.
    if (next_token_is(parser, TK_Ellipsis)) {
      parser_consume_token(parser, TK_Ellipsis);
      has_var_args = true;

      if (!next_token_is(parser, TK_RPar)) {
        const Token *peek = parser_peek_token(parser);
        printf(
            "%llu:%llu: parse_function_suffix: '...' must be the last in the "
            "parameter list; instead found '%s'.",
            peek->line, peek->col, peek->chars.data);
        __builtin_trap();
      }
      continue;
    }

    char *name = NULL;
    Type *param_ty =
        parse_type_for_declaration(parser, &name, /*storage=*/NULL);
    FunctionArg *storage = vector_append_storage(&param_tys);
    storage->name = name;
    storage->type = param_ty;

    if (!next_token_is(parser, TK_Comma))
      break;

    parser_consume_token(parser, TK_Comma);
  }

  parser_consume_token(parser, TK_RPar);

  // <blank> is a function (with params ...) returning <outer_ty>
  FunctionType *func = malloc(sizeof(FunctionType));
  function_type_construct(func, outer_ty, param_tys);
  func->has_var_args = has_var_args;

  if (type_usage_addr && *type_usage_addr == NULL) {
    Type **ret = &func->return_type;
    *type_usage_addr = ret;
  }

  return &func->type;
}

// outer_ty is everything that came before the trailing part of the declarator.
Type *parse_declarator_maybe_type_suffix(Parser *parser, Type *outer_ty,
                                         Type ***type_usage_addr) {
  // There may be optional () or [] after.
  switch (parser_peek_token(parser)->kind) {
    case TK_LSquareBrace:
      return parse_array_suffix(parser, outer_ty, type_usage_addr);
    case TK_LPar:
      return parse_function_suffix(parser, outer_ty, type_usage_addr);
    default:
      return outer_ty;
  }
}

// `type_usage_addr` is used for saving the single location of where the
// `type` is used when parsing the declarator. It is a triple pointer because
// it should point to the location the Type* is used.
Type *parse_declarator(Parser *parser, Type *type, char **name,
                       Type ***type_usage_addr) {
  type = maybe_parse_pointers_and_qualifiers(parser, type, type_usage_addr);

  const Token *tok = parser_peek_token(parser);

  switch (tok->kind) {
    case TK_Identifier:
      if (name)
        *name = strdup(tok->chars.data);
      parser_consume_token(parser, TK_Identifier);
      break;
    case TK_LPar:
      parser_consume_token(parser, TK_LPar);

      // Parse the inner declarator which kinda "wraps" the outer type.
      // We will pass a sentinel type that will be replaced once we finish
      // parsing any type suffixes.
      ReplacementSentinelType dummy = GetSentinel();
      Type **this_sentinel_addr = NULL;
      Type *inner_type =
          parse_declarator(parser, &dummy.type, name, &this_sentinel_addr);
      parser_consume_token(parser, TK_RPar);

      // Now parse any remaining suffix.
      type = parse_declarator_maybe_type_suffix(parser, type, type_usage_addr);

      if (this_sentinel_addr) {
        // Replace the sentinel with the outer type.
        *this_sentinel_addr = type;

        // The inner type represents the actual type.
        return inner_type;
      } else {
        // This sentinel was not used at all for the inner declarator. In that
        // case, the inner_type must be the sentinel itself. In this case, the
        // inner_type isn't needed and we can just return the outer type.
        assert(inner_type == &dummy.type);
        return type;
      }
    default:
      break;
  }

  return parse_declarator_maybe_type_suffix(parser, type, type_usage_addr);
}

// <SPECIFIER/QUALIFIER MIX> <DECLARATOR>
// DECLARATOR = <POINTER/QUALIFIER MIX> (nested declarators) (parens or sq
// braces)
Type *parse_type_for_declaration(Parser *parser, char **name,
                                 FoundStorageClasses *storage) {
  Type *type = parse_specifiers_and_qualifiers_and_storage(parser, storage);
  type = maybe_parse_pointers_and_qualifiers(parser, type,
                                             /*type_usage_addr=*/NULL);
  return parse_declarator(parser, type, name, /*type_usage_addr=*/NULL);
}

Type *parse_type(Parser *parser) {
  return parse_type_for_declaration(parser, /*name=*/NULL, /*storage=*/NULL);
}

void sizeof_destroy(Expr *expr);

const ExprVtable SizeOfVtable = {
    .kind = EK_SizeOf,
    .dtor = sizeof_destroy,
};

typedef struct {
  Expr expr;
  void *expr_or_type;
  bool is_expr;
} SizeOf;

void sizeof_construct(SizeOf *so, void *expr_or_type, bool is_expr) {
  expr_construct(&so->expr, &SizeOfVtable);
  so->expr_or_type = expr_or_type;
  so->is_expr = is_expr;
}

void sizeof_destroy(Expr *expr) {
  SizeOf *so = (SizeOf *)expr;
  if (so->is_expr) {
    expr_destroy(so->expr_or_type);
  } else {
    type_destroy(so->expr_or_type);
  }
  free(so->expr_or_type);
}

//
// <sizeof> = "sizeof" "(" (<expr> | <type>) ")"
//
Expr *parse_sizeof(Parser *parser) {
  parser_consume_token(parser, TK_SizeOf);
  parser_consume_token(parser, TK_LPar);

  SizeOf *so = malloc(sizeof(SizeOf));

  void *expr_or_type;
  bool is_expr;
  if (is_next_token_type_token(parser)) {
    expr_or_type = parse_type(parser);
    is_expr = false;
  } else {
    expr_or_type = parse_expr(parser);
    is_expr = true;
  }

  parser_consume_token(parser, TK_RPar);

  sizeof_construct(so, expr_or_type, is_expr);
  return &so->expr;
  ;
}

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

void unop_destroy(Expr *expr);

const ExprVtable UnOpVtable = {
    .kind = EK_UnOp,
    .dtor = unop_destroy,
};

typedef struct {
  Expr expr;
  Expr *subexpr;
  UnOpKind op;
} UnOp;

void unop_construct(UnOp *unop, Expr *subexpr, UnOpKind op) {
  expr_construct(&unop->expr, &UnOpVtable);
  unop->subexpr = subexpr;
  unop->op = op;
}

void unop_destroy(Expr *expr) {
  UnOp *unop = (UnOp *)expr;
  expr_destroy(unop->subexpr);
  free(unop->subexpr);
}

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

bool is_logical_binop(BinOpKind kind) {
  return kind == BOK_LogicalOr || kind == BOK_LogicalAnd;
}

bool is_assign_binop(BinOpKind kind) {
  return BOK_AssignFirst <= kind && kind <= BOK_AssignLast;
}

void binop_destroy(Expr *expr);

const ExprVtable BinOpVtable = {
    .kind = EK_BinOp,
    .dtor = binop_destroy,
};

typedef struct {
  Expr expr;
  Expr *lhs;
  Expr *rhs;
  BinOpKind op;
} BinOp;

void binop_construct(BinOp *binop, Expr *lhs, Expr *rhs, BinOpKind op) {
  expr_construct(&binop->expr, &BinOpVtable);
  binop->lhs = lhs;
  binop->rhs = rhs;
  binop->op = op;
}

void binop_destroy(Expr *expr) {
  BinOp *binop = (BinOp *)expr;
  expr_destroy(binop->lhs);
  free(binop->lhs);
  expr_destroy(binop->rhs);
  free(binop->rhs);
}

void conditional_destroy(Expr *expr);

const ExprVtable ConditionalVtable = {
    .kind = EK_Conditional,
    .dtor = conditional_destroy,
};

typedef struct {
  Expr expr;
  Expr *cond;
  Expr *true_expr;
  Expr *false_expr;
} Conditional;

void conditional_construct(Conditional *c, Expr *cond, Expr *true_expr,
                           Expr *false_expr) {
  expr_construct(&c->expr, &ConditionalVtable);
  c->cond = cond;
  c->true_expr = true_expr;
  c->false_expr = false_expr;
}

void conditional_destroy(Expr *expr) {
  Conditional *c = (Conditional *)expr;
  expr_destroy(c->cond);
  free(c->cond);
  expr_destroy(c->true_expr);
  free(c->true_expr);
  expr_destroy(c->false_expr);
  free(c->false_expr);
}

void declref_destroy(Expr *expr);

const ExprVtable DeclRefVtable = {
    .kind = EK_DeclRef,
    .dtor = declref_destroy,
};

typedef struct {
  Expr expr;
  char *name;
} DeclRef;

void declref_construct(DeclRef *ref, const char *name) {
  expr_construct(&ref->expr, &DeclRefVtable);
  size_t len = strlen(name);
  ref->name = malloc(sizeof(char) * (len + 1));
  memcpy(ref->name, name, len);
  ref->name[len] = 0;
}

void declref_destroy(Expr *expr) {
  DeclRef *ref = (DeclRef *)expr;
  free(ref->name);
}

void int_destroy(Expr *expr);

const ExprVtable IntVtable = {
    .kind = EK_Int,
    .dtor = int_destroy,
};

typedef struct {
  Expr expr;
  uint64_t val;
  BuiltinType type;
} Int;

void int_construct(Int *i, uint64_t val, BuiltinTypeKind kind) {
  expr_construct(&i->expr, &IntVtable);
  i->val = val;
  builtin_type_construct(&i->type, kind);
}

void int_destroy(Expr *expr) {
  Int *i = (Int *)expr;
  builtin_type_destroy(&i->type.type);
}

void string_literal_destroy(Expr *expr);

const ExprVtable StringLiteralVtable = {
    .kind = EK_String,
    .dtor = string_literal_destroy,
};

typedef struct {
  Expr expr;
  char *val;
} StringLiteral;

void string_literal_construct(StringLiteral *s, const char *val, size_t len) {
  expr_construct(&s->expr, &StringLiteralVtable);
  s->val = malloc(sizeof(char) * (len + 1));
  memcpy(s->val, val, len);
  s->val[len] = 0;
}

void string_literal_destroy(Expr *expr) {
  StringLiteral *s = (StringLiteral *)expr;
  free(s->val);
}

// For parsing "(" <expr> ")" but the initial opening LPar is consumed.
Expr *parse_parentheses_expr_tail(Parser *parser) {
  Expr *expr = parse_expr(parser);
  parser_consume_token(parser, TK_RPar);
  return expr;
}

//
// <primary_expr> = <identifier> | <constant> | <string_literal>
//                | "(" <expr> ")"
//
Expr *parse_primary_expr(Parser *parser) {
  const Token *tok = parser_peek_token(parser);

  if (tok->kind == TK_LPar) {
    parser_consume_token(parser, TK_LPar);
    return parse_parentheses_expr_tail(parser);
  }

  if (tok->kind == TK_Identifier) {
    DeclRef *ref = malloc(sizeof(DeclRef));
    declref_construct(ref, tok->chars.data);
    parser_consume_token(parser, TK_Identifier);
    return &ref->expr;
  }

  if (tok->kind == TK_IntLiteral) {
    // TODO: Rather than using strtoull, we should probably parse this
    // ourselves for better error handling. This is simpler for now.
    unsigned long long val = strtoull(tok->chars.data, NULL, 10);
    Int *i = malloc(sizeof(Int));
    // FIXME: This doesn't account for suffixes.
    int_construct(i, val, BTK_Int);
    parser_consume_token(parser, TK_IntLiteral);
    return &i->expr;
  }

  if (tok->kind == TK_StringLiteral) {
    size_t size = tok->chars.size;
    assert(size >= 2 &&
           "String literals from the lexer should have the start and end "
           "double quotes");

    StringLiteral *s = malloc(sizeof(StringLiteral));
    string_literal_construct(s, tok->chars.data + 1, size - 2);
    parser_consume_token(parser, TK_StringLiteral);
    return &s->expr;
  }

  printf("%llu:%llu: parse_primary_expr: Unhandled token (%d): '%s'\n",
         tok->line, tok->col, tok->kind, tok->chars.data);
  __builtin_trap();
  return NULL;
}

void index_destroy(Expr *expr);

const ExprVtable IndexVtable = {
    .kind = EK_UnOp,
    .dtor = index_destroy,
};

typedef struct {
  Expr expr;
  Expr *base;
  Expr *idx;
} Index;

void index_construct(Index *index_expr, Expr *base, Expr *idx) {
  expr_construct(&index_expr->expr, &IndexVtable);
  index_expr->base = base;
  index_expr->idx = idx;
}

void index_destroy(Expr *expr) {
  Index *index_expr = (Index *)expr;
  expr_destroy(index_expr->base);
  free(index_expr->base);
  expr_destroy(index_expr->idx);
  free(index_expr->idx);
}

void call_destroy(Expr *expr);

const ExprVtable CallVtable = {
    .kind = EK_Call,
    .dtor = call_destroy,
};

typedef struct {
  Expr expr;
  Expr *base;
  vector args;  // Vector of Expr*
} Call;

void call_construct(Call *call, Expr *base, vector v) {
  expr_construct(&call->expr, &CallVtable);
  call->base = base;
  call->args = v;
}

void call_destroy(Expr *expr) {
  Call *call = (Call *)expr;
  expr_destroy(call->base);
  free(call->base);

  for (size_t i = 0; i < call->args.size; ++i) {
    Expr **expr = vector_at(&call->args, i);
    expr_destroy(*expr);
    free(*expr);
  }
  vector_destroy(&call->args);
}

void member_access_destroy(Expr *expr);

const ExprVtable MemberAccessVtable = {
    .kind = EK_MemberAccess,
    .dtor = member_access_destroy,
};

typedef struct {
  Expr expr;
  Expr *base;
  char *member;
  bool is_arrow;
} MemberAccess;

void member_access_construct(MemberAccess *member_access, Expr *base,
                             const char *member, bool is_arrow) {
  expr_construct(&member_access->expr, &MemberAccessVtable);
  member_access->base = base;
  member_access->member = strdup(member);
  member_access->is_arrow = is_arrow;
}

void member_access_destroy(Expr *expr) {
  MemberAccess *member_access = (MemberAccess *)expr;
  expr_destroy(member_access->base);
  free(member_access->base);
  free(member_access->member);
}

//
// <argument_expr_list> = <assignment_expr> ("," <assignment_expr>)*
//
vector parse_argument_list(Parser *parser) {
  vector v;
  vector_construct(&v, sizeof(Expr *), alignof(Expr *));

  bool should_continue;
  do {
    should_continue = false;

    Expr *expr = parse_assignment_expr(parser);
    Expr **storage = vector_append_storage(&v);
    *storage = expr;

    if ((should_continue = (next_token_is(parser, TK_Comma))))
      parser_consume_token(parser, TK_Comma);

  } while (should_continue);

  return v;
}

//
// <postfix_expr> = <primary_expr>
//                | <postfix_expr> "[" <expr> "]"
//                | <postfix_expr> "(" <argument_expr_list>? ")"
//                | <postfix_expr> "." <identifier>
//                | <postfix_expr> "->" <identifier>
//                | <postfix_expr> "++"
//                | <postfix_expr> "--"
//
Expr *parse_postfix_expr(Parser *parser) {
  Expr *expr = parse_primary_expr(parser);

  bool should_continue = true;
  while (should_continue) {
    const Token *tok = parser_peek_token(parser);

    switch (tok->kind) {
      case TK_LSquareBrace: {
        parser_consume_token(parser, TK_LSquareBrace);
        Expr *idx = parse_expr(parser);
        parser_consume_token(parser, TK_RSquareBrace);

        Index *index = malloc(sizeof(Index));
        index_construct(index, expr, idx);
        expr = &index->expr;
        break;
      }

      case TK_LPar: {
        parser_consume_token(parser, TK_LPar);

        vector v;
        if (parser_peek_token(parser)->kind != TK_RPar)
          v = parse_argument_list(parser);
        else
          vector_construct(&v, sizeof(Expr *), alignof(Expr *));

        parser_consume_token(parser, TK_RPar);

        Call *call = malloc(sizeof(Call));
        call_construct(call, expr, v);
        expr = &call->expr;
        break;
      }

      case TK_Dot:
      case TK_Arrow: {
        bool is_arrow = tok->kind == TK_Arrow;
        size_t line = tok->line;
        size_t col = tok->col;

        parser_consume_token(parser, tok->kind);

        Token id = parser_pop_token(parser);
        assert(id.kind == TK_Identifier);

        MemberAccess *member_access = malloc(sizeof(MemberAccess));
        member_access_construct(member_access, expr, id.chars.data, is_arrow);
        member_access->expr.line = line;
        member_access->expr.col = col;
        expr = &member_access->expr;

        token_destroy(&id);
        break;
      }

      case TK_Inc: {
        parser_consume_token(parser, TK_Inc);
        UnOp *unop = malloc(sizeof(UnOp));
        unop_construct(unop, expr, UOK_PostInc);
        expr = &unop->expr;
        break;
      }

      case TK_Dec: {
        parser_consume_token(parser, TK_Dec);
        UnOp *unop = malloc(sizeof(UnOp));
        unop_construct(unop, expr, UOK_PostDec);
        expr = &unop->expr;
        break;
      }

      default:
        should_continue = false;
    }
  }
  return expr;
}

//
// <unary_epr> = <postfix_expr>
//             | ("++" | "--") <unary_expr>
//             | ("&" | "*" | "+" | "-" | "~" | "!") <cast_expr>
//             | "sizeof" "(" (<unary_expr> | <type>) ")"
//
Expr *parse_unary_expr(Parser *parser) {
  const Token *tok = parser_peek_token(parser);

  UnOpKind op;
  switch (tok->kind) {
    case TK_Inc:
      op = UOK_PreInc;
      break;
    case TK_Dec:
      op = UOK_PreDec;
      break;
    case TK_Ampersand:
      op = UOK_AddrOf;
      break;
    case TK_Star:
      op = UOK_Deref;
      break;
    case TK_Add:
      op = UOK_Plus;
      break;
    case TK_Sub:
      op = UOK_Negate;
      break;
    case TK_BitNot:
      op = UOK_BitNot;
      break;
    case TK_Not:
      op = UOK_Not;
      break;
    case TK_SizeOf:
      return parse_sizeof(parser);
    default:
      return parse_postfix_expr(parser);
  }

  parser_consume_token(parser, tok->kind);

  Expr *expr;
  if (tok->kind == TK_Inc || tok->kind == TK_Dec) {
    expr = parse_unary_expr(parser);
  } else {
    expr = parse_cast_expr(parser);
  }

  UnOp *unop = malloc(sizeof(UnOp));
  unop_construct(unop, expr, op);
  return &unop->expr;
}

void function_param_destroy(Expr *) {}

const ExprVtable FunctionParamVtable = {
    .kind = EK_FunctionParam,
    .dtor = function_param_destroy,
};

typedef struct {
  Expr expr;
  // The caller should not destroy or free these.
  const char *name;
  const Type *type;
} FunctionParam;

void function_param_construct(FunctionParam *param, const char *name,
                              const Type *type) {
  expr_construct(&param->expr, &FunctionParamVtable);
  param->name = name;
  param->type = type;
}

void cast_destroy(Expr *expr);

const ExprVtable CastVtable = {
    .kind = EK_Cast,
    .dtor = cast_destroy,
};

typedef struct {
  Expr expr;
  Expr *base;
  Type *to;
} Cast;

void cast_construct(Cast *cast, Expr *base, Type *to) {
  expr_construct(&cast->expr, &CastVtable);
  cast->base = base;
  cast->to = to;
}

void cast_destroy(Expr *expr) {
  Cast *cast = (Cast *)expr;
  expr_destroy(cast->base);
  free(cast->base);
  type_destroy(cast->to);
  free(cast->to);
}

bool is_token_type_token(const Parser *parser, const Token *tok) {
  bool is_type = is_builtin_type_token(tok->kind) ||
                 is_qualifier_token(tok->kind) || tok->kind == TK_Enum ||
                 tok->kind == TK_Struct || tok->kind == TK_Union;

  // Need to check for typedefs
  if (!is_type && tok->kind == TK_Identifier)
    is_type = parser_has_named_type(parser, tok->chars.data);

  return is_type;
}

bool is_next_token_type_token(Parser *parser) {
  return is_token_type_token(parser, parser_peek_token(parser));
}

// This only exists so we can dispatch to either a cast expression or a normal
// expression surrounded by parenthesis since we need to know if the contents
// after the LPar is a type or not.
Expr *parse_cast_or_paren_expr(Parser *parser) {
  parser_consume_token(parser, TK_LPar);

  if (!is_next_token_type_token(parser))
    return parse_parentheses_expr_tail(parser);

  // Definitely a type case.
  // "(" <type> ")" <cast_expr>
  Type *type = parse_type(parser);
  parser_consume_token(parser, TK_RPar);

  Expr *base = parse_cast_expr(parser);

  Cast *cast = malloc(sizeof(Cast));
  cast_construct(cast, base, type);
  return &cast->expr;
}

//
// <cast_epr> = <unary_expr>
//            | "(" <type> ")" <cast_expr>
//
Expr *parse_cast_expr(Parser *parser) {
  const Token *tok = parser_peek_token(parser);

  if (tok->kind == TK_LPar)
    return parse_cast_or_paren_expr(parser);

  return parse_unary_expr(parser);
}

//
// <multiplicative_epr> = <cast_expr>
//                      | <cast_expr> ("*" | "/" | "%") <multiplicative_expr>
//
Expr *parse_multiplicative_expr(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr *expr = parse_cast_expr(parser);
  assert(expr);

  const Token *token = parser_peek_token(parser);
  BinOpKind op;
  switch (token->kind) {
    case TK_Star:
      op = BOK_Mul;
      break;
    case TK_Div:
      op = BOK_Div;
      break;
    case TK_Mod:
      op = BOK_Mod;
      break;
    default:
      return expr;
  }

  parser_consume_token(parser, token->kind);

  Expr *rhs = parse_multiplicative_expr(parser);
  assert(rhs);

  BinOp *binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, op);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <additive_epr> = <multiplicative_expr>
//                | <multiplicative_expr> ("+" | "-") <additive_expr>
//
Expr *parse_additive_expr(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr *expr = parse_multiplicative_expr(parser);
  assert(expr);

  const Token *token = parser_peek_token(parser);
  BinOpKind op;
  switch (token->kind) {
    case TK_Add:
      op = BOK_Add;
      break;
    case TK_Sub:
      op = BOK_Sub;
      break;
    default:
      return expr;
  }

  parser_consume_token(parser, token->kind);

  Expr *rhs = parse_additive_expr(parser);
  assert(rhs);

  BinOp *binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, op);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <shift_epr> = <additive_expr>
//             | <additive_expr> ("<<" | ">>") <shift_expr>
//
Expr *parse_shift_expr(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr *expr = parse_additive_expr(parser);
  assert(expr);

  const Token *token = parser_peek_token(parser);
  BinOpKind op;
  switch (token->kind) {
    case TK_LShift:
      op = BOK_LShift;
      break;
    case TK_RShift:
      op = BOK_RShift;
      break;
    default:
      return expr;
  }

  parser_consume_token(parser, token->kind);

  Expr *rhs = parse_shift_expr(parser);
  assert(rhs);

  BinOp *binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, op);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <relational_expr> = <shift_expr>
//                   | <shift_expr> ("<" | ">" | ">=" | "<=") <relational_expr>
//
Expr *parse_relational_expr(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr *expr = parse_shift_expr(parser);
  assert(expr);

  const Token *token = parser_peek_token(parser);
  BinOpKind op;
  switch (token->kind) {
    case TK_Lt:
      op = BOK_Lt;
      break;
    case TK_Gt:
      op = BOK_Gt;
      break;
    case TK_Le:
      op = BOK_Le;
      break;
    case TK_Ge:
      op = BOK_Ge;
      break;
    default:
      return expr;
  }

  parser_consume_token(parser, token->kind);

  Expr *rhs = parse_relational_expr(parser);
  assert(rhs);

  BinOp *binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, op);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <equality_expr> = <relational_expr>
//                 | <relational_expr> ("==" | "!=") <equality_expr>
//
Expr *parse_equality_expr(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr *expr = parse_relational_expr(parser);
  assert(expr);

  const Token *token = parser_peek_token(parser);
  BinOpKind op;
  switch (token->kind) {
    case TK_Eq:
      op = BOK_Eq;
      break;
    case TK_Ne:
      op = BOK_Ne;
      break;
    default:
      return expr;
  }

  parser_consume_token(parser, token->kind);

  Expr *rhs = parse_equality_expr(parser);
  assert(rhs);

  BinOp *binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, op);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <and_expr> = <equality_expr>
//            | <equality_expr> "&" <and_expr>
//
Expr *parse_and_expr(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr *expr = parse_equality_expr(parser);
  assert(expr);

  const Token *token = parser_peek_token(parser);
  if (token->kind != TK_Ampersand)
    return expr;

  parser_consume_token(parser, TK_Ampersand);

  Expr *rhs = parse_and_expr(parser);
  assert(rhs);

  BinOp *binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, BOK_BitwiseAnd);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <exclusive_or_expr> = <and_expr>
//                     | <and_expr> "^" <exclusive_or_expr>
//
Expr *parse_exclusive_or_expr(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr *expr = parse_and_expr(parser);
  assert(expr);

  const Token *token = parser_peek_token(parser);
  if (token->kind != TK_Xor)
    return expr;

  parser_consume_token(parser, TK_Xor);

  Expr *rhs = parse_exclusive_or_expr(parser);
  assert(rhs);

  BinOp *binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, BOK_Xor);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <inclusive_or_expr> = <exclusive_or_expr>
//                     | <exclusive_or_expr> "|" <inclusive_or_expr>
//
Expr *parse_inclusive_or_expr(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr *expr = parse_exclusive_or_expr(parser);
  assert(expr);

  const Token *token = parser_peek_token(parser);
  if (token->kind != TK_Or)
    return expr;

  parser_consume_token(parser, TK_Or);

  Expr *rhs = parse_inclusive_or_expr(parser);
  assert(rhs);

  BinOp *binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, BOK_BitwiseOr);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <logical_and_expr> = <inclusive_or_expr>
//                    | <inclusive_or_expr> "&&" <logical_and_expr>
//
Expr *parse_logical_and_expr(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr *expr = parse_inclusive_or_expr(parser);
  assert(expr);

  const Token *token = parser_peek_token(parser);
  if (token->kind != TK_LogicalAnd)
    return expr;

  parser_consume_token(parser, TK_LogicalAnd);

  Expr *rhs = parse_logical_and_expr(parser);
  assert(rhs);

  BinOp *binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, BOK_LogicalAnd);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <logical_or_expr> = <logical_and_expr>
//                   | <logical_and_expr> "||" <logical_or_expr>
//
Expr *parse_logical_or_expr(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr *expr = parse_logical_and_expr(parser);
  assert(expr);

  const Token *token = parser_peek_token(parser);
  if (token->kind != TK_LogicalOr)
    return expr;

  parser_consume_token(parser, TK_LogicalOr);

  Expr *rhs = parse_logical_or_expr(parser);
  assert(rhs);

  BinOp *binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, BOK_LogicalOr);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <conditional_expr> = <logical_or_expr>
//                    | <logical_or_expr> "?" <expr> ":" <conditional_expr>
//
Expr *parse_conditional_expr(Parser *parser) {
  Expr *expr = parse_logical_or_expr(parser);
  assert(expr);

  const Token *token = parser_peek_token(parser);
  if (token->kind != TK_Question)
    return expr;

  parser_consume_token(parser, TK_Question);

  Expr *true_expr = parse_expr(parser);
  assert(true_expr);

  parser_consume_token(parser, TK_Colon);

  Expr *false_expr = parse_conditional_expr(parser);
  assert(false_expr);

  Conditional *cond = malloc(sizeof(Conditional));
  conditional_construct(cond, expr, true_expr, false_expr);

  return &cond->expr;
}

//
// <assignment_expr> = <conditional_expr>
//                   | <conditional_expr> <assignment_op> <assignment_expr>
//
// <assignment_op> = ("*" | "/" | "%" | "+" | "-" | "<<" | ">>" | "&" | "|" |
//                    "^")? "="
//
Expr *parse_assignment_expr(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr *expr = parse_conditional_expr(parser);
  assert(expr);

  BinOpKind op;
  const Token *token = parser_peek_token(parser);
  switch (token->kind) {
    case TK_Assign:
      op = BOK_Assign;
      break;
    case TK_MulAssign:
      op = BOK_MulAssign;
      break;
    case TK_DivAssign:
      op = BOK_DivAssign;
      break;
    case TK_AddAssign:
      op = BOK_AddAssign;
      break;
    case TK_SubAssign:
      op = BOK_SubAssign;
      break;
    case TK_LShiftAssign:
      op = BOK_LShiftAssign;
      break;
    case TK_RShiftAssign:
      op = BOK_RShiftAssign;
      break;
    case TK_AndAssign:
      op = BOK_AndAssign;
      break;
    case TK_OrAssign:
      op = BOK_OrAssign;
      break;
    case TK_XorAssign:
      op = BOK_XorAssign;
      break;
    default:
      return expr;
  }

  parser_consume_token(parser, token->kind);

  Expr *rhs = parse_assignment_expr(parser);
  assert(expr);

  BinOp *binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, op);
  binop->expr.line = line;
  binop->expr.col = col;
  return &binop->expr;
}

//
// <expr> = <assignment_expr> | <assignment_expr> "," <expr>
//
// Operator precedence: https://www.lysator.liu.se/c/ANSI-C-grammar-y.html
Expr *parse_expr(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  Expr *expr = parse_assignment_expr(parser);
  assert(expr);

  const Token *token = parser_peek_token(parser);
  if (token->kind != TK_Comma)
    return expr;

  parser_consume_token(parser, TK_Comma);
  Expr *rhs = parse_expr(parser);
  assert(rhs);

  BinOp *binop = malloc(sizeof(BinOp));
  binop_construct(binop, expr, rhs, BOK_Comma);
  binop->expr.line = line;
  binop->expr.col = col;

  return &binop->expr;
}

//
// <static_assert> = "static_assert" "(" <expr> ")"
//
Node *parse_static_assert(Parser *parser) {
  parser_consume_token(parser, TK_StaticAssert);
  parser_consume_token(parser, TK_LPar);

  Expr *expr = parse_expr(parser);

  parser_consume_token(parser, TK_RPar);

  StaticAssert *sa = malloc(sizeof(StaticAssert));
  static_assert_construct(sa, expr);

  return &sa->node;
}

//
// <typedef> = "typedef" <type_with_declarator>
//
Node *parse_typedef(Parser *parser) {
  parser_consume_token(parser, TK_Typedef);

  Typedef *td = malloc(sizeof(Typedef));
  typedef_construct(td);

  td->type = parse_type_for_declaration(parser, &td->name, /*storage=*/NULL);

  // We don't care about the value.
  assert(!parser_has_named_type(parser, td->name));
  parser_define_named_type(parser, td->name);

  return &td->node;
}

Statement *parse_compound_stmt(Parser *);

Statement *parse_statement(Parser *parser) {
  const Token *peek = parser_peek_token(parser);
  if (peek->kind == TK_LCurlyBrace)
    return parse_compound_stmt(parser);

  if (peek->kind == TK_If) {
    parser_consume_token(parser, TK_If);
    parser_consume_token(parser, TK_LPar);

    Expr *cond = parse_expr(parser);

    parser_consume_token(parser, TK_RPar);

    Statement *body = parse_statement(parser);

    IfStmt *ifstmt = malloc(sizeof(IfStmt));
    if_stmt_construct(ifstmt, cond, body);
    return &ifstmt->base;
  }

  if (peek->kind == TK_Return) {
    parser_consume_token(parser, TK_Return);

    Expr *expr;
    if (!next_token_is(parser, TK_Semicolon)) {
      expr = parse_expr(parser);
    } else {
      expr = NULL;
    }
    parser_consume_token(parser, TK_Semicolon);

    ReturnStmt *ret = malloc(sizeof(ReturnStmt));
    return_stmt_construct(ret, expr);
    return &ret->base;
  }

  // Default is an expression statement.
  Expr *expr = parse_expr(parser);
  parser_consume_token(parser, TK_Semicolon);
  ExprStmt *stmt = malloc(sizeof(ExprStmt));
  expr_stmt_construct(stmt, expr);
  return &stmt->base;
}

Statement *parse_compound_stmt(Parser *parser) {
  parser_consume_token(parser, TK_LCurlyBrace);

  vector body;
  vector_construct(&body, sizeof(Statement *), alignof(Statement *));

  const Token *peek;
  while ((peek = parser_peek_token(parser))->kind != TK_RCurlyBrace) {
    Statement **storage = vector_append_storage(&body);
    *storage = parse_statement(parser);
  }

  parser_consume_token(parser, TK_RCurlyBrace);

  CompoundStmt *cmpd = malloc(sizeof(CompoundStmt));
  compound_stmt_construct(cmpd, body);
  return &cmpd->base;
}

Node *parse_global_variable_or_function_definition(Parser *parser) {
  char *name = NULL;
  FoundStorageClasses storage;
  Type *type = parse_type_for_declaration(parser, &name, &storage);
  assert(name);

  if (storage.auto_) {
    const Token *tok = parser_peek_token(parser);
    printf("%llu:%llu: 'auto' can only be used at block scope\n", tok->line,
           tok->col);
    __builtin_trap();
  }

  if (type->vtable->kind == TK_FunctionType &&
      next_token_is(parser, TK_LCurlyBrace)) {
    Statement *cmpd = parse_compound_stmt(parser);

    FunctionDefinition *func_def = malloc(sizeof(FunctionDefinition));
    function_definition_construct(func_def, name, type, (CompoundStmt *)cmpd);

    if (storage.static_)
      func_def->is_extern = false;

    return &func_def->node;
  }

  GlobalVariable *gv = malloc(sizeof(GlobalVariable));
  global_variable_construct(gv, name, type);

  if (storage.static_)
    gv->is_extern = false;

  if (storage.thread_local_)
    gv->is_thread_local = true;

  // TODO: Parse potential initializer.
  if (next_token_is(parser, TK_Assign)) {
    parser_consume_token(parser, TK_Assign);
    Expr *init = parse_expr(parser);
    gv->initializer = init;
  }

  return &gv->node;
}

// NOTE: Callers of this function are in charge of destroying and freeing the
// poiner.
Node *parse_top_level_decl(Parser *parser) {
  const Token *token = parser_peek_token(parser);
  assert(token->kind != TK_Eof);

  Node *node;
  switch (token->kind) {
    case TK_Typedef:
      node = parse_typedef(parser);
      break;
    case TK_StaticAssert:
      node = parse_static_assert(parser);
      break;
    default:
      if (is_token_type_token(parser, token) ||
          is_storage_class_specifier_token(token->kind))
        node = parse_global_variable_or_function_definition(parser);
      else
        node = NULL;
  }

  if (node) {
    if (node->vtable->kind != NK_FunctionDefinition)
      parser_consume_token(parser, TK_Semicolon);
    return node;
  }

  printf("%llu:%llu: parse_top_level_decl: Unhandled token (%d): '%s'\n",
         token->line, token->col, token->kind, token->chars.data);
  __builtin_trap();
  return NULL;
}

///
/// End Parser Implementation
///

///
/// Start Parser Tests
///

static Type *ParseTypeString(const char *str, char **name) {
  StringInputStream ss;
  string_input_stream_construct(&ss, str);

  Parser p;
  parser_construct(&p, &ss.base);
  parser_define_named_type(&p, "size_t");  // Just let some tests use this.

  Type *type = parse_type_for_declaration(&p, name, /*storage=*/NULL);

  parser_destroy(&p);
  input_stream_destroy(&ss.base);

  return type;
}

static void TestSimpleDeclarationParse() {
  char *name;
  Type *type = ParseTypeString("int x", &name);

  assert(strcmp(name, "x") == 0);
  assert(type->vtable->kind == TK_BuiltinType);
  assert(((BuiltinType *)type)->kind == BTK_Int);
  assert(type->qualifiers == 0);

  type_destroy(type);
  free(name);
}

static void TestArrayParsing() {
  char *name;
  Type *type = ParseTypeString("int x[5]", &name);

  assert(strcmp(name, "x") == 0);
  assert(type->vtable->kind == TK_ArrayType);
  assert(type->qualifiers == 0);

  Type *elem = ((ArrayType *)type)->elem_type;
  assert(elem->vtable->kind == TK_BuiltinType);
  assert(((BuiltinType *)elem)->kind == BTK_Int);

  Expr *size = ((ArrayType *)type)->size;
  assert(size);
  assert(size->vtable->kind == EK_Int);
  assert(((Int *)size)->val == 5);

  type_destroy(type);
  free(name);
}

static void TestPointerParsing() {
  char *name;
  Type *type = ParseTypeString("int *x", &name);

  assert(strcmp(name, "x") == 0);
  assert(type->vtable->kind == TK_PointerType);
  assert(type->qualifiers == 0);

  Type *pointee = ((PointerType *)type)->pointee;
  assert(pointee->vtable->kind == TK_BuiltinType);
  assert(((BuiltinType *)pointee)->kind == BTK_Int);

  type_destroy(type);
  free(name);
}

static void TestArrayPointerParsing() {
  char *name;
  Type *type = ParseTypeString("int **x[][10]", &name);

  assert(strcmp(name, "x") == 0);

  // `x` is an array...
  assert(type->vtable->kind == TK_ArrayType);
  ArrayType *outer_arr = (ArrayType *)type;

  // of no specified size...
  Expr *size = outer_arr->size;
  assert(size == NULL);

  // to an array...
  Type *elem = outer_arr->elem_type;
  assert(elem->vtable->kind == TK_ArrayType);
  ArrayType *inner_arr = (ArrayType *)elem;

  // of size 10...
  Expr *inner_size = inner_arr->size;
  assert(inner_size != NULL);
  assert(inner_size->vtable->kind == EK_Int);
  assert(((Int *)inner_size)->val == 10);

  // of pointer...
  Type *inner_elem = ((ArrayType *)elem)->elem_type;
  assert(inner_elem->vtable->kind == TK_PointerType);
  PointerType *ptrptr = (PointerType *)inner_elem;

  // to pointer...
  assert(ptrptr->pointee->vtable->kind == TK_PointerType);
  PointerType *ptr = (PointerType *)ptrptr->pointee;

  // to ints...
  assert(ptr->pointee->vtable->kind == TK_BuiltinType);
  assert(((BuiltinType *)ptr->pointee)->kind == BTK_Int);

  type_destroy(type);
  free(name);
}

static void TestPointerQualifierParsing() {
  char *name;
  Type *type = ParseTypeString("const int * volatile x", &name);

  assert(strcmp(name, "x") == 0);
  assert(type->vtable->kind == TK_PointerType);
  assert(type->qualifiers == kVolatileMask);

  Type *pointee = ((PointerType *)type)->pointee;
  assert(pointee->vtable->kind == TK_BuiltinType);
  assert(((BuiltinType *)pointee)->kind == BTK_Int);
  assert(pointee->qualifiers == kConstMask);

  type_destroy(type);
  free(name);
}

void TestFunctionParsing() {
  char *name;
  Type *type = ParseTypeString("void *realloc(void *, size_t)", &name);

  assert(strcmp(name, "realloc") == 0);
  assert(type->vtable->kind == TK_FunctionType);
  FunctionType *func = (FunctionType *)type;
  assert(func->pos_args.size == 2);
  assert(!func->has_var_args);

  Type *arg0 = get_arg_type(func, 0);
  assert(arg0->vtable->kind == TK_PointerType);
  assert(is_builtin_type(((PointerType *)arg0)->pointee, BTK_Void));

  Type *arg1 = get_arg_type(func, 1);
  assert(arg1->vtable->kind == TK_NamedType);
  assert(strcmp(((NamedType *)arg1)->name, "size_t") == 0);

  Type *ret = func->return_type;
  assert(ret->vtable->kind == TK_PointerType);
  assert(is_builtin_type(((PointerType *)ret)->pointee, BTK_Void));

  type_destroy(type);
  free(name);
}

void TestFunctionParsing2() {
  char *name;
  Type *type = ParseTypeString("size_t strlen(const char *)", &name);

  assert(strcmp(name, "strlen") == 0);
  assert(type->vtable->kind == TK_FunctionType);
  FunctionType *func = (FunctionType *)type;
  assert(func->pos_args.size == 1);
  assert(!func->has_var_args);

  Type *arg0 = get_arg_type(func, 0);
  assert(arg0->vtable->kind == TK_PointerType);
  Type *pointee = ((PointerType *)arg0)->pointee;
  assert(is_builtin_type(pointee, BTK_Char));
  assert(type_is_const(pointee));

  Type *ret = func->return_type;
  assert(ret->vtable->kind == TK_NamedType);
  assert(strcmp(((NamedType *)ret)->name, "size_t") == 0);

  type_destroy(type);
  free(name);
}

void TestFunctionPointerParsing() {
  char *name;
  Type *type = ParseTypeString("int (*fptr)(void)", &name);

  assert(strcmp(name, "fptr") == 0);
  assert(type->vtable->kind == TK_PointerType);
  Type *nested = ((PointerType *)type)->pointee;

  assert(nested->vtable->kind == TK_FunctionType);
  FunctionType *func = (FunctionType *)nested;
  assert(func->pos_args.size == 1);
  assert(!func->has_var_args);

  assert(is_builtin_type(get_arg_type(func, 0), BTK_Void));
  assert(is_builtin_type(func->return_type, BTK_Int));

  type_destroy(type);
  free(name);
}

static void TestFunctionParsingVarArgs() {
  char *name;
  Type *type = ParseTypeString("int printf(const char *, ...)", &name);

  assert(strcmp(name, "printf") == 0);
  assert(type->vtable->kind == TK_FunctionType);
  FunctionType *func = (FunctionType *)type;
  assert(func->pos_args.size == 1);
  assert(func->has_var_args);

  Type *arg0 = get_arg_type(func, 0);
  assert(arg0->vtable->kind == TK_PointerType);
  Type *pointee = ((PointerType *)arg0)->pointee;
  assert(is_builtin_type(pointee, BTK_Char));
  assert(type_is_const(pointee));

  Type *ret = func->return_type;
  assert(is_builtin_type(ret, BTK_Int));

  type_destroy(type);
  free(name);
}

static void RunParserTests() {
  TestSimpleDeclarationParse();
  TestArrayParsing();
  TestPointerParsing();
  TestArrayPointerParsing();
  TestPointerQualifierParsing();
  TestFunctionParsing();
  TestFunctionParsing2();
  TestFunctionPointerParsing();
  TestFunctionParsingVarArgs();
}

///
/// End Parser Tests
///

///
/// Start Sema Implementation
///
/// This performs semantic analysis and makes ann needed adjustments to the AST
/// before passing it to the compiler.
///

typedef struct {
  TreeMap typedef_types;

  // This is a map of all globals seen in this TU. If any of them are
  // definitions, those globals will be stored here over the ones without
  // definitions.
  //
  // The values are either GlobalVariable or FunctionDefinition pointers.
  TreeMap globals;

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
} Sema;

void sema_handle_global_variable(Sema *sema, GlobalVariable *gv);

void sema_construct(Sema *sema) {
  string_tree_map_construct(&sema->typedef_types);
  string_tree_map_construct(&sema->globals);

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
    BuiltinType *chars = create_builtin_type(BTK_Char);
    type_set_const(&chars->type);
    pointer_type_construct(&sema->str_ty, &chars->type);
  }

  {
    BuiltinType *ret_ty = malloc(sizeof(BuiltinType));
    builtin_type_construct(ret_ty, BTK_Void);

    vector args;
    vector_construct(&args, sizeof(FunctionArg), alignof(FunctionArg));

    FunctionType *builtin_trap_type = malloc(sizeof(FunctionType));
    function_type_construct(builtin_trap_type, &ret_ty->type, args);

    global_variable_construct(&sema->builtin_trap, "__builtin_trap",
                              &builtin_trap_type->type);

    sema_handle_global_variable(sema, &sema->builtin_trap);
  }
}

void sema_destroy(Sema *sema) {
  tree_map_destroy(&sema->typedef_types);
  tree_map_destroy(&sema->globals);

  // NOTE: We don't need to destroy the builtin types since they don't have
  // any members that need cleanup.

  type_destroy(&sema->str_ty.type);

  global_variable_destroy(&sema->builtin_trap.node);
}

const BuiltinType *sema_get_integral_type_for_enum(const Sema *sema,
                                                   const EnumType *) {
  // FIXME: This is ok for small types but will fail for sufficiently large
  // enough enums.
  return &sema->bt_Int;
}

typedef struct {
  enum {
    // TODO: Add the other expression result kinds.
    RK_Boolean,
    RK_UnsignedLongLong,
  } result_kind;

  union {
    bool b;
    unsigned long long ull;
  } result;
} ConstExprResult;

ConstExprResult sema_eval_expr(const Sema *, const Expr *);
int compare_constexpr_result_types(const ConstExprResult *,
                                   const ConstExprResult *);

// FIXME: This should also have a typedef context.
const Type *sema_resolve_named_type_from_name(const Sema *sema,
                                              const char *name) {
  void *found;
  if (!tree_map_get(&sema->typedef_types, name, &found)) {
    printf("Unknown type '%s'\n", name);
    __builtin_trap();
  }
  Type *res = found;
  assert(res->vtable->kind != TK_NamedType &&
         "All typedef types should be flattened to an unnamed type when being "
         "added.");
  return res;
}

const Type *sema_resolve_named_type(const Sema *sema, const NamedType *nt) {
  return sema_resolve_named_type_from_name(sema, nt->name);
}

const Type *sema_resolve_maybe_named_type(const Sema *sema, const Type *type) {
  if (type->vtable->kind == TK_NamedType)
    return sema_resolve_named_type(sema, (const NamedType *)type);
  return type;
}

bool sema_is_pointer_type(const Sema *sema, const Type *type,
                          const TreeMap *local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  return is_pointer_type(type);
}

const Type *sema_get_pointee(const Sema *sema, const Type *type,
                             const TreeMap *local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  return get_pointee(type);
}

bool sema_is_struct_type(const Sema *sema, const Type *type,
                         const TreeMap *local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  return type->vtable->kind == TK_StructType;
}

// TODO: Pass fewer args around.
bool sema_is_pointer_to(const Sema *sema, const Type *type, TypeKind kind,
                        const TreeMap *local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  if (!is_pointer_type(type))
    return false;

  const Type *pointee = get_pointee(type);
  pointee = sema_resolve_maybe_named_type(sema, pointee);
  return pointee->vtable->kind == TK_StructType;
}

bool sema_types_are_compatible_impl(const Sema *sema, const Type *lhs,
                                    const Type *rhs, bool ignore_quals) {
  if (lhs->vtable->kind == TK_NamedType) {
    const Type *resolved_lhs = sema_resolve_named_type(sema, (NamedType *)lhs);
    return sema_types_are_compatible_impl(sema, resolved_lhs, rhs,
                                          ignore_quals);
  }

  if (rhs->vtable->kind == TK_NamedType) {
    const Type *resolved_rhs = sema_resolve_named_type(sema, (NamedType *)rhs);
    return sema_types_are_compatible_impl(sema, lhs, resolved_rhs,
                                          ignore_quals);
  }

  if (lhs->vtable->kind != rhs->vtable->kind)
    return false;

  if (!ignore_quals && lhs->qualifiers != rhs->qualifiers)
    return false;

  switch (lhs->vtable->kind) {
    case TK_BuiltinType: {
      if (((BuiltinType *)lhs)->kind != ((BuiltinType *)rhs)->kind)
        return false;
      break;
    }
    case TK_PointerType: {
      if (!sema_types_are_compatible_impl(sema, ((PointerType *)lhs)->pointee,
                                          ((PointerType *)rhs)->pointee,
                                          ignore_quals))
        return false;
      break;
    }
    case TK_ArrayType: {
      ArrayType *lhs_arr = (ArrayType *)lhs;
      ArrayType *rhs_arr = (ArrayType *)rhs;
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
      StructType *lhs_struct = (StructType *)lhs;
      StructType *rhs_struct = (StructType *)rhs;

      // No anonymous structs are compatible.
      if (lhs_struct->name == NULL || lhs_struct->name == NULL)
        return false;

      // This should be enough since we wouldn't be able to define different
      // structs with the same name.
      if (strcmp(lhs_struct->name, rhs_struct->name) != 0)
        return false;

      break;
    }
    case TK_EnumType: {
      EnumType *lhs_enum = (EnumType *)lhs;
      EnumType *rhs_enum = (EnumType *)rhs;

      // No anonymous enums are compatible.
      if (lhs_enum->name == NULL || lhs_enum->name == NULL)
        return false;

      // This should be enough since we wouldn't be able to define different
      // structs with the same name.
      if (strcmp(lhs_enum->name, rhs_enum->name) != 0)
        return false;

      break;
    }
    case TK_FunctionType: {
      FunctionType *lhs_func = (FunctionType *)lhs;
      FunctionType *rhs_func = (FunctionType *)rhs;
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
        Type *lhs_arg =
            ((FunctionArg *)vector_at(&lhs_func->pos_args, i))->type;
        Type *rhs_arg =
            ((FunctionArg *)vector_at(&rhs_func->pos_args, i))->type;
        if (!sema_types_are_compatible_impl(sema, lhs_arg, rhs_arg,
                                            ignore_quals))
          return false;
      }
      break;
    }
    case TK_NamedType:
    case TK_ReplacementSentinelType:
      printf("This type should not be handled here.\n");
      __builtin_trap();
  }

  return true;
}

bool sema_types_are_compatible(const Sema *sema, const Type *lhs,
                               const Type *rhs) {
  return sema_types_are_compatible_impl(sema, lhs, rhs, /*ignore_quals=*/false);
}

bool sema_types_are_compatible_ignore_quals(const Sema *sema, const Type *lhs,
                                            const Type *rhs) {
  return sema_types_are_compatible_impl(sema, lhs, rhs, /*ignore_quals=*/true);
}

bool sema_can_implicit_cast_to(const Sema *sema, const Type *from,
                               const Type *to) {
  printf("TODO: Unhandled implicit cast\n");
  __builtin_trap();
}

static void assert_types_are_compatible(const Sema *sema, const char *name,
                                        const Type *original, const Type *new) {
  if (!sema_types_are_compatible(sema, original, new)) {
    printf("redefinition of '%s' with a different type\n", name);
    __builtin_trap();
  }
}

void verify_function_type(Sema *sema, const FunctionType *func_ty) {
  size_t num_args = func_ty->pos_args.size;
  for (size_t i = 0; i < num_args; ++i) {
    FunctionArg *arg = vector_at(&func_ty->pos_args, i);
    if (is_void_type(arg->type)) {
      assert(num_args == 1);
      assert(!func_ty->has_var_args);
      assert(arg->name == NULL);
    }
  }
}

void sema_handle_function_definition(Sema *sema, FunctionDefinition *f) {
  assert(f->type->vtable->kind == TK_FunctionType);
  verify_function_type(sema, (const FunctionType *)f->type);

  void *val;
  if (!tree_map_get(&sema->globals, f->name, &val)) {
    // This func was not declared prior. Unconditionally save this one.
    tree_map_set(&sema->globals, f->name, f);
    return;
  }

  // This global was declared prior.
  Node *found = val;
  if (found->vtable->kind == NK_FunctionDefinition) {
    // Function re-definition.
    printf("Redefinition of function '%s'\n", f->name);
    __builtin_trap();
  }

  // This must be a global variable.
  assert(found->vtable->kind == NK_GlobalVariable);
  GlobalVariable *gv = val;
  if (gv->initializer) {
    printf("Redefinition of function '%s'\n", f->name);
    __builtin_trap();
  }

  assert_types_are_compatible(sema, gv->name, gv->type, f->type);
}

void sema_handle_global_variable(Sema *sema, GlobalVariable *gv) {
  void *val;
  if (!tree_map_get(&sema->globals, gv->name, &val)) {
    // This global was not declared prior. Unconditionally save this one.
    tree_map_set(&sema->globals, gv->name, gv);
    return;
  }

  // This global was declared prior. Check that we have the same compatible
  // types.
  Node *found_val = val;
  if (found_val->vtable->kind == NK_GlobalVariable) {
    GlobalVariable *found = val;
    assert_types_are_compatible(sema, gv->name, found->type, gv->type);

    // Ensure we don't have multiple definitions.
    if (found->initializer && gv->initializer) {
      printf(
          "sema_handle_global_variable: redefinition of '%s' with a "
          "different value\n",
          gv->name);
      __builtin_trap();
    }

    if (!found->initializer && gv->initializer) {
      // Save the one with the definition.
      tree_map_set(&sema->globals, gv->name, gv);
    }

    return;
  }

  // This must be a function.
  assert(found_val->vtable->kind == NK_FunctionDefinition);
  FunctionDefinition *found = val;
  assert_types_are_compatible(sema, gv->name, found->type, gv->type);
  assert(found->type->vtable->kind == TK_FunctionType);
  verify_function_type(sema, (const FunctionType *)found->type);

  // The new global variable must be a re-declaration since we can only
  // have one function definition.
  if (gv->initializer) {
    printf("Redefinition of function '%s'\n", gv->name);
    __builtin_trap();
  }
}

void sema_add_typedef_type(Sema *sema, const char *name, Type *type) {
  if (tree_map_get(&sema->typedef_types, name, NULL)) {
    printf("sema_add_typedef_type: typedef for '%s' already exists\n", name);
    __builtin_trap();
  }

  switch (type->vtable->kind) {
    case TK_FunctionType:
    case TK_BuiltinType:
    case TK_StructType:
    case TK_EnumType:  // TODO: Handle enum values.
    case TK_PointerType:
    case TK_ArrayType:
      tree_map_set(&sema->typedef_types, name, type);
      break;
    case TK_NamedType: {
      const NamedType *nt = (const NamedType *)type;
      void *found;
      if (!tree_map_get(&sema->typedef_types, nt->name, &found)) {
        printf("Unknown type '%s'\n", nt->name);
        __builtin_trap();
      }

      // NOTE: This effectively flattens the type tree and we lose information
      // regarding one typedef aliasing to another typedef.
      assert(((Type *)found)->vtable->kind != TK_NamedType &&
             "All typedef types should be flattened to an unnamed type when "
             "being added.");
      tree_map_set(&sema->typedef_types, nt->name, found);
      break;
    }
    case TK_ReplacementSentinelType:
      printf("Replacement sentinel type should not be used\n");
      __builtin_trap();
      break;
  }
}

const Type *sema_get_type_of_expr_in_ctx(const Sema *sema, const Expr *expr,
                                         const TreeMap *local_ctx);

const Type *sema_get_type_of_unop_expr(const Sema *sema, const UnOp *expr,
                                       const TreeMap *local_ctx) {
  switch (expr->op) {
    case UOK_Not:
      return &sema->bt_Bool.type;
    case UOK_BitNot:
      return sema_get_type_of_expr_in_ctx(sema, expr->subexpr, local_ctx);
    default:
      printf("TODO: Implement get type for unop kind %d\n", expr->op);
      __builtin_trap();
  }
}

size_t sema_eval_sizeof_type(const Sema *sema, const Type *type);

const Type *sema_get_corresponding_unsigned_type(const Sema *sema,
                                                 const BuiltinType *bt) {
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
      printf("Non-signed integral type\n");
      __builtin_trap();
  }
}

// Find the common type for usual arithmetic conversions.
const Type *sema_get_common_type(const Sema *sema, const Type *lhs_ty,
                                 const Type *rhs_ty, const TreeMap *local_ctx) {
  if (lhs_ty->vtable->kind == TK_NamedType)
    lhs_ty = sema_resolve_named_type(sema, (const NamedType *)lhs_ty);
  if (rhs_ty->vtable->kind == TK_NamedType)
    rhs_ty = sema_resolve_named_type(sema, (const NamedType *)rhs_ty);

  // TODO: Handle non-integral types also.
  assert(is_integral_type(lhs_ty) && is_integral_type(rhs_ty));

  // If the types are the same, that type is the common type.
  if (sema_types_are_compatible(sema, lhs_ty, rhs_ty))
    return lhs_ty;

  const BuiltinType *lhs_bt = (const BuiltinType *)lhs_ty;
  const BuiltinType *rhs_bt = (const BuiltinType *)rhs_ty;
  unsigned lhs_rank = get_integral_rank(lhs_bt);
  unsigned rhs_rank = get_integral_rank(rhs_bt);

  // If the types have the same signedness (both signed or both unsigned),
  // the operand whose type has the lesser conversion rank is implicitly
  // converted to the other type.
  if (is_unsigned_integral_type(lhs_ty) == is_unsigned_integral_type(rhs_ty)) {
    return lhs_rank > rhs_rank ? lhs_ty : rhs_ty;
  }

  const Type *unsigned_ty, *signed_ty;
  unsigned unsigned_rank, signed_rank;
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
                                              (const BuiltinType *)signed_ty);
}

const Type *sema_get_common_type_of_exprs(const Sema *sema, const Expr *lhs,
                                          const Expr *rhs,
                                          const TreeMap *local_ctx) {
  const Type *lhs_ty = sema_get_type_of_expr_in_ctx(sema, lhs, local_ctx);
  const Type *rhs_ty = sema_get_type_of_expr_in_ctx(sema, rhs, local_ctx);
  return sema_get_common_type(sema, lhs_ty, rhs_ty, local_ctx);
}

const Type *sema_get_type_of_binop_expr(const Sema *sema, const BinOp *expr,
                                        const TreeMap *local_ctx) {
  const Type *lhs_ty = sema_get_type_of_expr_in_ctx(sema, expr->lhs, local_ctx);
  const Type *rhs_ty = sema_get_type_of_expr_in_ctx(sema, expr->rhs, local_ctx);

  if (lhs_ty->vtable->kind == TK_NamedType)
    lhs_ty = sema_resolve_named_type(sema, (const NamedType *)lhs_ty);
  if (rhs_ty->vtable->kind == TK_NamedType)
    rhs_ty = sema_resolve_named_type(sema, (const NamedType *)rhs_ty);

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
    case BOK_Div:
    case BOK_Mod:
    case BOK_Xor:
    case BOK_BitwiseOr:
    case BOK_BitwiseAnd:
      return sema_get_common_type(sema, lhs_ty, rhs_ty, local_ctx);
    case BOK_Assign:
      return rhs_ty;
    default:
      printf("TODO: Implement get type for binop kind %d on %llu:%llu\n",
             expr->op, expr->expr.line, expr->expr.col);
      __builtin_trap();
  }
}

const StructType *sema_get_struct_type_from_member_access(
    const Sema *sema, const MemberAccess *access, const TreeMap *local_ctx) {
  const Type *base_ty =
      sema_get_type_of_expr_in_ctx(sema, access->base, local_ctx);
  base_ty = sema_resolve_maybe_named_type(sema, base_ty);

  if (access->is_arrow) {
    assert(sema_is_pointer_to(sema, base_ty, TK_StructType, local_ctx));
    base_ty = sema_get_pointee(sema, base_ty, local_ctx);
    base_ty = sema_resolve_maybe_named_type(sema, base_ty);
  }
  assert(sema_is_struct_type(sema, base_ty, local_ctx));

  return (const StructType *)base_ty;
}

// Infer the type of this expression. The caller of this is not in charge of
// destroying the type.
const Type *sema_get_type_of_expr_in_ctx(const Sema *sema, const Expr *expr,
                                         const TreeMap *local_ctx) {
  switch (expr->vtable->kind) {
    case EK_String:
      return &sema->str_ty.type;
    case EK_SizeOf:
      return sema_resolve_named_type_from_name(sema, "size_t");
    case EK_Int:
      return &((const Int *)expr)->type.type;
    case EK_DeclRef: {
      const DeclRef *decl = (const DeclRef *)expr;
      void *val;
      if (tree_map_get(&sema->globals, decl->name, &val)) {
        if (((Node *)val)->vtable->kind == NK_GlobalVariable) {
          GlobalVariable *gv = val;
          return gv->type;
        } else {
          FunctionDefinition *f = val;
          return f->type;
        }
      }

      // Not a global. Search the local scope.
      if (tree_map_get(local_ctx, decl->name, &val))
        return sema_get_type_of_expr_in_ctx(sema, val, local_ctx);

      printf("Unknown symbol '%s'\n", decl->name);
      __builtin_trap();
    }
    case EK_FunctionParam:
      return ((const FunctionParam *)expr)->type;
    case EK_UnOp:
      return sema_get_type_of_unop_expr(sema, (const UnOp *)expr, local_ctx);
    case EK_BinOp:
      return sema_get_type_of_binop_expr(sema, (const BinOp *)expr, local_ctx);
    case EK_Call: {
      const Call *call = (const Call *)expr;
      const Type *ty =
          sema_get_type_of_expr_in_ctx(sema, call->base, local_ctx);
      assert(ty->vtable->kind == TK_FunctionType);
      return ((const FunctionType *)ty)->return_type;
    }
    case EK_Cast: {
      const Cast *cast = (const Cast *)expr;
      return cast->to;
    }
    case EK_MemberAccess: {
      const MemberAccess *access = (const MemberAccess *)expr;
      const StructType *base_ty =
          sema_get_struct_type_from_member_access(sema, access, local_ctx);

      const StructMember *member =
          struct_get_member(base_ty, access->member, /*offset=*/NULL);
      return member->type;
    }
    case EK_Conditional: {
      const Conditional *conditional = (const Conditional *)expr;
      return sema_get_common_type_of_exprs(sema, conditional->true_expr,
                                           conditional->false_expr,
                                           local_ctx);
    }
    default:
      printf("TODO: Implement sema_get_type_of_expr for this expression %d\n",
             expr->vtable->kind);
      __builtin_trap();
  }
}

// Infer the type of this expression in the global context. The caller of this
// is not in charge of destroying the type.
const Type *sema_get_type_of_expr(const Sema *sema, const Expr *expr) {
  TreeMap local_ctx;
  string_tree_map_construct(&local_ctx);
  const Type *res = sema_get_type_of_expr_in_ctx(sema, expr, &local_ctx);
  tree_map_destroy(&local_ctx);
  return res;
}

static size_t builtin_type_get_size(const BuiltinType *bt) {
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
    case BTK_Bool:
      return sizeof(bool);
    case BTK_Void:
      printf("Attempting to get sizeof void\n");
      __builtin_trap();
  }
}

size_t sema_eval_sizeof_array(const Sema *, const ArrayType *);

size_t sema_eval_sizeof_type(const Sema *sema, const Type *type) {
  switch (type->vtable->kind) {
    case TK_BuiltinType:
      return builtin_type_get_size((const BuiltinType *)type);
    case TK_PointerType:
      // FIXME: The size of a pointer to different types is NOT guaranteed to
      // be the same size for each individual pointee type. All pointers only
      // happen to be the same size for the host platform I'm building this on.
      return sizeof(void *);
    case TK_NamedType: {
      const NamedType *nt = (const NamedType *)type;
      void *found;
      if (!tree_map_get(&sema->typedef_types, nt->name, &found)) {
        printf("Unknown type '%s'\n", nt->name);
        __builtin_trap();
      }
      return sema_eval_sizeof_type(sema, (const Type *)found);
    }
    case TK_EnumType:
      return builtin_type_get_size(
          sema_get_integral_type_for_enum(sema, (const EnumType *)type));
    case TK_ArrayType:
      return sema_eval_sizeof_array(sema, (const ArrayType *)type);
    case TK_FunctionType:
      printf("Cannot take sizeof function type!\n");
      __builtin_trap();
    case TK_StructType:
      printf("TODO: sema_eval_sizeof_type for StructType\n");
      __builtin_trap();
    case TK_ReplacementSentinelType:
      printf("Replacement sentinel type should not be used\n");
      __builtin_trap();
      break;
  }
}

int compare_constexpr_result_types(const ConstExprResult *lhs,
                                   const ConstExprResult *rhs) {
  assert(lhs->result_kind == rhs->result_kind);

  // TODO: Would be neat to see if the compiler optimizes this into a normal cmp
  // instruction.
  switch (lhs->result_kind) {
    case RK_Boolean:
      return !(lhs->result.b == rhs->result.b);
    case RK_UnsignedLongLong:
      if (lhs->result.ull < rhs->result.ull)
        return -1;
      if (lhs->result.ull > rhs->result.ull)
        return 1;
      return 0;
  }
}

const ConstExprResult TrueResult = {
    .result_kind = RK_Boolean,
    .result.b = true,
};

const ConstExprResult FalseResult = {
    .result_kind = RK_Boolean,
    .result.b = false,
};

size_t sema_eval_sizeof_array(const Sema *sema, const ArrayType *arr) {
  size_t elem_size = sema_eval_sizeof_type(sema, arr->elem_type);
  assert(arr->size);
  ConstExprResult num_elems = sema_eval_expr(sema, arr->size);
  assert(num_elems.result_kind == RK_UnsignedLongLong);
  return elem_size * num_elems.result.ull;
}

ConstExprResult sema_eval_binop(const Sema *sema, const BinOp *expr) {
  ConstExprResult lhs = sema_eval_expr(sema, expr->lhs);
  ConstExprResult rhs = sema_eval_expr(sema, expr->rhs);

  switch (expr->op) {
    case BOK_Eq:
      if (lhs.result_kind == rhs.result_kind)
        return compare_constexpr_result_types(&lhs, &rhs) == 0 ? TrueResult
                                                               : FalseResult;

      printf(
          "TODO: Implement constant evaluation for the == of other "
          "ConstExprResults\n");
      __builtin_trap();
      break;
    default:
      printf("TODO: Implement constant evaluation for the remaining binops\n");
      __builtin_trap();
      break;
  }
}

ConstExprResult sema_eval_sizeof(const Sema *sema, const SizeOf *so) {
  const Type *type;
  if (so->is_expr) {
    type = sema_get_type_of_expr(sema, (const Expr *)so->expr_or_type);
  } else {
    type = (const Type *)so->expr_or_type;
  }

  size_t size = sema_eval_sizeof_type(sema, type);
  ConstExprResult res = {
      .result_kind = RK_UnsignedLongLong,
      .result.ull = size,
  };
  return res;
}

ConstExprResult sema_eval_expr(const Sema *sema, const Expr *expr) {
  switch (expr->vtable->kind) {
    case EK_BinOp:
      return sema_eval_binop(sema, (const BinOp *)expr);
    case EK_SizeOf:
      return sema_eval_sizeof(sema, (const SizeOf *)expr);
    case EK_Int: {
      ConstExprResult res = {
          .result_kind = RK_UnsignedLongLong,
          .result.ull = ((const Int *)expr)->val,
      };
      return res;
    }
    default:
      printf(
          "TODO: Implement constant evaluation for the remaining "
          "expressions (%d)\n",
          expr->vtable->kind);
      __builtin_trap();
      break;
  }
}

void sema_verify_static_assert_condition(const Sema *sema, const Expr *cond) {
  ConstExprResult res = sema_eval_expr(sema, cond);
  switch (res.result_kind) {
    case RK_Boolean:
      if (!res.result.b) {
        printf("static_assert failed\n");
        __builtin_trap();
      }
      break;
    case RK_UnsignedLongLong:
      if (res.result.ull == 0) {
        printf("static_assert failed\n");
        __builtin_trap();
      }
      break;
  }
}

///
/// End Sema Implementation
///

///
/// Start Compiler Implementation
///

typedef struct {
  // The compiler does not own the Sema or the module. It only modifies them.
  LLVMModuleRef mod;
  Sema *sema;
} Compiler;

void compiler_construct(Compiler *compiler, LLVMModuleRef mod, Sema *sema) {
  compiler->mod = mod;
  compiler->sema = sema;
}

void compiler_destroy(Compiler *compiler) {}

LLVMTypeRef get_llvm_type(Compiler *compiler, const Type *type);

LLVMTypeRef get_llvm_struct_type(Compiler *compiler, const StructType *st) {
  void *llvm_struct;

  // First see if we already made one.
  if (st->name) {
    if ((llvm_struct = LLVMGetTypeByName(compiler->mod, st->name)))
      return llvm_struct;
  }

  // Doesn't exits. Create the struct body.
  vector elems;
  vector_construct(&elems, sizeof(LLVMTypeRef), alignof(LLVMTypeRef));
  for (size_t i = 0; i < st->members->size; ++i) {
    StructMember *member = vector_at(st->members, i);
    LLVMTypeRef *storage = vector_append_storage(&elems);
    *storage = get_llvm_type(compiler, member->type);
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

LLVMTypeRef get_opaque_ptr(Compiler *compiler) {
  return LLVMPointerTypeInContext(LLVMGetModuleContext(compiler->mod),
                                  /*AddressSpace=*/0);
}

LLVMTypeRef get_llvm_array_type(Compiler *compiler, const ArrayType *at) {
  if (!at->size)
    return get_opaque_ptr(compiler);

  ConstExprResult size = sema_eval_expr(compiler->sema, at->size);
  assert(size.result_kind == RK_UnsignedLongLong);

  LLVMTypeRef elem = get_llvm_type(compiler, at->elem_type);
  return LLVMArrayType(elem, (unsigned)size.result.ull);
}

LLVMTypeRef get_llvm_function_type(Compiler *compiler, const FunctionType *ft) {
  LLVMTypeRef ret = get_llvm_type(compiler, ft->return_type);

  vector params;
  vector_construct(&params, sizeof(LLVMTypeRef), alignof(LLVMTypeRef));
  for (size_t i = 0; i < ft->pos_args.size; ++i) {
    Type *arg_ty = ((FunctionArg *)vector_at(&ft->pos_args, i))->type;
    if (is_void_type(arg_ty))
      continue;
    LLVMTypeRef *storage = vector_append_storage(&params);
    *storage = get_llvm_type(compiler, arg_ty);
  }

  LLVMTypeRef res = LLVMFunctionType(ret, params.data, (unsigned)params.size,
                                     ft->has_var_args);

  vector_destroy(&params);

  return res;
}

LLVMTypeRef get_llvm_builtin_type(Compiler *compiler, const BuiltinType *bt) {
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
      return LLVMIntType((unsigned)(size * kCharBit));
    }
    case BTK_Float:
      return LLVMFloatTypeInContext(ctx);
    case BTK_Double:
      return LLVMDoubleTypeInContext(ctx);
    case BTK_LongDouble:
      return LLVMFP128TypeInContext(ctx);
    case BTK_Void:
      return LLVMVoidType();
    case BTK_Bool:
      return LLVMIntType(kCharBit);
  }
}

LLVMTypeRef get_llvm_type(Compiler *compiler, const Type *type) {
  switch (type->vtable->kind) {
    case TK_BuiltinType:
      return get_llvm_builtin_type(compiler, (const BuiltinType *)type);
    case TK_EnumType: {
      const BuiltinType *repr = sema_get_integral_type_for_enum(
          compiler->sema, (const EnumType *)type);
      return get_llvm_type(compiler, &repr->type);
    }
    case TK_NamedType:
      return get_llvm_type(
          compiler,
          sema_resolve_named_type(compiler->sema, (const NamedType *)type));
    case TK_StructType:
      return get_llvm_struct_type(compiler, (const StructType *)type);
    case TK_PointerType:
      return get_opaque_ptr(compiler);
    case TK_ArrayType:
      return get_llvm_array_type(compiler, (const ArrayType *)type);
    case TK_FunctionType:
      return get_llvm_function_type(compiler, (const FunctionType *)type);
    case TK_ReplacementSentinelType:
      printf("This type should not be handled here.\n");
      __builtin_trap();
  }
}

LLVMValueRef compile_expr(Compiler *compiler, LLVMBuilderRef builder,
                          const Expr *expr, TreeMap *local_ctx,
                          TreeMap *local_allocas);

LLVMTypeRef get_llvm_type_of_expr_global_ctx(Compiler *compiler,
                                             const Expr *expr);

LLVMValueRef compile_constant_expr(Compiler *compiler, const Expr *expr) {
  switch (expr->vtable->kind) {
    case EK_SizeOf: {
      ConstExprResult res =
          sema_eval_sizeof(compiler->sema, (const SizeOf *)expr);
      assert(res.result_kind == RK_UnsignedLongLong);
      LLVMTypeRef llvm_type = get_llvm_type_of_expr_global_ctx(compiler, expr);
      return LLVMConstInt(llvm_type, res.result.ull, /*IsSigned=*/0);
    }
    case EK_Int: {
      const Type *type = sema_get_type_of_expr(compiler->sema, expr);
      assert(type->vtable->kind == TK_BuiltinType);
      bool is_signed = !is_unsigned_integral_type(type);
      LLVMTypeRef llvm_type = get_llvm_type_of_expr_global_ctx(compiler, expr);
      return LLVMConstInt(llvm_type, ((const Int *)expr)->val, is_signed);
    }
    case EK_Cast: {
      const Cast *cast = (const Cast *)expr;
      const Type *from_ty = sema_get_type_of_expr(compiler->sema, cast->base);
      const Type *to_ty = cast->to;

      LLVMValueRef from = compile_constant_expr(compiler, cast->base);

      if (sema_types_are_compatible(compiler->sema, from_ty, to_ty))
        return from;

      if (is_integral_type(from_ty) && is_pointer_type(to_ty))
        return LLVMConstIntToPtr(from, get_llvm_type(compiler, to_ty));

      printf("TODO: Unhandled constant cast conversion\n");
      __builtin_trap();
    }
    default:
      printf("TODO: Implement IR constant expr evaluation for expr kind %d\n",
             expr->vtable->kind);
      __builtin_trap();
  }
}

LLVMValueRef maybe_compile_constant_implicit_cast(Compiler *compiler,
                                                  const Expr *expr,
                                                  const Type *to_ty) {
  LLVMValueRef from = compile_constant_expr(compiler, expr);
  const Type *from_ty = sema_get_type_of_expr(compiler->sema, expr);

  if (sema_types_are_compatible_ignore_quals(compiler->sema, from_ty, to_ty))
    return from;

  from_ty = sema_resolve_maybe_named_type(compiler->sema, from_ty);
  to_ty = sema_resolve_maybe_named_type(compiler->sema, to_ty);

  if (is_integral_type(from_ty) && is_integral_type(to_ty)) {
    unsigned long long from_val = LLVMConstIntGetZExtValue(from);
    size_t from_size = builtin_type_get_size((const BuiltinType *)from_ty);
    size_t to_size = builtin_type_get_size((const BuiltinType *)to_ty);
    if (from_size != to_size) {
      LLVMTypeRef llvm_to_ty = get_llvm_type(compiler, to_ty);
      bool is_signed = !is_unsigned_integral_type(to_ty);
      return LLVMConstInt(llvm_to_ty, from_val, is_signed);
    }
  }

  printf("TODO: Unhandled implicit constant cast conversion\n");
  __builtin_trap();
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

LLVMValueRef compile_to_bool(Compiler *compiler, LLVMBuilderRef builder,
                             const Expr *expr, TreeMap *local_ctx,
                             TreeMap *local_allocas) {
  const Type *type =
      sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);

  if (type->vtable->kind == TK_NamedType)
    type = sema_resolve_named_type(compiler->sema, (const NamedType *)type);

  switch (type->vtable->kind) {
    case TK_BuiltinType:
    case TK_PointerType:
    case TK_EnumType:
    case TK_ArrayType: {
      // Easiest (but not always correct) way is to compare against zero.
      LLVMValueRef val =
          compile_expr(compiler, builder, expr, local_ctx, local_allocas);
      LLVMTypeRef type = LLVMTypeOf(val);
      LLVMValueRef zero = LLVMConstNull(type);
      if (should_use_icmp(type)) {
        return LLVMBuildICmp(builder, LLVMIntNE, val, zero, "to_bool");
      } else {
        return LLVMBuildFCmp(builder, LLVMRealUNE, val, zero, "to_bool");
      }
    }
    case TK_StructType:
    case TK_NamedType:
    case TK_FunctionType:
    case TK_ReplacementSentinelType:
      printf("Cannot convert this type to bool %d\n", type->vtable->kind);
      __builtin_trap();
  }
}

LLVMValueRef compile_unop(Compiler *compiler, LLVMBuilderRef builder,
                          const UnOp *expr, TreeMap *local_ctx,
                          TreeMap *local_allocas) {
  switch (expr->op) {
    case UOK_Not: {
      // Result is always an i1.
      LLVMValueRef to_bool = compile_to_bool(compiler, builder, expr->subexpr,
                                             local_ctx, local_allocas);
      LLVMValueRef zero = LLVMConstNull(LLVMInt1Type());
      return LLVMBuildICmp(builder, LLVMIntEQ, to_bool, zero, "not");
    }
    case UOK_BitNot: {
      LLVMValueRef val = compile_expr(compiler, builder, expr->subexpr,
                                      local_ctx, local_allocas);
      LLVMValueRef ones = LLVMConstAllOnes(LLVMTypeOf(val));
      return LLVMBuildXor(builder, val, ones, "");
    }
    default:
      printf("TODO: implement compile_unop for this op %d\n", expr->op);
      __builtin_trap();
  }
}

LLVMValueRef compile_implicit_cast(Compiler *compiler, LLVMBuilderRef builder,
                                   const Expr *from, const Type *to,
                                   TreeMap *local_ctx, TreeMap *local_allocas) {
  LLVMValueRef llvm_from =
      compile_expr(compiler, builder, from, local_ctx, local_allocas);

  const Type *from_ty =
      sema_get_type_of_expr_in_ctx(compiler->sema, from, local_ctx);

  if (sema_types_are_compatible(compiler->sema, from_ty, to))
    return llvm_from;

  LLVMTypeRef llvm_to = get_llvm_type(compiler, to);
  LLVMTypeRef llvm_from_ty = LLVMTypeOf(llvm_from);

  if (llvm_to == llvm_from_ty)
    return llvm_from;  // TODO: Is this branch ever reached?

  if (LLVMGetTypeKind(llvm_from_ty) == LLVMIntegerTypeKind) {
    if (LLVMGetTypeKind(llvm_to) == LLVMIntegerTypeKind) {
      unsigned from_size = LLVMGetIntTypeWidth(llvm_from_ty);
      unsigned to_size = LLVMGetIntTypeWidth(llvm_to);
      if (from_size == to_size)
        return llvm_from;
      else if (from_size > to_size)
        return LLVMBuildTrunc(builder, llvm_from, llvm_to, "");
      else if (is_unsigned_integral_type(from_ty))
        return LLVMBuildZExt(builder, llvm_from, llvm_to, "");
      else
        return LLVMBuildSExt(builder, llvm_from, llvm_to, "");
    }
  }

  printf("TODO: Handle this implicit cast\n");
  LLVMDumpType(llvm_from_ty);
  LLVMDumpType(llvm_to);
  __builtin_trap();
}

LLVMValueRef compile_local_variable(Compiler *compiler, LLVMBuilderRef builder,
                                    const char *name, const Type *ty,
                                    TreeMap *local_ctx,
                                    TreeMap *local_allocas) {
  void *val;
  if (tree_map_get(local_allocas, name, &val))
    return val;

  LLVMTypeRef llvm_ty = get_llvm_type(compiler, ty);
  LLVMValueRef alloca = LLVMBuildAlloca(builder, llvm_ty, name);
  tree_map_set(local_allocas, name, alloca);
  return alloca;
}

LLVMValueRef compile_conditional(Compiler *compiler, LLVMBuilderRef builder,
                          const Conditional *expr, TreeMap *local_ctx,
                          TreeMap *local_allocas) {
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);

  LLVMValueRef cond =
      compile_expr(compiler, builder, expr->cond, local_ctx, local_allocas);

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
  LLVMValueRef true_expr = compile_expr(compiler, builder, expr->true_expr, local_ctx, local_allocas);
  LLVMBuildBr(builder, mergebb);
  ifbb = LLVMGetInsertBlock(builder);  // Codegen of 'if' can change the current
                                       // block, update ifbb for the PHI.

  // Emit potential else BB.
  LLVMAppendExistingBasicBlock(fn, elsebb);
  LLVMPositionBuilderAtEnd(builder, elsebb);
  LLVMValueRef false_expr = compile_expr(compiler, builder, expr->false_expr, local_ctx, local_allocas);
  LLVMBuildBr(builder, mergebb);
  elsebb =
      LLVMGetInsertBlock(builder);  // Codegen of 'else' can change the current
                                    // block, update ifbb for the PHI.

  // Emit merge BB.
  LLVMAppendExistingBasicBlock(fn, mergebb);
  LLVMPositionBuilderAtEnd(builder, mergebb);

  LLVMValueRef phi = LLVMBuildPhi(builder, 
}

LLVMValueRef compile_binop(Compiler *compiler, LLVMBuilderRef builder,
                           const BinOp *expr, TreeMap *local_ctx,
                           TreeMap *local_allocas) {
  LLVMValueRef lhs, rhs;
  if (is_logical_binop(expr->op)) {
    lhs =
        compile_to_bool(compiler, builder, expr->lhs, local_ctx, local_allocas);
    rhs =
        compile_to_bool(compiler, builder, expr->rhs, local_ctx, local_allocas);
  } else if (is_assign_binop(expr->op)) {
    const Type *lhs_ty =
        sema_get_type_of_expr_in_ctx(compiler->sema, expr->lhs, local_ctx);
    rhs = compile_implicit_cast(compiler, builder, expr->rhs, lhs_ty, local_ctx,
                                local_allocas);
    lhs = compile_expr(compiler, builder, expr->lhs, local_ctx, local_allocas);
  } else {
    const Type *common_ty = sema_get_common_type_of_exprs(
        compiler->sema, expr->lhs, expr->rhs, local_ctx);
    lhs = compile_implicit_cast(compiler, builder, expr->lhs, common_ty,
                                local_ctx, local_allocas);
    rhs = compile_implicit_cast(compiler, builder, expr->rhs, common_ty,
                                local_ctx, local_allocas);
  }

  LLVMValueRef res;

  switch (expr->op) {
    case BOK_Ne:
      res = LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "");
      break;
    case BOK_Eq:
      res = LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "");
      break;
    case BOK_Add:
      res = LLVMBuildAdd(builder, lhs, rhs, "");
      break;
    case BOK_Sub:
      res = LLVMBuildSub(builder, lhs, rhs, "");
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
      LLVMBuildStore(builder, rhs, lhs);
      res = rhs;
      break;
    default:
      printf("TODO: implement compile_binop for this op %d\n", expr->op);
      __builtin_trap();
  }

  const Type *res_ty =
      sema_get_type_of_expr_in_ctx(compiler->sema, &expr->expr, local_ctx);
  if (is_bool_type(res_ty)) {
    // Cast up any i1s to i8s since bools are always kCharBits.
    res = LLVMBuildZExt(builder, res, LLVMIntType(kCharBit), "");
  }

  return res;
}

LLVMTypeRef get_llvm_type_of_expr(Compiler *compiler, const Expr *expr,
                                  TreeMap *local_ctx) {
  return get_llvm_type(
      compiler, sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx));
}

LLVMTypeRef get_llvm_type_of_expr_global_ctx(Compiler *compiler,
                                             const Expr *expr) {
  TreeMap local_ctx;
  string_tree_map_construct(&local_ctx);
  LLVMTypeRef type = get_llvm_type(
      compiler, sema_get_type_of_expr_in_ctx(compiler->sema, expr, &local_ctx));
  tree_map_destroy(&local_ctx);
  return type;
}

// Returns a vector of LLVMValueRefs.
// `args` is a vector of Expr* (the same as `Call::args`).
vector compile_call_args(Compiler *compiler, LLVMBuilderRef builder,
                         const vector *args, TreeMap *local_ctx,
                         TreeMap *local_allocas) {
  vector llvm_args;
  vector_construct(&llvm_args, sizeof(LLVMValueRef), alignof(LLVMValueRef));
  for (size_t i = 0; i < args->size; ++i) {
    LLVMValueRef *storage = vector_append_storage(&llvm_args);
    const Expr *arg = *(const Expr **)vector_at(args, i);
    *storage = compile_expr(compiler, builder, arg, local_ctx, local_allocas);
  }
  return llvm_args;
}

LLVMValueRef compile_expr(Compiler *compiler, LLVMBuilderRef builder,
                          const Expr *expr, TreeMap *local_ctx,
                          TreeMap *local_allocas) {
  LLVMTypeRef type = get_llvm_type_of_expr(compiler, expr, local_ctx);
  switch (expr->vtable->kind) {
    case EK_String: {
      const StringLiteral *s = (const StringLiteral *)expr;
      return LLVMBuildGlobalStringPtr(builder, s->val, "");
    }
    case EK_Int: {
      const Int *i = (const Int *)expr;
      bool has_sign = !is_unsigned_integral_type(&i->type.type);
      return LLVMConstInt(type, i->val, has_sign);
    }
    case EK_SizeOf: {
      ConstExprResult res = sema_eval_sizeof(compiler->sema, (SizeOf *)expr);
      assert(res.result_kind == RK_UnsignedLongLong);
      return LLVMConstInt(type, res.result.ull, /*IsSigned=*/0);
    }
    case EK_UnOp:
      return compile_unop(compiler, builder, (const UnOp *)expr, local_ctx,
                          local_allocas);
    case EK_BinOp:
      return compile_binop(compiler, builder, (const BinOp *)expr, local_ctx,
                           local_allocas);
    case EK_DeclRef: {
      const DeclRef *decl = (const DeclRef *)expr;
      bool is_func = LLVMGetTypeKind(type) == LLVMFunctionTypeKind;

      LLVMValueRef val = NULL;
      tree_map_get(local_allocas, decl->name, (void *)&val);
      if (!val) {
        // If not in the local scope, check the global scope.
        //
        // TODO: How come there's no wrapper for Module::getNamedValue?
        // Without it, I need to explicitly call the individual getters for
        // functions, global variables, adn aliases.
        if (is_func)
          val = LLVMGetNamedFunction(compiler->mod, decl->name);
        else
          val = LLVMGetNamedGlobal(compiler->mod, decl->name);

        if (!val) {
          printf("Couldn't find alloca or global for '%s'\n", decl->name);
          LLVMValueRef fn =
              LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
          LLVMDumpValue(fn);
          __builtin_trap();
        }
      }

      if (is_func)
        return val;

      assert(LLVMGetTypeKind(LLVMTypeOf(val)) == LLVMPointerTypeKind);
      return LLVMBuildLoad2(builder, type, val, "");
    }
    case EK_Call: {
      const Call *call = (const Call *)expr;
      if (call->base->vtable->kind == EK_DeclRef) {
        const DeclRef *decl = (const DeclRef *)call->base;
        if (strcmp(decl->name, "__builtin_trap") == 0) {
          const char name[] = "llvm.debugtrap";
          unsigned intrinsic_id = LLVMLookupIntrinsicID(name, sizeof(name) - 1);
          LLVMValueRef intrinsic = LLVMGetIntrinsicDeclaration(
              compiler->mod, intrinsic_id, /*ParamTypes=*/NULL,
              /*ParamCount=*/0);
          LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);
          LLVMTypeRef intrinsic_ty = LLVMIntrinsicGetType(
              ctx, intrinsic_id, /*ParamTypes=*/NULL, /*ParamCount=*/0);
          return LLVMBuildCall2(builder, intrinsic_ty, intrinsic,
                                /*Args=*/NULL, /*NumArgs=*/0, "");
        }
      }

      const Type *func_ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, call->base, local_ctx);
      LLVMTypeRef llvm_func_ty = get_llvm_type(compiler, func_ty);
      LLVMValueRef llvm_func =
          compile_expr(compiler, builder, call->base, local_ctx, local_allocas);

      vector llvm_args = compile_call_args(compiler, builder, &call->args,
                                           local_ctx, local_allocas);

      LLVMValueRef res =
          LLVMBuildCall2(builder, llvm_func_ty, llvm_func, llvm_args.data,
                         (unsigned)llvm_args.size, "");

      vector_destroy(&llvm_args);

      return res;
    }
    case EK_MemberAccess: {
      const MemberAccess *access = (const MemberAccess *)expr;
      const StructType *base_ty = sema_get_struct_type_from_member_access(
          compiler->sema, access, local_ctx);
      size_t offset;
      const StructMember *member =
          struct_get_member(base_ty, access->member, &offset);
      LLVMValueRef base_llvm = compile_expr(compiler, builder, access->base,
                                            local_ctx, local_allocas);
      LLVMTypeRef llvm_base_ty = get_llvm_type(compiler, &base_ty->type);
      LLVMValueRef llvm_offset = LLVMConstInt(
          get_llvm_type(compiler, member->type), offset, /*signed=*/0);
      LLVMValueRef offsets[] = {llvm_offset};
      LLVMValueRef gep =
          LLVMBuildGEP2(builder, llvm_base_ty, base_llvm, offsets, 1, "");
      return gep;
    }
    case EK_Conditional: {
      const Conditional *conditional = (const Conditional *expr);
    }
    default:
      printf("TODO: implement compile_expr for this expr %d\n",
             expr->vtable->kind);
      __builtin_trap();
  }
}

void compile_statement(Compiler *compiler, LLVMBuilderRef builder,
                       const Statement *stmt, TreeMap *local_ctx,
                       TreeMap *local_allocas);

void compile_if_statement(Compiler *compiler, LLVMBuilderRef builder,
                          const IfStmt *stmt, TreeMap *local_ctx,
                          TreeMap *local_allocas) {
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);
  LLVMValueRef cond =
      compile_expr(compiler, builder, stmt->cond, local_ctx, local_allocas);

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
  compile_statement(compiler, builder, stmt->body, local_ctx, local_allocas);
  LLVMBuildBr(builder, mergebb);
  ifbb = LLVMGetInsertBlock(builder);  // Codegen of 'if' can change the current
                                       // block, update ifbb for the PHI.

  // Emit potential else BB.
  LLVMAppendExistingBasicBlock(fn, elsebb);
  LLVMPositionBuilderAtEnd(builder, elsebb);
  if (stmt->else_stmt) {
    compile_if_statement(compiler, builder, stmt->else_stmt, local_ctx,
                         local_allocas);
  }
  LLVMBuildBr(builder, mergebb);
  elsebb =
      LLVMGetInsertBlock(builder);  // Codegen of 'else' can change the current
                                    // block, update ifbb for the PHI.

  // Emit merge BB.
  LLVMAppendExistingBasicBlock(fn, mergebb);
  LLVMPositionBuilderAtEnd(builder, mergebb);
}

void compile_statement(Compiler *compiler, LLVMBuilderRef builder,
                       const Statement *stmt, TreeMap *local_ctx,
                       TreeMap *local_allocas) {
  switch (stmt->vtable->kind) {
    case SK_ExprStmt:
      compile_expr(compiler, builder, ((const ExprStmt *)stmt)->expr, local_ctx,
                   local_allocas);
      break;
    case SK_IfStmt:
      return compile_if_statement(compiler, builder, (const IfStmt *)stmt,
                                  local_ctx, local_allocas);
    case SK_ReturnStmt: {
      const ReturnStmt *ret = (const ReturnStmt *)stmt;
      if (!ret->expr) {
        LLVMBuildRetVoid(builder);
        return;
      }
      LLVMValueRef val =
          compile_expr(compiler, builder, ret->expr, local_ctx, local_allocas);
      LLVMBuildRet(builder, val);
      return;
    }
    default:
      printf("TODO: implement compile_statement for this stmt %d\n",
             stmt->vtable->kind);
      __builtin_trap();
  }
}

void compile_function_definition(Compiler *compiler,
                                 const FunctionDefinition *f) {
  LLVMTypeRef llvm_func_ty = get_llvm_type(compiler, f->type);
  LLVMValueRef func = LLVMAddFunction(compiler->mod, f->name, llvm_func_ty);
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, entry);

  FunctionType *func_ty = (FunctionType *)(f->type);

  // Storage for local variables.
  vector local_storage;
  vector_construct(&local_storage, sizeof(Expr *), alignof(Expr *));

  TreeMap local_ctx, local_allocas;
  string_tree_map_construct(&local_ctx);
  string_tree_map_construct(&local_allocas);

  for (size_t i = 0; i < func_ty->pos_args.size; ++i) {
    FunctionArg *arg = vector_at(&func_ty->pos_args, i);
    if (!arg->name)
      continue;

    // Copy the parameter locally.
    LLVMValueRef llvm_arg = LLVMGetParam(func, (unsigned)i);
    // LLVMSetValueName2(llvm_arg, arg->name, strlen(arg->name));
    LLVMValueRef alloca =
        LLVMBuildAlloca(builder, LLVMTypeOf(llvm_arg), arg->name);
    LLVMBuildStore(builder, llvm_arg, alloca);

    FunctionParam *param = malloc(sizeof(FunctionParam));
    function_param_construct(param, arg->name, arg->type);

    tree_map_set(&local_ctx, arg->name, param);
    tree_map_set(&local_allocas, arg->name, alloca);
  }

  // TODO: Track argument names.
  for (size_t i = 0; i < f->body->body.size; ++i) {
    Statement *stmt = *(Statement **)vector_at(&f->body->body, i);
    compile_statement(compiler, builder, stmt, &local_ctx, &local_allocas);
  }

  if (is_void_type(func_ty->return_type))
    LLVMBuildRetVoid(builder);

  // Cleanup locals.
  for (size_t i = 0; i < local_storage.size; ++i) {
    Expr **storage = vector_at(&local_storage, i);
    expr_destroy(*storage);
  }
  vector_destroy(&local_storage);
  tree_map_destroy(&local_ctx);
  tree_map_destroy(&local_allocas);

  if (LLVMVerifyFunction(func, LLVMPrintMessageAction)) {
    printf("Verify function '%s' failed\n", f->name);
    LLVMDumpValue(func);
    __builtin_trap();
  }
}

void compile_global_variable(Compiler *compiler, const GlobalVariable *gv) {
  LLVMTypeRef ty = get_llvm_type(compiler, gv->type);

  if (gv->type->vtable->kind == TK_FunctionType) {
    LLVMAddFunction(compiler->mod, gv->name, ty);
    assert(!gv->initializer &&
           "If this had an initializer, it would be a function definition.");
    return;
  }

  LLVMValueRef glob = LLVMAddGlobal(compiler->mod, ty, gv->name);
  if (gv->initializer) {
    // TODO: This would be handled much easier if we had a node for implicit
    // casts.
    LLVMValueRef val = maybe_compile_constant_implicit_cast(
        compiler, gv->initializer, gv->type);
    LLVMSetInitializer(glob, val);
  }
}

///
/// End Compiler Implementation
///

int main(int argc, char **argv) {
  printf("Running initial tests...");
  RunStringTests();
  RunVectorTests();
  RunTreeMapTests();
  RunParserTests();
  printf("PASSED\n");

  if (argc < 2) {
    printf("Usage: %s input_file\n", argv[0]);
    return -1;
  }

  const char *input_filename = argv[1];

  // LLVM Initialization
  LLVMModuleRef mod = LLVMModuleCreateWithName(input_filename);

  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86Target();
  LLVMInitializeX86TargetMC();
  LLVMInitializeX86AsmParser();
  LLVMInitializeX86AsmPrinter();

  char *triple = LLVMGetDefaultTargetTriple();

  char *error;
  LLVMTargetRef target;
  if (LLVMGetTargetFromTriple(triple, &target, &error)) {
    printf("llvm error: %s\n", error);
    LLVMDisposeMessage(error);
    LLVMDisposeMessage(triple);
    return -1;
  }

  char *cpu = LLVMGetHostCPUName();
  char *features = LLVMGetHostCPUFeatures();

  LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
      target, triple, cpu, features, LLVMCodeGenLevelNone, LLVMRelocPIC,
      LLVMCodeModelDefault);

  LLVMDisposeMessage(triple);
  LLVMDisposeMessage(cpu);
  LLVMDisposeMessage(features);

  LLVMTargetDataRef data_layout = LLVMCreateTargetDataLayout(target_machine);
  LLVMSetModuleDataLayout(mod, data_layout);

  // Parsing + compiling
  FileInputStream input;
  file_input_stream_construct(&input, input_filename);
  Parser parser;
  parser_construct(&parser, &input.base);

  Sema sema;
  sema_construct(&sema);

  Compiler compiler;
  compiler_construct(&compiler, mod, &sema);

  vector nodes_to_destroy;
  vector_construct(&nodes_to_destroy, sizeof(Node *), alignof(Node *));

  while (1) {
    const Token *token = parser_peek_token(&parser);
    if (token->kind == TK_Eof) {
      break;
    } else {
      printf("Evaluating top level node at %llu:%llu (%s)\n", token->line,
             token->col, token->chars.data);

      // Note that the parse_* functions destroy the tokens.
      Node *top_level_decl = parse_top_level_decl(&parser);
      assert(top_level_decl);

      switch (top_level_decl->vtable->kind) {
        case NK_Typedef: {
          Typedef *td = (Typedef *)top_level_decl;
          sema_add_typedef_type(&sema, td->name, td->type);
          // if (td->type->vtable->kind == TK_StructType) {
          //   printf("TODO: Handle typedef structs\n");
          //   __builtin_trap();
          // }
          break;
        }
        case NK_StaticAssert: {
          StaticAssert *sa = (StaticAssert *)top_level_decl;
          sema_verify_static_assert_condition(&sema, sa->expr);
          break;
        }
        case NK_GlobalVariable: {
          GlobalVariable *gv = (GlobalVariable *)top_level_decl;
          sema_handle_global_variable(&sema, gv);
          compile_global_variable(&compiler, gv);
          break;
        }
        case NK_FunctionDefinition: {
          FunctionDefinition *f = (FunctionDefinition *)top_level_decl;
          sema_handle_function_definition(&sema, f);
          compile_function_definition(&compiler, f);
          break;
        }
      }

      Node **storage = vector_append_storage(&nodes_to_destroy);
      *storage = top_level_decl;

      // LLVMDumpModule(mod);
      if (LLVMVerifyModule(mod, LLVMPrintMessageAction, NULL)) {
        printf("Verify module failed\n");
        LLVMDumpModule(mod);
        __builtin_trap();
      }
    }
  }

  for (size_t i = 0; i < nodes_to_destroy.size; ++i) {
    Node *node = (Node *)vector_at(&nodes_to_destroy, i);
    node->vtable->dtor(node);
  }
  vector_destroy(&nodes_to_destroy);

  compiler_destroy(&compiler);
  sema_destroy(&sema);
  parser_destroy(&parser);
  input_stream_destroy(&input.base);

  return 0;
}
