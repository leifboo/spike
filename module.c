
#include "module.h"

#include "behavior.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


static Behavior *ClassModule;
Module *builtInModule;


/*------------------------------------------------------------------------*/
/* methods */

static Object *Module_print(Object *self) {
    printf("a Module at %p\n", self);
}


/*------------------------------------------------------------------------*/
/* C API */

void SpkClassModule_init(void) {
    ClassModule = SpkBehavior_new(0 /*ClassObject*/ , 0 /*builtInModule*/);
    ClassModule->print = &Module_print;
    ClassModule->instVarOffset = offsetof(ModuleSubclass, variables);
    
    builtInModule = SpkModule_new(0);
    
    ClassModule->module = builtInModule;
}

Module *SpkModule_new(unsigned int nGlobals) {
    Module *newModule;
    
    newModule = (Module *)malloc(ClassModule->instVarOffset + nGlobals*sizeof(Object *));
    newModule->base.klass = ClassModule;
    return newModule;
}
