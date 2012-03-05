
#ifndef __st_h__
#define __st_h__


#include "obj.h"


typedef struct SymbolNode SymbolNode, XSymbolNode;
typedef struct STEntry STEntry;
typedef struct ContextClass ContextClass;
typedef struct Scope Scope;
typedef struct SymbolTable SymbolTable, XSymbolTable;


struct SymbolNode {
    Object base;
    struct STEntry *entry;
    struct Symbol *sym;
};


struct STEntry {
    Object base;
    Scope *scope;
    STEntry *nextInScope;
    STEntry *shadow;
    SymbolNode *sym;
    struct Expr *def;
};


struct ContextClass {
    Object base;
    Scope *scope;
    unsigned int level;
    /*Opcode*/ unsigned int pushOpcode, storeOpcode;
    unsigned int nDefs;
};


struct Scope {
    Object base;
    Scope *outer;
    STEntry *entryList;
    ContextClass *context;
};


struct SymbolTable {
    Object base;
    Scope *currentScope;
    struct IdentityDictionary *symbolNodes;
};


SymbolNode *SymbolNode_FromSymbol(SymbolTable *st, struct Symbol *sym);
SymbolNode *SymbolNode_FromCString(SymbolTable *st, const char *str);
int SymbolNode_IsSpec(SymbolNode *node);

SymbolTable *SymbolTable_New(void);
void SymbolTable_EnterScope(SymbolTable *st, int enterNewContext);
void SymbolTable_ExitScope(SymbolTable *st);
Unknown *SymbolTable_Insert(SymbolTable *st, struct Expr *def,
                                  Unknown *requestor);
struct Expr *SymbolTable_Lookup(SymbolTable *st, SymbolNode *sym);
Unknown *SymbolTable_Bind(SymbolTable *st, struct Expr *expr,
                                Unknown *requestor);


extern struct ClassTmpl ClassXSymbolNodeTmpl, ClassXSTEntryTmpl,
    ClassXContextClassTmpl, ClassXScopeTmpl, ClassXSymbolTableTmpl;


#endif /* __st_h__ */
