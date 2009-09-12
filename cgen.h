
#ifndef __spk_cgen_h__
#define __spk_cgen_h__


#include <stddef.h>


struct SpkMethod;
struct SpkModule;
struct SpkStmt;


struct SpkModule *SpkCodeGen_GenerateCode(struct SpkStmt *tree);
struct SpkMethod *SpkCodeGen_NewNativeAccessor(unsigned int kind,
                                               unsigned int type,
                                               size_t offset);


#endif /* __spk_cgen_h__ */
