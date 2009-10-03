
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

#include <stdio.h>
#include <string.h>


SpkModule *Spk_heart;


int Spk_Main(int argc, char **argv) {
    int i, showHelp, error, disassemble;
    char *arg, *sourceFilename;
    SpkModule *module;
    SpkUnknown *result; SpkInteger *resultInt;
    SpkUnknown *entry, *tmp;
    SpkArray *argvObj, *args;
    
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
                Spk_declareBuiltIn = 0;
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
    
    if (!Spk_Boot())
        return 1;
    
    module = SpkCompiler_CompileFile(sourceFilename);
    if (!module)
        return 1;
    
    switch (disassemble) {
    case 1:
        SpkDisassembler_DisassembleModule(module, stdout);
        return 0;
    case 2:
        SpkDisassembler_DisassembleModuleAsCCode(module, stdout);
        return 0;
    }
    
    argvObj = SpkArray_New(argc - 1);
    for (i = 1; i < argc; ++i) {
        tmp = (SpkUnknown *)SpkString_FromCString(argv[i]);
        SpkArray_SetItem(argvObj, i - 1, tmp);
        Spk_DECREF(tmp);
    }
    args = SpkArray_New(1);
    SpkArray_SetItem(args, 0, (SpkUnknown *)argvObj);
    Spk_DECREF(argvObj);
    
    entry = Spk_SendMessage(
        Spk_GLOBAL(theInterpreter),
        (SpkUnknown *)module,
        Spk_METHOD_NAMESPACE_RVALUE,
        Spk_main,
        Spk_emptyArgs
        );
    if (!entry) {
        return 1;
    }
    result = Spk_SendMessage(
        Spk_GLOBAL(theInterpreter),
        entry,
        Spk_METHOD_NAMESPACE_RVALUE,
        Spk___apply__,
        (SpkUnknown *)args
        );
    
    Spk_DECREF(args);
    
    if (!result) {
        return 1;
    }
    resultInt = Spk_CAST(Integer, result);
    if (resultInt) {
        return SpkInteger_AsCLong(resultInt);
    }
    return 0;
}


int main(int argc, char **argv) {
    return Spk_Main(argc, argv);
}
