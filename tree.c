
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

static Unknown *Stmt_asModuleDef(Unknown *self, Unknown *arg0, Unknown *arg1) {
    INCREF(self); /* XXX: account for stolen reference */
    return (Unknown *)Parser_NewModuleDef((Stmt *)self);
}

static Unknown *Stmt_check(Unknown *_self, Unknown *arg0, Unknown *requestor) {
    Stmt *self;
    SymbolTable *st;
    
    self = (Stmt *)_self;
    st = CAST(XSymbolTable, arg0);
    if (!st) {
        Halt(HALT_TYPE_ERROR, "a symbol table is required");
        return 0;
    }
    return (Unknown *)StaticChecker_Check(self, st, requestor);
}

static Unknown *Stmt_generateCode(Unknown *self, Unknown *arg0, Unknown *arg1) {
    ModuleTmpl *moduleTmpl;
    
    moduleTmpl = CodeGen_GenerateCode((Stmt *)self);
    if (!moduleTmpl)
        return 0;
    /* We need an object, so create the class. */
    return (Unknown *)ModuleClass_New(moduleTmpl);
}

static Unknown *Stmt_source(Unknown *self, Unknown *sourcePathname, Unknown *arg1) {
    Stmt *tree;
    
    tree = (Stmt *)self;
    INCREF(tree); /* XXX: account for stolen reference */
    Parser_Source(&tree, sourcePathname);
    return (Unknown *)tree;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

/* Expr */

static void Expr_zero(Object *_self) {
    Expr *self = (Expr *)_self;
    size_t baseSize = offsetof(Expr, kind);
    (*CLASS(XExpr)->superclass->zero)(_self);
    memset((char *)self + baseSize,
           0,
           sizeof(Expr) - baseSize);
}

static void Expr_dealloc(Object *_self, Unknown **l) {
    Expr *self;
    
    self = (Expr *)_self;
    
    XLDECREF(self->declSpecs, l);
    XLDECREF(self->next, l);
    XLDECREF(self->nextArg, l);
    XLDECREF(self->cond, l);
    XLDECREF(self->left, l);
    XLDECREF(self->right, l);
    XLDECREF(self->var, l);
    XLDECREF(self->sym, l);
    
    switch (self->kind) {
    case EXPR_BLOCK:
        XLDECREF(self->aux.block.stmtList, l);
        break;
    case EXPR_KEYWORD:
        XLDECREF(self->aux.keywords, l);
        break;
    case EXPR_LITERAL:
        XLDECREF(self->aux.literalValue, l);
        break;
    case EXPR_NAME:
        switch (self->aux.nameKind) {
        case EXPR_NAME_UNK:
            break;
        case EXPR_NAME_DEF:
            break;
        case EXPR_NAME_REF:
            XLDECREF(self->u.ref.def, l);
            break;
        }
        break;
    default:
        break;
    }
        
    return (*CLASS(XExpr)->superclass->dealloc)(_self, l);
}


/* Stmt */

static void Stmt_zero(Object *_self) {
    Stmt *self = (Stmt *)_self;
    size_t baseSize = offsetof(Stmt, kind);
    (*CLASS(XStmt)->superclass->zero)(_self);
    memset((char *)self + baseSize,
           0,
           sizeof(Stmt) - baseSize);
}

static void Stmt_dealloc(Object *_self, Unknown **l) {
    Stmt *self;
    
    self = (Stmt *)_self;
    
    XLDECREF(self->next, l);
    XLDECREF(self->top, l);
    XLDECREF(self->bottom, l);
    XLDECREF(self->init, l);
    XLDECREF(self->expr, l);
    XLDECREF(self->incr, l);
    
    switch (self->kind) {
    case STMT_DEF_CLASS:
        XLDECREF(self->u.klass.superclassName, l);
        break;
    case STMT_DEF_METHOD:
        XLDECREF(self->u.method.name, l);
        break;
    case STMT_DEF_MODULE:
        XLDECREF(self->u.module.predefList.first, l);
        break;
    case STMT_PRAGMA_SOURCE:
        XLDECREF(self->u.source, l);
    default:
        break;
    }
    
    return (*CLASS(XStmt)->superclass->dealloc)(_self, l);
}


/*------------------------------------------------------------------------*/
/* class templates */

typedef struct ExprSubclass {
    Expr base;
    Unknown *variables[1]; /* stretchy */
} ExprSubclass;

static MethodTmpl ExprMethods[] = {
    { 0 }
};

ClassTmpl ClassXExprTmpl = {
    HEART_CLASS_TMPL(XExpr, Object), {
        /*accessors*/ 0,
        ExprMethods,
        /*lvalueMethods*/ 0,
        offsetof(ExprSubclass, variables),
        /*itemSize*/ 0,
        &Expr_zero,
        &Expr_dealloc
    }, /*meta*/ {
        0
    }
};


typedef struct StmtSubclass {
    Stmt base;
    Unknown *variables[1]; /* stretchy */
} StmtSubclass;

static MethodTmpl StmtMethods[] = {
    { "asModuleDef",  NativeCode_ARGS_0, &Stmt_asModuleDef },
    { "check",        NativeCode_ARGS_2, &Stmt_check },
    { "generateCode", NativeCode_ARGS_0, &Stmt_generateCode },
    { "source",       NativeCode_ARGS_1, &Stmt_source },
    { 0 }
};

ClassTmpl ClassXStmtTmpl = {
    HEART_CLASS_TMPL(XStmt, Object), {
        /*accessors*/ 0,
        StmtMethods,
        /*lvalueMethods*/ 0,
        offsetof(StmtSubclass, variables),
        /*itemSize*/ 0,
        &Stmt_zero,
        &Stmt_dealloc
    }, /*meta*/ {
        0
    }
};
