
#ifndef __tree_h__
#define __tree_h__


#include "oper.h"

#include <stddef.h>


typedef enum ExprKind {
    EXPR_ATTR,
    EXPR_BINARY,
    EXPR_INT,
    EXPR_NAME,
    EXPR_POSTFIX,
    EXPR_SELF,
    EXPR_SUPER
} ExprKind;

typedef enum StmtKind {
    STMT_COMPOUND,
    STMT_DEF,
    STMT_DEF_CLASS,
    STMT_EXPR,
    STMT_IF_ELSE,
    STMT_RETURN
} StmtKind;


typedef struct Expr Expr;
typedef struct ExprList ExprList;
typedef struct Stmt Stmt;
typedef struct StmtList StmtList;

struct Expr {
    ExprKind kind;
    Oper oper;
    Expr *next, *cond, *left, *right;
    struct SymbolNode *sym;
    long intValue;
    union {
        struct {
            Expr *def;
            Expr *nextUndef;
            unsigned int level;
        } ref;
        struct {
            unsigned int level;
            unsigned int index;
            Expr *nextMultipleDef;
        } def;
    } u;
    unsigned int codeOffset;
};

struct ExprList {
    Expr *first, *last;
};

struct Stmt {
    StmtKind kind;
    Stmt *next, *top, *bottom;
    Expr *expr;
    union {
        struct {
            struct SymbolNode *name;
            Expr *argList;
            size_t argumentCount;
            size_t localCount;
        } method;
        struct {
            Expr *super;
        } klass;
    } u;
    unsigned int codeOffset;
};

struct StmtList {
    Stmt *first, *last;
};


#endif /* __tree_h__ */
