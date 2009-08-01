
#include "tree.h"

#include "class.h"
#include <string.h>


SpkBehavior *Spk_ClassExpr, *Spk_ClassStmt;


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Expr_zero(SpkObject *_self) {
    SpkExpr *self = (SpkExpr *)_self;
    size_t baseSize = offsetof(SpkExpr, kind);
    (*Spk_ClassExpr->superclass->zero)(_self);
    memset((char *)self + baseSize,
           0,
           sizeof(SpkExpr) - baseSize);
}

static void Stmt_zero(SpkObject *_self) {
    SpkStmt *self = (SpkStmt *)_self;
    size_t baseSize = offsetof(SpkStmt, kind);
    (*Spk_ClassStmt->superclass->zero)(_self);
    memset((char *)self + baseSize,
           0,
           sizeof(SpkStmt) - baseSize);
}


/*------------------------------------------------------------------------*/
/* class templates */

static SpkMethodTmpl ExprMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassExprTmpl = {
    "Expr", {
        /*accessors*/ 0,
        ExprMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkExprSubclass, variables),
        /*itemSize*/ 0,
        &Expr_zero
    }, /*meta*/ {
    }
};


static SpkMethodTmpl StmtMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassStmtTmpl = {
    "Stmt", {
        /*accessors*/ 0,
        StmtMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkStmtSubclass, variables),
        /*itemSize*/ 0,
        &Stmt_zero
    }, /*meta*/ {
    }
};
