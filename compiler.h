
#ifndef __spk_compiler_h__
#define __spk_compiler_h__


#include <stdio.h>


struct SpkModuleTmpl;


struct SpkModuleTmpl *SpkCompiler_CompileFileStream(FILE *);
struct SpkModuleTmpl *SpkCompiler_CompileFile(const char *);
struct SpkModuleTmpl *SpkCompiler_CompileString(const char *);


#endif /* __spk_compiler_h__ */
