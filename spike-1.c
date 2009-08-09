
#include "array.h"
#include "behavior.h"
#include "boot.h"
#include "cgen.h"
#include "disasm.h"
#include "host.h"
#include "int.h"
#include "interp.h"
#include "module.h"
#include "native.h"
#include "parser.h"
#include "rodata.h"
#include "scheck.h"
#include "st.h"
#include "str.h"
#include "tree.h"

#include <stdio.h>
#include <string.h>


int main(int argc, char **argv) {
    int i, showHelp, error, disassemble;
    char *arg, *sourceFilename;
    FILE *stream;
    SpkSymbolTable *st;
    SpkStmt *tree;
    SpkModule *module;
    SpkUnknown *result; SpkInteger *resultInt;
    unsigned int dataSize;
    SpkStmtList predefList, rootClassList;
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
        return 0;
    }
    
    if (!sourceFilename) {
        printf("Yip!\n");
        return 1;
    }
    
    Spk_Bootstrap();
    
    stream = fopen(sourceFilename, "r");
    if (!stream) {
        fprintf(stderr, "%s: cannot open '%s'\n", argv[0], sourceFilename);
        return 1;
    }
    
    st = SpkSymbolTable_New();
    
    tree = SpkParser_ParseFile(stream, st);
    if (!tree) {
        return 1;
    }
    
    fclose(stream);
    
    tmp = SpkStaticChecker_Check(tree, st, &dataSize, &predefList, &rootClassList);
    if (!tmp)
        return 1;
    Spk_DECREF(tmp);
    tree = SpkParser_NewModuleDef(tree);
    module = SpkCodeGen_GenerateCode(tree, dataSize, predefList.first, rootClassList.first);
    
    theInterpreter = SpkInterpreter_New();
    
    if (disassemble) {
        SpkDisassembler_DisassembleModule(module, stdout);
        return 0;
    }
    
    argvObj = SpkArray_new(argc - 1);
    for (i = 1; i < argc; ++i) {
        tmp = (SpkUnknown *)SpkString_fromCString(argv[i]);
        SpkArray_setItem(argvObj, i - 1, tmp);
        Spk_DECREF(tmp);
    }
    args = SpkArray_new(1);
    SpkArray_setItem(args, 0, (SpkUnknown *)argvObj);
    Spk_DECREF(argvObj);
    
    entry = Spk_SendMessage(
        theInterpreter,
        (SpkUnknown *)module,
        Spk_METHOD_NAMESPACE_RVALUE,
        Spk_main,
        Spk_emptyArgs
        );
    if (!entry) {
        return 1;
    }
    result = Spk_SendMessage(
        theInterpreter,
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
        return SpkInteger_asLong(resultInt);
    }
    return 0;
}
