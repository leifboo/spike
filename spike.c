
#include "behavior.h"
#include "cgen.h"
#include "int.h"
#include "interp.h"
#include "module.h"
#include "parser.h"
#include "tree.h"
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv) {
    Stmt *tree;
    Module *module;
    Object *obj, *result;
    Symbol *entry;
    unsigned int dataSize;
    
    if (argc != 2) {
        printf("Yip!\n");
        return 1;
    }
    
    SpkClassBehavior_init();
    SpkClassModule_init();
    SpkClassInterpreter_init();
    
    SpkClassBehavior_init2();

    SpkClassBoolean_init();
    SpkClassInteger_init();
    
    tree = SpkParser_ParseFile(argv[1]);
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
    SpkDisassembler_disassembleModule(module, stdout);
    
    obj = (Object *)malloc(module->firstClass->instanceSize);
    obj->klass = module->firstClass;
    entry = SpkSymbol_get("main");
    result = SpkInterpreter_start(obj, entry);
    if (!result) {
        return 1;
    }
    result->klass->print(result);
    
    return 0;
}
