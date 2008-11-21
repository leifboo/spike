
#ifndef __st_h__
#define __st_h__


typedef struct SymbolNode SymbolNode;
typedef struct STEntry STEntry;
typedef struct ContextClass ContextClass;
typedef struct Scope Scope;
typedef struct SymbolTable SymbolTable;


struct SymbolNode {
    SymbolNode *next;
    
    struct STEntry *entry;
    SymbolNode *nextError;
    struct Expr *multipleDefList, *undefList;
    
    struct Symbol *sym;
};


struct STEntry {
    Scope *scope;
    STEntry *nextInScope;
    STEntry *shadow;
    SymbolNode *sym;
    struct Expr *def;
};


struct ContextClass {
    Scope *scope;
    unsigned int level;
    unsigned int nDefs;
};


struct Scope {
    Scope *outer;
    STEntry *entryList;
    ContextClass *context;
};


struct SymbolTable {
    Scope *currentScope;
    SymbolNode *errorList;
};


SymbolNode *SpkSymbolNode_Get(struct Symbol *sym);
SymbolTable *SpkSymbolTable_New();
void SpkSymbolTable_Destroy(SymbolTable *st);
void SpkSymbolTable_EnterScope(SymbolTable *st, int enterNewContext);
void SpkSymbolTable_ExitScope(SymbolTable *st);
void SpkSymbolTable_Insert(SymbolTable *st, struct Expr *def);
struct Expr *SpkSymbolTable_Lookup(SymbolTable *st, SymbolNode *sym);
void SpkSymbolTable_Bind(SymbolTable *st, struct Expr *expr);


#endif /* __st_h__ */
