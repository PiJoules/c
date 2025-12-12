#include "ast-dump.h"

#include <stdio.h>

#include "common.h"
#include "expr.h"
#include "top-level-node.h"
#include "type.h"
#include "vector.h"

static const char* kPadding = "  ";

static void print_padding(int n) {
  while (n--) printf("%s", kPadding);
}

//
// NOTE: All dump_* functions must finish by printing a newline.
// All dump_* functions will start by printing padding.
//

static void dump_type(const Type* type, int leading_padding,
                      const char* prefix) {
  print_padding(leading_padding);
  printf("%s", prefix);

  switch (type->vtable->kind) {
    case TK_BuiltinType: {
      const BuiltinType* bt = (const BuiltinType*)type;
      switch (bt->kind) {
        case BTK_Char:
          printf("BuiltinType kind:char\n");
          break;
        case BTK_SignedChar:
          printf("BuiltinType kind:signed char\n");
          break;
        case BTK_UnsignedChar:
          printf("BuiltinType kind:unsigned char\n");
          break;
        case BTK_Short:
          printf("BuiltinType kind:short\n");
          break;
        case BTK_UnsignedShort:
          printf("BuiltinType kind:unsigned short\n");
          break;
        case BTK_Int:
          printf("BuiltinType kind:int\n");
          break;
        case BTK_UnsignedInt:
          printf("BuiltinType kind:unsigned int\n");
          break;
        case BTK_Long:
          printf("BuiltinType kind:long\n");
          break;
        case BTK_UnsignedLong:
          printf("BuiltinType kind:unsigned long\n");
          break;
        case BTK_LongLong:
          printf("BuiltinType kind:long long\n");
          break;
        case BTK_UnsignedLongLong:
          printf("BuiltinType kind:unsigned long long\n");
          break;
        case BTK_Float:
          printf("BuiltinType kind:float\n");
          break;
        case BTK_Double:
          printf("BuiltinType kind:double\n");
          break;
        case BTK_LongDouble:
          printf("BuiltinType kind:long double\n");
          break;
        case BTK_Float128:
          printf("BuiltinType kind:float128\n");
          break;
        case BTK_ComplexFloat:
          printf("BuiltinType kind:complex float\n");
          break;
        case BTK_ComplexDouble:
          printf("BuiltinType kind:complex double\n");
          break;
        case BTK_ComplexLongDouble:
          printf("BuiltinType kind:complex long\n");
          break;
        case BTK_Void:
          printf("BuiltinType kind:void\n");
          break;
        case BTK_Bool:
          printf("BuiltinType kind:bool\n");
          break;
        case BTK_BuiltinVAList:
          printf("BuiltinType kind:builtin va list\n");
          break;
      }
      break;
    }
    case TK_EnumType:
      UNREACHABLE_MSG("TODO: Handle this");
    case TK_NamedType:
      UNREACHABLE_MSG("TODO: Handle this");
    case TK_StructType:
      UNREACHABLE_MSG("TODO: Handle this");
    case TK_UnionType:
      UNREACHABLE_MSG("TODO: Handle this");
    case TK_PointerType: {
      const PointerType* pt = (const PointerType*)type;

      printf("PointerType\n");
      leading_padding++;

      dump_type(pt->pointee, leading_padding, "pointee: ");

      leading_padding--;

      break;
    }
    case TK_ArrayType:
      UNREACHABLE_MSG("TODO: Handle this");
    case TK_FunctionType: {
      const FunctionType* func_ty = (const FunctionType*)type;
      printf("FunctionType (%p) has_var_args:%d\n", func_ty,
             func_ty->has_var_args);
      leading_padding++;

      dump_type(func_ty->return_type, leading_padding, "return_type: ");

      const FunctionArg* start = vector_begin(&func_ty->pos_args);
      for (const FunctionArg* it = start; it != vector_end(&func_ty->pos_args);
           ++it) {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        if (it->name) {
          sprintf(buffer, "arg (%s): ", it->name);
        } else {
          sprintf(buffer, "arg: ");
        }
        dump_type(it->type, leading_padding, buffer);
      }

      leading_padding--;
      break;
    }
    case TK_ReplacementSentinelType:
      UNREACHABLE_MSG("This type should not be handled here.");
  }
}

static void dump_expr(const Expr* expr, int leading_padding,
                      const char* prefix) {
  switch (expr->vtable->kind) {
    case EK_DeclRef: {
      // const DeclRef* decl = (const DeclRef*)expr;
      UNREACHABLE_MSG("TODO: Handle this");
    }
    case EK_SizeOf: {
      UNREACHABLE_MSG("TODO: Handle this");
    }
    case EK_AlignOf: {
      UNREACHABLE_MSG("TODO: Handle this");
    }
    case EK_Conditional: {
      UNREACHABLE_MSG("TODO: Handle this");
    }
    case EK_PrettyFunction: {
      UNREACHABLE_MSG("TODO: Handle this");
    }
    case EK_String: {
      // const StringLiteral* s = (const StringLiteral*)expr;
      UNREACHABLE_MSG("TODO: Handle this");
    }
    case EK_Bool: {
      UNREACHABLE_MSG("TODO: Handle this");
    }
    case EK_Char: {
      UNREACHABLE_MSG("TODO: Handle this");
    }
    case EK_Int: {
      UNREACHABLE_MSG("TODO: Handle this");
    }
    case EK_BinOp: {
      UNREACHABLE_MSG("TODO: Handle this");
    }
    case EK_Cast: {
      // const Cast* cast = (const Cast*)expr;
      UNREACHABLE_MSG("TODO: Handle this");
    }
    case EK_InitializerList: {
      // const InitializerList* init = (const InitializerList*)expr;
      UNREACHABLE_MSG("TODO: Handle this");
    }
    case EK_UnOp:
      UNREACHABLE_MSG("TODO: Handle this");
    case EK_Index:
      UNREACHABLE_MSG("TODO: Handle this");
    case EK_MemberAccess:
      UNREACHABLE_MSG("TODO: Handle this");
    case EK_Call:
      UNREACHABLE_MSG("TODO: Handle this");
    case EK_FunctionParam:
      UNREACHABLE_MSG("TODO: Handle this");
    case EK_StmtExpr:
      UNREACHABLE_MSG("TODO: Handle this");
  }
}

static void dump_top_level_node(const TopLevelNode* node, int leading_padding) {
  switch (node->vtable->kind) {
    case TLNK_Typedef: {
      // Typedef* td = (Typedef*)node;
      UNREACHABLE_MSG("TODO: Handle this");
      break;
    }
    case TLNK_StaticAssert: {
      // StaticAssert* sa = (StaticAssert*)node;
      UNREACHABLE_MSG("TODO: Handle this");
      break;
    }
    case TLNK_GlobalVariable: {
      GlobalVariable* gv = (GlobalVariable*)node;

      print_padding(leading_padding);
      printf("GlobalVariable (%p) name:%s is_extern:%d is_thread_local:%d\n",
             gv, gv->name, gv->is_extern, gv->is_thread_local);
      ++leading_padding;

      dump_type(gv->type, leading_padding, /*prefix=*/"type: ");
      if (gv->initializer) {
        dump_expr(gv->initializer, leading_padding, /*prefix=*/"initializer: ");
      } else {
        print_padding(leading_padding);
        printf("initializer: NONE\n");
      }

      --leading_padding;

      break;
    }
    case TLNK_FunctionDefinition: {
      // FunctionDefinition* f = (FunctionDefinition*)node;
      UNREACHABLE_MSG("TODO: Handle this");
      break;
    }
    case TLNK_StructDeclaration: {
      // StructDeclaration* decl = (StructDeclaration*)node;
      UNREACHABLE_MSG("TODO: Handle this");
      break;
    }
    case TLNK_EnumDeclaration: {
      // EnumDeclaration* decl = (EnumDeclaration*)node;
      UNREACHABLE_MSG("TODO: Handle this");
      break;
    }
    case TLNK_UnionDeclaration: {
      // UnionDeclaration* decl = (UnionDeclaration*)node;
      UNREACHABLE_MSG("TODO: Handle this");
      break;
    }
  }
}

static void dump_ast_impl(const vector* ast_nodes, int leading_padding) {
  for (const TopLevelNode** it = vector_begin(ast_nodes);
       it != vector_end(ast_nodes); ++it) {
    dump_top_level_node(*it, leading_padding);
  }
}

void dump_ast(const vector* ast_nodes) {
  dump_ast_impl(ast_nodes, /*leading_padding=*/0);
}
