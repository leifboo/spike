
#include "array.h"
#include "behavior.h"
#include "boot.h"
#include "compiler.h"
#include "disasm.h"
#include "heart.h"
#include "int.h"
#include "interp.h"
#include "module.h"
#include "native.h"
#include "rodata.h"
#include "scheck.h"
#include "str.h"

#include "bool.h"
#include "float.h"
#include "int.h"
#include "io.h"
#include "str.h"

#include <stdio.h>
#include <string.h>


#define CLASS_TMPL_DEF(c) Class ## c ## Tmpl
#define CBR(c, s) &CLASS_TMPL_DEF(c)

ClassBootRec classBootRec[] = {
    /***CBR(VariableObject, Object),*/
    /******/CBR(String,  VariableObject),
    /**/CBR(Boolean,    Object),
    /******/CBR(False,  Object),
    /******/CBR(True,   Object),
    /**/CBR(Integer,    Object),
    /**/CBR(Float,      Object),
    /**/CBR(FileStream, Object),
    0
};


Module *heart;


static int Main(int argc, char **argv) {
    int i, showHelp, error, disassemble;
    char *arg, *sourceFilename;
    ModuleTmpl *moduleTmpl; ModuleClass *moduleClass; Object *module;
    Unknown *result; Integer *resultInt;
    Unknown *tmp;
    Array *argvObj, *args;
    
    sourceFilename = 0;
    showHelp = error = disassemble = 0;
    for (i = 1; i < argc; ++i) {
        arg = argv[i];
        if (*arg != '-') {
            sourceFilename = arg;
            break;
        }
        ++arg;
        if (*arg == '-') {
            ++arg;
            if (strcmp(arg, "help") == 0) {
                showHelp = 1;
                continue;
            }
            if (strcmp(arg, "disassemble") == 0) {
                disassemble = 1;
                continue;
            }
            if (strcmp(arg, "gen-c-code") == 0) {
                disassemble = 2;
                declareBuiltIn = 0;
                continue;
            }
            fprintf(stderr, "%s: unrecognized option %s\n", argv[0], argv[i]);
            error = 1;
        } else {
            switch (*arg) {
            case '?':
            case 'h':
                showHelp = 1;
                break;
            default:
                fprintf(stderr, "%s: unrecognized option %s\n", argv[0], argv[i]);
                error = 1;
                break;
            }
        }
    }
    
    if (error) {
        return 1;
    }
    if (showHelp) {
        printf("-?, -h, --help    Display this help and exit.\n");
        printf("--disassemble     Display code disassembly and exit.\n");
        printf("--gen-c-code      Display C code disassembly and exit.\n");
        return 0;
    }
    
    if (!sourceFilename) {
        printf("Yip!\n");
        return 1;
    }
    
    if (!Boot())
        return 1;
    
    arg = sourceFilename + strlen(sourceFilename) - 3;
    if (arg > sourceFilename && 0 == strcmp(arg, ".sm")) {
        moduleTmpl = Compiler_CompileModule(sourceFilename);
    } else {
        moduleTmpl = Compiler_CompileFile(sourceFilename);
    }
    if (!moduleTmpl)
        return 1;
    
    switch (disassemble) {
    case 1:
        Disassembler_DisassembleModule(moduleTmpl, stdout);
        return 0;
    case 2:
        Disassembler_DisassembleModuleAsCCode(moduleTmpl, stdout);
        return 0;
    }
    
    /* Create and initialize the module. */
    moduleClass = ModuleClass_New(moduleTmpl);
    module = Object_New((Behavior *)moduleClass);
    if (!module) {
        return 1;
    }
    tmp = Send(GLOBAL(theInterpreter), (Unknown *)module, _init, 0);
    if (!tmp)
        return 0;
    
    /* Build 'argv'. */
    argvObj = Array_New(argc - 1);
    for (i = 1; i < argc; ++i) {
        tmp = (Unknown *)String_FromCString(argv[i]);
        Array_SetItem(argvObj, i - 1, tmp);
    }
    args = Array_New(1);
    Array_SetItem(args, 0, (Unknown *)argvObj);
    
    /* Call 'main'. */
    result = SendMessage(
        GLOBAL(theInterpreter),
        (Unknown *)module,
        METHOD_NAMESPACE_RVALUE,
        _main,
        (Unknown *)args
        );
    
    if (!result) {
        return 1;
    }
    resultInt = CAST(Integer, result);
    if (resultInt) {
        return Integer_AsCLong(resultInt);
    }
    return 0;
}


int main(int argc, char **argv) {
    return Main(argc, argv);
}
