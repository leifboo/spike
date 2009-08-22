
#ifndef __spk_scheck_h__
#define __spk_scheck_h__


#include "obj.h"


struct SpkStmt;
struct SpkStmtList;
struct SpkSymbolTable;


SpkUnknown *SpkStaticChecker_Check(struct SpkStmt *tree,
                                   struct SpkSymbolTable *st,
                                   SpkUnknown *notifier,
                                   unsigned int *pDataSize,
                                   struct SpkStmtList *predefList,
                                   struct SpkStmtList *rootClassList);


#endif /* __spk_scheck_h__ */
