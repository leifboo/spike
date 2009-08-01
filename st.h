
#ifndef __spk_st_h__
#define __spk_st_h__


#include "obj.h"


typedef struct SpkSymbolNode SpkSymbolNode;
typedef struct SpkSTEntry SpkSTEntry;
typedef struct SpkContextClass SpkContextClass;
typedef struct SpkScope SpkScope;
typedef struct SpkSymbolTable SpkSymbolTable;


struct SpkSymbolNode {
    SpkObject base;
    
    struct SpkSTEntry *entry;
    SpkSymbolNode *nextError;
    struct SpkExpr *multipleDefList, *undefList;
    
    SpkUnknown *sym;
};


struct SpkSTEntry {
    SpkObject base;
    SpkScope *scope;
    SpkSTEntry *nextInScope;
    SpkSTEntry *shadow;
    SpkSymbolNode *sym;
    struct SpkExpr *def;
};


struct SpkContextClass {
    SpkObject base;
    SpkScope *scope;
    unsigned int level;
    /*SpkOpcode*/ unsigned int pushOpcode, storeOpcode;
    unsigned int nDefs;
};


struct SpkScope {
    SpkObject base;
    SpkScope *outer;
    SpkSTEntry *entryList;
    SpkContextClass *context;
};


struct SpkSymbolTable {
    SpkObject base;
    SpkScope *currentScope;
    SpkSymbolNode *errorList;
    SpkUnknown *symbolNodes;
};


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


SpkSymbolNode *SpkSymbolNode_FromSymbol(SpkSymbolTable *st, SpkUnknown *sym);
SpkSymbolNode *SpkSymbolNode_FromString(SpkSymbolTable *st, const char *str);
SpkSymbolTable *SpkSymbolTable_New(void);
void SpkSymbolTable_Destroy(SpkSymbolTable *st);
void SpkSymbolTable_EnterScope(SpkSymbolTable *st, int enterNewContext);
void SpkSymbolTable_ExitScope(SpkSymbolTable *st);
void SpkSymbolTable_Insert(SpkSymbolTable *st, struct SpkExpr *def);
struct SpkExpr *SpkSymbolTable_Lookup(SpkSymbolTable *st, SpkSymbolNode *sym);
void SpkSymbolTable_Bind(SpkSymbolTable *st, struct SpkExpr *expr);


extern struct SpkBehavior *Spk_ClassSymbolNode, *Spk_ClassSTEntry,
    *Spk_ClassContextClass, *Spk_ClassScope, *Spk_ClassSymbolTable;
extern struct SpkClassTmpl Spk_ClassSymbolNodeTmpl, Spk_ClassSTEntryTmpl,
    Spk_ClassContextClassTmpl, Spk_ClassScopeTmpl, Spk_ClassSymbolTableTmpl;


#endif /* __spk_st_h__ */
