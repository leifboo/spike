
#ifndef __scheck_h__
#define __scheck_h__


#include "obj.h"


struct Stmt;
struct StmtList;
struct SymbolTable;


Unknown *StaticChecker_DeclareBuiltIn(struct SymbolTable *st,
                                            Unknown *requestor);
Unknown *StaticChecker_Check(struct Stmt *tree,
                                   struct SymbolTable *st,
                                   Unknown *requestor);


extern int declareBuiltIn; /* XXX */
extern int declareObject;


#endif /* __scheck_h__ */
