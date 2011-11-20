
#ifndef __spk_tree_h__
#define __spk_tree_h__


#include "obj.h"
#include "oper.h"


typedef enum SpkExprKind {
    Spk_EXPR_AND,
    Spk_EXPR_ASSIGN,
    Spk_EXPR_ATTR,
    Spk_EXPR_ATTR_VAR,
    Spk_EXPR_BINARY,
    Spk_EXPR_BLOCK,
    Spk_EXPR_CALL,
    Spk_EXPR_COMPOUND,
    Spk_EXPR_COND,
    Spk_EXPR_ID,
    Spk_EXPR_KEYWORD,
    Spk_EXPR_LITERAL,
    Spk_EXPR_NAME,
    Spk_EXPR_NI,
    Spk_EXPR_OR,
    Spk_EXPR_POSTOP,
    Spk_EXPR_PREOP,
    Spk_EXPR_UNARY
} SpkExprKind;

typedef enum SpkStmtKind {
    Spk_STMT_BREAK,
    Spk_STMT_COMPOUND,
    Spk_STMT_CONTINUE,
    Spk_STMT_DEF_CLASS,
    Spk_STMT_DEF_METHOD,
    Spk_STMT_DEF_MODULE,
    Spk_STMT_DEF_SPEC,
    Spk_STMT_DEF_VAR,
    Spk_STMT_DO_WHILE,
    Spk_STMT_EXPR,
    Spk_STMT_FOR,
    Spk_STMT_IF_ELSE,
    Spk_STMT_PRAGMA_SOURCE,
    Spk_STMT_RETURN,
    Spk_STMT_WHILE,
    Spk_STMT_YIELD
} SpkStmtKind;

typedef enum SpkExprNameKind {
    Spk_EXPR_NAME_UNK,
    Spk_EXPR_NAME_DEF,
    Spk_EXPR_NAME_REF
} SpkExprNameKind;

enum {
    Spk_SPEC_TYPE           = 0x0000000F,
    Spk_SPEC_TYPE_OBJ       = 0x00000001,
    Spk_SPEC_TYPE_INT       = 0x00000002,
    Spk_SPEC_TYPE_CHAR      = 0x00000003,
    
    Spk_SPEC_STORAGE        = 0x000000F0,
    Spk_SPEC_STORAGE_IMPORT = 0x00000010,
    Spk_SPEC_STORAGE_EXPORT = 0x00000020,
    Spk_SPEC_STORAGE_EXTERN = 0x00000030,
    
    Spk_SPEC_CALL_CONV      = 0x00000F00,
    Spk_SPEC_CALL_CONV_C    = 0x00000100
};


typedef unsigned int Label;


typedef struct SpkExpr SpkExpr, SpkXExpr;
typedef struct SpkExprList SpkExprList;
typedef struct SpkArgList SpkArgList;
typedef struct SpkStmt SpkStmt;
typedef struct SpkStmtList SpkStmtList;
typedef struct SpkStmtPair SpkStmtPair;


struct SpkExprList {
    SpkExpr *first, *last;
};

struct SpkArgList {
    SpkExpr *fixed, *var;
};

struct SpkStmtList {
    SpkStmt *first, *last;
    SpkExpr *expr;
};

struct SpkStmtPair {
    SpkStmt *top, *bottom;
};


struct SpkExpr {
    SpkObject base;
    SpkExprKind kind;
    SpkOper oper;
    SpkExpr *declSpecs; unsigned int specifiers;
    SpkExpr *next, *nextArg, *cond, *left, *right, *var;
    struct SpkSymbolNode *sym;
    unsigned int lineNo;
    union {
        SpkUnknown *literalValue;
        struct {
            SpkExpr *argList;
            SpkStmt *stmtList;
            size_t argumentCount;
            size_t localCount;
        } block;
        SpkUnknown *keywords;
        SpkExprNameKind nameKind;
    } aux;
    union {
        struct {
            SpkExpr *def;
            unsigned int level;
        } ref;
        struct {
            /* XXX: These three should be shared. */
            unsigned int level, /*SpkOpcode*/ pushOpcode, storeOpcode;
            unsigned int index;
            int weak;
            SpkStmt *stmt;
            void *code;
            SpkUnknown *initValue; /* XXX: Ugly? */
        } def;
    } u;
    size_t codeOffset, endOffset;
    Label label, endLabel;
};


struct SpkStmt {
    SpkObject base;
    SpkStmtKind kind;
    SpkStmt *next, *top, *bottom;
    SpkExpr *init, *expr, *incr;
    union {
        struct {
            unsigned int ns;
            struct SpkSymbolNode *name;
            struct { SpkExpr *fixed, *var; } argList;
            size_t minArgumentCount;
            size_t maxArgumentCount;
            size_t localCount;
            unsigned int blockCount;
        } method;
        struct {
            SpkExpr *superclassName;
            SpkStmt *superclassDef;
            SpkStmt *firstSubclassDef, *lastSubclassDef;
            SpkStmt *nextSubclassDef;
            SpkStmt *nextRootClassDef;
            size_t instVarCount;
            size_t classVarCount;
            int predefined;
        } klass;
        struct {
            unsigned int dataSize;
            SpkStmtList predefList, rootClassList;
        } module;
        struct {
            unsigned int mask;
            unsigned int value;
        } spec;
        SpkUnknown *source;
    } u;
    size_t codeOffset;
    Label label;
};


extern struct SpkClassTmpl Spk_ClassXExprTmpl, Spk_ClassXStmtTmpl;


#define IS_EXTERN(e) (((e)->specifiers & Spk_SPEC_STORAGE) == Spk_SPEC_STORAGE_EXTERN)
#define IS_CDECL(e) (((e)->specifiers & Spk_SPEC_CALL_CONV) == Spk_SPEC_CALL_CONV_C)


#endif /* __spk_tree_h__ */
