
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
    Spk_EXPR_SYMBOL,
    Spk_EXPR_UNARY
} SpkExprKind;

typedef enum SpkStmtKind {
    Spk_STMT_BREAK,
    Spk_STMT_COMPOUND,
    Spk_STMT_CONTINUE,
    Spk_STMT_DEF_ARG,
    Spk_STMT_DEF_CLASS,
    Spk_STMT_DEF_METHOD,
    Spk_STMT_DEF_VAR,
    Spk_STMT_DO_WHILE,
    Spk_STMT_EXPR,
    Spk_STMT_FOR,
    Spk_STMT_IF_ELSE,
    Spk_STMT_IMPORT,
    Spk_STMT_RAISE,
    Spk_STMT_RETURN,
    Spk_STMT_WHILE,
    Spk_STMT_YIELD
} SpkStmtKind;


typedef struct SpkExpr SpkExpr;
typedef struct SpkExprList SpkExprList;
typedef struct SpkArgList SpkArgList;
typedef struct SpkStmt SpkStmt;
typedef struct SpkStmtList SpkStmtList;
typedef struct SpkStmtPair SpkStmtPair;

struct SpkExpr {
    SpkObject base;
    SpkExprKind kind;
    SpkOper oper;
    SpkExpr *next, *nextArg, *cond, *left, *right, *var;
    struct SpkSymbolNode *sym;
    union {
        SpkUnknown *literalValue;
        struct {
            SpkStmt *stmtList;
            size_t argumentCount;
            size_t localCount;
        } block;
        SpkUnknown *keywords;
    } aux;
    union {
        struct {
            SpkExpr *def;
            SpkExpr *nextUndef;
            unsigned int level;
        } ref;
        struct {
            /* XXX: These three should be shared. */
            unsigned int level, /*SpkOpcode*/ pushOpcode, storeOpcode;
            unsigned int index;
            int weak;
            SpkExpr *nextMultipleDef;
            SpkStmt *stmt;
            SpkUnknown *initValue; /* XXX: Ugly? */
        } def;
    } u;
    size_t codeOffset;
    size_t endLabel;
};

struct SpkExprList {
    SpkExpr *first, *last;
};

struct SpkArgList {
    SpkExpr *fixed, *var;
};

struct SpkStmt {
    SpkObject base;
    SpkStmtKind kind;
    SpkStmt *next, *top, *bottom;
    SpkExpr *init, *expr, *incr;
    union {
        struct {
            unsigned int namespace;
            struct SpkSymbolNode *name;
            struct { SpkExpr *fixed, *var; } argList;
            size_t argumentCount;
            size_t localCount;
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
    } u;
    size_t codeOffset;
};

struct SpkStmtList {
    SpkStmt *first, *last;
};

struct SpkStmtPair {
    SpkStmt *top, *bottom;
};


typedef struct SpkExprSubclass {
    SpkExpr base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkExprSubclass;

typedef struct SpkStmtSubclass {
    SpkStmt base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkStmtSubclass;


extern struct SpkBehavior *Spk_ClassExpr, *Spk_ClassStmt;
extern struct SpkClassTmpl Spk_ClassExprTmpl, Spk_ClassStmtTmpl;


#endif /* __spk_tree_h__ */
