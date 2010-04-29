
#include "st.h"

#include "class.h"
#include "heart.h"
#include "host.h"
#include "interp.h"
#include "native.h"
#include "rodata.h"
#include "tree.h"
#include <assert.h>
#include <string.h>


static void declareType(SpkSymbolTable *st, SpkUnknown *name) {
    SpkExpr *nameExpr;
    SpkStmt *typeDef;
    
    nameExpr = (SpkExpr *)SpkObject_New(Spk_CLASS(Expr));        
    nameExpr->kind = Spk_EXPR_NAME;
    
    typeDef = (SpkStmt *)SpkObject_New(Spk_CLASS(Stmt));
    typeDef->kind = Spk_STMT_DEF_TYPE;
    typeDef->expr = nameExpr;
    typeDef->expr->sym = SpkSymbolNode_FromSymbol(st, name);
    typeDef->expr->u.def.stmt = typeDef;
    
    SpkSymbolTable_Insert(st, typeDef->expr, 0);
}


/*------------------------------------------------------------------------*/
/* C API */

SpkSymbolNode *SpkSymbolNode_FromSymbol(SpkSymbolTable *st, SpkUnknown *sym) {
    SpkSymbolNode *s;
    
    s = (SpkSymbolNode *)SpkHost_SymbolValue(st->symbolNodes, sym);
    if (s) {
        return s;
    }
    
    s = (SpkSymbolNode *)SpkObject_New(Spk_CLASS(SymbolNode));
    Spk_INCREF(sym);
    Spk_XDECREF(s->sym);
    s->sym = sym;
    
    SpkHost_DefineSymbol(st->symbolNodes, sym, (SpkUnknown *)s);
    
    return s;
}

SpkSymbolNode *SpkSymbolNode_FromCString(SpkSymbolTable *st, const char *str) {
    SpkUnknown *sym;
    SpkSymbolNode *node;
    
    sym = SpkHost_SymbolFromCString(str);
    node = SpkSymbolNode_FromSymbol(st, sym);
    Spk_DECREF(sym);
    return node;
}

int SpkSymbolNode_IsType(SpkSymbolNode *node) {
    SpkExpr *def;
    
    return node->sym == Spk_obj;

    /* XXX */
    if (!node->entry)
        return 0;
    def = node->entry->def;
    return (def->kind == Spk_EXPR_NAME &&
            def->aux.nameKind == Spk_EXPR_NAME_DEF &&
            def->u.def.stmt->kind == Spk_STMT_DEF_TYPE);
}


SpkSymbolTable *SpkSymbolTable_New() {
    SpkSymbolTable *newSymbolTable;
    
    newSymbolTable = (SpkSymbolTable *)SpkObject_New(Spk_CLASS(SymbolTable));
    Spk_XDECREF(newSymbolTable->symbolNodes);
    newSymbolTable->symbolNodes = SpkHost_NewSymbolDict();
    
    if (0) {
        /* XXX: pushOpcode/storeOpcode breaks with this approach */
        SpkSymbolTable_EnterScope(newSymbolTable, 1); /* meta scope */
        declareType(newSymbolTable, Spk_obj);
    }
    
    return newSymbolTable;
}

void SpkSymbolTable_EnterScope(SpkSymbolTable *st, int enterNewContext) {
    SpkScope *newScope, *outerScope;
    SpkContextClass *newContext;
    
    outerScope = st->currentScope;
    
    newScope = (SpkScope *)SpkObject_New(Spk_CLASS(Scope));
    Spk_XINCREF(outerScope);
    Spk_XDECREF(newScope->outer);
    newScope->outer = outerScope;
    if (enterNewContext) {
        newContext = (SpkContextClass *)SpkObject_New(Spk_CLASS(ContextClass));
        Spk_INCREF(newScope);
        Spk_XDECREF(newContext->scope);
        newContext->scope = newScope;
        if (outerScope) {
            newContext->level = outerScope->context->level + 1;
            switch (outerScope->context->pushOpcode) {
            case Spk_OPCODE_NOP:
                newContext->pushOpcode = Spk_OPCODE_PUSH_GLOBAL;
                newContext->storeOpcode = Spk_OPCODE_STORE_GLOBAL;
                break;
            case Spk_OPCODE_PUSH_GLOBAL:
                newContext->pushOpcode = Spk_OPCODE_PUSH_INST_VAR;
                newContext->storeOpcode = Spk_OPCODE_STORE_INST_VAR;
                break;
            case Spk_OPCODE_PUSH_INST_VAR:
            case Spk_OPCODE_PUSH_LOCAL: /* nested block */
                newContext->pushOpcode = Spk_OPCODE_PUSH_LOCAL;
                newContext->storeOpcode = Spk_OPCODE_STORE_LOCAL;
                break;
            }
        } else { /* built-in scope */
            newContext->level = 0;
            newContext->pushOpcode = Spk_OPCODE_NOP;
            newContext->storeOpcode = Spk_OPCODE_NOP;
        }
        newContext->nDefs = 0;
        Spk_XDECREF(newScope->context);
        newScope->context = newContext; /* note cycle */
    } else {
        Spk_INCREF(outerScope->context);
        Spk_XDECREF(newScope->context);
        newScope->context = outerScope->context;
    }
    
    Spk_XDECREF(st->currentScope);
    st->currentScope = newScope;
    
    return;
}

void SpkSymbolTable_ExitScope(SpkSymbolTable *st) {
    /* XXX: Clients must match every call to SpkSymbolTable_EnterScope
       with a call to this function in order to break cycles. */
    SpkScope *oldScope;
    SpkSTEntry *entry;
    SpkContextClass *tmp;
    
    oldScope = st->currentScope;
    
    for (entry = oldScope->entryList; entry; entry = entry->nextInScope) {
        SpkSymbolNode *sym; SpkSTEntry *tmp;
        
        sym = entry->sym;
        assert(sym->entry == entry);
        /* this also breaks a cycle */
        tmp = sym->entry;
        Spk_XINCREF(entry->shadow);
        sym->entry = entry->shadow;
        Spk_DECREF(tmp);
    }
    
    if (oldScope->entryList) {
        /* break cycle */
        SpkSTEntry *tmp = oldScope->entryList;
        oldScope->entryList = 0;
        Spk_DECREF(tmp);
    }
    
    /* break cycle */
    tmp = oldScope->context;
    oldScope->context = 0;
    Spk_DECREF(tmp);
    
    Spk_XINCREF(oldScope->outer);
    st->currentScope = oldScope->outer;
    Spk_DECREF(oldScope);
}

SpkUnknown *SpkSymbolTable_Insert(SpkSymbolTable *st, SpkExpr *def,
                                  SpkUnknown *requestor)
{
    SpkSymbolNode *sym;
    SpkSTEntry *oldEntry, *newEntry;
    SpkScope *cs;
    SpkUnknown *tmp;
    
    assert(def->kind == Spk_EXPR_NAME);
    def->aux.nameKind = Spk_EXPR_NAME_DEF;
    
    sym = def->sym;
    oldEntry = sym->entry;
    cs = st->currentScope;
    
    if (oldEntry &&
        (oldEntry->scope == cs ||
         oldEntry->scope->context->level == 0 /*built-in*/ )) {
        /* multiply defined */
        if (oldEntry->scope->context->level != 0 &&
            def->u.def.weak &&
            oldEntry->def->u.def.weak) {
            def->u.def.level       = oldEntry->def->u.def.level;
            def->u.def.pushOpcode  = oldEntry->def->u.def.pushOpcode;
            def->u.def.storeOpcode = oldEntry->def->u.def.storeOpcode;
            def->u.def.index       = oldEntry->def->u.def.index;
            Spk_INCREF(Spk_GLOBAL(xvoid));
            return Spk_GLOBAL(xvoid);
        }
        tmp = Spk_Send(Spk_GLOBAL(theInterpreter), requestor, Spk_redefinedSymbol, def, 0);
        if (!tmp)
            return 0;
        Spk_DECREF(tmp);
        Spk_INCREF(Spk_GLOBAL(xvoid));
        return Spk_GLOBAL(xvoid);
    }
    
    newEntry = (SpkSTEntry *)SpkObject_New(Spk_CLASS(STEntry));
    Spk_XDECREF(newEntry->scope);
    Spk_XDECREF(newEntry->nextInScope);
    Spk_XDECREF(newEntry->shadow);
    Spk_XDECREF(newEntry->sym);
    Spk_XDECREF(newEntry->def);
    Spk_INCREF(cs);
    Spk_XINCREF(oldEntry);
    Spk_INCREF(sym);
    Spk_INCREF(def);
    newEntry->scope = cs; /* note cycle */
    newEntry->nextInScope = cs->entryList; /* steal reference */
    newEntry->shadow = oldEntry;
    newEntry->sym = sym;
    newEntry->def = def;
    
    cs->entryList = newEntry; /* steal new reference */
    Spk_INCREF(newEntry);
    sym->entry = newEntry; /* note cycle */
    
    def->u.def.level = cs->context->level;
    if (cs->context->pushOpcode != Spk_OPCODE_NOP) { /* not built-in scope */
        def->u.def.pushOpcode = cs->context->pushOpcode;
        def->u.def.storeOpcode = cs->context->storeOpcode;
    }
    /* XXX: We could overlap scopes with the addition of a 'clear'
       opcode. */
    def->u.def.index = cs->context->nDefs++;
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}

SpkExpr *SpkSymbolTable_Lookup(SpkSymbolTable *st, SpkSymbolNode *sym) {
    SpkSTEntry *entry;
    
    entry = sym->entry;
    return entry ? entry->def : 0;
}

SpkUnknown *SpkSymbolTable_Bind(SpkSymbolTable *st, SpkExpr *expr,
                                SpkUnknown *requestor)
{
    SpkSymbolNode *sym;
    SpkSTEntry *entry;
    SpkUnknown *tmp;
    
    assert(expr->kind == Spk_EXPR_NAME);
    expr->aux.nameKind = Spk_EXPR_NAME_REF;
    
    sym = expr->sym;
    entry = sym->entry;
    if (entry) {
        expr->u.ref.def = entry->def;  Spk_INCREF(entry->def);
        expr->u.ref.level = st->currentScope->context->level;
        Spk_INCREF(Spk_GLOBAL(xvoid));
        return Spk_GLOBAL(xvoid);
    }
    
    /* undefined */
    tmp = Spk_Send(Spk_GLOBAL(theInterpreter), requestor, Spk_undefinedSymbol, expr, 0);
    if (!tmp)
        return 0;
    Spk_DECREF(tmp);
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}


/*------------------------------------------------------------------------*/
/* methods */

static SpkUnknown *SymbolTable_init(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkSymbolTable *self;
    
    self = (SpkSymbolTable *)_self;
    Spk_XDECREF(self->symbolNodes);
    self->symbolNodes = SpkHost_NewSymbolDict();
    
    if (0) {
        /* XXX */
        SpkSymbolTable_EnterScope(self, 1); /* meta scope */
        declareType(self, Spk_obj);
    }
    
    Spk_INCREF(_self);
    return _self;
}


/*------------------------------------------------------------------------*/
/* meta-methods */

static SpkUnknown *ClassSymbolTable_new(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkUnknown *newSymbolTable, *tmp;
    
    newSymbolTable = Spk_Send(Spk_GLOBAL(theInterpreter), Spk_SUPER, Spk_new, 0);
    if (!newSymbolTable)
        return 0;
    tmp = newSymbolTable;
    newSymbolTable = Spk_Send(Spk_GLOBAL(theInterpreter), newSymbolTable, Spk_init, 0);
    Spk_DECREF(tmp);
    return newSymbolTable;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

/* SymbolNode */

static void SymbolNode_zero(SpkObject *_self) {
    SpkSymbolNode *self = (SpkSymbolNode *)_self;
    (*Spk_CLASS(SymbolNode)->superclass->zero)(_self);
    self->entry = 0;
    self->sym = 0;
}

static void SymbolNode_dealloc(SpkObject *_self, SpkUnknown **l) {
    SpkSymbolNode *self = (SpkSymbolNode *)_self;
    Spk_XLDECREF(self->entry, l);
    Spk_XLDECREF(self->sym, l);
    (*Spk_CLASS(SymbolNode)->superclass->dealloc)(_self, l);
}


/* STEntry */

static void STEntry_zero(SpkObject *_self) {
    SpkSTEntry *self = (SpkSTEntry *)_self;
    (*Spk_CLASS(STEntry)->superclass->zero)(_self);
    self->scope = 0;
    self->nextInScope = 0;
    self->shadow = 0;
    self->sym = 0;
    self->def = 0;
}

static void STEntry_dealloc(SpkObject *_self, SpkUnknown **l) {
    SpkSTEntry *self = (SpkSTEntry *)_self;
    Spk_XLDECREF(self->scope, l);
    Spk_XLDECREF(self->nextInScope, l);
    Spk_XLDECREF(self->shadow, l);
    Spk_XLDECREF(self->sym, l);
    Spk_XLDECREF(self->def, l);
    (*Spk_CLASS(STEntry)->superclass->dealloc)(_self, l);
}


/* ContextClass */

static void ContextClass_zero(SpkObject *_self) {
    SpkContextClass *self = (SpkContextClass *)_self;
    (*Spk_CLASS(ContextClass)->superclass->zero)(_self);
    self->scope = 0;
}

static void ContextClass_dealloc(SpkObject *_self, SpkUnknown **l) {
    SpkContextClass *self = (SpkContextClass *)_self;
    Spk_XLDECREF(self->scope, l);
    (*Spk_CLASS(ContextClass)->superclass->dealloc)(_self, l);
}


/* Scope */

static void Scope_zero(SpkObject *_self) {
    SpkScope *self = (SpkScope *)_self;
    (*Spk_CLASS(Scope)->superclass->zero)(_self);
    self->outer = 0;
    self->entryList = 0;
    self->context = 0;
}

static void Scope_dealloc(SpkObject *_self, SpkUnknown **l) {
    SpkScope *self = (SpkScope *)_self;
    Spk_XLDECREF(self->outer, l);
    Spk_XLDECREF(self->entryList, l);
    Spk_XLDECREF(self->context, l);
    (*Spk_CLASS(Scope)->superclass->dealloc)(_self, l);
}


/* SymbolTable */

static void SymbolTable_zero(SpkObject *_self) {
    SpkSymbolTable *self = (SpkSymbolTable *)_self;
    (*Spk_CLASS(SymbolTable)->superclass->zero)(_self);
    self->currentScope = 0;
    self->symbolNodes = 0;
}

static void SymbolTable_dealloc(SpkObject *_self, SpkUnknown **l) {
    SpkSymbolTable *self = (SpkSymbolTable *)_self;
    Spk_XLDECREF(self->currentScope, l);
    Spk_XLDECREF(self->symbolNodes, l);
    (*Spk_CLASS(SymbolTable)->superclass->dealloc)(_self, l);
}


/*------------------------------------------------------------------------*/
/* class templates */

typedef struct SpkSymbolNodeSubclass {
    SpkSymbolNode base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkSymbolNodeSubclass;

typedef struct SpkSTEntrySubclass {
    SpkSTEntry base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkSTEntrySubclass;

typedef struct SpkContextClassSubclass {
    SpkContextClass base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkContextClassSubclass;

typedef struct SpkScopeSubclass {
    SpkScope base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkScopeSubclass;

typedef struct SpkSymbolTableSubclass {
    SpkSymbolTable base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkSymbolTableSubclass;


SpkClassTmpl Spk_ClassSymbolNodeTmpl = {
    Spk_HEART_CLASS_TMPL(SymbolNode, Object), {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(SpkSymbolNodeSubclass, variables),
        /*itemSize*/ 0,
        &SymbolNode_zero,
        &SymbolNode_dealloc
    }, /*meta*/ {
        0
    }
};


SpkClassTmpl Spk_ClassSTEntryTmpl = {
    Spk_HEART_CLASS_TMPL(STEntry, Object), {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(SpkSTEntrySubclass, variables),
        /*itemSize*/ 0,
        &STEntry_zero,
        &STEntry_dealloc
    }, /*meta*/ {
        0
    }
};


SpkClassTmpl Spk_ClassContextClassTmpl = {
    Spk_HEART_CLASS_TMPL(ContextClass, Object), {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(SpkContextClassSubclass, variables),
        /*itemSize*/ 0,
        &ContextClass_zero,
        &ContextClass_dealloc
    }, /*meta*/ {
        0
    }
};


SpkClassTmpl Spk_ClassScopeTmpl = {
    Spk_HEART_CLASS_TMPL(Scope, Object), {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(SpkScopeSubclass, variables),
        /*itemSize*/ 0,
        &Scope_zero,
        &Scope_dealloc
    }, /*meta*/ {
        0
    }
};


static SpkMethodTmpl SymbolTableMethods[] = {
    { "init", SpkNativeCode_ARGS_0, &SymbolTable_init },
    { 0 }
};

static SpkMethodTmpl ClassSymbolTableMethods[] = {
    { "new",   SpkNativeCode_ARGS_0, &ClassSymbolTable_new },
    { 0 }
};

SpkClassTmpl Spk_ClassSymbolTableTmpl = {
    Spk_HEART_CLASS_TMPL(SymbolTable, Object), {
        /*accessors*/ 0,
        SymbolTableMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkSymbolTableSubclass, variables),
        /*itemSize*/ 0,
        &SymbolTable_zero,
        &SymbolTable_dealloc
    }, /*meta*/ {
        /*accessors*/ 0,
        ClassSymbolTableMethods,
        /*lvalueMethods*/ 0
    }
};
