
#ifndef __spk_scheck_h__
#define __spk_scheck_h__


#include "obj.h"


struct SpkStmt;
struct SpkStmtList;
struct SpkSymbolTable;


SpkUnknown *SpkStaticChecker_Check(struct SpkStmt *tree,
                                   struct SpkSymbolTable *st,
                                   SpkUnknown *requestor);


#endif /* __spk_scheck_h__ */
