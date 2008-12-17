
#ifndef __scheck_h__
#define __scheck_h__


struct BootRec;
struct Stmt;
struct StmtList;


int SpkStaticChecker_Check(struct Stmt *tree, struct BootRec *bootRec,
                           unsigned int *pDataSize,
                           struct StmtList *predefList, struct StmtList *rootClassList);


#endif /* __scheck_h__ */
