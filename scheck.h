
#ifndef __spk_scheck_h__
#define __spk_scheck_h__


#include "obj.h"


struct SpkStmt;
struct SpkStmtList;
struct SpkSymbolTable;


SpkUnknown *SpkStaticChecker_Check(struct SpkStmt *tree,
                                   struct SpkSymbolTable *st,
                                   SpkUnknown *requestor);


extern int Spk_declareBuiltIn; /* XXX */


#endif /* __spk_scheck_h__ */
