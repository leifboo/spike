
#include "tree.h"

#include "cgen.h"
#include "class.h"
#include "heart.h"
#include "parser.h"
#include "scheck.h"
#include "st.h"

#include <string.h>


/*------------------------------------------------------------------------*/
/* methods */

static SpkUnknown *Stmt_asModuleDef(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    Spk_INCREF(self); /* XXX: account for stolen reference */
    return (SpkUnknown *)SpkParser_NewModuleDef((SpkStmt *)self);
}

static SpkUnknown *Stmt_check(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *requestor) {
    SpkStmt *self;
    SpkSymbolTable *st;
    
    self = (SpkStmt *)_self;
    st = Spk_CAST(SymbolTable, arg0);
    if (!st) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a symbol table is required");
        return 0;
    }
    return (SpkUnknown *)SpkStaticChecker_Check(self, st, requestor);
}

static SpkUnknown *Stmt_generateCode(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return (SpkUnknown *)SpkCodeGen_GenerateCode((SpkStmt *)self);
}

static SpkUnknown *Stmt_source(SpkUnknown *self, SpkUnknown *sourcePathname, SpkUnknown *arg1) {
    SpkStmt *tree;
    
    tree = (SpkStmt *)self;
    Spk_INCREF(tree); /* XXX: account for stolen reference */
    SpkParser_Source(&tree, sourcePathname);
    return (SpkUnknown *)tree;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Expr_zero(SpkObject *_self) {
    SpkExpr *self = (SpkExpr *)_self;
    size_t baseSize = offsetof(SpkExpr, kind);
    (*Spk_CLASS(Expr)->superclass->zero)(_self);
    memset((char *)self + baseSize,
           0,
           sizeof(SpkExpr) - baseSize);
}

static void Stmt_zero(SpkObject *_self) {
    SpkStmt *self = (SpkStmt *)_self;
    size_t baseSize = offsetof(SpkStmt, kind);
    (*Spk_CLASS(Stmt)->superclass->zero)(_self);
    memset((char *)self + baseSize,
           0,
           sizeof(SpkStmt) - baseSize);
}


/*------------------------------------------------------------------------*/
/* memory layout of instances */

/* Expr */

static void Expr_traverse_init(SpkObject *self) {
    (*Spk_CLASS(Expr)->superclass->traverse.init)(self);
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
    default:
        break;
    }
        
    return (*Spk_CLASS(Expr)->superclass->traverse.current)(_self);
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
    default:
        break;
    }
    
    (*Spk_CLASS(Expr)->superclass->traverse.next)(_self);
}


/* Stmt */

static void Stmt_traverse_init(SpkObject *self) {
    (*Spk_CLASS(Stmt)->superclass->traverse.init)(self);
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
    case Spk_STMT_PRAGMA_SOURCE:
        if (self->u.source)
            return (SpkUnknown **)&self->u.source;
    default:
        break;
    }
    
    return (*Spk_CLASS(Stmt)->superclass->traverse.current)(_self);
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
    case Spk_STMT_PRAGMA_SOURCE:
        if (self->u.source) {
            self->u.source = 0;
            return;
        }
        break;
    default:
        break;
    }
    
    (*Spk_CLASS(Stmt)->superclass->traverse.next)(_self);
}


/*------------------------------------------------------------------------*/
/* class templates */

typedef struct SpkExprSubclass {
    SpkExpr base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkExprSubclass;

static SpkMethodTmpl ExprMethods[] = {
    { 0 }
};

static SpkTraverse Expr_traverse = {
    &Expr_traverse_init,
    &Expr_traverse_current,
    &Expr_traverse_next,
};

SpkClassTmpl Spk_ClassExprTmpl = {
    Spk_HEART_CLASS_TMPL(Expr, Object), {
        /*accessors*/ 0,
        ExprMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkExprSubclass, variables),
        /*itemSize*/ 0,
        &Expr_zero,
        /*dealloc*/ 0,
        &Expr_traverse
    }, /*meta*/ {
        0
    }
};


typedef struct SpkStmtSubclass {
    SpkStmt base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkStmtSubclass;

static SpkMethodTmpl StmtMethods[] = {
    { "asModuleDef",  SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Stmt_asModuleDef },
    { "check",        SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_2, &Stmt_check },
    { "generateCode", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Stmt_generateCode },
    { "source",       SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_1, &Stmt_source },
    { 0 }
};

static SpkTraverse Stmt_traverse = {
    &Stmt_traverse_init,
    &Stmt_traverse_current,
    &Stmt_traverse_next,
};

SpkClassTmpl Spk_ClassStmtTmpl = {
    Spk_HEART_CLASS_TMPL(Stmt, Object), {
        /*accessors*/ 0,
        StmtMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkStmtSubclass, variables),
        /*itemSize*/ 0,
        &Stmt_zero,
        /*dealloc*/ 0,
        &Stmt_traverse
    }, /*meta*/ {
        0
    }
};
