
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
    st = Spk_CAST(XSymbolTable, arg0);
    if (!st) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a symbol table is required");
        return 0;
    }
    return (SpkUnknown *)SpkStaticChecker_Check(self, st, requestor);
}

static SpkUnknown *Stmt_generateCode(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkModuleTmpl *moduleTmpl;
    
    moduleTmpl = SpkCodeGen_GenerateCode((SpkStmt *)self);
    if (!moduleTmpl)
        return 0;
    /* We need an object, so create the class. */
    return (SpkUnknown *)SpkModuleClass_New(moduleTmpl);
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

/* Expr */

static void Expr_zero(SpkObject *_self) {
    SpkExpr *self = (SpkExpr *)_self;
    size_t baseSize = offsetof(SpkExpr, kind);
    (*Spk_CLASS(XExpr)->superclass->zero)(_self);
    memset((char *)self + baseSize,
           0,
           sizeof(SpkExpr) - baseSize);
}

static void Expr_dealloc(SpkObject *_self, SpkUnknown **l) {
    SpkExpr *self;
    
    self = (SpkExpr *)_self;
    
    Spk_XLDECREF(self->declSpecs, l);
    Spk_XLDECREF(self->next, l);
    Spk_XLDECREF(self->nextArg, l);
    Spk_XLDECREF(self->cond, l);
    Spk_XLDECREF(self->left, l);
    Spk_XLDECREF(self->right, l);
    Spk_XLDECREF(self->var, l);
    Spk_XLDECREF(self->sym, l);
    
    switch (self->kind) {
    case Spk_EXPR_BLOCK:
        Spk_XLDECREF(self->aux.block.stmtList, l);
        break;
    case Spk_EXPR_KEYWORD:
        Spk_XLDECREF(self->aux.keywords, l);
        break;
    case Spk_EXPR_LITERAL:
        Spk_XLDECREF(self->aux.literalValue, l);
        break;
    case Spk_EXPR_NAME:
        switch (self->aux.nameKind) {
        case Spk_EXPR_NAME_UNK:
            break;
        case Spk_EXPR_NAME_DEF:
            break;
        case Spk_EXPR_NAME_REF:
            Spk_XLDECREF(self->u.ref.def, l);
            break;
        }
        break;
    default:
        break;
    }
        
    return (*Spk_CLASS(XExpr)->superclass->dealloc)(_self, l);
}


/* Stmt */

static void Stmt_zero(SpkObject *_self) {
    SpkStmt *self = (SpkStmt *)_self;
    size_t baseSize = offsetof(SpkStmt, kind);
    (*Spk_CLASS(XStmt)->superclass->zero)(_self);
    memset((char *)self + baseSize,
           0,
           sizeof(SpkStmt) - baseSize);
}

static void Stmt_dealloc(SpkObject *_self, SpkUnknown **l) {
    SpkStmt *self;
    
    self = (SpkStmt *)_self;
    
    Spk_XLDECREF(self->next, l);
    Spk_XLDECREF(self->top, l);
    Spk_XLDECREF(self->bottom, l);
    Spk_XLDECREF(self->init, l);
    Spk_XLDECREF(self->expr, l);
    Spk_XLDECREF(self->incr, l);
    
    switch (self->kind) {
    case Spk_STMT_DEF_CLASS:
        Spk_XLDECREF(self->u.klass.superclassName, l);
        break;
    case Spk_STMT_DEF_METHOD:
        Spk_XLDECREF(self->u.method.name, l);
        break;
    case Spk_STMT_DEF_MODULE:
        Spk_XLDECREF(self->u.module.predefList.first, l);
        break;
    case Spk_STMT_PRAGMA_SOURCE:
        Spk_XLDECREF(self->u.source, l);
    default:
        break;
    }
    
    return (*Spk_CLASS(XStmt)->superclass->dealloc)(_self, l);
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

SpkClassTmpl Spk_ClassXExprTmpl = {
    Spk_HEART_CLASS_TMPL(XExpr, Object), {
        /*accessors*/ 0,
        ExprMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkExprSubclass, variables),
        /*itemSize*/ 0,
        &Expr_zero,
        &Expr_dealloc
    }, /*meta*/ {
        0
    }
};


typedef struct SpkStmtSubclass {
    SpkStmt base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkStmtSubclass;

static SpkMethodTmpl StmtMethods[] = {
    { "asModuleDef",  SpkNativeCode_ARGS_0, &Stmt_asModuleDef },
    { "check",        SpkNativeCode_ARGS_2, &Stmt_check },
    { "generateCode", SpkNativeCode_ARGS_0, &Stmt_generateCode },
    { "source",       SpkNativeCode_ARGS_1, &Stmt_source },
    { 0 }
};

SpkClassTmpl Spk_ClassXStmtTmpl = {
    Spk_HEART_CLASS_TMPL(XStmt, Object), {
        /*accessors*/ 0,
        StmtMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkStmtSubclass, variables),
        /*itemSize*/ 0,
        &Stmt_zero,
        &Stmt_dealloc
    }, /*meta*/ {
        0
    }
};
