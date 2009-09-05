
#ifndef __spk_compiler_h__
#define __spk_compiler_h__


#include <stdio.h>


struct SpkModule;


struct SpkModule *SpkCompiler_CompileFileStream(FILE *);
struct SpkModule *SpkCompiler_CompileFile(const char *);
struct SpkModule *SpkCompiler_CompileString(const char *);


#endif /* __spk_compiler_h__ */
