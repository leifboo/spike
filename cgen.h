
#ifndef __cgen_h__
#define __cgen_h__


struct Module;
struct Stmt;


struct Module *SpkCodeGen_generateCode(struct Stmt *tree, unsigned int dataSize,
                                       struct Stmt *predefList, struct Stmt *rootClassList);


#endif /* __cgen_h__ */
