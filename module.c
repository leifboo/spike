
#include "module.h"

#include "behavior.h"
#include "dict.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


static Behavior *ClassModule;
Module *builtInModule;


/*------------------------------------------------------------------------*/
/* methods */

static Object *Module_print(Object *self, Object *arg0, Object *arg1) {
    printf("<Module object at %p>", self);
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    { "print", SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &Module_print },
    { 0, 0, 0}
};

static SpkClassTmpl tmpl = {
    offsetof(ModuleSubclass, variables),
    sizeof(Module),
    0,
    methods
};


/*------------------------------------------------------------------------*/
/* C API */

void SpkClassModule_init(void) {
    ClassModule = SpkBehavior_new(ClassObject, 0 /*builtInModule*/, 0);
    builtInModule = SpkModule_new(0);
    ClassModule->module = builtInModule;
}

void SpkClassModule_init2(void) {
    ClassModule->methodDict->base.klass = ClassIdentityDictionary;
    SpkBehavior_initFromTemplate(ClassModule, &tmpl);
}

Module *SpkModule_new(unsigned int nGlobals) {
    Module *newModule;
    
    newModule = (Module *)malloc(ClassModule->instVarOffset + nGlobals*sizeof(Object *));
    newModule->base.klass = ClassModule;
    return newModule;
}
