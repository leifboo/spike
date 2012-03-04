
#ifndef __compiler_h__
#define __compiler_h__


#include <stdio.h>


struct ModuleTmpl;


struct ModuleTmpl *Compiler_CompileFileStream(FILE *);
struct ModuleTmpl *Compiler_CompileFile(const char *);
struct ModuleTmpl *Compiler_CompileString(const char *);
struct ModuleTmpl *Compiler_CompileModule(const char *);


#endif /* __compiler_h__ */
