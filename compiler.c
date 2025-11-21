#include <ctype.h>
#include <stddef.h>
#include <stdint.h>

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
void *memset(void *, int ch, size_t);
int strcmp(const char *, const char *);
char *strdup(const char *);

unsigned long long strtoull(const char *restrict, char **restrict, int);

void assert(bool b) {
  if (!b)
    __builtin_trap();
}

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
void LLVMDisposeModule(LLVMModuleRef M);
void LLVMDisposeTargetData(LLVMTargetDataRef TD);
void LLVMDisposeTargetMachine(LLVMTargetMachineRef T);
void LLVMDeleteBasicBlock(LLVMBasicBlockRef BB);
void LLVMDisposeBuilder(LLVMBuilderRef Builder);
LLVMBool LLVMPrintModuleToFile(LLVMModuleRef M, const char *Filename,
                               char **ErrorMessage);

typedef enum { LLVMAssemblyFile, LLVMObjectFile } LLVMCodeGenFileType;
LLVMBool LLVMTargetMachineEmitToFile(LLVMTargetMachineRef T, LLVMModuleRef M,
                                     const char *Filename,
                                     LLVMCodeGenFileType codegen,
                                     char **ErrorMessage);

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
LLVMTypeRef LLVMGetReturnType(LLVMTypeRef FunctionTy);
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

LLVMBasicBlockRef LLVMGetEntryBasicBlock(LLVMValueRef Fn);
LLVMBasicBlockRef LLVMAppendBasicBlock(LLVMValueRef Fn, const char *Name);
LLVMBuilderRef LLVMCreateBuilder(void);
void LLVMPositionBuilderAtEnd(LLVMBuilderRef Builder, LLVMBasicBlockRef Block);
void LLVMPositionBuilder(LLVMBuilderRef Builder, LLVMBasicBlockRef Block,
                         LLVMValueRef Instr);
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
LLVMValueRef LLVMBuildUnreachable(LLVMBuilderRef);
unsigned LLVMLookupIntrinsicID(const char *Name, size_t NameLen);
LLVMValueRef LLVMGetIntrinsicDeclaration(LLVMModuleRef Mod, unsigned ID,
                                         LLVMTypeRef *ParamTypes,
                                         size_t ParamCount);
LLVMTypeRef LLVMIntrinsicGetType(LLVMContextRef Ctx, unsigned ID,
                                 LLVMTypeRef *ParamTypes, size_t ParamCount);
LLVMValueRef LLVMBuildExtractValue(LLVMBuilderRef, LLVMValueRef AggVal,
                                   unsigned Index, const char *Name);
LLVMValueRef LLVMBuildRetVoid(LLVMBuilderRef);
LLVMValueRef LLVMBuildRet(LLVMBuilderRef, LLVMValueRef V);
LLVMValueRef LLVMConstIntToPtr(LLVMValueRef ConstantVal, LLVMTypeRef ToType);
LLVMValueRef LLVMConstPtrToInt(LLVMValueRef ConstantVal, LLVMTypeRef ToType);
LLVMValueRef LLVMBuildPtrToInt(LLVMBuilderRef, LLVMValueRef Val,
                               LLVMTypeRef DestTy, const char *Name);
LLVMValueRef LLVMBuildIntToPtr(LLVMBuilderRef, LLVMValueRef Val,
                               LLVMTypeRef DestTy, const char *Name);
LLVMValueRef LLVMBuildMul(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS,
                          const char *Name);
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
LLVMValueRef LLVMBuildPhi(LLVMBuilderRef, LLVMTypeRef Ty, const char *Name);
void LLVMAddIncoming(LLVMValueRef PhiNode, LLVMValueRef *IncomingValues,
                     LLVMBasicBlockRef *IncomingBlocks, unsigned Count);
LLVMValueRef LLVMBuildURem(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS,
                           const char *Name);
LLVMValueRef LLVMBuildSRem(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS,
                           const char *Name);
LLVMValueRef LLVMBuildShl(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS,
                          const char *Name);
LLVMValueRef LLVMBuildAShr(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS,
                           const char* Name);
LLVMValueRef LLVMGetLastInstruction(LLVMBasicBlockRef BB);
LLVMValueRef LLVMConstStruct(LLVMValueRef *ConstantVals, unsigned Count,
                             LLVMBool Packed);
LLVMValueRef LLVMBuildMemSet(LLVMBuilderRef B, LLVMValueRef Ptr,
                             LLVMValueRef Val, LLVMValueRef Len,
                             unsigned Align);

typedef enum {
  LLVMArgumentValueKind,
  LLVMBasicBlockValueKind,
  LLVMMemoryUseValueKind,
  LLVMMemoryDefValueKind,
  LLVMMemoryPhiValueKind,

  LLVMFunctionValueKind,
  LLVMGlobalAliasValueKind,
  LLVMGlobalIFuncValueKind,
  LLVMGlobalVariableValueKind,
  LLVMBlockAddressValueKind,
  LLVMConstantExprValueKind,
  LLVMConstantArrayValueKind,
  LLVMConstantStructValueKind,
  LLVMConstantVectorValueKind,

  LLVMUndefValueValueKind,
  LLVMConstantAggregateZeroValueKind,
  LLVMConstantDataArrayValueKind,
  LLVMConstantDataVectorValueKind,
  LLVMConstantIntValueKind,
  LLVMConstantFPValueKind,
  LLVMConstantPointerNullValueKind,
  LLVMConstantTokenNoneValueKind,

  LLVMMetadataAsValueValueKind,
  LLVMInlineAsmValueKind,

  LLVMInstructionValueKind,
  LLVMPoisonValueValueKind,
  LLVMConstantTargetNoneValueKind,
  LLVMConstantPtrAuthValueKind,
} LLVMValueKind;
LLVMValueKind LLVMGetValueKind(LLVMValueRef Val);

LLVMValueRef LLVMIsATerminatorInst(LLVMValueRef Inst);

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
unsigned LLVMPointerSize(LLVMTargetDataRef TD);
LLVMTargetDataRef LLVMGetModuleDataLayout(LLVMModuleRef M);

typedef struct LLVMOpaqueDIBuilder *LLVMDIBuilderRef;
LLVMDIBuilderRef LLVMCreateDIBuilder(LLVMModuleRef M);
void LLVMDisposeDIBuilder(LLVMDIBuilderRef Builder);
typedef struct LLVMOpaqueMetadata *LLVMMetadataRef;
typedef enum {
  LLVMDIFlagZero = 0,
  LLVMDIFlagPrivate = 1,
  LLVMDIFlagProtected = 2,
  LLVMDIFlagPublic = 3,
  LLVMDIFlagFwdDecl = 1 << 2,
  LLVMDIFlagAppleBlock = 1 << 3,
  LLVMDIFlagReservedBit4 = 1 << 4,
  LLVMDIFlagVirtual = 1 << 5,
  LLVMDIFlagArtificial = 1 << 6,
  LLVMDIFlagExplicit = 1 << 7,
  LLVMDIFlagPrototyped = 1 << 8,
  LLVMDIFlagObjcClassComplete = 1 << 9,
  LLVMDIFlagObjectPointer = 1 << 10,
  LLVMDIFlagVector = 1 << 11,
  LLVMDIFlagStaticMember = 1 << 12,
  LLVMDIFlagLValueReference = 1 << 13,
  LLVMDIFlagRValueReference = 1 << 14,
  LLVMDIFlagReserved = 1 << 15,
  LLVMDIFlagSingleInheritance = 1 << 16,
  LLVMDIFlagMultipleInheritance = 2 << 16,
  LLVMDIFlagVirtualInheritance = 3 << 16,
  LLVMDIFlagIntroducedVirtual = 1 << 18,
  LLVMDIFlagBitField = 1 << 19,
  LLVMDIFlagNoReturn = 1 << 20,
  LLVMDIFlagTypePassByValue = 1 << 22,
  LLVMDIFlagTypePassByReference = 1 << 23,
  LLVMDIFlagEnumClass = 1 << 24,
  LLVMDIFlagFixedEnum = LLVMDIFlagEnumClass,  // Deprecated.
  LLVMDIFlagThunk = 1 << 25,
  LLVMDIFlagNonTrivial = 1 << 26,
  LLVMDIFlagBigEndian = 1 << 27,
  LLVMDIFlagLittleEndian = 1 << 28,
  LLVMDIFlagIndirectVirtualBase = (1 << 2) | (1 << 5),
  LLVMDIFlagAccessibility =
      LLVMDIFlagPrivate | LLVMDIFlagProtected | LLVMDIFlagPublic,
  LLVMDIFlagPtrToMemberRep = LLVMDIFlagSingleInheritance |
                             LLVMDIFlagMultipleInheritance |
                             LLVMDIFlagVirtualInheritance
} LLVMDIFlags;
typedef enum {
  LLVMDWARFSourceLanguageC89,
  LLVMDWARFSourceLanguageC,
  LLVMDWARFSourceLanguageAda83,
  LLVMDWARFSourceLanguageC_plus_plus,
  LLVMDWARFSourceLanguageCobol74,
  LLVMDWARFSourceLanguageCobol85,
  LLVMDWARFSourceLanguageFortran77,
  LLVMDWARFSourceLanguageFortran90,
  LLVMDWARFSourceLanguagePascal83,
  LLVMDWARFSourceLanguageModula2,
  // New in DWARF v3:
  LLVMDWARFSourceLanguageJava,
  LLVMDWARFSourceLanguageC99,
  LLVMDWARFSourceLanguageAda95,
  LLVMDWARFSourceLanguageFortran95,
  LLVMDWARFSourceLanguagePLI,
  LLVMDWARFSourceLanguageObjC,
  LLVMDWARFSourceLanguageObjC_plus_plus,
  LLVMDWARFSourceLanguageUPC,
  LLVMDWARFSourceLanguageD,
  // New in DWARF v4:
  LLVMDWARFSourceLanguagePython,
  // New in DWARF v5:
  LLVMDWARFSourceLanguageOpenCL,
  LLVMDWARFSourceLanguageGo,
  LLVMDWARFSourceLanguageModula3,
  LLVMDWARFSourceLanguageHaskell,
  LLVMDWARFSourceLanguageC_plus_plus_03,
  LLVMDWARFSourceLanguageC_plus_plus_11,
  LLVMDWARFSourceLanguageOCaml,
  LLVMDWARFSourceLanguageRust,
  LLVMDWARFSourceLanguageC11,
  LLVMDWARFSourceLanguageSwift,
  LLVMDWARFSourceLanguageJulia,
  LLVMDWARFSourceLanguageDylan,
  LLVMDWARFSourceLanguageC_plus_plus_14,
  LLVMDWARFSourceLanguageFortran03,
  LLVMDWARFSourceLanguageFortran08,
  LLVMDWARFSourceLanguageRenderScript,
  LLVMDWARFSourceLanguageBLISS,
  LLVMDWARFSourceLanguageKotlin,
  LLVMDWARFSourceLanguageZig,
  LLVMDWARFSourceLanguageCrystal,
  LLVMDWARFSourceLanguageC_plus_plus_17,
  LLVMDWARFSourceLanguageC_plus_plus_20,
  LLVMDWARFSourceLanguageC17,
  LLVMDWARFSourceLanguageFortran18,
  LLVMDWARFSourceLanguageAda2005,
  LLVMDWARFSourceLanguageAda2012,
  LLVMDWARFSourceLanguageHIP,
  LLVMDWARFSourceLanguageAssembly,
  LLVMDWARFSourceLanguageC_sharp,
  LLVMDWARFSourceLanguageMojo,
  LLVMDWARFSourceLanguageGLSL,
  LLVMDWARFSourceLanguageGLSL_ES,
  LLVMDWARFSourceLanguageHLSL,
  LLVMDWARFSourceLanguageOpenCL_CPP,
  LLVMDWARFSourceLanguageCPP_for_OpenCL,
  LLVMDWARFSourceLanguageSYCL,
  LLVMDWARFSourceLanguageRuby,
  LLVMDWARFSourceLanguageMove,
  LLVMDWARFSourceLanguageHylo,
  LLVMDWARFSourceLanguageMetal,

  // Vendor extensions:
  LLVMDWARFSourceLanguageMips_Assembler,
  LLVMDWARFSourceLanguageGOOGLE_RenderScript,
  LLVMDWARFSourceLanguageBORLAND_Delphi
} LLVMDWARFSourceLanguage;
typedef enum {
  LLVMDWARFEmissionNone = 0,
  LLVMDWARFEmissionFull,
  LLVMDWARFEmissionLineTablesOnly
} LLVMDWARFEmissionKind;
LLVMMetadataRef LLVMDIBuilderCreateFunction(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    size_t NameLen, const char *LinkageName, size_t LinkageNameLen,
    LLVMMetadataRef File, unsigned LineNo, LLVMMetadataRef Ty,
    LLVMBool IsLocalToUnit, LLVMBool IsDefinition, unsigned ScopeLine,
    LLVMDIFlags Flags, LLVMBool IsOptimized);
LLVMMetadataRef LLVMDIBuilderCreateCompileUnit(
    LLVMDIBuilderRef Builder, LLVMDWARFSourceLanguage Lang,
    LLVMMetadataRef FileRef, const char *Producer, size_t ProducerLen,
    LLVMBool isOptimized, const char *Flags, size_t FlagsLen,
    unsigned RuntimeVer, const char *SplitName, size_t SplitNameLen,
    LLVMDWARFEmissionKind Kind, unsigned DWOId, LLVMBool SplitDebugInlining,
    LLVMBool DebugInfoForProfiling, const char *SysRoot, size_t SysRootLen,
    const char *SDK, size_t SDKLen);
LLVMMetadataRef LLVMDIBuilderCreateFile(LLVMDIBuilderRef Builder,
                                        const char *Filename,
                                        size_t FilenameLen,
                                        const char *Directory,
                                        size_t DirectoryLen);
LLVMMetadataRef LLVMDIScopeGetFile(LLVMMetadataRef Scope);

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
  struct TreeMapNode *left;
  struct TreeMapNode *right;
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
  map->key_ctor = ctor ? ctor : &key_ctor_default;
  map->key_dtor = dtor ? dtor : &key_dtor_default;
}

void tree_map_destroy(TreeMap *map) {
  if (map->left) {
    tree_map_destroy(map->left);
    free(map->left);
    map->left = NULL;
  }

  if (map->right) {
    tree_map_destroy(map->right);
    free(map->right);
    map->right = NULL;
  }

  map->key_dtor(map->key);
  map->key = NULL;
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

void tree_map_clear(TreeMap *map) {
  tree_map_destroy(map);
  tree_map_construct(map, map->cmp, map->key_ctor, map->key_dtor);
}

void tree_map_clone(TreeMap *dst, const TreeMap *src) {
  tree_map_clear(dst);

  if (!src->key)
    return;

  dst->key = src->key_ctor(src->key);
  dst->value = src->value;

  if (src->left) {
    dst->left = malloc(sizeof(TreeMap));
    tree_map_construct(dst->left, src->cmp, src->key_ctor, src->key_dtor);
    tree_map_clone(dst->left, src->left);
  } else {
    dst->left = NULL;
  }

  if (src->right) {
    dst->right = malloc(sizeof(TreeMap));
    tree_map_construct(dst->right, src->cmp, src->key_ctor, src->key_dtor);
    tree_map_clone(dst->right, src->right);
  } else {
    dst->right = NULL;
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

static void TestTreeMapClone() {
  TreeMap m;
  string_tree_map_construct(&m);

  const char *val = "val";
  tree_map_set(&m, "key", (char *)val);

  tree_map_clear(&m);
  assert(m.key == NULL);
  assert(m.left == NULL);
  assert(m.right == NULL);

  TreeMap m2;
  string_tree_map_construct(&m2);
  tree_map_set(&m2, "key", (char *)val);
  const char *val2 = "val2";
  tree_map_set(&m2, "key2", (char *)val2);

  tree_map_clone(&m, &m2);
  assert(strcmp(m.key, "key") == 0);
  assert(strcmp(m2.key, "key") == 0);

  void *res = NULL;
  assert(tree_map_get(&m, "key", &res));
  assert(strcmp(res, val) == 0);
  assert(tree_map_get(&m2, "key", &res));
  assert(strcmp(res, val) == 0);
  assert(tree_map_get(&m, "key2", &res));
  assert(strcmp(res, val2) == 0);
  assert(tree_map_get(&m2, "key2", &res));
  assert(strcmp(res, val2) == 0);

  tree_map_destroy(&m);
  tree_map_destroy(&m2);
}

static void TestTreeMapCloneEmpty() {
  TreeMap m;
  TreeMap m2;
  string_tree_map_construct(&m);
  string_tree_map_construct(&m2);

  tree_map_clone(&m, &m2);
  assert(m.key == NULL);
  assert(m2.key == NULL);

  tree_map_destroy(&m);
  tree_map_destroy(&m2);
}

static void RunTreeMapTests() {
  TestTreeMapConstruction();
  TestTreeMapInsertion();
  TestTreeMapOverrideKeyValue();
  TestTreeMapClone();
  TestTreeMapCloneEmpty();
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

void string_append_range(string *s, const char *s2, size_t len) {
  string_reserve(s, s->size + len + 1);  // +1 for null terminator
  memcpy(s->data + s->size, s2, len);
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

  string_destroy(&s);
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
  bool (*eof)(struct InputStream *);
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
bool input_stream_eof(InputStream *is) { return is->vtable->eof(is); }

void file_input_stream_destroy(InputStream *);
int file_input_stream_read(InputStream *);
bool file_input_stream_eof(InputStream *);

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

bool file_input_stream_eof(InputStream *is) {
  FileInputStream *fis = (FileInputStream *)is;
  return (bool)feof(fis->input_file);
}

void string_input_stream_destroy(InputStream *);
int string_input_stream_read(InputStream *);
bool string_input_stream_eof(InputStream *);

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
  return (int)sis->string[sis->pos_++];
}

bool string_input_stream_eof(InputStream *is) {
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
  TK_AlignOf,
  TK_If,
  TK_Else,
  TK_While,
  TK_For,
  TK_Switch,
  TK_Break,
  TK_Continue,
  TK_Case,
  TK_Default,
  TK_True,  // TODO: I believe these are macros instead of actual keywords.
  TK_False,
  TK_Attribute,

  TK_Identifier,

  // Literals
  TK_IntLiteral,
  TK_StringLiteral,
  TK_CharLiteral,

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
  size_t line;
  size_t col;
} Token;

typedef struct {
  InputStream *input;

  // 0 indicates no lookahead. Non-zero means we have a lookahead.
  int lookahead;

  // Lexer users should never access these directly. Instead refer to the line
  // and col returned in the Token.
  size_t line_;
  size_t col_;
} Lexer;

static bool is_eof(int c) { return c < 0; }

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
  for (;;) {
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

char handle_escape_char(int c) {
  switch (c) {
    case 'n':
      return '\n';
    case 't':
      return '\t';
    case '\'':
      return '\'';
    case '\\':
      return '\\';
    default:
      printf("Unhandled escape character '%c'\n", c);
      __builtin_trap();
  }
}

// Callers of this function should destroy the token.
Token lex(Lexer *lexer) {
  Token tok;
  token_construct(&tok);
  tok.kind = TK_Eof;  // Default value

  int c;
  for (;;) {
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
        for (; lexer_get_char(lexer) != '\n';);
        string_clear(&tok.chars);
        continue;
      }

      if (lexer_peek_char(lexer) == '*') {
        // This is a comment. Consume all characters until the final `*/`.
        for (;;) {
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
        printf("%zu:%zu: Expected 3 '.' for elipses but found 2.", tok.line,
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

  if (c == '|') {
    // Disambiguate between bitwise-or and logical-or.
    if (lexer_peek_then_consume_char(lexer, '|')) {
      string_append_char(&tok.chars, '|');
      tok.kind = TK_LogicalOr;
    } else if (lexer_peek_then_consume_char(lexer, '=')) {
      string_append_char(&tok.chars, '=');
      tok.kind = TK_OrAssign;
    } else {
      tok.kind = TK_Or;
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
    } else if (lexer_peek_then_consume_char(lexer, '<')) {
      string_append_char(&tok.chars, '<');
      if (lexer_peek_then_consume_char(lexer, '=')) {
        string_append_char(&tok.chars, '=');
        tok.kind = TK_LShiftAssign;
      } else {
        tok.kind = TK_LShift;
      }
    } else {
      tok.kind = TK_Lt;
    }
    return tok;
  }

  if (c == '>') {
    if (lexer_peek_then_consume_char(lexer, '=')) {
      string_append_char(&tok.chars, '=');
      tok.kind = TK_Ge;
    } else if (lexer_peek_then_consume_char(lexer, '>')) {
      string_append_char(&tok.chars, '>');
      if (lexer_peek_then_consume_char(lexer, '=')) {
        string_append_char(&tok.chars, '=');
        tok.kind = TK_RShiftAssign;
      } else {
        tok.kind = TK_RShift;
      }
    } else {
      tok.kind = TK_Gt;
    }
    return tok;
  }

  if (c == '"') {
    // TODO: Handle escape chars.
    for (; !lexer_peek_then_consume_char(lexer, '"');) {
      int c = lexer_get_char(lexer);
      if (is_eof(c)) {
        printf("Got EOF before finishing string parsing\n");
        __builtin_trap();
      }

      if (c == '\\')
        c = handle_escape_char(lexer_get_char(lexer));

      string_append_char(&tok.chars, (char)c);
    }

    string_append_char(&tok.chars, '"');
    tok.kind = TK_StringLiteral;
    return tok;
  }

  if (c == '\'') {
    tok.kind = TK_CharLiteral;
    int c = lexer_get_char(lexer);
    assert(!is_eof(c));

    if (c == '\\')
      c = handle_escape_char(lexer_get_char(lexer));

    string_append_char(&tok.chars, (char)c);

    if (!lexer_peek_then_consume_char(lexer, '\'')) {
      printf("%zu:%zu: Expected `'` for ending char.\n", tok.line, tok.col);
      __builtin_trap();
    }

    string_append_char(&tok.chars, '\'');
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

    for (; isdigit(lexer_peek_char(lexer));)
      string_append_char(&tok.chars, (char)lexer_get_char(lexer));

    return tok;
  }

  // Keywords
  for (; is_kw_char(lexer_peek_char(lexer));) {
    string_append_char(&tok.chars, (char)lexer_get_char(lexer));
  }

  if (string_equals(&tok.chars, "char")) {
    tok.kind = TK_Char;
  } else if (string_equals(&tok.chars, "bool")) {
    tok.kind = TK_Bool;
  } else if (string_equals(&tok.chars, "short")) {
    tok.kind = TK_Short;
  } else if (string_equals(&tok.chars, "int")) {
    tok.kind = TK_Int;
  } else if (string_equals(&tok.chars, "unsigned")) {
    tok.kind = TK_Unsigned;
  } else if (string_equals(&tok.chars, "signed")) {
    tok.kind = TK_Signed;
  } else if (string_equals(&tok.chars, "long")) {
    tok.kind = TK_Long;
  } else if (string_equals(&tok.chars, "float")) {
    tok.kind = TK_Float;
  } else if (string_equals(&tok.chars, "double")) {
    tok.kind = TK_Double;
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
  } else if (string_equals(&tok.chars, "__attribute__")) {
    tok.kind = TK_Attribute;
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
  } else if (string_equals(&tok.chars, "alignof")) {
    tok.kind = TK_AlignOf;
  } else if (string_equals(&tok.chars, "if")) {
    tok.kind = TK_If;
  } else if (string_equals(&tok.chars, "else")) {
    tok.kind = TK_Else;
  } else if (string_equals(&tok.chars, "while")) {
    tok.kind = TK_While;
  } else if (string_equals(&tok.chars, "for")) {
    tok.kind = TK_For;
  } else if (string_equals(&tok.chars, "switch")) {
    tok.kind = TK_Switch;
  } else if (string_equals(&tok.chars, "break")) {
    tok.kind = TK_Break;
  } else if (string_equals(&tok.chars, "continue")) {
    tok.kind = TK_Continue;
  } else if (string_equals(&tok.chars, "case")) {
    tok.kind = TK_Case;
  } else if (string_equals(&tok.chars, "default")) {
    tok.kind = TK_Default;
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
  } else if (string_equals(&tok.chars, "true")) {
    tok.kind = TK_True;
  } else if (string_equals(&tok.chars, "false")) {
    tok.kind = TK_False;
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
typedef void (*TypeDump)(const struct Type *);

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

void type_dump(const Type *type) {
  if (!type->vtable->dump)
    printf("TODO: Implement dump for type %d\n", type->vtable->kind);
  type->vtable->dump(type);
}

void type_set_const(Type *type) { type->qualifiers |= kConstMask; }
void type_set_volatile(Type *type) { type->qualifiers |= kVolatileMask; }
void type_set_restrict(Type *type) { type->qualifiers |= kRestrictMask; }

bool type_is_const(Type *type) { return (bool)(type->qualifiers & kConstMask); }
bool type_is_volatile(Type *type) {
  return (bool)(type->qualifiers & kVolatileMask);
}
bool type_is_restrict(Type *type) {
  return (bool)(type->qualifiers & kRestrictMask);
}

typedef struct {
  Type type;
} ReplacementSentinelType;

void replacement_sentinel_type_destroy(Type *) {}

const TypeVtable ReplacementSentinelTypeVtable = {
    .kind = TK_ReplacementSentinelType,
    .dtor = &replacement_sentinel_type_destroy,
    .dump = NULL,
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
void builtin_type_dump(const Type *);

const TypeVtable BuiltinTypeVtable = {
    .kind = TK_BuiltinType,
    .dtor = builtin_type_destroy,
    .dump = builtin_type_dump,
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

void builtin_type_dump(const Type *type) {
  const BuiltinType *bt = (const BuiltinType *)type;
  printf("BuiltinType kind=%d\n", bt->kind);
}

bool is_builtin_type(const Type *type, BuiltinTypeKind kind) {
  if (type->vtable->kind != TK_BuiltinType)
    return false;
  return ((const BuiltinType *)type)->kind == kind;
}

bool is_pointer_type(const Type *type) {
  return type->vtable->kind == TK_PointerType;
}

bool is_array_type(const Type *type) {
  return type->vtable->kind == TK_ArrayType;
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
void struct_type_dump(const Type *);

const TypeVtable StructTypeVtable = {
    .kind = TK_StructType,
    .dtor = struct_type_destroy,
    .dump = struct_type_dump,
};

struct Expr;

typedef struct {
  Type *type;
  char *name;  // Optional. Allocated by the creator of this StructMember.
  struct Expr *bitfield;  // Optional.
} StructMember;

void expr_destroy(struct Expr *);

void struct_member_destroy(StructMember *member) {
  if (member->name)
    free(member->name);

  type_destroy(member->type);
  free(member->type);

  if (member->bitfield) {
    expr_destroy(member->bitfield);
    free(member->bitfield);
  }
}

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

  // Empty structs are not allowed in C.
  if (members)
    assert(members->size > 0);

  st->packed = false;
}

void struct_type_destroy(Type *type) {
  StructType *st = (StructType *)type;

  if (st->name)
    free(st->name);

  if (st->members) {
    for (size_t i = 0; i < st->members->size; ++i) {
      StructMember *sm = vector_at(st->members, i);
      struct_member_destroy(sm);
    }
    vector_destroy(st->members);
    free(st->members);
  }
}

void struct_type_dump(const Type *type) {
  const StructType *st = (const StructType *)type;
  printf("<StructType name=%s packed=%d>\n", st->name, st->packed);
  if (st->members) {
    for (size_t i = 0; i < st->members->size; ++i) {
      StructMember *sm = vector_at(st->members, i);
      printf("StructMember name=%s type=\n", sm->name);
      type_dump(sm->type);
    }
  }
  printf("</StructType>\n");
}

const StructMember *struct_get_member(const StructType *st, const char *name,
                                      size_t *offset) {
  if (!st->members) {
    printf("No members in struct '%s'\n", st->name);
    __builtin_trap();
  }

  for (size_t i = 0; i < st->members->size; ++i) {
    const StructMember *member = vector_at(st->members, i);
    if (strcmp(member->name, name) == 0) {
      if (offset)
        *offset = i;
      return member;
    }
  }

  printf("No member named '%s'\n", name);
  __builtin_trap();
}

void enum_type_destroy(Type *);
void enum_type_dump(const Type *);

const TypeVtable EnumTypeVtable = {
    .kind = TK_EnumType,
    .dtor = enum_type_destroy,
    .dump = enum_type_dump,
};

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
      EnumMember *em = vector_at(et->members, i);

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

void enum_type_dump(const Type *type) {
  const EnumType *et = (const EnumType *)type;
  printf("<EnumType name=%s>\n", et->name);
}

void function_type_destroy(Type *);
void function_type_dump(const Type *);

const TypeVtable FunctionTypeVtable = {
    .kind = TK_FunctionType,
    .dtor = function_type_destroy,
    .dump = function_type_dump,
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

void function_type_dump(const Type *type) {
  const FunctionType *ft = (const FunctionType *)type;
  printf("<FunctionType has_var_args=%d>\n", ft->has_var_args);
  printf("return_type ");
  type_dump(ft->return_type);
  for (size_t i = 0; i < ft->pos_args.size; ++i) {
    FunctionArg *a = vector_at(&ft->pos_args, i);
    printf("pos_arg i=%zu name=%s ", i, a->name);
    type_dump(a->type);
  }
  printf("</FunctionType>\n");
}

Type *get_arg_type(const FunctionType *func, size_t i) {
  FunctionArg *a = vector_at(&func->pos_args, i);
  return a->type;
}

void array_type_destroy(Type *);

const TypeVtable ArrayTypeVtable = {
    .kind = TK_ArrayType,
    .dtor = array_type_destroy,
    .dump = NULL,
};

typedef struct {
  Type type;
  Type *elem_type;
  struct Expr *size;  // NULL indicates no specified size.
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
void pointer_type_dump(const Type *);

const TypeVtable PointerTypeVtable = {
    .kind = TK_PointerType,
    .dtor = pointer_type_destroy,
    .dump = pointer_type_dump,
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

void pointer_type_dump(const Type *type) {
  const PointerType *pt = (const PointerType *)type;
  printf("<PointerType>\n");
  type_dump(pt->pointee);
  printf("</PointerType>\n");
}

PointerType *create_pointer_to(Type *type) {
  PointerType *ptr = malloc(sizeof(PointerType));
  pointer_type_construct(ptr, type);
  return ptr;
}

void non_owning_pointer_type_destroy(Type *) {}
void non_owning_pointer_type_dump(const Type *);

const TypeVtable NonOwningPointerTypeVtable = {
    .kind = TK_PointerType,
    .dtor = non_owning_pointer_type_destroy,
    .dump = non_owning_pointer_type_dump,
};

// This is just like a PointerType with the only difference being it doesn't
// "own" the underlying pointee type. That is, it will not destroy and free
// the underlying type on this type's destruction. This should only be used by
// Sema which needs to lazily create pointer types via AddressOf.
//
// This is meant to act as a POD, so users creating this object do not need to
// explicitly destroy it.
typedef struct {
  Type type;
  const Type *pointee;
} NonOwningPointerType;

void non_owning_pointer_type_construct(NonOwningPointerType *ptr,
                                       const Type *pointee) {
  type_construct(&ptr->type, &NonOwningPointerTypeVtable);
  ptr->pointee = pointee;
}

void non_owning_pointer_type_dump(const Type *type) {
  const NonOwningPointerType *pt = (const NonOwningPointerType *)type;
  printf("<NonOwningPointerType>\n");
  type_dump(pt->pointee);
  printf("</NonOwningPointerType>\n");
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
void named_type_dump(const Type *);

const TypeVtable NamedTypeVtable = {
    .kind = TK_NamedType,
    .dtor = named_type_destroy,
    .dump = named_type_dump,
};

typedef struct {
  Type type;
  char *name;
} NamedType;

void named_type_construct(NamedType *nt, const char *name) {
  type_construct(&nt->type, &NamedTypeVtable);
  nt->name = strdup(name);
}

void named_type_destroy(Type *type) { free(((NamedType *)type)->name); }

void named_type_dump(const Type *type) {
  const NamedType *nt = (const NamedType *)type;
  printf("NamedType name=%s\n", nt->name);
}

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
    printf("%zu:%zu: Expected token %d but found %d: '%s'\n", tok->line,
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
    printf("%zu:%zu: Expected token kind %d but found %d: '%s'\n", next.line,
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
  EK_AlignOf,
  EK_UnOp,
  EK_BinOp,
  EK_Conditional,  // x ? y : z
  EK_DeclRef,
  EK_Int,
  EK_Bool,  // true/false
  EK_String,
  EK_Char,
  EK_InitializerList,
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
  size_t line;
  size_t col;
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
  NK_StructDeclaration,
  NK_EnumDeclaration,
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

  if (td->type) {
    type_destroy(td->type);
    free(td->type);
  }

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
  Expr *initializer;  // Optional.

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
  SK_Declaration,
  SK_ForStmt,
  SK_WhileStmt,
  SK_SwitchStmt,
  SK_BreakStmt,
  SK_ContinueStmt,
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

void declaration_destroy(Statement *);

const StatementVtable DeclarationVtable = {
    .kind = SK_Declaration,
    .dtor = declaration_destroy,
};

typedef struct {
  Statement base;
  char *name;
  Type *type;
  Expr *initializer;  // Optional.
} Declaration;

void declaration_construct(Declaration *decl, const char *name, Type *type,
                           Expr *init) {
  statement_construct(&decl->base, &DeclarationVtable);
  decl->name = strdup(name);
  decl->type = type;
  decl->initializer = init;
}

void declaration_destroy(Statement *stmt) {
  Declaration *decl = (Declaration *)stmt;
  free(decl->name);
  type_destroy(decl->type);
  free(decl->type);
  if (decl->initializer) {
    expr_destroy(decl->initializer);
    free(decl->initializer);
  }
}

void continue_stmt_destroy(Statement *) {}

const StatementVtable ContinueStmtVtable = {
    .kind = SK_ContinueStmt,
    .dtor = continue_stmt_destroy,
};

typedef struct {
  Statement base;
} ContinueStmt;

void continue_stmt_construct(ContinueStmt *stmt) {
  statement_construct(&stmt->base, &ContinueStmtVtable);
}

void break_stmt_destroy(Statement *) {}

const StatementVtable BreakStmtVtable = {
    .kind = SK_BreakStmt,
    .dtor = break_stmt_destroy,
};

typedef struct {
  Statement base;
} BreakStmt;

void break_stmt_construct(BreakStmt *stmt) {
  statement_construct(&stmt->base, &BreakStmtVtable);
}

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
  Statement *else_stmt;  // Optional.
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
    statement_destroy(ifstmt->else_stmt);
    free(ifstmt->else_stmt);
  }
}

void switch_stmt_destroy(Statement *);

const StatementVtable SwitchStmtVtable = {
    .kind = SK_SwitchStmt,
    .dtor = switch_stmt_destroy,
};

typedef struct {
  Expr *cond;

  // Vector of Statement pointers.
  vector stmts;
} SwitchCase;

void switch_case_destroy(SwitchCase *switch_case) {
  expr_destroy(switch_case->cond);
  free(switch_case->cond);

  for (size_t i = 0; i < switch_case->stmts.size; ++i) {
    Statement **stmt = vector_at(&switch_case->stmts, i);
    statement_destroy(*stmt);
    free(*stmt);
  }

  vector_destroy(&switch_case->stmts);
}

typedef struct {
  Statement base;
  Expr *cond;

  // Vector of SwitchCases.
  vector cases;

  // Vector of Statement pointers.
  //
  // Optional. If not provided, then this switch has no default branch.
  vector *default_stmts;
} SwitchStmt;

void switch_stmt_construct(SwitchStmt *stmt, Expr *cond, vector cases,
                           vector *default_stmts) {
  statement_construct(&stmt->base, &SwitchStmtVtable);
  stmt->cond = cond;
  stmt->cases = cases;
  stmt->default_stmts = default_stmts;
}

void switch_stmt_destroy(Statement *stmt) {
  SwitchStmt *switch_stmt = (SwitchStmt *)stmt;

  expr_destroy(switch_stmt->cond);
  free(switch_stmt->cond);

  for (size_t i = 0; i < switch_stmt->cases.size; ++i) {
    SwitchCase *switch_case = vector_at(&switch_stmt->cases, i);
    switch_case_destroy(switch_case);
  }
  vector_destroy(&switch_stmt->cases);

  if (switch_stmt->default_stmts) {
    for (size_t i = 0; i < switch_stmt->default_stmts->size; ++i) {
      Statement **stmt = vector_at(switch_stmt->default_stmts, i);
      statement_destroy(*stmt);
      free(*stmt);
    }
    vector_destroy(switch_stmt->default_stmts);
    free(switch_stmt->default_stmts);
  }
}

void for_stmt_destroy(Statement *);

const StatementVtable ForStmtVtable = {
    .kind = SK_ForStmt,
    .dtor = for_stmt_destroy,
};

struct ForStmt {
  Statement base;
  Statement *init;  // Optional. This can be either an ExprStmt or
                    // Declaration.
  Expr *cond;       // Optional.
  Expr *iter;       // Optional.
  Statement *body;  // Optional.
};
typedef struct ForStmt ForStmt;

void for_stmt_construct(ForStmt *stmt, Statement *init, Expr *cond, Expr *iter,
                        Statement *body) {
  statement_construct(&stmt->base, &ForStmtVtable);
  stmt->init = init;
  stmt->cond = cond;
  stmt->iter = iter;
  stmt->body = body;
}

void for_stmt_destroy(Statement *stmt) {
  ForStmt *for_stmt = (ForStmt *)stmt;
  if (for_stmt->init) {
    statement_destroy(for_stmt->init);
    free(for_stmt->init);
  }
  if (for_stmt->cond) {
    expr_destroy(for_stmt->cond);
    free(for_stmt->cond);
  }
  if (for_stmt->iter) {
    expr_destroy(for_stmt->iter);
    free(for_stmt->iter);
  }
  if (for_stmt->body) {
    statement_destroy(for_stmt->body);
    free(for_stmt->body);
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

void struct_declaration_destroy(Node *);

const NodeVtable StructDeclarationVtable = {
    .kind = NK_StructDeclaration,
    .dtor = struct_declaration_destroy,
};

typedef struct {
  Node node;
  StructType type;
} StructDeclaration;

void struct_declaration_construct(StructDeclaration *decl, char *name,
                                  vector *members) {
  node_construct(&decl->node, &StructDeclarationVtable);
  assert(name);  // The name for a struct declaration is mandatory.
  struct_type_construct(&decl->type, name, members);
}

void struct_declaration_destroy(Node *node) {
  StructDeclaration *decl = (StructDeclaration *)node;
  type_destroy(&decl->type.type);
}

void enum_declaration_destroy(Node*);

const NodeVtable EnumDeclarationVtable = {
    .kind = NK_EnumDeclaration,
    .dtor = enum_declaration_destroy,
};

typedef struct {
  Node node;
  EnumType type;
} EnumDeclaration;

void enum_declaration_construct(EnumDeclaration* decl, char* name,
                                vector* members) {
  node_construct(&decl->node, &EnumDeclarationVtable);
  enum_type_construct(&decl->type, name, members);
}

void enum_declaration_destroy(Node* node) {
  EnumDeclaration* decl = (EnumDeclaration*)node;
  type_destroy(&decl->type.type);
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
  for (;;) {
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
  for (; next_token_is(parser, TK_Star);) {
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

// TODO: Don't handle attributes for now. Just consume them.
void parser_consume_attribute(Parser* parser) {
  parser_consume_token(parser, TK_Attribute);
  parser_consume_token(parser, TK_LPar);
  parser_consume_token(parser, TK_LPar);

  // This is a cheeky way of just consuming the whole attribute.
  // Consume tokens until we match the opening left parenthesis.
  int par_count = 2;
  for (; par_count;) {
    Token tok = parser_pop_token(parser);

    if (tok.kind == TK_RPar) {
      par_count--;
    } else if (tok.kind == TK_LPar) {
      par_count++;
    }

    token_destroy(&tok);
  }
}

void parse_struct_name_and_members(Parser *parser, char **name,
                                   vector **members) {
  assert(name);
  assert(members);

  parser_consume_token(parser, TK_Struct);

  const Token *peek = parser_peek_token(parser);
  if (peek->kind == TK_Identifier) {
    *name = strdup(peek->chars.data);
    parser_consume_token(parser, TK_Identifier);
  } else {
    *name = NULL;
  }

  if (!next_token_is(parser, TK_LCurlyBrace))
    return;

  parser_consume_token(parser, TK_LCurlyBrace);

  *members = malloc(sizeof(vector));
  vector_construct(*members, sizeof(StructMember), alignof(StructMember));
  for (; !next_token_is(parser, TK_RCurlyBrace);) {
    char *member_name = NULL;
    Type *member_ty =
        parse_type_for_declaration(parser, &member_name, /*storage=*/NULL);
    assert(member_name);

    Expr *bitfield = NULL;
    if (next_token_is(parser, TK_Colon)) {
      parser_consume_token(parser, TK_Colon);
      bitfield = parse_expr(parser);
    }

    StructMember *member = vector_append_storage(*members);
    member->type = member_ty;
    member->name = member_name;
    member->bitfield = bitfield;

    if (parser_peek_token(parser)->kind == TK_Attribute) {
      parser_consume_attribute(parser);
    }

    parser_consume_token(parser, TK_Semicolon);
  }

  parser_consume_token(parser, TK_RCurlyBrace);
}

StructType *parse_struct_type(Parser *parser) {
  char *name = NULL;
  vector *members = NULL;
  parse_struct_name_and_members(parser, &name, &members);

  StructType *struct_ty = malloc(sizeof(StructType));
  struct_type_construct(struct_ty, name, members);
  return struct_ty;
}

void parse_enum_name_and_members(Parser* parser, char** name,
                                 vector** members) {
  assert(name);
  assert(members);

  parser_consume_token(parser, TK_Enum);

  const Token* peek = parser_peek_token(parser);
  if (peek->kind == TK_Identifier) {
    *name = strdup(peek->chars.data);
    parser_consume_token(parser, TK_Identifier);
  } else {
    *name = NULL;
  }

  if (!next_token_is(parser, TK_LCurlyBrace))
    return;

  parser_consume_token(parser, TK_LCurlyBrace);

  *members = malloc(sizeof(vector));
  vector_construct(*members, sizeof(EnumMember), alignof(EnumMember));
  for (; !next_token_is(parser, TK_RCurlyBrace);) {
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

    EnumMember* member = vector_append_storage(*members);
    member->name = member_name;
    member->value = member_val;

    parser_consume_token_if_matches(parser, TK_Comma);
  }

  parser_consume_token(parser, TK_RCurlyBrace);
}

EnumType* parse_enum_type(Parser* parser) {
  char* name = NULL;
  vector* members = NULL;
  parse_enum_name_and_members(parser, &name, &members);

  EnumType* enum_ty = malloc(sizeof(EnumType));
  enum_type_construct(enum_ty, name, members);
  return enum_ty;
}

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

Type *parse_specifiers_and_qualifiers_and_storage(
    Parser *parser, FoundStorageClasses *storage) {
  Qualifiers quals = 0;

  struct TypeSpecifier spec = {};

  StructType *struct_ty;
  EnumType *enum_ty;
  bool should_stop = false;
  char *name;
  for (; !should_stop;) {
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
      case TK_Signed:
        spec.unsigned_ = 0;
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
  } else if (spec.double_) {
    kind = BTK_Double;
  } else if (spec.bool_) {
    kind = BTK_Bool;
  } else if (spec.void_) {
    kind = BTK_Void;
  } else {
    const Token *tok = parser_peek_token(parser);
    printf(
        "%zu:%zu: parse_specifiers_and_qualifiers: Unhandled token (%d): "
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
  for (; !next_token_is(parser, TK_RPar);) {
    // Handle varags.
    if (next_token_is(parser, TK_Ellipsis)) {
      parser_consume_token(parser, TK_Ellipsis);
      has_var_args = true;

      if (!next_token_is(parser, TK_RPar)) {
        const Token *peek = parser_peek_token(parser);
        printf(
            "%zu:%zu: parse_function_suffix: '...' must be the last in the "
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

  // https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html
  //
  // An attribute specifier list may appear immediately before the comma, ‘=’,
  // or semicolon terminating the declaration of an identifier other than a
  // function definition. Such attribute specifiers apply to the declared object
  // or function. Where an assembler name for an object or function is specified
  // (see Controlling Names Used in Assembler Code), the attribute must follow
  // the asm specification.
  bool found_attr = parser_peek_token(parser)->kind == TK_Attribute;

  for (; parser_peek_token(parser)->kind == TK_Attribute;)
    parser_consume_attribute(parser);

  if (found_attr)
    assert(parser_peek_token(parser)->kind == TK_Semicolon);

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
//
// `name` is an optional pointer to a char*. If provided, and a name is found,
// the string address will be stored in `name. Callers of this are in charge of
// freeing the string.
//
// `storage` is an optional parameter to indicate storage classes found.
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

void alignof_destroy(Expr *expr);

const ExprVtable AlignOfVtable = {
    .kind = EK_AlignOf,
    .dtor = alignof_destroy,
};

typedef struct {
  Expr expr;
  void *expr_or_type;
  bool is_expr;
} AlignOf;

void alignof_construct(AlignOf *ao, void *expr_or_type, bool is_expr) {
  expr_construct(&ao->expr, &AlignOfVtable);
  ao->expr_or_type = expr_or_type;
  ao->is_expr = is_expr;
}

void alignof_destroy(Expr *expr) {
  AlignOf *ao = (AlignOf *)expr;
  if (ao->is_expr) {
    expr_destroy(ao->expr_or_type);
  } else {
    type_destroy(ao->expr_or_type);
  }
  free(ao->expr_or_type);
}

//
// <sizeof> = "sizeof" "(" (<expr> | <type>) ")"
//
Expr *parse_sizeof(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

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
  so->expr.line = line;
  so->expr.col = col;
  return &so->expr;
}

//
// <alignof> = "alignof" "(" (<expr> | <type>) ")"
//
Expr *parse_alignof(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  parser_consume_token(parser, TK_AlignOf);
  parser_consume_token(parser, TK_LPar);

  AlignOf *ao = malloc(sizeof(AlignOf));

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

  alignof_construct(ao, expr_or_type, is_expr);
  ao->expr.line = line;
  ao->expr.col = col;
  return &ao->expr;
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

bool can_be_used_for_ptr_arithmetic(BinOpKind kind) {
  switch (kind) {
    case BOK_Add:
    case BOK_Sub:
      return true;  // TODO: Add more cases here.
    default:
      return false;
  }
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

  assert(lhs->line != 0 && lhs->col != 0);
  assert(rhs->line != 0 && rhs->col != 0);
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

void bool_destroy(Expr *) {}

const ExprVtable BoolVtable = {
    .kind = EK_Bool,
    .dtor = bool_destroy,
};

typedef struct {
  Expr expr;
  bool val;
} Bool;

void bool_construct(Bool *b, bool val) {
  expr_construct(&b->expr, &BoolVtable);
  b->val = val;
}

void char_destroy(Expr *) {}

const ExprVtable CharVtable = {
    .kind = EK_Char,
    .dtor = char_destroy,
};

typedef struct {
  Expr expr;
  char val;
} Char;

void char_construct(Char *c, char val) {
  expr_construct(&c->expr, &CharVtable);
  c->val = val;
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

void initializer_list_destroy(Expr *expr);

const ExprVtable InitializerListVtable = {
    .kind = EK_InitializerList,
    .dtor = initializer_list_destroy,
};

typedef struct {
  char *name;  // Optional.
  Expr *expr;
} InitializerListElem;

typedef struct {
  Expr expr;

  // This is a vector of InitializerListElems.
  vector elems;
} InitializerList;

void initializer_list_construct(InitializerList *init, vector elems) {
  expr_construct(&init->expr, &InitializerListVtable);
  init->elems = elems;
}

void initializer_list_destroy(Expr *expr) {
  InitializerList *init = (InitializerList *)expr;
  for (size_t i = 0; i < init->elems.size; ++i) {
    InitializerListElem *elem = vector_at(&init->elems, i);
    if (elem->name)
      free(elem->name);
    expr_destroy(elem->expr);
    free(elem->expr);
  }
  vector_destroy(&init->elems);
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

    string str;
    string_construct(&str);
    string_append_range(&str, tok->chars.data + 1, size - 2);
    parser_consume_token(parser, TK_StringLiteral);

    for (; next_token_is(parser, TK_StringLiteral);) {
      // Merge all adjacent string literals.
      const Token *tok = parser_peek_token(parser);
      assert(tok->chars.size >= 2 &&
             "String literals from the lexer should have the start and end "
             "double quotes");
      string_append_range(&str, tok->chars.data + 1, tok->chars.size - 2);
      parser_consume_token(parser, TK_StringLiteral);
    }

    StringLiteral *s = malloc(sizeof(StringLiteral));
    string_literal_construct(s, str.data, str.size);

    string_destroy(&str);

    return &s->expr;
  }

  if (tok->kind == TK_True || tok->kind == TK_False) {
    Bool *b = malloc(sizeof(Bool));
    bool_construct(b, tok->kind == TK_True);
    parser_skip_next_token(parser);
    return &b->expr;
  }

  if (tok->kind == TK_CharLiteral) {
    size_t size = tok->chars.size;
    assert(size == 3 && tok->chars.data[0] == '\'' &&
           tok->chars.data[2] == '\'' &&
           "Char literals from the lexer should have the start and end "
           "single quotes");

    Char *c = malloc(sizeof(Char));
    char_construct(c, tok->chars.data[1]);
    parser_consume_token(parser, TK_CharLiteral);
    return &c->expr;
  }

  vector elems;
  vector_construct(&elems, sizeof(InitializerListElem),
                   alignof(InitializerListElem));

  if (tok->kind == TK_LCurlyBrace) {
    parser_consume_token(parser, TK_LCurlyBrace);

    for (; !next_token_is(parser, TK_RCurlyBrace);) {
      char *name = NULL;

      if (next_token_is(parser, TK_Dot)) {
        // Designated initializer.
        parser_consume_token(parser, TK_Dot);

        const Token *next = parser_peek_token(parser);
        name = strdup(next->chars.data);

        parser_consume_token(parser, TK_Identifier);
        parser_consume_token(parser, TK_Assign);
      }

      // TODO: Should this be just `parse_expr`? Using `parse_expr` may consume
      // the following comma in a comma expression.
      Expr *expr = parse_assignment_expr(parser);
      InitializerListElem *elem = vector_append_storage(&elems);
      elem->name = name;
      elem->expr = expr;

      // Consume optional `,`.
      if (next_token_is(parser, TK_Comma))
        parser_consume_token(parser, TK_Comma);
      else
        break;
    }
    parser_consume_token(parser, TK_RCurlyBrace);

    InitializerList *init = malloc(sizeof(InitializerList));
    initializer_list_construct(init, elems);
    return &init->expr;
  }

  printf("%zu:%zu: parse_primary_expr: Unhandled token (%d): '%s'\n", tok->line,
         tok->col, tok->kind, tok->chars.data);
  __builtin_trap();
  return NULL;
}

void index_destroy(Expr *expr);

const ExprVtable IndexVtable = {
    .kind = EK_Index,
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
  for (;;) {
    should_continue = false;

    Expr *expr = parse_assignment_expr(parser);
    Expr **storage = vector_append_storage(&v);
    *storage = expr;

    if ((should_continue = (next_token_is(parser, TK_Comma))))
      parser_consume_token(parser, TK_Comma);

    if (!should_continue)
      break;
  }

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
Expr *parse_postfix_expr_with_primary(Parser *parser, Expr *expr) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  bool should_continue = true;
  for (; should_continue;) {
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

        parser_consume_token(parser, tok->kind);

        Token id = parser_pop_token(parser);
        assert(id.kind == TK_Identifier);

        MemberAccess *member_access = malloc(sizeof(MemberAccess));
        member_access_construct(member_access, expr, id.chars.data, is_arrow);
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
  expr->line = line;
  expr->col = col;
  return expr;
}

Expr *parse_postfix_expr(Parser *parser) {
  Expr *expr = parse_primary_expr(parser);
  return parse_postfix_expr_with_primary(parser, expr);
}

//
// <unary_epr> = <postfix_expr>
//             | ("++" | "--") <unary_expr>
//             | ("&" | "*" | "+" | "-" | "~" | "!") <cast_expr>
//             | "sizeof" "(" (<unary_expr> | <type>) ")"
//
Expr *parse_unary_expr(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

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
    case TK_AlignOf:
      return parse_alignof(parser);
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
  unop->expr.line = line;
  unop->expr.col = col;
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
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  parser_consume_token(parser, TK_LPar);

  if (!is_next_token_type_token(parser)) {
    Expr *expr = parse_parentheses_expr_tail(parser);
    return parse_postfix_expr_with_primary(parser, expr);
  }

  // Definitely a type case.
  // "(" <type> ")" <cast_expr>
  Type *type = parse_type(parser);
  parser_consume_token(parser, TK_RPar);

  Expr *base = parse_cast_expr(parser);

  Cast *cast = malloc(sizeof(Cast));
  cast_construct(cast, base, type);
  cast->expr.line = line;
  cast->expr.col = col;
  return &cast->expr;
}

//
// <cast_epr> = <unary_expr>
//            | "(" <type> ")" <cast_expr>
//
Expr *parse_cast_expr(Parser *parser) {
  const Token *tok = parser_peek_token(parser);

  if (tok->kind == TK_LPar) {
    Expr* e = parse_cast_or_paren_expr(parser);
    assert(e->line && e->col);
    return e;
  }

  Expr* e = parse_unary_expr(parser);
  assert(e->line && e->col);
  return e;
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
  if (!(expr->line && expr->col)) {
    printf("line: %zu, col: %zu\n", expr->line, expr->col);
  }
  assert(expr->line && expr->col);

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
  assert(expr->line && expr->col);

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
  assert(line && col);

  Expr *expr = parse_additive_expr(parser);
  assert(expr);
  assert(expr->line && expr->col);

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
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

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
  cond->expr.line = line;
  cond->expr.col = col;
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
  assert(expr->line && expr->col);

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

Statement *parse_expr_statement(Parser *parser) {
  Expr *expr = parse_expr(parser);
  parser_consume_token(parser, TK_Semicolon);

  ExprStmt *stmt = malloc(sizeof(ExprStmt));
  expr_stmt_construct(stmt, expr);
  return &stmt->base;
}

Statement *parse_declaration(Parser *parser) {
  FoundStorageClasses storage;
  char *name = NULL;
  Type *type = parse_type_for_declaration(parser, &name, &storage);
  assert(name);

  Expr *init = NULL;
  if (next_token_is(parser, TK_Assign)) {
    parser_consume_token(parser, TK_Assign);
    init = parse_expr(parser);
  }

  parser_consume_token(parser, TK_Semicolon);

  Declaration *decl = malloc(sizeof(Declaration));
  declaration_construct(decl, name, type, init);
  free(name);
  return &decl->base;
}

// This consumes any trailing semicolons when needed.
Statement *parse_statement(Parser *parser) {
  for (; next_token_is(parser, TK_Semicolon);)
    parser_consume_token(parser, TK_Semicolon);

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

    if (next_token_is(parser, TK_Else)) {
      parser_consume_token(parser, TK_Else);
      ifstmt->else_stmt = parse_statement(parser);
    }

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

  if (peek->kind == TK_Switch) {
    parser_consume_token(parser, TK_Switch);

    parser_consume_token(parser, TK_LPar);
    Expr *cond = parse_expr(parser);
    parser_consume_token(parser, TK_RPar);

    parser_consume_token(parser, TK_LCurlyBrace);

    // Vector of SwitchCases - same as SwitchStmt::cases.
    vector cases;
    vector_construct(&cases, sizeof(SwitchCase), alignof(SwitchCase));

    vector *default_stmts = NULL;

    for (;;) {
      if (next_token_is(parser, TK_RCurlyBrace))
        break;

      if (next_token_is(parser, TK_LSquareBrace)) {
        // Expect a C23 attribute like [[fallthrough]].
        parser_consume_token(parser, TK_LSquareBrace);
        parser_consume_token(parser, TK_LSquareBrace);

        const Token *attr = parser_peek_token(parser);
        assert(attr->kind == TK_Identifier);
        if (string_equals(&attr->chars, "fallthrough")) {
          // TODO: If this project is ever working, come back and potentially
          // warn on missing fallthroughs.
          parser_consume_token(parser, TK_Identifier);
        } else {
          printf("%zu:%zu: Unknown attribute '%s'\n", attr->line, attr->col,
                 attr->chars.data);
          __builtin_trap();
        }

        parser_consume_token(parser, TK_RSquareBrace);
        parser_consume_token(parser, TK_RSquareBrace);
        parser_consume_token(parser, TK_Semicolon);
      }

      if (next_token_is(parser, TK_Case)) {
        parser_consume_token(parser, TK_Case);

        Expr *case_expr = parse_expr(parser);

        parser_consume_token(parser, TK_Colon);

        vector case_stmts;
        vector_construct(&case_stmts, sizeof(Statement *),
                         alignof(Statement *));
        for (; !(next_token_is(parser, TK_Case) ||
                 next_token_is(parser, TK_Default) ||
                 next_token_is(parser, TK_RCurlyBrace) ||
                 next_token_is(parser, TK_LSquareBrace));) {
          Statement *stmt = parse_statement(parser);
          *(Statement **)vector_append_storage(&case_stmts) = stmt;
        }

        SwitchCase *switch_case = vector_append_storage(&cases);
        switch_case->cond = case_expr;
        switch_case->stmts = case_stmts;
      } else if (next_token_is(parser, TK_Default)) {
        parser_consume_token(parser, TK_Default);
        parser_consume_token(parser, TK_Colon);

        assert(default_stmts == NULL);
        default_stmts = malloc(sizeof(vector));
        vector_construct(default_stmts, sizeof(Statement *),
                         alignof(Statement *));
        for (; !(next_token_is(parser, TK_Case) ||
                 next_token_is(parser, TK_Default) ||
                 next_token_is(parser, TK_RCurlyBrace) ||
                 next_token_is(parser, TK_LSquareBrace));) {
          Statement *stmt = parse_statement(parser);
          *(Statement **)vector_append_storage(default_stmts) = stmt;
        }
      } else {
        const Token *peek = parser_peek_token(parser);
        printf("Neither case nor default: '%s'\n", peek->chars.data);
        __builtin_trap();
      }
    }
    parser_consume_token(parser, TK_RCurlyBrace);

    SwitchStmt *switch_stmt = malloc(sizeof(SwitchStmt));
    switch_stmt_construct(switch_stmt, cond, cases, default_stmts);
    return &switch_stmt->base;
  }

  if (peek->kind == TK_Continue) {
    parser_consume_token(parser, TK_Continue);
    parser_consume_token(parser, TK_Semicolon);

    ContinueStmt *cnt = malloc(sizeof(ContinueStmt));
    continue_stmt_construct(cnt);
    return &cnt->base;
  }

  if (peek->kind == TK_Break) {
    parser_consume_token(parser, TK_Break);
    parser_consume_token(parser, TK_Semicolon);

    BreakStmt *brk = malloc(sizeof(BreakStmt));
    break_stmt_construct(brk);
    return &brk->base;
  }

  if (peek->kind == TK_For) {
    parser_consume_token(parser, TK_For);
    parser_consume_token(parser, TK_LPar);

    Statement *init = NULL;
    Expr *cond = NULL;
    Expr *iter = NULL;
    Statement *body = NULL;

    // Parse optional initializer expr.
    if (!next_token_is(parser, TK_Semicolon)) {
      if (is_next_token_type_token(parser)) {
        init = parse_declaration(parser);
      } else {
        init = parse_expr_statement(parser);
      }
    } else {
      parser_consume_token(parser, TK_Semicolon);
    }

    // Parse optional condition expr.
    if (!next_token_is(parser, TK_Semicolon))
      cond = parse_expr(parser);
    parser_consume_token(parser, TK_Semicolon);

    // Parse optional iterating expr.
    if (!next_token_is(parser, TK_RPar))
      iter = parse_expr(parser);

    parser_consume_token(parser, TK_RPar);

    // Parse optional body.
    if (!next_token_is(parser, TK_Semicolon)) {
      body = parse_statement(parser);
    } else {
      parser_consume_token(parser, TK_Semicolon);
    }

    ForStmt *for_stmt = malloc(sizeof(ForStmt));
    for_stmt_construct(for_stmt, init, cond, iter, body);
    return &for_stmt->base;
  }

  if (is_token_type_token(parser, peek)) {
    // This is a declaration.
    return parse_declaration(parser);
  }

  // Default is an expression statement.
  return parse_expr_statement(parser);
}

Statement *parse_compound_stmt(Parser *parser) {
  parser_consume_token(parser, TK_LCurlyBrace);

  vector body;
  vector_construct(&body, sizeof(Statement *), alignof(Statement *));

  const Token *peek;
  for (; (peek = parser_peek_token(parser))->kind != TK_RCurlyBrace;) {
    Statement **storage = vector_append_storage(&body);
    *storage = parse_statement(parser);
  }

  parser_consume_token(parser, TK_RCurlyBrace);

  CompoundStmt *cmpd = malloc(sizeof(CompoundStmt));
  compound_stmt_construct(cmpd, body);
  return &cmpd->base;
}

Node *parse_global_variable_or_function_definition(Parser *parser) {
  size_t line = parser_peek_token(parser)->line;
  size_t col = parser_peek_token(parser)->col;

  char *name = NULL;
  FoundStorageClasses storage = {};
  Type *type = parse_type_for_declaration(parser, &name, &storage);
  if (!name) {
    printf("No name for global on %zu:%zu\n", line, col);
  }
  assert(name);

  if (storage.auto_) {
    const Token *tok = parser_peek_token(parser);
    printf("%zu:%zu: 'auto' can only be used at block scope\n", tok->line,
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

    free(name);

    return &func_def->node;
  }

  GlobalVariable *gv = malloc(sizeof(GlobalVariable));
  global_variable_construct(gv, name, type);

  if (storage.static_)
    gv->is_extern = false;

  if (storage.thread_local_)
    gv->is_thread_local = true;

  if (next_token_is(parser, TK_Assign)) {
    parser_consume_token(parser, TK_Assign);
    Expr *init = parse_expr(parser);
    gv->initializer = init;
  }

  free(name);

  return &gv->node;
}

Node *parse_struct_declaration(Parser *parser) {
  char *name = NULL;
  vector *members = NULL;
  parse_struct_name_and_members(parser, &name, &members);

  StructDeclaration *decl = malloc(sizeof(StructDeclaration));
  struct_declaration_construct(decl, name, members);
  return &decl->node;
}

Node* parse_enum_declaration(Parser* parser) {
  char* name = NULL;
  vector* members = NULL;
  parse_enum_name_and_members(parser, &name, &members);

  EnumDeclaration* decl = malloc(sizeof(EnumDeclaration));
  enum_declaration_construct(decl, name, members);
  return &decl->node;
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
    case TK_Struct:
      node = parse_struct_declaration(parser);
      break;
    case TK_Enum:
      node = parse_enum_declaration(parser);
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

  printf("%zu:%zu: parse_top_level_decl: Unhandled token (%d): '%s'\n",
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
  free(type);
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
  free(type);
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
  free(type);
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
  free(type);
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
  free(type);
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
  free(type);
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
  free(type);
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
  free(type);
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
  free(type);
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
  TreeMap struct_types;
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
  // the address of operator. This is a vector of NonOwningPointerTypes.
  vector address_of_storage;
} Sema;

void sema_handle_global_variable(Sema *sema, GlobalVariable *gv);

void sema_construct(Sema *sema) {
  string_tree_map_construct(&sema->typedef_types);
  string_tree_map_construct(&sema->struct_types);
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

  vector_construct(&sema->address_of_storage, sizeof(NonOwningPointerType),
                   alignof(NonOwningPointerType));
}

void sema_destroy(Sema *sema) {
  tree_map_destroy(&sema->typedef_types);
  tree_map_destroy(&sema->struct_types);
  tree_map_destroy(&sema->enum_types);
  tree_map_destroy(&sema->globals);
  tree_map_destroy(&sema->enum_values);
  tree_map_destroy(&sema->enum_names);

  // NOTE: We don't need to destroy the builtin types since they don't have
  // any members that need cleanup.

  type_destroy(&sema->str_ty.type);

  global_variable_destroy(&sema->builtin_trap.node);

  vector_destroy(&sema->address_of_storage);
}

const BuiltinType *sema_get_integral_type_for_enum(const Sema *sema,
                                                   const EnumType *) {
  // FIXME: This is ok for small types but will fail for sufficiently large
  // enough enums.
  return &sema->bt_Int;
}

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

ConstExprResult sema_eval_expr(Sema *, const Expr *);
int compare_constexpr_result_types(const ConstExprResult *,
                                   const ConstExprResult *);

uint64_t result_to_u64(const ConstExprResult *res) {
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

const StructType *sema_resolve_struct_type(const Sema *sema,
                                           const StructType *st) {
  if (st->members)
    return st;

  void *found;
  if (!tree_map_get(&sema->struct_types, st->name, &found)) {
    printf("No struct named '%s'\n", st->name);
    __builtin_trap();
  }

  return found;
}

const StructMember *sema_get_struct_member(const Sema *sema,
                                           const StructType *st,
                                           const char *name, size_t *offset) {
  return struct_get_member(sema_resolve_struct_type(sema, st), name, offset);
}

const Type *sema_resolve_maybe_struct_type(const Sema *sema, const Type *type) {
  if (type->vtable->kind == TK_StructType)
    return &sema_resolve_struct_type(sema, (const StructType *)type)->type;
  return type;
}

bool sema_is_enum_type(const Sema *sema, const Type *type,
                       const TreeMap *local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  return type->vtable->kind == TK_EnumType;
}

bool sema_is_pointer_type(const Sema *sema, const Type *type,
                          const TreeMap *local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  return is_pointer_type(type);
}

bool sema_is_array_type(const Sema *sema, const Type *type,
                        const TreeMap *local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  return is_array_type(type);
}

const ArrayType *sema_get_array_type(const Sema *sema, const Type *type,
                                     const TreeMap *local_ctx) {
  type = sema_resolve_maybe_named_type(sema, type);
  if (is_array_type(type))
    return (const ArrayType *)type;
  return NULL;
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

bool sema_types_are_compatible_impl(Sema *sema, const Type *lhs,
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

  // TODO: Should this be handled here? Also is this correct?
  // Handling comparisons of pointers and functions.
  // if (lhs->vtable->kind == TK_PointerType &&
  //    rhs->vtable->kind == TK_FunctionType) {
  //  const PointerType *ptr = (const PointerType *)lhs;
  //  return sema_types_are_compatible_impl(sema, ptr->pointee, rhs,
  //                                        ignore_quals);
  //}
  // if (rhs->vtable->kind == TK_PointerType &&
  //    lhs->vtable->kind == TK_FunctionType) {
  //  const PointerType *ptr = (const PointerType *)rhs;
  //  return sema_types_are_compatible_impl(sema, lhs, ptr->pointee,
  //                                        ignore_quals);
  //}

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
      const Type *lhs_pointee = ((PointerType *)lhs)->pointee;
      const Type *rhs_pointee = ((PointerType *)rhs)->pointee;
      if (!sema_types_are_compatible_impl(sema, lhs_pointee, rhs_pointee,
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

      // If one is declared with a tag, the other must also be declared with the
      // same tag.
      if ((lhs_struct->name && !rhs_struct->name) ||
          (!lhs_struct->name && rhs_struct->name))
        return false;

      // This should be enough since we wouldn't be able to define different
      // structs with the same name.
      if (lhs_struct->name && rhs_struct->name &&
          strcmp(lhs_struct->name, rhs_struct->name) != 0)
        return false;

      if (lhs_struct->members && rhs_struct->members) {
        if (lhs_struct->members->size != rhs_struct->members->size)
          return false;

        for (size_t i = 0; i < lhs_struct->members->size; ++i) {
          StructMember *lhs_member = vector_at(lhs_struct->members, i);
          StructMember *rhs_member = vector_at(rhs_struct->members, i);
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

      break;
    }
    case TK_EnumType: {
      // If a fixed underlying type is not specified, the enum is compatible
      // with one of: char, a signed int type, or an unsigned int type. This is
      // implementation defined.
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

bool sema_types_are_compatible(Sema *sema, const Type *lhs, const Type *rhs) {
  return sema_types_are_compatible_impl(sema, lhs, rhs, /*ignore_quals=*/false);
}

bool sema_types_are_compatible_ignore_quals(Sema *sema, const Type *lhs,
                                            const Type *rhs) {
  return sema_types_are_compatible_impl(sema, lhs, rhs, /*ignore_quals=*/true);
}

bool sema_can_implicit_cast_to(Sema *sema, const Type *from, const Type *to) {
  printf("TODO: Unhandled implicit cast\n");
  __builtin_trap();
}

static void assert_types_are_compatible(Sema *sema, const char *name,
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

  assert(!tree_map_get(&sema->enum_values, f->name, NULL));

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
  assert(!tree_map_get(&sema->enum_values, gv->name, NULL));

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

void sema_handle_struct_declaration_impl(Sema *sema, StructType *struct_ty) {
  // This is an anonymous struct.
  if (!struct_ty->name)
    return;

  // We received a struct declaration but not definition.
  if (!struct_ty->members)
    return;

  void *found;
  if (tree_map_get(&sema->struct_types, struct_ty->name, &found)) {
    StructType *found_struct = found;
    if (found_struct->members) {
      printf("Duplicate struct definition '%s'\n", struct_ty->name);
      __builtin_trap();
    }
  }

  tree_map_set(&sema->struct_types, struct_ty->name, struct_ty);
}

void sema_handle_struct_declaration(Sema *sema, StructDeclaration *decl) {
  sema_handle_struct_declaration_impl(sema, &decl->type);
}

void sema_handle_enum_declaration_impl(Sema* sema, EnumType* enum_ty) {
  if (!enum_ty->members)
    return;

  if (enum_ty->name) {
    void *found;
    if (tree_map_get(&sema->enum_types, enum_ty->name, &found)) {
      EnumType *found_enum = found;
      if (found_enum->members) {
        printf("Duplicate enum definition '%s'\n", enum_ty->name);
        __builtin_trap();
      }
    }

    tree_map_set(&sema->enum_types, enum_ty->name, enum_ty);
  }

  // Register the individual members.
  int val = 0;
  for (size_t i = 0; i < enum_ty->members->size; ++i) {
    EnumMember *member = vector_at(enum_ty->members, i);
    assert(!tree_map_get(&sema->enum_values, member->name, NULL));

    if (member->value) {
      ConstExprResult res = sema_eval_expr(sema, member->value);
      switch (res.result_kind) {
        case RK_Boolean:
          __builtin_trap();
        case RK_Int:
          val = res.result.i;
          break;
        case RK_UnsignedLongLong:
          val = (int)res.result.ull;
          break;
      }
    }

    tree_map_set(&sema->enum_values, member->name, (void *)(intptr_t)val);
    tree_map_set(&sema->enum_names, member->name, enum_ty);

    val++;
  }
}

void sema_handle_enum_declaration(Sema* sema, EnumDeclaration* decl) {
  sema_handle_enum_declaration_impl(sema, &decl->type);
}

void sema_add_typedef_type(Sema *sema, const char *name, Type *type) {
  if (tree_map_get(&sema->typedef_types, name, NULL)) {
    printf("sema_add_typedef_type: typedef for '%s' already exists\n", name);
    __builtin_trap();
  }

  if (type->vtable->kind == TK_StructType) {
    StructType *struct_ty = (StructType *)type;
    sema_handle_struct_declaration_impl(sema, struct_ty);
  } else if (type->vtable->kind == TK_EnumType) {
    EnumType *enum_ty = (EnumType *)type;
    sema_handle_enum_declaration_impl(sema, enum_ty);
  }

  switch (type->vtable->kind) {
    case TK_StructType:
    case TK_FunctionType:
    case TK_BuiltinType:
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
      tree_map_set(&sema->typedef_types, name, found);
      break;
    }
    case TK_ReplacementSentinelType:
      printf("Replacement sentinel type should not be used\n");
      __builtin_trap();
      break;
  }
}

const Type *sema_get_type_of_expr_in_ctx(Sema *sema, const Expr *expr,
                                         const TreeMap *local_ctx);

const Type *sema_get_type_of_unop_expr(Sema *sema, const UnOp *expr,
                                       const TreeMap *local_ctx) {
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
      const Type *sub_type =
          sema_get_type_of_expr_in_ctx(sema, expr->subexpr, local_ctx);
      NonOwningPointerType *ptr =
          vector_append_storage(&sema->address_of_storage);
      non_owning_pointer_type_construct(ptr, sub_type);
      return &ptr->type;
    }
    case UOK_Deref: {
      const Type *sub_type =
          sema_get_type_of_expr_in_ctx(sema, expr->subexpr, local_ctx);
      if (sema_is_pointer_type(sema, sub_type, local_ctx))
        return sema_get_pointee(sema, sub_type, local_ctx);

      if (sema_is_array_type(sema, sub_type, local_ctx))
        return sema_get_array_type(sema, sub_type, local_ctx)->elem_type;

      printf("Attempting to dereference type that is not a pointer or array\n");
      __builtin_trap();
    }
    default:
      printf("TODO: Implement get type for unop kind %d\n", expr->op);
      __builtin_trap();
  }
}

size_t sema_eval_sizeof_type(Sema *sema, const Type *type);

bool sema_is_unsigned_integral_type(const Sema *sema, const Type *type) {
  type = sema_resolve_maybe_named_type(sema, type);
  return is_unsigned_integral_type(type);
}

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
const Type *sema_get_common_arithmetic_type(Sema *sema, const Type *lhs_ty,
                                            const Type *rhs_ty,
                                            const TreeMap *local_ctx) {
  if (lhs_ty->vtable->kind == TK_NamedType)
    lhs_ty = sema_resolve_named_type(sema, (const NamedType *)lhs_ty);
  if (rhs_ty->vtable->kind == TK_NamedType)
    rhs_ty = sema_resolve_named_type(sema, (const NamedType *)rhs_ty);

  // If the types are the same, that type is the common type.
  if (sema_types_are_compatible_ignore_quals(sema, lhs_ty, rhs_ty))
    return lhs_ty;

  // TODO: Handle non-integral types also.
  assert(is_integral_type(lhs_ty) && is_integral_type(rhs_ty));

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

  const Type *unsigned_ty;
  const Type *signed_ty;
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
                                              (const BuiltinType *)signed_ty);
}

const Type *sema_get_common_arithmetic_type_of_exprs(Sema *sema,
                                                     const Expr *lhs,
                                                     const Expr *rhs,
                                                     const TreeMap *local_ctx) {
  const Type *lhs_ty = sema_get_type_of_expr_in_ctx(sema, lhs, local_ctx);
  const Type *rhs_ty = sema_get_type_of_expr_in_ctx(sema, rhs, local_ctx);
  return sema_get_common_arithmetic_type(sema, lhs_ty, rhs_ty, local_ctx);
}

const Type *sema_get_type_of_binop_expr(Sema *sema, const BinOp *expr,
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
    default:
      printf("TODO: Implement get type for binop kind %d on %zu:%zu\n",
             expr->op, expr->expr.line, expr->expr.col);
      __builtin_trap();
  }
}

const StructType *sema_get_struct_type_from_member_access(
    Sema *sema, const MemberAccess *access, const TreeMap *local_ctx) {
  const Type *base_ty =
      sema_get_type_of_expr_in_ctx(sema, access->base, local_ctx);
  base_ty = sema_resolve_maybe_named_type(sema, base_ty);

  if (access->is_arrow) {
    assert(sema_is_pointer_to(sema, base_ty, TK_StructType, local_ctx));
    base_ty = sema_get_pointee(sema, base_ty, local_ctx);
    base_ty = sema_resolve_maybe_named_type(sema, base_ty);
  }
  assert(sema_is_struct_type(sema, base_ty, local_ctx));

  return sema_resolve_struct_type(sema, (const StructType *)base_ty);
}

bool sema_is_function_or_function_ptr(Sema *sema, const Type *ty,
                                      const TreeMap *local_ctx) {
  if (ty->vtable->kind == TK_FunctionType)
    return true;
  return sema_is_pointer_type(sema, ty, local_ctx) &&
         sema_get_pointee(sema, ty, local_ctx)->vtable->kind == TK_FunctionType;
}

const FunctionType *sema_get_function(Sema *sema, const Type *ty,
                                      const TreeMap *local_ctx) {
  if (ty->vtable->kind == TK_FunctionType)
    return (const FunctionType *)ty;

  ty = sema_get_pointee(sema, ty, local_ctx);
  assert(ty->vtable->kind == TK_FunctionType);
  return (const FunctionType *)ty;
}

// Infer the type of this expression. The caller of this is not in charge of
// destroying the type.
const Type *sema_get_type_of_expr_in_ctx(Sema *sema, const Expr *expr,
                                         const TreeMap *local_ctx) {
  switch (expr->vtable->kind) {
    case EK_String:
      return &sema->str_ty.type;
    case EK_SizeOf:
    case EK_AlignOf:
      return sema_resolve_named_type_from_name(sema, "size_t");
    case EK_Int:
      return &((const Int *)expr)->type.type;
    case EK_Bool:
      return &sema->bt_Bool.type;
    case EK_Char:
      return &sema->bt_Char.type;
    case EK_DeclRef: {
      const DeclRef *decl = (const DeclRef *)expr;
      void *val;
      if (tree_map_get(&sema->globals, decl->name, &val)) {
        if (((Node *)val)->vtable->kind == NK_GlobalVariable) {
          GlobalVariable *gv = val;
          return gv->type;
        } else {
          assert(((Node *)val)->vtable->kind == NK_FunctionDefinition);
          FunctionDefinition *f = val;
          return f->type;
        }
      }

      // Not a global. Search the local scope.
      if (tree_map_get(local_ctx, decl->name, &val))
        return (const Type *)val;

      // Maybe an enum?
      if (tree_map_get(&sema->enum_names, decl->name, &val))
        return (const Type *)val;

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
      assert(sema_is_function_or_function_ptr(sema, ty, local_ctx));
      return sema_get_function(sema, ty, local_ctx)->return_type;
    }
    case EK_Cast: {
      const Cast *cast = (const Cast *)expr;
      return cast->to;
    }
    case EK_MemberAccess: {
      const MemberAccess *access = (const MemberAccess *)expr;
      const StructType *base_ty =
          sema_get_struct_type_from_member_access(sema, access, local_ctx);

      const StructMember *member = sema_get_struct_member(
          sema, base_ty, access->member, /*offset=*/NULL);
      return member->type;
    }
    case EK_Conditional: {
      const Conditional *conditional = (const Conditional *)expr;
      return sema_get_common_arithmetic_type_of_exprs(
          sema, conditional->true_expr, conditional->false_expr, local_ctx);
    }
    case EK_Index: {
      const Index *index = (const Index *)expr;
      const Type *base_ty =
          sema_get_type_of_expr_in_ctx(sema, index->base, local_ctx);
      base_ty = sema_resolve_maybe_named_type(sema, base_ty);
      if (base_ty->vtable->kind == TK_PointerType)
        return sema_get_pointee(sema, base_ty, local_ctx);

      if (base_ty->vtable->kind == TK_ArrayType)
        return sema_get_array_type(sema, base_ty, local_ctx)->elem_type;

      printf("Attempting to index a type that isn't a pointer or array: %d\n",
             base_ty->vtable->kind);
      __builtin_trap();
    }
    default:
      printf(
          "TODO: Implement sema_get_type_of_expr_in_ctx for this expression "
          "%d\n",
          expr->vtable->kind);
      __builtin_trap();
  }
}

// Infer the type of this expression in the global context. The caller of this
// is not in charge of destroying the type.
const Type *sema_get_type_of_expr(Sema *sema, const Expr *expr) {
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

static size_t builtin_type_get_align(const BuiltinType *bt) {
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
    case BTK_Bool:
      return alignof(bool);
    case BTK_Void:
      printf("Attempting to get alignof void\n");
      __builtin_trap();
  }
}

size_t sema_eval_sizeof_array(Sema *, const ArrayType *);
size_t sema_eval_alignof_array(Sema *, const ArrayType *);

size_t sema_eval_alignof_type(Sema *sema, const Type *type) {
  switch (type->vtable->kind) {
    case TK_BuiltinType:
      return builtin_type_get_align((const BuiltinType *)type);
    case TK_PointerType:
      // TODO: These are host values. Also is the alignment of all pointer
      // types guaranteed to be the same?
      return alignof(void *);
    case TK_NamedType: {
      const NamedType *nt = (const NamedType *)type;
      void *found;
      if (!tree_map_get(&sema->typedef_types, nt->name, &found)) {
        printf("Unknown type '%s'\n", nt->name);
        __builtin_trap();
      }
      return sema_eval_alignof_type(sema, (const Type *)found);
    }
    case TK_EnumType:
      return builtin_type_get_align(
          sema_get_integral_type_for_enum(sema, (const EnumType *)type));
    case TK_ArrayType:
      return sema_eval_alignof_array(sema, (const ArrayType *)type);
    case TK_FunctionType:
      printf("Cannot take alignof function type!\n");
      __builtin_trap();
    case TK_StructType: {
      const StructType *struct_ty =
          sema_resolve_struct_type(sema, (const StructType *)type);
      assert(struct_ty->members);
      size_t max_align = 0;
      for (size_t i = 0; i < struct_ty->members->size; ++i) {
        const StructMember *member = vector_at(struct_ty->members, i);
        size_t member_align = sema_eval_alignof_type(sema, member->type);
        if (member_align > max_align)
          max_align = member_align;
      }
      return max_align;
    }
    case TK_ReplacementSentinelType:
      printf("Replacement sentinel type should not be used\n");
      __builtin_trap();
      break;
  }
}

size_t sema_eval_sizeof_struct_type(Sema *sema, const StructType *type) {
  type = sema_resolve_struct_type(sema, type);
  if (!type->members) {
    assert(type->name);
    printf("Taking sizeof incomplete struct type '%s'\n", type->name);
    __builtin_trap();
  }

  size_t size = 0;
  for (size_t i = 0; i < type->members->size; ++i) {
    // TODO: Account for bitfields.
    const StructMember *member = vector_at(type->members, i);
    const Type *member_ty = member->type;
    size_t align = sema_eval_alignof_type(sema, member_ty);
    if (size % align != 0)
      size = align_up(size, align);
    size += sema_eval_sizeof_type(sema, member_ty);
  }
  return size;
}

size_t sema_eval_sizeof_type(Sema *sema, const Type *type) {
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
      return sema_eval_sizeof_struct_type(sema, (const StructType *)type);
    case TK_ReplacementSentinelType:
      printf("Replacement sentinel type should not be used\n");
      __builtin_trap();
      break;
  }
}

int compare_constexpr_result_types(const ConstExprResult *lhs,
                                   const ConstExprResult *rhs) {
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

size_t sema_eval_sizeof_array(Sema *sema, const ArrayType *arr) {
  size_t elem_size = sema_eval_sizeof_type(sema, arr->elem_type);
  assert(arr->size);
  ConstExprResult num_elems = sema_eval_expr(sema, arr->size);
  assert(num_elems.result_kind == RK_UnsignedLongLong);
  return elem_size * num_elems.result.ull;
}

size_t sema_eval_alignof_array(Sema *sema, const ArrayType *arr) {
  return sema_eval_alignof_type(sema, arr->elem_type);
}

ConstExprResult sema_eval_binop(Sema *sema, const BinOp *expr) {
  ConstExprResult lhs = sema_eval_expr(sema, expr->lhs);
  ConstExprResult rhs = sema_eval_expr(sema, expr->rhs);

  switch (expr->op) {
    case BOK_Eq: {
      ConstExprResult res;
      res.result_kind = RK_Boolean;
      res.result.b = compare_constexpr_result_types(&lhs, &rhs) == 0;
      return res;
    }
    case BOK_LShift:
      switch (lhs.result_kind) {
        case RK_Boolean:
          printf("Cannot shift boolean\n");
          __builtin_trap();
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
          printf("Cannot shift boolean\n");
          __builtin_trap();
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
          printf("TODO: Check if booleans can be compared %zu:%zu\n",
                 expr->expr.line, expr->expr.col);
          __builtin_trap();
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
      printf(
          "TODO: Implement constant evaluation for the remaining binops %d\n",
          expr->op);
      __builtin_trap();
      break;
  }
}

ConstExprResult sema_eval_alignof(Sema *sema, const AlignOf *ao) {
  const Type *type;
  if (ao->is_expr) {
    type = sema_get_type_of_expr(sema, (const Expr *)ao->expr_or_type);
  } else {
    type = (const Type *)ao->expr_or_type;
  }

  size_t size = sema_eval_alignof_type(sema, type);
  ConstExprResult res;
  res.result_kind = RK_UnsignedLongLong;
  res.result.ull = size;
  return res;
}

ConstExprResult sema_eval_sizeof(Sema *sema, const SizeOf *so) {
  const Type *type;
  if (so->is_expr) {
    type = sema_get_type_of_expr(sema, (const Expr *)so->expr_or_type);
  } else {
    type = (const Type *)so->expr_or_type;
  }

  size_t size = sema_eval_sizeof_type(sema, type);
  ConstExprResult res;
  res.result_kind = RK_UnsignedLongLong;
  res.result.ull = size;
  return res;
}

ConstExprResult sema_eval_expr(Sema *sema, const Expr *expr) {
  switch (expr->vtable->kind) {
    case EK_BinOp:
      return sema_eval_binop(sema, (const BinOp *)expr);
    case EK_SizeOf:
      return sema_eval_sizeof(sema, (const SizeOf *)expr);
    case EK_Int: {
      ConstExprResult res;
      res.result_kind = RK_Int;
      res.result.i = (int)((const Int *)expr)->val;
      return res;
    }
    case EK_UnOp: {
      const UnOp *unop = (const UnOp *)expr;
      ConstExprResult sub_res = sema_eval_expr(sema, unop->subexpr);
      switch (unop->op) {
        case UOK_Negate:
          switch (sub_res.result_kind) {
            case RK_Boolean:
              printf("Cannot negate boolean\n");
              __builtin_trap();
            case RK_Int:
              sub_res.result.i = -sub_res.result.i;
              return sub_res;
            case RK_UnsignedLongLong:
              sub_res.result.ull = -sub_res.result.ull;
              return sub_res;
          }
          break;
        default:
          printf("TODO: Handle constexpr eval for other unop kinds %d\n",
                 unop->op);
          __builtin_trap();
      }
    }
    case EK_DeclRef: {
      const DeclRef *decl = (const DeclRef *)expr;
      void *val;
      if (tree_map_get(&sema->enum_values, decl->name, &val)) {
        ConstExprResult res;
        res.result_kind = RK_Int;
        res.result.i = (int)(intptr_t)val;
        return res;
      }

      if (tree_map_get(&sema->globals, decl->name, &val)) {
        assert(((Node *)val)->vtable->kind == NK_GlobalVariable);
        const GlobalVariable *gv = val;
        assert(gv->initializer);
        return sema_eval_expr(sema, gv->initializer);
      }

      printf("Unknown global '%s'\n", decl->name);
      __builtin_trap();
    }
    case EK_Conditional: {
      const Conditional* conditional = (const Conditional*)expr;
      ConstExprResult cond_res = sema_eval_expr(sema, conditional->cond);
      assert(cond_res.result_kind == RK_Boolean);
      return sema_eval_expr(sema, cond_res.result.b ? conditional->true_expr
                                                    : conditional->false_expr);
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

void sema_verify_static_assert_condition(Sema *sema, const Expr *cond) {
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
  printf("static_assert failed\n");
  __builtin_trap();
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
  LLVMDIBuilderRef dibuilder;
  LLVMMetadataRef dicu;
  LLVMMetadataRef difile;
} Compiler;

void compiler_construct(Compiler *compiler, LLVMModuleRef mod, Sema *sema,
                        LLVMDIBuilderRef dibuilder) {
  compiler->mod = mod;
  compiler->sema = sema;
  compiler->dibuilder = dibuilder;
  compiler->difile = LLVMDIBuilderCreateFile(dibuilder, "<input>", 7, "", 0);
  compiler->dicu = LLVMDIBuilderCreateCompileUnit(
      dibuilder, LLVMDWARFSourceLanguageC, compiler->difile,
      /*Producer=*/"", /*ProducerLen=*/0, /*IsOptimized=*/0,
      /*Flags=*/"", /*FlagsLen=*/0, /*RuntimeVer=*/0, /*SplitName=*/"",
      /*SplitNameLen=*/0, LLVMDWARFEmissionFull, /*DWOId=*/0,
      /*SplitDebugInlining=*/0, /*DebugInfoForProfiling=*/0, /*SysRoot=*/"",
      /*SysRootLen=*/0, /*SDK=*/"", /*SDKLen=*/0);
}

void compiler_destroy(Compiler *compiler) {}

LLVMTypeRef get_llvm_type(Compiler *compiler, const Type *type);

LLVMTypeRef get_llvm_struct_type(Compiler *compiler, const StructType *st) {
  st = sema_resolve_struct_type(compiler->sema, st);

  void *llvm_struct;

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
  LLVMTypeRef elem = get_llvm_type(compiler, at->elem_type);
  switch (size.result_kind) {
    case RK_UnsignedLongLong:
      return LLVMArrayType(elem, (unsigned)size.result.ull);
    case RK_Int:
      return LLVMArrayType(elem, (unsigned)size.result.i);
    case RK_Boolean:
      return LLVMArrayType(elem, (unsigned)size.result.b);
  }
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

LLVMValueRef maybe_compile_constant_implicit_cast(Compiler *compiler,
                                                  const Expr *expr,
                                                  const Type *to_ty);

LLVMValueRef get_named_global(Compiler *compiler, const char *name) {
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
    void *res;
    if (tree_map_get(&compiler->sema->enum_values, name, &res)) {
      EnumType *enum_ty = NULL;
      tree_map_get(&compiler->sema->enum_names, name, (void *)&enum_ty);
      assert(enum_ty);
      LLVMTypeRef llvm_ty = get_llvm_type(compiler, &enum_ty->type);
      return LLVMConstInt(llvm_ty, (unsigned long long)res,
                          /*IsSigned=*/1);
    }
  }

  return val;
}

LLVMValueRef compile_constant_expr(Compiler *compiler, const Expr *expr,
                                   const Type *to_ty) {
  switch (expr->vtable->kind) {
    case EK_DeclRef: {
      const DeclRef *decl = (const DeclRef *)expr;
      LLVMValueRef glob = get_named_global(compiler, decl->name);
      assert(glob);
      return glob;
    }
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
    case EK_BinOp: {
      LLVMTypeRef llvm_type = get_llvm_type_of_expr_global_ctx(compiler, expr);
      ConstExprResult res = sema_eval_expr(compiler->sema, expr);
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
      const Cast *cast = (const Cast *)expr;
      const Type *from_ty = sema_get_type_of_expr(compiler->sema, cast->base);
      const Type *to_ty = cast->to;

      LLVMValueRef from = compile_constant_expr(compiler, cast->base, to_ty);

      if (sema_types_are_compatible(compiler->sema, from_ty, to_ty))
        return from;

      if (is_integral_type(from_ty) && is_pointer_type(to_ty))
        return LLVMConstIntToPtr(from, get_llvm_type(compiler, to_ty));

      printf("TODO: Unhandled constant cast conversion\n");
      __builtin_trap();
    }
    case EK_InitializerList: {
      const InitializerList *init = (const InitializerList *)expr;
      LLVMValueRef *constants = malloc(sizeof(LLVMValueRef) * init->elems.size);
      for (size_t i = 0; i < init->elems.size; ++i) {
        const InitializerListElem *elem = vector_at(&init->elems, i);
        const Type *elem_ty = sema_get_type_of_expr(compiler->sema, elem->expr);
        constants[i] =
            maybe_compile_constant_implicit_cast(compiler, elem->expr, elem_ty);
      }

      LLVMValueRef res;
      TreeMap dummy_ctx;  // TODO: Have global version of sema_is_array_type.
      string_tree_map_construct(&dummy_ctx);
      if (sema_is_array_type(compiler->sema, to_ty, &dummy_ctx)) {
        printf("TODO: Handle this\n");
        __builtin_trap();
      } else {
        assert(sema_is_struct_type(compiler->sema, to_ty, &dummy_ctx));
        res = LLVMConstStruct(constants, (unsigned)init->elems.size,
                              /*Packed=*/0);
      }
      free(constants);

      return res;
    }
    case EK_UnOp: {
      const UnOp *unop = (const UnOp *)expr;
      switch (unop->op) {
        case UOK_AddrOf:
          return compile_constant_expr(compiler, unop->subexpr, to_ty);
        default:
          printf(
              "TODO: Implement IR constant expr evaluation for unop expr op "
              "%d\n",
              unop->op);
          __builtin_trap();
      }
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
  LLVMValueRef from = compile_constant_expr(compiler, expr, to_ty);

  // We cannot disambiguate if an initializer list is for an array or struct.
  // They should not be treated like normal expressions, so just use the
  // destination type in this case.
  const Type *from_ty = expr->vtable->kind == EK_InitializerList
                            ? to_ty
                            : sema_get_type_of_expr(compiler->sema, expr);

  from_ty = sema_resolve_maybe_named_type(compiler->sema, from_ty);
  to_ty = sema_resolve_maybe_named_type(compiler->sema, to_ty);

  if (sema_types_are_compatible_ignore_quals(compiler->sema, from_ty, to_ty))
    return from;

  if (from_ty->vtable->kind == TK_EnumType) {
    from_ty = &sema_get_integral_type_for_enum(compiler->sema,
                                               (const EnumType *)from_ty)
                   ->type;
  }
  if (to_ty->vtable->kind == TK_EnumType) {
    to_ty = &sema_get_integral_type_for_enum(compiler->sema,
                                             (const EnumType *)to_ty)
                 ->type;
  }

  if (is_integral_type(from_ty) && is_integral_type(to_ty)) {
    unsigned long long from_val = LLVMConstIntGetZExtValue(from);
    // size_t from_size = builtin_type_get_size((const BuiltinType *)from_ty);
    // size_t to_size = builtin_type_get_size((const BuiltinType *)to_ty);
    LLVMTypeRef llvm_to_ty = get_llvm_type(compiler, to_ty);
    bool is_signed = !is_unsigned_integral_type(to_ty);
    return LLVMConstInt(llvm_to_ty, from_val, is_signed);
  }

  printf(
      "TODO: Unhandled implicit constant cast conversion:\n"
      "lhs: %d %d (%s) %p\n"
      "rhs: %d %d (%s) %p\n",
      from_ty->vtable->kind, from_ty->qualifiers,
      (from_ty->vtable->kind == TK_StructType
           ? ((const StructType *)from_ty)->name
           : ""),
      from_ty, to_ty->vtable->kind, to_ty->qualifiers,
      (to_ty->vtable->kind == TK_StructType ? ((const StructType *)to_ty)->name
                                            : ""),
      to_ty);
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

// NOTE: This always returns an i1.
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

LLVMValueRef compile_lvalue_ptr(Compiler *compiler, LLVMBuilderRef builder,
                                const Expr *expr, TreeMap *local_ctx,
                                TreeMap *local_allocas);

LLVMTypeRef get_llvm_type_of_expr(Compiler *compiler, const Expr *expr,
                                  TreeMap *local_ctx);

LLVMValueRef compile_unop_lvalue_ptr(Compiler *compiler, LLVMBuilderRef builder,
                                     const UnOp *expr, TreeMap *local_ctx,
                                     TreeMap *local_allocas) {
  switch (expr->op) {
    case UOK_Deref: {
      return compile_expr(compiler, builder, expr->subexpr, local_ctx,
                          local_allocas);
    }
    default:
      printf("TODO: Implement compile_unop_lvalue_ptr for this op %d\n",
             expr->op);
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
      LLVMValueRef res =
          LLVMBuildICmp(builder, LLVMIntEQ, to_bool, zero, "not");
      return LLVMBuildZExt(builder, res, LLVMInt8Type(), "");
    }
    case UOK_BitNot: {
      LLVMValueRef val = compile_expr(compiler, builder, expr->subexpr,
                                      local_ctx, local_allocas);
      LLVMValueRef ones = LLVMConstAllOnes(LLVMTypeOf(val));
      return LLVMBuildXor(builder, val, ones, "");
    }
    case UOK_PreDec:
    case UOK_PostDec:
    case UOK_PreInc:
    case UOK_PostInc: {
      bool is_pre = expr->op == UOK_PreInc || expr->op == UOK_PreDec;
      bool is_inc = expr->op == UOK_PreInc || expr->op == UOK_PostInc;
      LLVMValueRef ptr = compile_lvalue_ptr(compiler, builder, expr->subexpr,
                                            local_ctx, local_allocas);
      LLVMTypeRef llvm_type =
          get_llvm_type_of_expr(compiler, expr->subexpr, local_ctx);
      LLVMValueRef val = LLVMBuildLoad2(builder, llvm_type, ptr, "");
      LLVMValueRef one = LLVMConstInt(llvm_type, 1, /*signed=*/0);
      LLVMValueRef postop = is_inc ? LLVMBuildAdd(builder, val, one, "")
                                   : LLVMBuildSub(builder, val, one, "");
      LLVMBuildStore(builder, postop, ptr);
      return is_pre ? postop : val;
    }
    case UOK_Negate: {
      LLVMValueRef val = compile_expr(compiler, builder, expr->subexpr,
                                      local_ctx, local_allocas);
      LLVMValueRef zero = LLVMConstNull(LLVMTypeOf(val));
      return LLVMBuildSub(builder, zero, val, "");
    }
    case UOK_AddrOf:
      return compile_lvalue_ptr(compiler, builder, expr->subexpr, local_ctx,
                                local_allocas);
    case UOK_Deref: {
      LLVMTypeRef llvm_ty =
          get_llvm_type_of_expr(compiler, &expr->expr, local_ctx);
      LLVMValueRef ptr = compile_expr(compiler, builder, expr->subexpr,
                                      local_ctx, local_allocas);
      return LLVMBuildLoad2(builder, llvm_ty, ptr, "");
    }
    default:
      printf("TODO: Implement compile_unop for this op %d\n", expr->op);
      __builtin_trap();
  }
}

LLVMValueRef compile_implicit_cast(Compiler *compiler, LLVMBuilderRef builder,
                                   const Expr *from, const Type *to,
                                   TreeMap *local_ctx, TreeMap *local_allocas) {
  to = sema_resolve_maybe_named_type(compiler->sema, to);
  if (is_builtin_type(to, BTK_Bool)) {
    LLVMValueRef res =
        compile_to_bool(compiler, builder, from, local_ctx, local_allocas);
    return LLVMBuildZExt(builder, res, LLVMInt8Type(), "");
  }

  LLVMValueRef llvm_from =
      compile_expr(compiler, builder, from, local_ctx, local_allocas);

  const Type *from_ty =
      sema_get_type_of_expr_in_ctx(compiler->sema, from, local_ctx);

  if (sema_types_are_compatible(compiler->sema, from_ty, to))
    return llvm_from;

  LLVMTypeRef llvm_to_ty = get_llvm_type(compiler, to);
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
      LLVMTargetDataRef data_layout = LLVMGetModuleDataLayout(compiler->mod);
      unsigned ptr_size = LLVMPointerSize(data_layout) * kCharBit;

      assert(from_size <= ptr_size);

      if (from_size < ptr_size)
        llvm_from =
            LLVMBuildZExt(builder, llvm_from, LLVMIntType(ptr_size), "");

      return LLVMBuildIntToPtr(builder, llvm_from, llvm_to_ty, "");
    }
  }

  printf("TODO: Handle this implicit cast\n");
  LLVMDumpValue(llvm_from);
  LLVMDumpType(llvm_to_ty);
  __builtin_trap();
}

// This creates an alloca in this function but ensures it's always at the start
// of the function. Having an alloca in the middle of the function can cause
// the stack pointer to keep decrementing if it's in a loop.
LLVMValueRef build_alloca_at_func_start(LLVMBuilderRef builder,
                                        const char *name, LLVMTypeRef llvm_ty) {
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMBasicBlockRef current_bb = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry_bb = LLVMGetEntryBasicBlock(fn);

  // Place builder before this instruction. If it's null, place the builder
  // at the end of the bb.
  LLVMPositionBuilder(builder, entry_bb, LLVMGetLastInstruction(entry_bb));

  LLVMValueRef alloca = LLVMBuildAlloca(builder, llvm_ty, name);

  LLVMPositionBuilderAtEnd(builder, current_bb);

  return alloca;
}

LLVMValueRef compile_conditional(Compiler *compiler, LLVMBuilderRef builder,
                                 const Conditional *expr, TreeMap *local_ctx,
                                 TreeMap *local_allocas) {
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);

  LLVMValueRef cond =
      compile_to_bool(compiler, builder, expr->cond, local_ctx, local_allocas);

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
  LLVMValueRef true_expr = compile_expr(compiler, builder, expr->true_expr,
                                        local_ctx, local_allocas);
  LLVMBuildBr(builder, mergebb);
  ifbb = LLVMGetInsertBlock(builder);  // Codegen of 'if' can change the current
                                       // block, update ifbb for the PHI.

  // Emit potential else BB.
  LLVMAppendExistingBasicBlock(fn, elsebb);
  LLVMPositionBuilderAtEnd(builder, elsebb);
  LLVMValueRef false_expr = compile_expr(compiler, builder, expr->false_expr,
                                         local_ctx, local_allocas);
  LLVMBuildBr(builder, mergebb);
  elsebb =
      LLVMGetInsertBlock(builder);  // Codegen of 'else' can change the current
                                    // block, update ifbb for the PHI.

  // Emit merge BB.
  LLVMAppendExistingBasicBlock(fn, mergebb);
  LLVMPositionBuilderAtEnd(builder, mergebb);

  const Type *common_ty = sema_get_common_arithmetic_type_of_exprs(
      compiler->sema, expr->true_expr, expr->false_expr, local_ctx);
  LLVMValueRef phi =
      LLVMBuildPhi(builder, get_llvm_type(compiler, common_ty), "");
  LLVMValueRef incoming_vals[] = {true_expr, false_expr};
  LLVMBasicBlockRef incoming_blocks[] = {ifbb, elsebb};
  LLVMAddIncoming(phi, incoming_vals, incoming_blocks, 2);
  return phi;
}

LLVMValueRef compile_logical_binop(Compiler *compiler, LLVMBuilderRef builder,
                                   const Expr *lhs, const Expr *rhs,
                                   BinOpKind op, TreeMap *local_ctx,
                                   TreeMap *local_allocas) {
  // Account for short-circuit evaluation.
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);

  LLVMValueRef lhs_val =
      compile_to_bool(compiler, builder, lhs, local_ctx, local_allocas);
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
      printf("Unhandled local operator %d\n", op);
      __builtin_trap();
  }

  // BB for evaluating RHS.
  LLVMPositionBuilderAtEnd(builder, eval_rhs_bb);
  LLVMValueRef rhs_val =
      compile_to_bool(compiler, builder, rhs, local_ctx, local_allocas);
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

LLVMValueRef compile_binop(Compiler *compiler, LLVMBuilderRef builder,
                           const BinOp *expr, TreeMap *local_ctx,
                           TreeMap *local_allocas) {
  const Type *lhs_ty =
      sema_get_type_of_expr_in_ctx(compiler->sema, expr->lhs, local_ctx);
  const Type *rhs_ty =
      sema_get_type_of_expr_in_ctx(compiler->sema, expr->rhs, local_ctx);
  const Expr *maybe_ptr = NULL;
  const Expr *offset;
  const Type *maybe_ptr_ty;
  if (sema_is_pointer_type(compiler->sema, lhs_ty, local_ctx)) {
    maybe_ptr = expr->lhs;
    maybe_ptr_ty = lhs_ty;
    offset = expr->rhs;
  } else if (sema_is_pointer_type(compiler->sema, rhs_ty, local_ctx)) {
    maybe_ptr = expr->rhs;
    maybe_ptr_ty = rhs_ty;
    offset = expr->lhs;
  }

  if (maybe_ptr && can_be_used_for_ptr_arithmetic(expr->op)) {
    const Type *pointee_ty =
        sema_get_pointee(compiler->sema, maybe_ptr_ty, local_ctx);
    LLVMTypeRef llvm_base_ty = get_llvm_type(compiler, pointee_ty);
    LLVMValueRef llvm_base =
        compile_expr(compiler, builder, maybe_ptr, local_ctx, local_allocas);
    LLVMValueRef llvm_offset =
        compile_expr(compiler, builder, offset, local_ctx, local_allocas);
    LLVMValueRef offsets[] = {llvm_offset};
    LLVMValueRef gep =
        LLVMBuildGEP2(builder, llvm_base_ty, llvm_base, offsets, 1, "");
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
  const Type *common_ty;
  if (is_logical_binop(expr->op)) {
    return compile_logical_binop(compiler, builder, expr->lhs, expr->rhs,
                                 expr->op, local_ctx, local_allocas);
  } else if (is_assign_binop(expr->op)) {
    rhs = compile_implicit_cast(compiler, builder, expr->rhs, lhs_ty, local_ctx,
                                local_allocas);
    lhs = compile_lvalue_ptr(compiler, builder, expr->lhs, local_ctx,
                             local_allocas);
  } else if (expr->op == BOK_Eq || expr->op == BOK_Ne) {
    if (sema_is_pointer_type(compiler->sema, lhs_ty, local_ctx) &&
        sema_is_pointer_type(compiler->sema, rhs_ty, local_ctx)) {
      common_ty = sema_get_type_of_binop_expr(compiler->sema, expr, local_ctx);
    } else {
      common_ty = sema_get_common_arithmetic_type(compiler->sema, lhs_ty,
                                                  rhs_ty, local_ctx);
    }
    lhs = compile_implicit_cast(compiler, builder, expr->lhs, common_ty,
                                local_ctx, local_allocas);
    rhs = compile_implicit_cast(compiler, builder, expr->rhs, common_ty,
                                local_ctx, local_allocas);
  } else {
    common_ty = sema_get_common_arithmetic_type(compiler->sema, lhs_ty, rhs_ty,
                                                local_ctx);
    lhs = compile_implicit_cast(compiler, builder, expr->lhs, common_ty,
                                local_ctx, local_allocas);
    rhs = compile_implicit_cast(compiler, builder, expr->rhs, common_ty,
                                local_ctx, local_allocas);
  }

  LLVMValueRef res;

  switch (expr->op) {
    case BOK_Lt: {
      LLVMIntPredicate op =
          sema_is_unsigned_integral_type(compiler->sema, common_ty)
              ? LLVMIntULT
              : LLVMIntSLT;
      res = LLVMBuildICmp(builder, op, lhs, rhs, "");
      break;
    }
    case BOK_Gt: {
      LLVMIntPredicate op =
          sema_is_unsigned_integral_type(compiler->sema, common_ty)
              ? LLVMIntUGT
              : LLVMIntSGT;
      res = LLVMBuildICmp(builder, op, lhs, rhs, "");
      break;
    }
    case BOK_Le: {
      LLVMIntPredicate op =
          sema_is_unsigned_integral_type(compiler->sema, common_ty)
              ? LLVMIntULE
              : LLVMIntSLE;
      res = LLVMBuildICmp(builder, op, lhs, rhs, "");
      break;
    }
    case BOK_Ge: {
      LLVMIntPredicate op =
          sema_is_unsigned_integral_type(compiler->sema, common_ty)
              ? LLVMIntUGE
              : LLVMIntSGE;
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
      if (sema_is_unsigned_integral_type(compiler->sema, common_ty))
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
    case BOK_AddAssign: {
      LLVMTypeRef llvm_lhs_ty = get_llvm_type(compiler, lhs_ty);
      LLVMValueRef lhs_val = LLVMBuildLoad2(builder, llvm_lhs_ty, lhs, "");
      res = LLVMBuildAdd(builder, lhs_val, rhs, "");
      LLVMBuildStore(builder, res, lhs);
      break;
    }
    case BOK_OrAssign: {
      LLVMTypeRef llvm_lhs_ty = get_llvm_type(compiler, lhs_ty);
      LLVMValueRef lhs_val = LLVMBuildLoad2(builder, llvm_lhs_ty, lhs, "");
      res = LLVMBuildOr(builder, lhs_val, rhs, "");
      LLVMBuildStore(builder, res, lhs);
      break;
    }
    case BOK_LShiftAssign:
    case BOK_RShiftAssign: {
      LLVMTypeRef llvm_lhs_ty = get_llvm_type(compiler, lhs_ty);
      LLVMValueRef lhs_val = LLVMBuildLoad2(builder, llvm_lhs_ty, lhs, "");
      if (expr->op == BOK_LShiftAssign) {
        res = LLVMBuildShl(builder, lhs_val, rhs, "");
      } else {
        res = LLVMBuildAShr(builder, lhs_val, rhs, "");
      }
      LLVMBuildStore(builder, res, lhs);
      break;
    }
    default:
      printf("TODO: Implement compile_binop for this op %d\n", expr->op);
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
  const Type *ty =
      sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);
  return get_llvm_type(compiler, ty);
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
// `func_args` is a vector of FunctionArg (same as `FunctionType::pos_args`).
vector compile_call_args(Compiler *compiler, LLVMBuilderRef builder,
                         const vector *args, const vector *func_args,
                         TreeMap *local_ctx, TreeMap *local_allocas) {
  vector llvm_args;
  vector_construct(&llvm_args, sizeof(LLVMValueRef), alignof(LLVMValueRef));
  for (size_t i = 0; i < args->size; ++i) {
    LLVMValueRef *storage = vector_append_storage(&llvm_args);
    const Expr *arg = *(const Expr **)vector_at(args, i);

    if (i < func_args->size) {
      const FunctionArg *func_arg_ty = vector_at(func_args, i);
      *storage = compile_implicit_cast(
          compiler, builder, arg, func_arg_ty->type, local_ctx, local_allocas);
    } else {
      // This should be a varargs function.
      const Type *arg_ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, arg, local_ctx);
      *storage = compile_implicit_cast(compiler, builder, arg, arg_ty,
                                       local_ctx, local_allocas);
    }
  }
  return llvm_args;
}

LLVMValueRef call_llvm_debugtrap(Compiler *compiler, LLVMBuilderRef builder) {
  // FIXME: We should be able to do `sizeof(name)`.
  const char *name = "llvm.debugtrap";
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
    case EK_Bool: {
      const Bool *b = (const Bool *)expr;
      return LLVMConstInt(type, b->val, /*UsSugbed=*/false);
    }
    case EK_Char: {
      const Char *c = (const Char *)expr;
      assert(c->val >= 0);
      return LLVMConstInt(type, (unsigned long long)c->val, /*UsSugbed=*/false);
    }
    case EK_SizeOf: {
      ConstExprResult res =
          sema_eval_sizeof(compiler->sema, (const SizeOf *)expr);
      assert(res.result_kind == RK_UnsignedLongLong);
      return LLVMConstInt(type, res.result.ull, /*IsSigned=*/0);
    }
    case EK_AlignOf: {
      ConstExprResult res =
          sema_eval_alignof(compiler->sema, (const AlignOf *)expr);
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
      if (is_func) {
        LLVMValueRef val = LLVMGetNamedFunction(compiler->mod, decl->name);
        assert(val);
        return val;
      }

      const Type *decl_ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);
      if (decl_ty->vtable->kind == TK_EnumType) {
        void *enum_val;
        if (tree_map_get(&compiler->sema->enum_values, decl->name, &enum_val)) {
          LLVMTypeRef enum_ty =
              get_llvm_type_of_expr(compiler, expr, local_ctx);
          return LLVMConstInt(enum_ty, (uintptr_t)enum_val, /*signed=*/1);
        }

        printf("Couldn't find value for enum member '%s'\n", decl->name);
        __builtin_trap();
      }

      LLVMValueRef val =
          compile_lvalue_ptr(compiler, builder, expr, local_ctx, local_allocas);

      if (sema_is_array_type(compiler->sema, decl_ty, local_ctx))
        return val;

      return LLVMBuildLoad2(builder, type, val, "");
    }
    case EK_Call: {
      const Call *call = (const Call *)expr;
      if (call->base->vtable->kind == EK_DeclRef) {
        const DeclRef *decl = (const DeclRef *)call->base;
        if (strcmp(decl->name, "__builtin_trap") == 0)
          return call_llvm_debugtrap(compiler, builder);
      }

      const Type *ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, call->base, local_ctx);
      const FunctionType *func_ty =
          sema_get_function(compiler->sema, ty, local_ctx);
      LLVMTypeRef llvm_func_ty = get_llvm_type(compiler, &func_ty->type);
      LLVMValueRef llvm_func =
          compile_expr(compiler, builder, call->base, local_ctx, local_allocas);

      vector llvm_args =
          compile_call_args(compiler, builder, &call->args, &func_ty->pos_args,
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
      const StructMember *member = sema_get_struct_member(
          compiler->sema, base_ty, access->member, &offset);

      LLVMValueRef ptr;
      if (access->is_arrow) {
        ptr = compile_expr(compiler, builder, access->base, local_ctx,
                           local_allocas);
        LLVMTypeRef llvm_base_ty = get_llvm_type(compiler, &base_ty->type);
        LLVMValueRef llvm_offset =
            LLVMConstInt(LLVMInt32Type(), offset, /*signed=*/0);
        LLVMValueRef offsets[] = {LLVMConstNull(LLVMInt32Type()), llvm_offset};
        ptr = LLVMBuildGEP2(builder, llvm_base_ty, ptr, offsets, 2, "");
      } else {
        ptr = compile_lvalue_ptr(compiler, builder, expr, local_ctx,
                                 local_allocas);
      }
      return LLVMBuildLoad2(builder, get_llvm_type(compiler, member->type), ptr,
                            "");
    }
    case EK_Conditional: {
      const Conditional *conditional = (const Conditional *)expr;
      return compile_conditional(compiler, builder, conditional, local_ctx,
                                 local_allocas);
    }
    case EK_Cast: {
      const Cast *cast = (const Cast *)expr;
      return compile_implicit_cast(compiler, builder, cast->base, cast->to,
                                   local_ctx, local_allocas);
    }
    case EK_Index: {
      LLVMValueRef ptr =
          compile_lvalue_ptr(compiler, builder, expr, local_ctx, local_allocas);
      const Type *ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);
      return LLVMBuildLoad2(builder, get_llvm_type(compiler, ty), ptr, "");
    }
    default:
      printf("TODO: Implement compile_expr for this expr %d\n",
             expr->vtable->kind);
      __builtin_trap();
  }
}

void compile_statement(Compiler *compiler, LLVMBuilderRef builder,
                       const Statement *stmt, TreeMap *local_ctx,
                       TreeMap *local_allocas, LLVMBasicBlockRef,
                       LLVMBasicBlockRef);

void compile_if_statement(Compiler *compiler, LLVMBuilderRef builder,
                          const IfStmt *stmt, TreeMap *local_ctx,
                          TreeMap *local_allocas, LLVMBasicBlockRef break_bb,
                          LLVMBasicBlockRef cont_bb) {
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);
  LLVMValueRef cond =
      compile_to_bool(compiler, builder, stmt->cond, local_ctx, local_allocas);

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

  {
    TreeMap local_ctx_cpy;
    TreeMap local_allocas_cpy;
    string_tree_map_construct(&local_ctx_cpy);
    string_tree_map_construct(&local_allocas_cpy);
    tree_map_clone(&local_ctx_cpy, local_ctx);
    tree_map_clone(&local_allocas_cpy, local_allocas);

    compile_statement(compiler, builder, stmt->body, &local_ctx_cpy,
                      &local_allocas_cpy, break_bb, cont_bb);

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
                        local_allocas, break_bb, cont_bb);

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

void compile_for_statement(Compiler *compiler, LLVMBuilderRef builder,
                           const ForStmt *stmt, TreeMap *local_ctx,
                           TreeMap *local_allocas, LLVMBasicBlockRef break_bb,
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
                      &local_allocas_cpy, break_bb, cont_bb);
  }

  LLVMBasicBlockRef for_start =
      LLVMAppendBasicBlockInContext(ctx, fn, "for_start");
  LLVMBuildBr(builder, for_start);
  LLVMPositionBuilderAtEnd(builder, for_start);

  LLVMBasicBlockRef for_end = LLVMCreateBasicBlockInContext(ctx, "for_end");
  LLVMBasicBlockRef for_iter = LLVMCreateBasicBlockInContext(ctx, "for_iter");

  // Check the condition if any.
  if (stmt->cond) {
    LLVMValueRef cond = compile_to_bool(compiler, builder, stmt->cond,
                                        &local_ctx_cpy, &local_allocas_cpy);
    LLVMBasicBlockRef for_body = LLVMCreateBasicBlockInContext(ctx, "for_body");
    LLVMBuildCondBr(builder, cond, for_body, for_end);

    LLVMAppendExistingBasicBlock(fn, for_body);
    LLVMPositionBuilderAtEnd(builder, for_body);
  }

  // Now emit the body.
  if (stmt->body) {
    compile_statement(compiler, builder, stmt->body, &local_ctx_cpy,
                      &local_allocas_cpy, for_end, for_iter);
  }

  // Branch to the iterator.
  if (!last_instruction_is_terminator(builder))
    LLVMBuildBr(builder, for_iter);

  LLVMAppendExistingBasicBlock(fn, for_iter);
  LLVMPositionBuilderAtEnd(builder, for_iter);

  // Then do the iter.
  if (stmt->iter) {
    compile_expr(compiler, builder, stmt->iter, &local_ctx_cpy,
                 &local_allocas_cpy);
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

// TODO: This should just be a switch instruction!!!
void compile_switch_statement(Compiler *compiler, LLVMBuilderRef builder,
                              const SwitchStmt *stmt, TreeMap *local_ctx,
                              TreeMap *local_allocas,
                              LLVMBasicBlockRef break_bb,
                              LLVMBasicBlockRef cont_bb) {
  LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMContextRef ctx = LLVMGetModuleContext(compiler->mod);

  LLVMValueRef check =
      compile_expr(compiler, builder, stmt->cond, local_ctx, local_allocas);

  LLVMBasicBlockRef end_bb = LLVMCreateBasicBlockInContext(ctx, "switch_end");

  LLVMValueRef should_fallthrough = LLVMConstNull(LLVMInt1Type());

  for (size_t i = 0; i < stmt->cases.size; ++i) {
    const SwitchCase *switch_case = vector_at(&stmt->cases, i);

    LLVMBasicBlockRef case_bb = LLVMAppendBasicBlockInContext(ctx, fn, "case");
    LLVMBasicBlockRef next_bb = LLVMCreateBasicBlockInContext(ctx, "next");

    const Type *common_ty = sema_get_common_arithmetic_type_of_exprs(
        compiler->sema, stmt->cond, switch_case->cond, local_ctx);

    LLVMValueRef case_val =
        compile_implicit_cast(compiler, builder, switch_case->cond, common_ty,
                              local_ctx, local_allocas);
    LLVMValueRef cond = LLVMBuildICmp(builder, LLVMIntEQ, check, case_val, "");
    should_fallthrough = LLVMBuildOr(builder, cond, should_fallthrough, "");
    LLVMBuildCondBr(builder, should_fallthrough, case_bb, next_bb);

    LLVMPositionBuilderAtEnd(builder, case_bb);

    bool last_stmt_terminates = false;
    for (size_t j = 0; j < switch_case->stmts.size; ++j) {
      Statement **case_stmt = vector_at(&switch_case->stmts, j);
      compile_statement(compiler, builder, *case_stmt, local_ctx, local_allocas,
                        end_bb, cont_bb);

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
      Statement **default_stmt = vector_at(stmt->default_stmts, i);
      compile_statement(compiler, builder, *default_stmt, local_ctx,
                        local_allocas, end_bb, cont_bb);

      if (last_instruction_is_terminator(builder))
        break;
    }
  }

  if (!last_instruction_is_terminator(builder))
    LLVMBuildBr(builder, end_bb);

  LLVMAppendExistingBasicBlock(fn, end_bb);
  LLVMPositionBuilderAtEnd(builder, end_bb);
}

void compile_statement(Compiler *compiler, LLVMBuilderRef builder,
                       const Statement *stmt, TreeMap *local_ctx,
                       TreeMap *local_allocas, LLVMBasicBlockRef break_bb,
                       LLVMBasicBlockRef cont_bb) {
  switch (stmt->vtable->kind) {
    case SK_ExprStmt:
      compile_expr(compiler, builder, ((const ExprStmt *)stmt)->expr, local_ctx,
                   local_allocas);
      return;
    case SK_IfStmt:
      return compile_if_statement(compiler, builder, (const IfStmt *)stmt,
                                  local_ctx, local_allocas, break_bb, cont_bb);
    case SK_ForStmt:
      return compile_for_statement(compiler, builder, (const ForStmt *)stmt,
                                   local_ctx, local_allocas, break_bb, cont_bb);
    case SK_ReturnStmt: {
      const ReturnStmt *ret = (const ReturnStmt *)stmt;
      if (!ret->expr) {
        LLVMBuildRetVoid(builder);
        return;
      }
      LLVMValueRef val =
          compile_expr(compiler, builder, ret->expr, local_ctx, local_allocas);

      // TODO: There should be an ImplicitCast AST node that we can parser over
      // rather than doing this here.

      const Type *expr_ty =
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
    case SK_CompoundStmt: {
      TreeMap local_ctx_cpy;
      TreeMap local_allocas_cpy;
      string_tree_map_construct(&local_ctx_cpy);
      string_tree_map_construct(&local_allocas_cpy);
      tree_map_clone(&local_ctx_cpy, local_ctx);
      tree_map_clone(&local_allocas_cpy, local_allocas);

      const CompoundStmt *compound = (const CompoundStmt *)stmt;
      for (size_t i = 0; i < compound->body.size; ++i) {
        Statement *stmt = *(Statement **)vector_at(&compound->body, i);
        compile_statement(compiler, builder, stmt, &local_ctx_cpy,
                          &local_allocas_cpy, break_bb, cont_bb);
        if (last_instruction_is_terminator(builder))
          break;
      }

      tree_map_destroy(&local_ctx_cpy);
      tree_map_destroy(&local_allocas_cpy);
      return;
    }
    case SK_Declaration: {
      const Declaration *decl = (const Declaration *)stmt;

      LLVMTypeRef llvm_ty = NULL;
      if (sema_is_array_type(compiler->sema, decl->type, local_ctx)) {
        const ArrayType *arr_ty =
            sema_get_array_type(compiler->sema, decl->type, local_ctx);
        if (!arr_ty->size) {
          // Get the type from the initializer.
          assert(decl->initializer);

          if (decl->initializer->vtable->kind == EK_InitializerList) {
            // Since this is an array, we can just get the first element.
            const InitializerList *init =
                (const InitializerList *)decl->initializer;

            assert(init->elems.size > 0);
            InitializerListElem *first_elem = vector_at(&init->elems, 0);
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
        llvm_ty = get_llvm_type(compiler, decl->type);

      LLVMValueRef alloca =
          build_alloca_at_func_start(builder, decl->name, llvm_ty);

      if (decl->initializer) {
        if (decl->initializer->vtable->kind == EK_InitializerList) {
          const InitializerList *init =
              (const InitializerList *)decl->initializer;

          size_t agg_size;
          bool is_array =
              sema_is_array_type(compiler->sema, decl->type, local_ctx);
          if (is_array) {
            const ArrayType *arr_ty =
                sema_get_array_type(compiler->sema, decl->type, local_ctx);
            if (arr_ty->size) {
              agg_size = sema_eval_sizeof_type(compiler->sema, decl->type);
            } else {
              assert(init->elems.size > 0);
              agg_size =
                  sema_eval_sizeof_type(compiler->sema, arr_ty->elem_type) *
                  init->elems.size;
            }
          } else {
            agg_size = sema_eval_sizeof_type(compiler->sema, decl->type);
          }
          LLVMBuildMemSet(
              builder, alloca, LLVMConstNull(LLVMInt8Type()),
              LLVMConstInt(LLVMInt32Type(), agg_size, /*IsSigned=*/0),
              /*Align=*/0);

          for (size_t i = 0; i < init->elems.size; ++i) {
            const InitializerListElem *elem = vector_at(&init->elems, i);
            LLVMValueRef val = compile_expr(compiler, builder, elem->expr,
                                            local_ctx, local_allocas);

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
          LLVMValueRef init =
              compile_implicit_cast(compiler, builder, decl->initializer,
                                    decl->type, local_ctx, local_allocas);
          LLVMBuildStore(builder, init, alloca);
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
      const SwitchStmt *switch_stmt = (const SwitchStmt *)stmt;
      compile_switch_statement(compiler, builder, switch_stmt, local_ctx,
                               local_allocas, break_bb, cont_bb);
      return;
    }
    default:
      printf("TODO: Implement compile_statement for this stmt %d\n",
             stmt->vtable->kind);
      __builtin_trap();
  }
}

void compile_function_definition(Compiler *compiler,
                                 const FunctionDefinition *f) {
  LLVMTypeRef llvm_func_ty = get_llvm_type(compiler, f->type);

  LLVMValueRef func = get_named_global(compiler, f->name);
  if (!func)
    func = LLVMAddFunction(compiler->mod, f->name, llvm_func_ty);
  assert(LLVMGetTypeKind(LLVMTypeOf(func)) == LLVMPointerTypeKind);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, entry);

  // LLVMMetadataRef subprogram = LLVMDIBuilderCreateFunction(
  //   compiler->dibuilder, /*Scope=*/NULL, f->name, strlen(f->name),
  //   f->name, strlen(f->name), compiler->difile, );

  FunctionType *func_ty = (FunctionType *)(f->type);

  TreeMap local_ctx;
  TreeMap local_allocas;
  string_tree_map_construct(&local_ctx);
  string_tree_map_construct(&local_allocas);

  for (size_t i = 0; i < func_ty->pos_args.size; ++i) {
    FunctionArg *arg = vector_at(&func_ty->pos_args, i);
    if (!arg->name)
      continue;

    // Copy the parameter locally.
    LLVMValueRef llvm_arg = LLVMGetParam(func, (unsigned)i);
    LLVMValueRef alloca =
        build_alloca_at_func_start(builder, arg->name, LLVMTypeOf(llvm_arg));
    LLVMBuildStore(builder, llvm_arg, alloca);

    tree_map_set(&local_ctx, arg->name, arg->type);
    tree_map_set(&local_allocas, arg->name, alloca);
  }

  compile_statement(compiler, builder, &f->body->base, &local_ctx,
                    &local_allocas, NULL, NULL);

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

LLVMValueRef compile_lvalue_ptr(Compiler *compiler, LLVMBuilderRef builder,
                                const Expr *expr, TreeMap *local_ctx,
                                TreeMap *local_allocas) {
  switch (expr->vtable->kind) {
    case EK_DeclRef: {
      const DeclRef *decl = (const DeclRef *)expr;

      LLVMValueRef val = NULL;
      tree_map_get(local_allocas, decl->name, (void *)&val);

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
      const MemberAccess *access = (const MemberAccess *)expr;
      const StructType *base_ty = sema_get_struct_type_from_member_access(
          compiler->sema, access, local_ctx);
      size_t offset;
      sema_get_struct_member(compiler->sema, base_ty, access->member, &offset);

      LLVMValueRef base_llvm;
      if (access->is_arrow) {
        base_llvm = compile_expr(compiler, builder, access->base, local_ctx,
                                 local_allocas);
      } else {
        base_llvm = compile_lvalue_ptr(compiler, builder, access->base,
                                       local_ctx, local_allocas);
      }

      LLVMTypeRef llvm_base_ty = get_llvm_type(compiler, &base_ty->type);
      LLVMValueRef llvm_offset =
          LLVMConstInt(LLVMInt32Type(), offset, /*signed=*/0);
      LLVMValueRef offsets[] = {LLVMConstNull(LLVMInt32Type()), llvm_offset};
      LLVMValueRef gep =
          LLVMBuildGEP2(builder, llvm_base_ty, base_llvm, offsets, 2, "");
      return gep;
    }
    case EK_UnOp:
      return compile_unop_lvalue_ptr(compiler, builder, (const UnOp *)expr,
                                     local_ctx, local_allocas);
    case EK_Cast: {
      const Cast *cast = (const Cast *)expr;
      const Type *src_ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, expr, local_ctx);
      const Type *dst_ty = cast->to;
      LLVMTypeRef llvm_src_ty = get_llvm_type(compiler, src_ty);
      LLVMTypeRef llvm_dst_ty = get_llvm_type(compiler, dst_ty);
      if (LLVMGetTypeKind(llvm_src_ty) == LLVMGetTypeKind(llvm_dst_ty))
        return compile_lvalue_ptr(compiler, builder, cast->base, local_ctx,
                                  local_allocas);

      printf("Unhandled lvalue ptr cast from %d to %d\n",
             LLVMGetTypeKind(llvm_src_ty), LLVMGetTypeKind(llvm_dst_ty));
      __builtin_trap();
    }
    case EK_Index: {
      const Index *index = (const Index *)expr;
      const Expr *base = index->base;
      const Type *base_ty =
          sema_get_type_of_expr_in_ctx(compiler->sema, base, local_ctx);

      LLVMValueRef idx =
          compile_expr(compiler, builder, index->idx, local_ctx, local_allocas);

      LLVMValueRef base_ptr =
          compile_expr(compiler, builder, base, local_ctx, local_allocas);

      assert(LLVMGetTypeKind(LLVMTypeOf(base_ptr)) == LLVMPointerTypeKind);

      if (sema_is_pointer_type(compiler->sema, base_ty, local_ctx)) {
        LLVMValueRef offsets[] = {idx};

        const Type *pointee_ty =
            sema_get_pointee(compiler->sema, base_ty, local_ctx);
        LLVMTypeRef llvm_pointee_ty = get_llvm_type(compiler, pointee_ty);
        return LLVMBuildGEP2(builder, llvm_pointee_ty, base_ptr, offsets, 1,
                             "idx_ptr");
      } else if (sema_is_array_type(compiler->sema, base_ty, local_ctx)) {
        // TODO: This could probably be consolidated with the branch above.

        LLVMValueRef offsets[] = {idx};

        const Type *elem_ty =
            sema_get_array_type(compiler->sema, base_ty, local_ctx)->elem_type;

        LLVMTypeRef llvm_elem_ty = get_llvm_type(compiler, elem_ty);
        return LLVMBuildGEP2(builder, llvm_elem_ty, base_ptr, offsets, 1,
                             "idx_arr");
      }

      printf("Indexing non-ptr or arr type\n");
      __builtin_trap();
    }
    default:
      printf("TODO: Handle lvalue for EK %d\n", expr->vtable->kind);
      __builtin_trap();
  }
}

void compile_global_variable(Compiler *compiler, const GlobalVariable *gv) {
  LLVMTypeRef ty = get_llvm_type(compiler, gv->type);

  if (gv->type->vtable->kind == TK_FunctionType) {
    if (!get_named_global(compiler, gv->name)) {
      LLVMAddFunction(compiler->mod, gv->name, ty);
      assert(!gv->initializer &&
             "If this had an initializer, it would be a function definition.");
    }
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
  RunStringTests();
  RunVectorTests();
  RunTreeMapTests();
  RunParserTests();

  if (argc < 2) {
    printf("Usage: %s input_file\n", argv[0]);
    return -1;
  }

  const char *input_filename = argv[1];

  // LLVM Initialization
  LLVMModuleRef mod = LLVMModuleCreateWithName(input_filename);
  LLVMDIBuilderRef dibuilder = LLVMCreateDIBuilder(mod);

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
  compiler_construct(&compiler, mod, &sema, dibuilder);

  vector nodes_to_destroy;
  vector_construct(&nodes_to_destroy, sizeof(Node *), alignof(Node *));

  for (;;) {
    const Token *token = parser_peek_token(&parser);
    if (token->kind == TK_Eof) {
      break;
    } else {
      // printf("Evaluating top level node at %llu:%llu (%s)\n", token->line,
      //        token->col, token->chars.data);

      // Note that the parse_* functions destroy the tokens.
      Node *top_level_decl = parse_top_level_decl(&parser);
      assert(top_level_decl);

      switch (top_level_decl->vtable->kind) {
        case NK_Typedef: {
          Typedef *td = (Typedef *)top_level_decl;
          sema_add_typedef_type(&sema, td->name, td->type);
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
        case NK_StructDeclaration: {
          StructDeclaration *decl = (StructDeclaration *)top_level_decl;
          sema_handle_struct_declaration(&sema, decl);
          break;
        }
        case NK_EnumDeclaration: {
          EnumDeclaration* decl = (EnumDeclaration*)top_level_decl;
          sema_handle_enum_declaration(&sema, decl);
          break;
        }
      }

      Node **storage = vector_append_storage(&nodes_to_destroy);
      *storage = top_level_decl;

      if (LLVMVerifyModule(mod, LLVMPrintMessageAction, NULL)) {
        printf("Verify module failed\n");
        LLVMDumpModule(mod);
        __builtin_trap();
      }
    }
  }

  if (LLVMPrintModuleToFile(mod, "out.ll", &error)) {
    printf("llvm error: %s\n", error);
    LLVMDisposeMessage(error);
    return -1;
  }
  if (LLVMTargetMachineEmitToFile(target_machine, mod, "out.obj",
                                  LLVMObjectFile, &error)) {
    printf("llvm error: %s\n", error);
    LLVMDisposeMessage(error);
    return -1;
  }

  for (size_t i = 0; i < nodes_to_destroy.size; ++i) {
    Node *node = *(Node **)vector_at(&nodes_to_destroy, i);
    node_destroy(node);
    free(node);
  }
  vector_destroy(&nodes_to_destroy);

  compiler_destroy(&compiler);
  sema_destroy(&sema);
  parser_destroy(&parser);
  input_stream_destroy(&input.base);

  LLVMDisposeModule(mod);
  LLVMDisposeTargetData(data_layout);
  LLVMDisposeTargetMachine(target_machine);
  LLVMDisposeDIBuilder(dibuilder);

  return 0;
}
