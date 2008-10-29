
#include "behavior.h"
#include "cgen.h"
#include "int.h"
#include "interp.h"
#include "module.h"
#include "parser.h"
#include "str.h"
#include "tree.h"
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv) {
    int i, showHelp, error, disassemble;
    char *arg, *sourceFilename ;
    Stmt *tree;
    Module *module;
    Object *obj, *result;
    Symbol *entry;
    unsigned int dataSize;
    
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
    
    /* XXX: program arguments */

    SpkClassObject_init();
    SpkClassBehavior_init();
    SpkClassModule_init();
    SpkClassIdentityDictionary_init();
    SpkClassInterpreter_init();
    
    SpkClassBehavior_init2();
    SpkClassObject_init2();
    SpkClassModule_init2();

    SpkClassBoolean_init();
    SpkClassInteger_init();
    SpkClassString_init();
    
    tree = SpkParser_ParseFile(sourceFilename);
    if (!tree) {
        return 1;
    }
    
    if (SpkStaticChecker_Check(tree, &dataSize) == -1) {
        return 1;
    }
    module = SpkCodeGen_generateCode(tree, dataSize);
    if (!module->firstClass) {
        fprintf(stderr, "no classes\n");
        return 1;
    }
    
    if (disassemble) {
        SpkDisassembler_disassembleModule(module, stdout);
        return 0;
    }
    
    obj = (Object *)malloc(module->firstClass->instanceSize);
    obj->klass = module->firstClass;
    entry = SpkSymbol_get("main");
    result = SpkInterpreter_start(obj, entry);
    
    if (!result) {
        return 1;
    }
    if (result->klass == ClassInteger) {
        return SpkInteger_asLong((Integer *)result);
    }
    return 0;
}
