
#include "module.h"

#include "behavior.h"
#include "dict.h"
#include <stdio.h>
#include <stdlib.h>


Behavior *Spk_ClassModule;
Module *Spk_builtInModule;


/*------------------------------------------------------------------------*/
/* methods */

static Object *Module_print(Object *self, Object *arg0, Object *arg1) {
    printf("<Module object at %p>", self);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    { "print", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Module_print },
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassModuleTmpl = {
    "Module",
    offsetof(ModuleSubclass, variables),
    sizeof(Module),
    0,
    0,
    methods
};


/*------------------------------------------------------------------------*/
/* C API */

Module *SpkModule_new(Behavior *moduleSubclass) {
    Module *newModule;
    
    newModule = (Module *)malloc(moduleSubclass->instanceSize);
    newModule->base.klass = moduleSubclass;
    newModule->literals = 0;
    newModule->firstClass = 0;
    return newModule;
}
