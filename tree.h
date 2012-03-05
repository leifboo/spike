
#ifndef __tree_h__
#define __tree_h__


#include "obj.h"
#include "oper.h"


typedef enum ExprKind {
    EXPR_AND,
    EXPR_ASSIGN,
    EXPR_ATTR,
    EXPR_ATTR_VAR,
    EXPR_BINARY,
    EXPR_BLOCK,
    EXPR_CALL,
    EXPR_COMPOUND,
    EXPR_COND,
    EXPR_ID,
    EXPR_KEYWORD,
    EXPR_LITERAL,
    EXPR_NAME,
    EXPR_NI,
    EXPR_OR,
    EXPR_POSTOP,
    EXPR_PREOP,
    EXPR_UNARY
} ExprKind;

typedef enum StmtKind {
    STMT_BREAK,
    STMT_COMPOUND,
    STMT_CONTINUE,
    STMT_DEF_CLASS,
    STMT_DEF_METHOD,
    STMT_DEF_MODULE,
    STMT_DEF_SPEC,
    STMT_DEF_VAR,
    STMT_DO_WHILE,
    STMT_EXPR,
    STMT_FOR,
    STMT_IF_ELSE,
    STMT_PRAGMA_SOURCE,
    STMT_RETURN,
    STMT_WHILE,
    STMT_YIELD
} StmtKind;

typedef enum ExprNameKind {
    EXPR_NAME_UNK,
    EXPR_NAME_DEF,
    EXPR_NAME_REF
} ExprNameKind;

enum {
    SPEC_TYPE           = 0x0000000F,
    SPEC_TYPE_OBJ       = 0x00000001,
    SPEC_TYPE_INT       = 0x00000002,
    SPEC_TYPE_CHAR      = 0x00000003,
    
    SPEC_STORAGE        = 0x000000F0,
    SPEC_STORAGE_IMPORT = 0x00000010,
    SPEC_STORAGE_EXPORT = 0x00000020,
    SPEC_STORAGE_EXTERN = 0x00000030,
    
    SPEC_CALL_CONV      = 0x00000F00,
    SPEC_CALL_CONV_C    = 0x00000100
};


typedef unsigned int Label;


typedef struct Expr Expr, XExpr;
typedef struct ExprList ExprList;
typedef struct ArgList ArgList;
typedef struct Stmt Stmt;
typedef struct StmtList StmtList;
typedef struct StmtPair StmtPair;


struct ExprList {
    Expr *first, *last;
};

struct ArgList {
    Expr *fixed, *var;
};

struct StmtList {
    Stmt *first, *last;
    Expr *expr;
};

struct StmtPair {
    Stmt *top, *bottom;
};


struct Expr {
    Object base;
    ExprKind kind;
    Oper oper;
    Expr *declSpecs; unsigned int specifiers;
    Expr *next, *nextArg, *cond, *left, *right, *var;
    struct SymbolNode *sym;
    unsigned int lineNo;
    union {
        Unknown *literalValue;
        struct {
            Expr *argList;
            Stmt *stmtList;
            size_t argumentCount;
            size_t localCount;
        } block;
        Unknown *keywords;
        ExprNameKind nameKind;
    } aux;
    union {
        struct {
            Expr *def;
            unsigned int level;
        } ref;
        struct {
            /* XXX: These three should be shared. */
            unsigned int level, /*Opcode*/ pushOpcode, storeOpcode;
            unsigned int index;
            int weak;
            Stmt *stmt;
            void *code;
            Unknown *initValue; /* XXX: Ugly? */
        } def;
    } u;
    size_t codeOffset, endOffset;
    Label label, endLabel;
};


struct Stmt {
    Object base;
    StmtKind kind;
    Stmt *next, *top, *bottom;
    Expr *init, *expr, *incr;
    union {
        struct {
            unsigned int ns;
            struct SymbolNode *name;
            struct { Expr *fixed, *var; } argList;
            size_t minArgumentCount;
            size_t maxArgumentCount;
            size_t localCount;
            unsigned int blockCount;
        } method;
        struct {
            Expr *superclassName;
            Stmt *superclassDef;
            Stmt *firstSubclassDef, *lastSubclassDef;
            Stmt *nextSubclassDef;
            Stmt *nextRootClassDef;
            size_t instVarCount;
            size_t classVarCount;
            int predefined;
        } klass;
        struct {
            unsigned int dataSize;
            StmtList predefList, rootClassList;
        } module;
        struct {
            unsigned int mask;
            unsigned int value;
        } spec;
        struct String *source;
    } u;
    size_t codeOffset;
    Label label;
};


extern struct ClassTmpl ClassXExprTmpl, ClassXStmtTmpl;


#define IS_EXTERN(e) (((e)->specifiers & SPEC_STORAGE) == SPEC_STORAGE_EXTERN)
#define IS_CDECL(e) (((e)->specifiers & SPEC_CALL_CONV) == SPEC_CALL_CONV_C)


#endif /* __tree_h__ */
