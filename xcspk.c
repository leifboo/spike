
#include "boot.h"
#include "compiler.h"
#include "disasm.h"
#include "module.h"
#include "scheck.h"

#include "float.h"
#include "int.h"
#include "io.h"
#include "str.h"

#include <stdio.h>


#define CLASS_TMPL_DEF(c) Class ## c ## Tmpl
#define CLASS(c, s) &CLASS_TMPL_DEF(c)

ClassBootRec classBootRec[] = {
    /***CLASS(VariableObject, Object),*/
    /******/CLASS(String,  VariableObject),
    /**/CLASS(Integer,    Object),
    /**/CLASS(Float,      Object),
    /**/CLASS(FileStream, Object),
    0
};


ModuleTmpl ModulemoduleTmpl = { { "heart" } };

struct Module *heart;


int main(int argc, char **argv) {
    int i;
    struct ModuleTmpl *module;
    const char *outputFilename = "xgenerated.c";
    FILE *out;
    
    if (argc < 2) {
        fprintf(stderr, "Spike Bootstrapping Compiler\n");
        fprintf(stderr, "usage: %s FILE ...\n", argv[0]);
        return 1;
    }
    
    if (!Boot())
        return 1;
    
    module = 0;
    declareBuiltIn = 0;
    for (i = 1; i < argc; ++i) {
        module = Compiler_CompileFile(argv[i]);
        break;
    }
    
    if (!module)
        return 1;
    
    out = fopen(outputFilename, "w");
    if (!out) {
        fprintf(stderr, "%s: cannot open '%s'\n", argv[0], outputFilename);
        return 1;
    }
    Disassembler_DisassembleModuleAsCCode(module, out);
    fclose(out);
    
    return 0;
}

