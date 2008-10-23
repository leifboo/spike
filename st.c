
#include "st.h"

#include "interp.h"
#include "tree.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>


SymbolNode *SpkSymbolNode_Get(const char *str) {
    /* Just a simple linked list for now! */
    static SymbolNode *list = 0;
    Symbol *sym;
    SymbolNode *s;
    
    sym = SpkSymbol_get(str);
    for (s = list; s; s = s->next) {
        if (s->sym == sym) {
            return s;
        }
    }
    
    s = (SymbolNode *)malloc(sizeof(SymbolNode));
    s->next = list;
    s->entry = 0;
    s->nextError = 0;
    s->multipleDefList = 0;
    s->undefList = 0;
    s->sym = sym;
    list = s;
    
    return s;
}

static void registerError(SymbolTable *st, SymbolNode *sym) {
    /* add 'sym' to the error list, if it isn't already on it */
    
    if (!sym->multipleDefList && !sym->undefList) {
        sym->nextError = st->errorList;
        st->errorList = sym;
    }
}

SymbolTable *SpkSymbolTable_New() {
    SymbolTable *newST;
    
    newST = (SymbolTable *)malloc(sizeof(SymbolTable));
    newST->currentScope = 0;
    newST->errorList = 0;
    return newST;
}

void SpkSymbolTable_Destroy(SymbolTable *st) {
    while (st->currentScope) {
        SpkSymbolTable_ExitScope(st);
    }
    free(st);
}

void SpkSymbolTable_EnterScope(SymbolTable *st, int enterNewContext) {
    Scope *newScope, *outerScope;
    ContextClass *newContext;
    
    outerScope = st->currentScope;
    
    newScope = (Scope *)malloc(sizeof(Scope));
    newScope->outer = outerScope;
    newScope->entryList = 0;
    if (enterNewContext) {
        newContext = (ContextClass *)malloc(sizeof(ContextClass));
        newContext->scope = newScope;
        newContext->level = outerScope ? outerScope->context->level + 1 : 0;
        newContext->nDefs = 0;
        newScope->context = newContext;
    } else {
        newScope->context = outerScope->context;
    }
    
    st->currentScope = newScope;
    
    return;
}

void SpkSymbolTable_ExitScope(SymbolTable *st) {
    Scope *oldScope;
    STEntry *entry, *nextInScope;
    SymbolNode *sym;
    
    oldScope = st->currentScope;
    for (entry = oldScope->entryList; entry; entry = nextInScope) {
        sym = entry->sym;
        assert(sym->entry == entry);
        sym->entry = entry->shadow;
        nextInScope = entry->nextInScope;
        free(entry);
    }
    if (oldScope->context->scope == oldScope) {
        free(oldScope->context);
    }
    st->currentScope = oldScope->outer;
    free(oldScope);
}

void SpkSymbolTable_Insert(SymbolTable *st, Expr *def) {
    SymbolNode *sym;
    STEntry *oldEntry, *newEntry;
    Scope *cs;
    
    assert(def->kind == EXPR_NAME);
    
    sym = def->sym;
    oldEntry = sym->entry;
    cs = st->currentScope;
    
    if (oldEntry && oldEntry->scope == cs) {
        /* multiply defined */
        registerError(st, sym);
        def->u.def.nextMultipleDef = sym->multipleDefList;
        sym->multipleDefList = def;
        return;
    }
    
    newEntry = (STEntry *)malloc(sizeof(STEntry));
    newEntry->scope = cs;
    newEntry->nextInScope = cs->entryList;
    newEntry->shadow = oldEntry;
    newEntry->sym = sym;
    newEntry->def = def;
    
    cs->entryList = newEntry;
    sym->entry = newEntry;
    
    def->u.def.level = cs->context->level;
    def->u.def.index = cs->context->nDefs++;
}

Expr *SpkSymbolTable_Lookup(SymbolTable *st, SymbolNode *sym) {
    STEntry *entry;
    
    entry = sym->entry;
    return entry ? entry->def : 0;
}

void SpkSymbolTable_Bind(SymbolTable *st, Expr *expr) {
    SymbolNode *sym;
    STEntry *entry;
    
    assert(expr->kind == EXPR_NAME);
    
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
