
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
/* memory layout of instances */

/* Expr */

static void Expr_traverse_init(SpkObject *self) {
    (*Spk_ClassExpr->superclass->traverse.init)(self);
}

static SpkUnknown **Expr_traverse_current(SpkObject *_self) {
    SpkExpr *self;
    
    self = (SpkExpr *)_self;
    
    if (self->next)
        return (SpkUnknown **)&self->next;
    if (self->nextArg)
        return (SpkUnknown **)&self->nextArg;
    if (self->cond)
        return (SpkUnknown **)&self->cond;
    if (self->left)
        return (SpkUnknown **)&self->left;
    if (self->right)
        return (SpkUnknown **)&self->right;
    if (self->var)
        return (SpkUnknown **)&self->var;
    if (self->sym)
        return (SpkUnknown **)&self->sym;
    
    switch (self->kind) {
    case Spk_EXPR_BLOCK:
        if (self->aux.block.stmtList)
            return (SpkUnknown **)&self->aux.block.stmtList;
        break;
    case Spk_EXPR_KEYWORD:
        if (self->aux.keywords)
            return (SpkUnknown **)&self->aux.keywords;
        break;
    case Spk_EXPR_LITERAL:
        if (self->aux.literalValue)
            return (SpkUnknown **)&self->aux.literalValue;
        break;
    case Spk_EXPR_NAME:
        switch (self->aux.nameKind) {
        case Spk_EXPR_NAME_UNK:
            break;
        case Spk_EXPR_NAME_DEF:
            break;
        case Spk_EXPR_NAME_REF:
            if (self->u.ref.def) {
                return (SpkUnknown **)&self->u.ref.def;
            }
            break;
        }
        break;
    }
        
    return (*Spk_ClassExpr->superclass->traverse.current)(_self);
}

static void Expr_traverse_next(SpkObject *_self) {
    SpkExpr *self;
    
    self = (SpkExpr *)_self;
    
    if (self->next) {
        self->next = 0;
        return;
    }
    if (self->nextArg) {
        self->nextArg = 0;
        return;
    }
    if (self->cond) {
        self->cond = 0;
        return;
    }
    if (self->left) {
        self->left = 0;
        return;
    }
    if (self->right) {
        self->right = 0;
        return;
    }
    if (self->var) {
        self->var = 0;
        return;
    }
    if (self->sym) {
        self->sym = 0;
        return;
    }
    
    switch (self->kind) {
    case Spk_EXPR_BLOCK:
        if (self->aux.block.stmtList) {
            self->aux.block.stmtList = 0;
            return;
        }
        break;
    case Spk_EXPR_KEYWORD:
        if (self->aux.keywords) {
            self->aux.keywords = 0;
            return;
        }
        break;
    case Spk_EXPR_LITERAL:
        if (self->aux.literalValue) {
            self->aux.literalValue = 0;
            return;
        }
        break;
    case Spk_EXPR_NAME:
        switch (self->aux.nameKind) {
        case Spk_EXPR_NAME_UNK:
            break;
        case Spk_EXPR_NAME_DEF:
            break;
        case Spk_EXPR_NAME_REF:
            if (self->u.ref.def) {
                self->u.ref.def = 0;
                return;
            }
            break;
        }
        break;
    }
    
    (*Spk_ClassExpr->superclass->traverse.next)(_self);
}


/* Stmt */

static void Stmt_traverse_init(SpkObject *self) {
    (*Spk_ClassStmt->superclass->traverse.init)(self);
}

static SpkUnknown **Stmt_traverse_current(SpkObject *_self) {
    SpkStmt *self;
    
    self = (SpkStmt *)_self;
    
    if (self->next)
        return (SpkUnknown **)&self->next;
    if (self->top)
        return (SpkUnknown **)&self->top;
    if (self->bottom)
        return (SpkUnknown **)&self->bottom;
    if (self->init)
        return (SpkUnknown **)&self->init;
    if (self->expr)
        return (SpkUnknown **)&self->expr;
    if (self->incr)
        return (SpkUnknown **)&self->incr;
    
    switch (self->kind) {
    case Spk_STMT_DEF_CLASS:
        if (self->u.klass.superclassName)
            return (SpkUnknown **)&self->u.klass.superclassName;
        break;
    case Spk_STMT_DEF_METHOD:
        if (self->u.method.name)
            return (SpkUnknown **)&self->u.method.name;
        break;
    }
    
    return (*Spk_ClassStmt->superclass->traverse.current)(_self);
}

static void Stmt_traverse_next(SpkObject *_self) {
    SpkStmt *self;
    
    self = (SpkStmt *)_self;
    
    if (self->next) {
        self->next = 0;
        return;
    }
    if (self->top) {
        self->top = 0;
        return;
    }
    if (self->bottom) {
        self->bottom = 0;
        return;
    }
    if (self->init) {
        self->init = 0;
        return;
    }
    if (self->expr) {
        self->expr = 0;
        return;
    }
    if (self->incr) {
        self->incr = 0;
        return;
    }
    
    switch (self->kind) {
    case Spk_STMT_DEF_CLASS:
        if (self->u.klass.superclassName) {
            self->u.klass.superclassName = 0;
            return;
        }
        break;
    case Spk_STMT_DEF_METHOD:
        if (self->u.method.name) {
            self->u.method.name = 0;
            return;
        }
        break;
    }
    
    (*Spk_ClassStmt->superclass->traverse.next)(_self);
}


/*------------------------------------------------------------------------*/
/* class templates */

static SpkMethodTmpl ExprMethods[] = {
    { 0, 0, 0}
};

static SpkTraverse Expr_traverse = {
    &Expr_traverse_init,
    &Expr_traverse_current,
    &Expr_traverse_next,
};

SpkClassTmpl Spk_ClassExprTmpl = {
    "Expr", {
        /*accessors*/ 0,
        ExprMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkExprSubclass, variables),
        /*itemSize*/ 0,
        &Expr_zero,
        /*dealloc*/ 0,
        &Expr_traverse
    }, /*meta*/ {
    }
};


static SpkMethodTmpl StmtMethods[] = {
    { 0, 0, 0}
};

static SpkTraverse Stmt_traverse = {
    &Stmt_traverse_init,
    &Stmt_traverse_current,
    &Stmt_traverse_next,
};

SpkClassTmpl Spk_ClassStmtTmpl = {
    "Stmt", {
        /*accessors*/ 0,
        StmtMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkStmtSubclass, variables),
        /*itemSize*/ 0,
        &Stmt_zero,
        /*dealloc*/ 0,
        &Stmt_traverse
    }, /*meta*/ {
    }
};
