
#ifndef __spk_cgen_h__
#define __spk_cgen_h__


struct SpkModule;
struct SpkStmt;


struct SpkModule *SpkCodeGen_GenerateCode(struct SpkStmt *tree);


#endif /* __spk_cgen_h__ */
