
#include "st.h"

#include "class.h"
#include "heart.h"
#include "host.h"
#include "interp.h"
#include "native.h"
#include "rodata.h"
#include "scheck.h"
#include "tree.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>


/*------------------------------------------------------------------------*/
/* C API */

SymbolNode *SymbolNode_FromSymbol(SymbolTable *st, Unknown *sym) {
    SymbolNode *s;
    
    s = (SymbolNode *)Host_SymbolValue(st->symbolNodes, sym);
    if (s) {
        return s;
    }
    
    s = (SymbolNode *)Object_New(CLASS(XSymbolNode));
    s->sym = sym;
    
    Host_DefineSymbol(st->symbolNodes, sym, (Unknown *)s);
    
    return s;
}

SymbolNode *SymbolNode_FromCString(SymbolTable *st, const char *str) {
    Unknown *sym;
    SymbolNode *node;
    
    sym = Host_SymbolFromCString(str);
    node = SymbolNode_FromSymbol(st, sym);
    return node;
}

int SymbolNode_IsSpec(SymbolNode *node) {
    Expr *def;
    
    if (0 /*cxxgen*/ ) {
        if (isupper(Host_SymbolAsCString(node->sym)[0]))
            return 1;
    }

    if (!node->entry)
        return 0;
    def = node->entry->def;
    return (def &&
            def->kind == EXPR_NAME &&
            def->aux.nameKind == EXPR_NAME_DEF &&
            def->u.def.stmt &&
            def->u.def.stmt->kind == STMT_DEF_SPEC);
}


SymbolTable *SymbolTable_New() {
    SymbolTable *newSymbolTable;
    
    newSymbolTable = (SymbolTable *)Object_New(CLASS(XSymbolTable));
    newSymbolTable->symbolNodes = Host_NewSymbolDict();
    return newSymbolTable;
}

void SymbolTable_EnterScope(SymbolTable *st, int enterNewContext) {
    Scope *newScope, *outerScope;
    ContextClass *newContext;
    
    outerScope = st->currentScope;
    
    newScope = (Scope *)Object_New(CLASS(XScope));
    newScope->outer = outerScope;
    if (enterNewContext) {
        newContext = (ContextClass *)Object_New(CLASS(XContextClass));
        newContext->scope = newScope;
        if (outerScope) {
            newContext->level = outerScope->context->level + 1;
            switch (outerScope->context->pushOpcode) {
            case OPCODE_NOP:
                newContext->pushOpcode = OPCODE_PUSH_GLOBAL;
                newContext->storeOpcode = OPCODE_STORE_GLOBAL;
                break;
            case OPCODE_PUSH_GLOBAL:
                newContext->pushOpcode = OPCODE_PUSH_INST_VAR;
                newContext->storeOpcode = OPCODE_STORE_INST_VAR;
                break;
            case OPCODE_PUSH_INST_VAR:
            case OPCODE_PUSH_LOCAL: /* nested block */
                newContext->pushOpcode = OPCODE_PUSH_LOCAL;
                newContext->storeOpcode = OPCODE_STORE_LOCAL;
                break;
            }
        } else { /* built-in scope */
            newContext->level = 0;
            newContext->pushOpcode = OPCODE_NOP;
            newContext->storeOpcode = OPCODE_NOP;
        }
        newContext->nDefs = 0;
        newScope->context = newContext;
    } else {
        newScope->context = outerScope->context;
    }
    
    st->currentScope = newScope;
    
    return;
}

void SymbolTable_ExitScope(SymbolTable *st) {
    Scope *oldScope;
    STEntry *entry;
    
    oldScope = st->currentScope;
    
    for (entry = oldScope->entryList; entry; entry = entry->nextInScope) {
        SymbolNode *sym;
        
        sym = entry->sym;
        assert(sym->entry == entry);
        sym->entry = entry->shadow;
    }
    
    st->currentScope = oldScope->outer;
}

Unknown *SymbolTable_Insert(SymbolTable *st, Expr *def,
                                  Unknown *requestor)
{
    SymbolNode *sym;
    STEntry *oldEntry, *newEntry;
    Scope *cs;
    Unknown *tmp;
    
    assert(def->kind == EXPR_NAME);
    def->aux.nameKind = EXPR_NAME_DEF;
    
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
            return GLOBAL(xvoid);
        }
        tmp = Send(GLOBAL(theInterpreter), requestor, redefinedSymbol, def, 0);
        if (!tmp)
            return 0;
        return GLOBAL(xvoid);
    }
    
    newEntry = (STEntry *)Object_New(CLASS(XSTEntry));
    newEntry->scope = cs;
    newEntry->nextInScope = cs->entryList;
    newEntry->shadow = oldEntry;
    newEntry->sym = sym;
    newEntry->def = def;
    
    cs->entryList = newEntry;
    sym->entry = newEntry;
    
    def->u.def.level = cs->context->level;
    if (cs->context->pushOpcode != OPCODE_NOP) { /* not built-in scope */
        def->u.def.pushOpcode = cs->context->pushOpcode;
        def->u.def.storeOpcode = cs->context->storeOpcode;
    }
    /* XXX: We could overlap scopes with the addition of a 'clear'
       opcode. */
    def->u.def.index = cs->context->nDefs++;
    
    return GLOBAL(xvoid);
}

Expr *SymbolTable_Lookup(SymbolTable *st, SymbolNode *sym) {
    STEntry *entry;
    
    entry = sym->entry;
    return entry ? entry->def : 0;
}

Unknown *SymbolTable_Bind(SymbolTable *st, Expr *expr,
                                Unknown *requestor)
{
    SymbolNode *sym;
    STEntry *entry;
    Unknown *tmp;
    
    assert(expr->kind == EXPR_NAME);
    expr->aux.nameKind = EXPR_NAME_REF;
    
    sym = expr->sym;
    entry = sym->entry;
    if (entry) {
        expr->u.ref.def = entry->def;
        expr->u.ref.level = st->currentScope->context->level;
        return GLOBAL(xvoid);
    }
    
    /* undefined */
    tmp = Send(GLOBAL(theInterpreter), requestor, undefinedSymbol, expr, 0);
    if (!tmp)
        return 0;
    return GLOBAL(xvoid);
}


/*------------------------------------------------------------------------*/
/* methods */

static Unknown *SymbolNode_isSpec(Unknown *self, Unknown *arg0, Unknown *arg1) {
    int isSpec; Unknown *result;
    
    isSpec = SymbolNode_IsSpec((SymbolNode *)self);
    if (GLOBAL(xtrue)) {
        result = isSpec ? GLOBAL(xtrue) : GLOBAL(xfalse);
    } else /* compiling "bool.spk"; true & false don't exist yet */ {
        result = isSpec ? one : zero;
    }
    return result;
}


static Unknown *SymbolTable_init(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    SymbolTable *self;
    
    self = (SymbolTable *)_self;
    self->symbolNodes = Host_NewSymbolDict();
    
    return _self;
}

static Unknown *SymbolTable_declareBuiltIn(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    SymbolTable *self = (SymbolTable *)_self;
    return StaticChecker_DeclareBuiltIn(self, arg0);
}

static Unknown *SymbolTable_symbolNodeForSymbol(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    SymbolTable *self = (SymbolTable *)_self;
    return (Unknown *)SymbolNode_FromSymbol(self, arg0);
}


/*------------------------------------------------------------------------*/
/* meta-methods */

static Unknown *ClassSymbolTable_new(Unknown *self, Unknown *arg0, Unknown *arg1) {
    Unknown *newSymbolTable;
    
    newSymbolTable = Send(GLOBAL(theInterpreter), SUPER, new, 0);
    if (!newSymbolTable)
        return 0;
    newSymbolTable = Send(GLOBAL(theInterpreter), newSymbolTable, init, 0);
    return newSymbolTable;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

/* SymbolNode */

static void SymbolNode_zero(Object *_self) {
    SymbolNode *self = (SymbolNode *)_self;
    (*CLASS(XSymbolNode)->superclass->zero)(_self);
    self->entry = 0;
    self->sym = 0;
}


/* STEntry */

static void STEntry_zero(Object *_self) {
    STEntry *self = (STEntry *)_self;
    (*CLASS(XSTEntry)->superclass->zero)(_self);
    self->scope = 0;
    self->nextInScope = 0;
    self->shadow = 0;
    self->sym = 0;
    self->def = 0;
}


/* ContextClass */

static void ContextClass_zero(Object *_self) {
    ContextClass *self = (ContextClass *)_self;
    (*CLASS(XContextClass)->superclass->zero)(_self);
    self->scope = 0;
}


/* Scope */

static void Scope_zero(Object *_self) {
    Scope *self = (Scope *)_self;
    (*CLASS(XScope)->superclass->zero)(_self);
    self->outer = 0;
    self->entryList = 0;
    self->context = 0;
}


/* SymbolTable */

static void SymbolTable_zero(Object *_self) {
    SymbolTable *self = (SymbolTable *)_self;
    (*CLASS(XSymbolTable)->superclass->zero)(_self);
    self->currentScope = 0;
    self->symbolNodes = 0;
}


/*------------------------------------------------------------------------*/
/* class templates */

typedef struct SymbolNodeSubclass {
    SymbolNode base;
    Unknown *variables[1]; /* stretchy */
} SymbolNodeSubclass;

typedef struct STEntrySubclass {
    STEntry base;
    Unknown *variables[1]; /* stretchy */
} STEntrySubclass;

typedef struct ContextClassSubclass {
    ContextClass base;
    Unknown *variables[1]; /* stretchy */
} ContextClassSubclass;

typedef struct ScopeSubclass {
    Scope base;
    Unknown *variables[1]; /* stretchy */
} ScopeSubclass;

typedef struct SymbolTableSubclass {
    SymbolTable base;
    Unknown *variables[1]; /* stretchy */
} SymbolTableSubclass;


static MethodTmpl SymbolNodeMethods[] = {
    { "isSpec", NativeCode_ARGS_0, &SymbolNode_isSpec },
    { 0 }
};

ClassTmpl ClassXSymbolNodeTmpl = {
    HEART_CLASS_TMPL(XSymbolNode, Object), {
        /*accessors*/ 0,
        SymbolNodeMethods,
        /*lvalueMethods*/ 0,
        offsetof(SymbolNodeSubclass, variables),
        /*itemSize*/ 0,
        &SymbolNode_zero
    }, /*meta*/ {
        0
    }
};


ClassTmpl ClassXSTEntryTmpl = {
    HEART_CLASS_TMPL(XSTEntry, Object), {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(STEntrySubclass, variables),
        /*itemSize*/ 0,
        &STEntry_zero
    }, /*meta*/ {
        0
    }
};


ClassTmpl ClassXContextClassTmpl = {
    HEART_CLASS_TMPL(XContextClass, Object), {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(ContextClassSubclass, variables),
        /*itemSize*/ 0,
        &ContextClass_zero
    }, /*meta*/ {
        0
    }
};


ClassTmpl ClassXScopeTmpl = {
    HEART_CLASS_TMPL(XScope, Object), {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(ScopeSubclass, variables),
        /*itemSize*/ 0,
        &Scope_zero
    }, /*meta*/ {
        0
    }
};


static MethodTmpl SymbolTableMethods[] = {
    { "init", NativeCode_ARGS_0, &SymbolTable_init },
    { "declareBuiltIn", NativeCode_ARGS_1, &SymbolTable_declareBuiltIn },
    { "symbolNodeForSymbol:", NativeCode_ARGS_1, &SymbolTable_symbolNodeForSymbol },
    { 0 }
};

static MethodTmpl ClassSymbolTableMethods[] = {
    { "new",   NativeCode_ARGS_0, &ClassSymbolTable_new },
    { 0 }
};

ClassTmpl ClassXSymbolTableTmpl = {
    HEART_CLASS_TMPL(XSymbolTable, Object), {
        /*accessors*/ 0,
        SymbolTableMethods,
        /*lvalueMethods*/ 0,
        offsetof(SymbolTableSubclass, variables),
        /*itemSize*/ 0,
        &SymbolTable_zero
    }, /*meta*/ {
        /*accessors*/ 0,
        ClassSymbolTableMethods,
        /*lvalueMethods*/ 0
    }
};
