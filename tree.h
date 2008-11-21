
#ifndef __tree_h__
#define __tree_h__


#include "oper.h"

#include <stddef.h>


typedef enum ExprKind {
    EXPR_AND,
    EXPR_ASSIGN,
    EXPR_ATTR,
    EXPR_BINARY,
    EXPR_CHAR,
    EXPR_COND,
    EXPR_ID,
    EXPR_INT,
    EXPR_NAME,
    EXPR_NI,
    EXPR_OR,
    EXPR_POSTFIX,
    EXPR_POSTOP,
    EXPR_PREOP,
    EXPR_STR,
    EXPR_UNARY
} ExprKind;

typedef enum StmtKind {
    STMT_BREAK,
    STMT_COMPOUND,
    STMT_CONTINUE,
    STMT_DEF_CLASS,
    STMT_DEF_METHOD,
    STMT_DEF_VAR,
    STMT_DO_WHILE,
    STMT_EXPR,
    STMT_FOR,
    STMT_IF_ELSE,
    STMT_RETURN,
    STMT_WHILE
} StmtKind;


typedef struct Expr Expr;
typedef struct ExprList ExprList;
typedef struct Stmt Stmt;
typedef struct StmtList StmtList;

struct Expr {
    ExprKind kind;
    Oper oper;
    Expr *next, *nextArg, *cond, *left, *right;
    struct SymbolNode *sym;
    long intValue;
    struct Char *charValue;
    struct String *strValue;
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
            Stmt *stmt;
            struct Object *initValue; /* XXX: Ugly? */
            /*opcode_t*/ unsigned int pushOpcode;
        } def;
    } u;
    size_t codeOffset;
    size_t endLabel;
};

struct ExprList {
    Expr *first, *last;
};

struct Stmt {
    StmtKind kind;
    Stmt *next, *top, *bottom;
    Expr *init, *expr, *incr;
    union {
        struct {
            struct SymbolNode *name;
            Expr *argList;
            size_t argumentCount;
            size_t localCount;
        } method;
        struct {
            Expr *super;
            size_t instVarCount;
        } klass;
    } u;
    size_t codeOffset;
};

struct StmtList {
    Stmt *first, *last;
};


#endif /* __tree_h__ */
