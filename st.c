
#include "st.h"

#include "class.h"
#include "host.h"
#include "interp.h"
#include "tree.h"
#include <assert.h>
#include <string.h>


SpkBehavior *Spk_ClassSymbolNode, *Spk_ClassSTEntry,
    *Spk_ClassContextClass, *Spk_ClassScope, *Spk_ClassSymbolTable;


SpkSymbolNode *SpkSymbolNode_FromSymbol(SpkSymbolTable *st, SpkUnknown *sym) {
    SpkSymbolNode *s;
    
    s = (SpkSymbolNode *)SpkHost_SymbolValue(st->symbolNodes, sym);
    if (s) {
        return s;
    }
    
    s = (SpkSymbolNode *)SpkObject_New(Spk_ClassSymbolNode);
    s->entry = 0;
    s->nextError = 0;
    s->multipleDefList = 0;
    s->undefList = 0;
    s->sym = sym;  Spk_INCREF(sym);
    
    SpkHost_DefineSymbol(st->symbolNodes, sym, (SpkUnknown *)s);
    Spk_DECREF(s);
    
    return s;
}

SpkSymbolNode *SpkSymbolNode_FromString(SpkSymbolTable *st, const char *str) {
    SpkUnknown *sym;
    SpkSymbolNode *node;
    
    sym = SpkHost_SymbolFromString(str);
    node = SpkSymbolNode_FromSymbol(st, sym);
    Spk_DECREF(sym);
    return node;
}

static void registerError(SpkSymbolTable *st, SpkSymbolNode *sym) {
    /* add 'sym' to the error list, if it isn't already on it */
    
    if (!sym->multipleDefList && !sym->undefList) {
        sym->nextError = st->errorList;
        st->errorList = sym;
    }
}

SpkSymbolTable *SpkSymbolTable_New() {
    SpkSymbolTable *newST;
    
    newST = (SpkSymbolTable *)SpkObject_New(Spk_ClassSymbolTable);
    newST->currentScope = 0;
    newST->errorList = 0;
    newST->symbolNodes = SpkHost_NewSymbolDict();
    return newST;
}

void SpkSymbolTable_Destroy(SpkSymbolTable *st) {
    while (st->currentScope) {
        SpkSymbolTable_ExitScope(st);
    }
    Spk_DECREF(st->symbolNodes);
    Spk_DECREF(st);
}

void SpkSymbolTable_EnterScope(SpkSymbolTable *st, int enterNewContext) {
    SpkScope *newScope, *outerScope;
    SpkContextClass *newContext;
    
    outerScope = st->currentScope;
    
    newScope = (SpkScope *)SpkObject_New(Spk_ClassScope);
    newScope->outer = outerScope;
    newScope->entryList = 0;
    if (enterNewContext) {
        newContext = (SpkContextClass *)SpkObject_New(Spk_ClassContextClass);
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
        newScope->context = newContext;
    } else {
        newScope->context = outerScope->context;
    }
    
    st->currentScope = newScope;
    
    return;
}

void SpkSymbolTable_ExitScope(SpkSymbolTable *st) {
    SpkScope *oldScope;
    SpkSTEntry *entry, *nextInScope;
    SpkSymbolNode *sym;
    
    oldScope = st->currentScope;
    for (entry = oldScope->entryList; entry; entry = nextInScope) {
        sym = entry->sym;
        assert(sym->entry == entry);
        sym->entry = entry->shadow;
        nextInScope = entry->nextInScope;
        Spk_DECREF(entry);
    }
    if (oldScope->context->scope == oldScope) {
        Spk_DECREF(oldScope->context);
    }
    st->currentScope = oldScope->outer;
    Spk_DECREF(oldScope);
}

void SpkSymbolTable_Insert(SpkSymbolTable *st, SpkExpr *def) {
    SpkSymbolNode *sym;
    SpkSTEntry *oldEntry, *newEntry;
    SpkScope *cs;
    
    assert(def->kind == Spk_EXPR_NAME);
    
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
            return;
        }
        registerError(st, sym);
        def->u.def.nextMultipleDef = sym->multipleDefList;
        sym->multipleDefList = def;
        return;
    }
    
    newEntry = (SpkSTEntry *)SpkObject_New(Spk_ClassSTEntry);
    newEntry->scope = cs;
    newEntry->nextInScope = cs->entryList;
    newEntry->shadow = oldEntry;
    newEntry->sym = sym;
    newEntry->def = def;
    
    cs->entryList = newEntry;
    sym->entry = newEntry;
    
    def->u.def.level = cs->context->level;
    if (cs->context->pushOpcode != Spk_OPCODE_NOP) { /* not built-in scope */
        def->u.def.pushOpcode = cs->context->pushOpcode;
        def->u.def.storeOpcode = cs->context->storeOpcode;
    }
    /* XXX: We could overlap scopes with the addition of a 'clear'
       opcode. */
    def->u.def.index = cs->context->nDefs++;
}

SpkExpr *SpkSymbolTable_Lookup(SpkSymbolTable *st, SpkSymbolNode *sym) {
    SpkSTEntry *entry;
    
    entry = sym->entry;
    return entry ? entry->def : 0;
}

void SpkSymbolTable_Bind(SpkSymbolTable *st, SpkExpr *expr) {
    SpkSymbolNode *sym;
    SpkSTEntry *entry;
    
    assert(expr->kind == Spk_EXPR_NAME);
    
    sym = expr->sym;
    entry = sym->entry;
    if (entry) {
        expr->u.ref.def = entry->def;
        expr->u.ref.level = st->currentScope->context->level;
        return;
    }
    
    /* undefined */
    registerError(st, sym);
    expr->u.ref.nextUndef = sym->undefList;
    sym->undefList = expr;
}


/*------------------------------------------------------------------------*/
/* class templates */

SpkClassTmpl Spk_ClassSymbolNodeTmpl = {
    "SymbolNode", {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(SpkSymbolNodeSubclass, variables)
    }, /*meta*/ {
    }
};

SpkClassTmpl Spk_ClassSTEntryTmpl = {
    "STEntry", {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(SpkSTEntrySubclass, variables)
    }, /*meta*/ {
    }
};

SpkClassTmpl Spk_ClassContextClassTmpl = {
    "ContextClass", {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(SpkContextClassSubclass, variables)
    }, /*meta*/ {
    }
};

SpkClassTmpl Spk_ClassScopeTmpl = {
    "Scope", {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(SpkScopeSubclass, variables)
    }, /*meta*/ {
    }
};

SpkClassTmpl Spk_ClassSymbolTableTmpl = {
    "SymbolTable", {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(SpkSymbolTableSubclass, variables)
    }, /*meta*/ {
    }
};
