
#ifndef __cgen_h__
#define __cgen_h__


#include <stddef.h>


struct Method;
struct ModuleTmpl;
struct Stmt;


struct ModuleTmpl *CodeGen_GenerateCode(struct Stmt *tree);
struct Method *CodeGen_NewNativeAccessor(unsigned int kind,
                                               unsigned int type,
                                               size_t offset);


#endif /* __cgen_h__ */
